#include <ntifs.h>
#include <ntddk.h>
#include <Ntstrsafe.h>
#include <wdm.h>
#include <wdmsec.h>
#include "demo.h"

// TARGETLIBS=$(DDK_LIB_PATH)\ntstrsafe.lib

extern PDRIVER_OBJECT g_poDriverObject;			// 驱动对象
extern PUNICODE_STRING g_psRegistryPath;		// 驱动注册路径
extern PDEVICE_OBJECT g_demo_cdo = NULL;		// 生成的设备对象

void DemoMain()
{
	StringOperationSample();
	ListOperationSample();
	SpinLockOperationSample();
	MemoryOperationSample();
	EventOperationSample();
	RegistryKeyOperationSample();

	FileOperationSample();
	DeviceControlSample();

	KdBreakPoint();
}


/*	UNICODE_STRING 操作*/
void StringOperationSample(void)
{
	// 字符串初始化
	UNICODE_STRING uFirstString1 = { 0 };
	RtlInitUnicodeString(&uFirstString1, L"Hello Kernel String struct UNICODE_STRING\n");	// 并没有为uFirstString1.Buffer分配字符串,直接让他指向常量
	DbgPrint("[dbg]: Length=%d MaximumLength=%d uFirstString1=\"%wZ \" 1" , uFirstString1.Length, uFirstString1.MaximumLength, &uFirstString1);

	WCHAR strBuffer2[] = L"Hello Kernel String\n";
	UNICODE_STRING uFirstString2 = { 0 };
	RtlInitUnicodeString(&uFirstString2, strBuffer2);			// uFirstString2指向了strBuffer2,这个时候改变strBuffer2字符串里的值,uFirstString2也会跟着改变
	strBuffer2[0] = L'W';
	DbgPrint("[dbg]: uFirstString2=\"%wZ \" 2", &uFirstString2);

	
	// 字符串拷贝
	WCHAR strBuffer3[128] = { 0 };
	UNICODE_STRING uFirstString3 = { 0 };
	RtlInitEmptyUnicodeString(&uFirstString3, strBuffer3, sizeof(strBuffer3));
		// 这个函数需要使用#include <Ntstrsafe.h> TARGETLIBS=$(DDK_LIB_PATH)\ntstrsafe.lib, 这个函数只能在IRQL PASSIVELEVEL下使用
	RtlUnicodeStringCopyString(&uFirstString3, L"Hello Kernel String struct 3\n");
	DbgPrint("[dbg]: uFirstString3=\"%wZ \" ", &uFirstString3);

}



/* LIST_ENTRY 链表结构操作 */
void ListOperationSample(void)
{
	// 使用自定义节点结构嵌入系统LIST_ENTRY
	typedef struct _TestListEntery
	{
		ULONG m_ulDataA;
		ULONG m_ulDataB;
		LIST_ENTRY m_ListEntry;		// 其中Blink指向前一个节点的LIST_ENTRY结构,Flink指向后一个节点LIST_ENTRY
		ULONG m_ulDataC;
		ULONG m_ulDataD;
	}TestListEntery, * PTestListEntery;


	LIST_ENTRY ListHeader = { 0 };		// 头节点
	TestListEntery EnteryA = { 0 };
	TestListEntery EnteryB = { 0 };
	TestListEntery EnteryC = { 0 };

	EnteryA.m_ulDataA = 'A';
	EnteryB.m_ulDataA = 'B';
	EnteryC.m_ulDataA = 'C';

	InitializeListHead(&ListHeader);	

	// 节点插入
	InsertHeadList(&ListHeader, &EnteryA.m_ListEntry);	
	InsertHeadList(&ListHeader, &EnteryB.m_ListEntry);
	InsertTailList(&ListHeader, &EnteryC.m_ListEntry);

	// 节点移除
	RemoveHeadList(&ListHeader);

	// 链表遍历
	DbgPrint("[dbg]: start Ergodic List\n");
	PLIST_ENTRY pListEntery = ListHeader.Flink;
	while (pListEntery != &ListHeader)
	{
	/* 由于 pListEntery->Flink并不是直接指向下一个节点的结构头,而是LIST_ENTRY的位置,需要使用 CONTAINING_RECORD(Address, type, Field) 转换让pTestEntry指向结构头地址 */
		PTestListEntery pTestEntry = CONTAINING_RECORD(pListEntery, TestListEntery, m_ListEntry);
		DbgPrint("[dbg]: ListPtr=%p Entry=%p Tag=%c\n", pListEntery, pTestEntry, pTestEntry->m_ulDataA);
		pListEntery = pListEntery->Flink;
	}
	DbgPrint("[dbg]: end Ergodic List\n");

	return;
}



void SpinLockOperationSample(void)
{
	// TODO: 自旋锁配合链表使用
	typedef struct _FILE_INFO
	{
		LIST_ENTRY m_ListEntry;
		WCHAR m_strFileNmae[260];
	}FILE_INFO, *PFILE_INFO;

	KIRQL irql = 0;
	UNREFERENCED_PARAMETER(irql);

	LIST_ENTRY my_list_head = { 0 };			// 链表头
	FILE_INFO my_file_info = { 0 };
	KSPIN_LOCK my_list_lock = { 0 };		//链表的锁,写成全局变量或者静态变量

	InitializeListHead(&my_list_head);
	ExInterlockedInsertHeadList(&my_list_head, (PLIST_ENTRY)&my_file_info, &my_list_lock);

	// 链表遍历
	PLIST_ENTRY pListEntery = my_list_head.Flink;
	while (pListEntery != &my_list_head)
	{
		/* 由于 pListEntery->Flink并不是直接指向下一个节点的结构头,而是LIST_ENTRY的位置,需要使用 CONTAINING_RECORD(Address, type, Field) 转换让pTestEntry指向结构头地址 */
		PFILE_INFO pTestEntry = CONTAINING_RECORD(pListEntery, FILE_INFO, m_ListEntry);
		if (!pTestEntry)
			break;
		DbgPrint("[dbg]: FileName= %ws \n", pTestEntry->m_strFileNmae);
		pListEntery = pListEntery->Flink;
	}

	ExInterlockedRemoveHeadList(&my_list_head, &my_list_lock);
	// 队列自旋锁
	KSPIN_LOCK my_Queue_SpinLock = { 0 };
	KLOCK_QUEUE_HANDLE my_lock_queue_handle = { 0 };
	KeInitializeSpinLock(&my_Queue_SpinLock);

	KeAcquireInStackQueuedSpinLock(&my_Queue_SpinLock, &my_lock_queue_handle);
	// ... do something
	KeReleaseInStackQueuedSpinLock(&my_lock_queue_handle);

	return;
}

void MemoryOperationSample(void)
{
	PNPAGED_LOOKASIDE_LIST pLookAsideList = NULL;
	BOOLEAN bSucc = FALSE;
	BOOLEAN bInit = FALSE;
	PVOID pFirstMemory = NULL;
	PVOID pSecondMemory = NULL;

	do
	{
		if(NULL == (pLookAsideList = (PNPAGED_LOOKASIDE_LIST)ExAllocatePoolWithTag(NonPagedPool, sizeof(NPAGED_LOOKASIDE_LIST), 'a')))
			break;
		memset(pLookAsideList, 0, sizeof(NPAGED_LOOKASIDE_LIST));

		// 初始化旁视列表对象
		ExInitializeNPagedLookasideList(pLookAsideList, NULL, NULL, 0, 128, 'a', 0);
		bInit = TRUE;

		if(NULL == (pFirstMemory = ExAllocateFromNPagedLookasideList(pLookAsideList)))
			break;
		if (NULL == (pSecondMemory = ExAllocateFromNPagedLookasideList(pLookAsideList)))
			break;

		DbgPrint("[dbg]: First: %p, Second:%p\n", pFirstMemory, pSecondMemory);

		// 释放pFirstMemory
		ExFreeToNPagedLookasideList(pLookAsideList, pFirstMemory);

		// 再次分配,查看内存是否是从刚通过旁视列表释放的内存
		if (NULL == (pFirstMemory = ExAllocateFromNPagedLookasideList(pLookAsideList)))
			break;

		DbgPrint("[dbg]: ReAlloc First: %p\n", pFirstMemory);

		bSucc = TRUE;
	} while (FALSE);

	if (pFirstMemory && pLookAsideList)
	{
		ExFreeToNPagedLookasideList(pLookAsideList, pFirstMemory);
		pFirstMemory = NULL;
	}
	if (pSecondMemory && pLookAsideList)
	{
		ExFreeToNPagedLookasideList(pLookAsideList, pSecondMemory);
		pSecondMemory = NULL;
	}
	if (pLookAsideList)
	{
		ExFreePoolWithTag(pLookAsideList, 'a');
		pLookAsideList = NULL;
	}
	return;
}

BOOLEAN EventOperationSample()
{
/*
	typedef enum _ACCESS_MASK {
		EVENT_ALL_ACCESS, EVENT_QUERY_STATE, EVENT_MODIFY_STATE...
	}ACCESS_MASK; // EVENT打开权限

	typedef enum _EVENT_TYPE {
		SynchronizationEvent, NotificationEvent, ...
	}EVENT_TYPE; // EVENT类型

	typedef enum _ObjectType	//指向对象类型的指针 
	{
		*ExEventObjectType,
		* ExSemaphoreObjectType,
		* IoFileObjectType,
		* PsProcessType,
		* PsThreadType,
		* SeTokenObjectType,
		* TmEnlistmentObjectType,
		* TmResourceManagerObjectType,
		* TmTransactionManagerObjectType,
		* TmTransactionObjectType
		// 如果 ObjectType 不是 NULL，则操作系统会验证提供的对象类型是否与 Handle 指定的对象的对象类型匹配 
	}OBJECT_TYPE;

*/
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	BOOLEAN bSucc = FALSE;
	HANDLE hCreateEvent = NULL;
	PVOID pCreateEventObject = NULL;
	HANDLE hOpenEvent = NULL;
	PVOID pOpenEventObject = NULL;

	do
	{
		OBJECT_ATTRIBUTES objAttr = { 0 };
		UNICODE_STRING uNameString = { 0 };

		RtlInitUnicodeString(&uNameString, L"\\BaseNamedObjects\\TestEvent");
		InitializeObjectAttributes(&objAttr, &uNameString, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

		//TODO: 创建Event对象
		nStatus = ZwCreateEvent(&hCreateEvent, EVENT_ALL_ACCESS, &objAttr, SynchronizationEvent, FALSE);
		if (NULL == hCreateEvent)
		{
			DbgPrint("[dbg]: ZwCreateEvent Faild! STATUS=%x \n", nStatus);
			break;
		}
		nStatus = ObReferenceObjectByHandle(hCreateEvent, EVENT_ALL_ACCESS, *ExEventObjectType, KernelMode, &pCreateEventObject, NULL);
		if (NULL == pCreateEventObject)
		{
			DbgPrint("[dbg]: ZwCreateEvent of ObReferenceObjectByHandle Faild! STATUS=%x \n" , nStatus);
			break;
		}

		//TODO: 打开Event对象
		nStatus = ZwOpenEvent(&hOpenEvent, EVENT_ALL_ACCESS, &objAttr);
		if (NULL ==  hOpenEvent)
		{
			DbgPrint("[dbg]: ZwOpenEvent Faild! STATUS=%x \n", nStatus);
			break;
		}
		nStatus = ObReferenceObjectByHandle(hOpenEvent, EVENT_ALL_ACCESS, *ExEventObjectType, KernelMode, &pOpenEventObject, NULL);
		if (NULL == pOpenEventObject)
		{
			DbgPrint("[dbg]: ZwOpenEvent of ObReferenceObjectByHandle Faild! STATUS=%x \n", nStatus);
			break;
		}

		DbgPrint("[dbg]: Create Handle:%p, Create Pointer=%p\n", hCreateEvent, pCreateEventObject);
		DbgPrint("[dbg]: Open Handle:%p, Open Pointer=%p\n", hCreateEvent, pCreateEventObject);
		bSucc = TRUE;
	} while (FALSE);

	if (pCreateEventObject)
		ObDereferenceObject(pCreateEventObject);
	if (hCreateEvent)
		ZwClose(hCreateEvent);
	if (pOpenEventObject)
		ObDereferenceObject(pCreateEventObject);
	if (hOpenEvent)
		ZwClose(hOpenEvent);

	return bSucc;
}

BOOLEAN RegistryKeyOperationSample()
{

	PUNICODE_STRING pRegistryPath = g_psRegistryPath;
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	BOOLEAN bStatus = FALSE;

	HANDLE hKey = NULL;
	PKEY_VALUE_PARTIAL_INFORMATION pKeyDate = NULL;
	UNICODE_STRING usKeyValueName = { 0 };
	UNICODE_STRING usKeyValueText = { 0 };
	ULONG ulNewStartValue = 0;

	OBJECT_ATTRIBUTES ObjAttr = { 0 };
	ULONG ulDisposition = 0;

	InitializeObjectAttributes(&ObjAttr, pRegistryPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

	do
	{
		// 打开注册表
		nStatus = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &ObjAttr, 0, NULL, REG_OPTION_VOLATILE, &ulDisposition);
		if (!NT_SUCCESS(nStatus))
		{
			DbgPrint("[dbg]: Create RegistryKey Faild! STATUS=%x \n", nStatus);
			return FALSE;
		}

		// 修改注册表值
		ulNewStartValue = 2;
		RtlInitUnicodeString(&usKeyValueName, L"Start");
		nStatus = ZwSetValueKey(hKey, &usKeyValueName, 0, REG_DWORD, (PVOID)&ulNewStartValue, sizeof(ulNewStartValue));
		if (!NT_SUCCESS(nStatus))
		{
			DbgPrint("[dbg]: Set RegistryKey  \"Start\" Faild! STATUS=%x \n", nStatus);
			bStatus = FALSE;
			break;
		}
		RtlInitUnicodeString(&usKeyValueName, L"UserName");
		RtlInitUnicodeString(&usKeyValueText, L"Mohui");
		nStatus = ZwSetValueKey(hKey, &usKeyValueName, 0, REG_EXPAND_SZ, (PVOID)usKeyValueText.Buffer, usKeyValueText.Length);
		if (!NT_SUCCESS(nStatus))
		{
			DbgPrint("[dbg]: Set RegistryKey  \"UserName\" Faild! STATUS=%x \n", nStatus);
			bStatus = FALSE;
			break;
		}

		// 读取注册表,
		// 由于不知道键值的大小,先使用ZwQueryValueKey(,,KeyValuePartialInformation,NULL,0,&datasize) 读取键值信息
		// ZwQueryValueKey()函数会返回STATUS_BUFFER_TOO_SMALL空间不足,并且datasize会被赋值键值vaule所需要的内存大小
		RtlInitUnicodeString(&usKeyValueName, L"ImagePath");
		ulNewStartValue = 0;
		nStatus = ZwQueryValueKey(hKey, &usKeyValueName, KeyValuePartialInformation, NULL, 0, &ulNewStartValue);
		if (0 == ulNewStartValue || nStatus != STATUS_BUFFER_TOO_SMALL)
		{
			DbgPrint("[dbg]: ZwQueryValueKey Get key Information  Faild! STATUS=%x \n", nStatus);
			bStatus = FALSE;
			break;
		}

		pKeyDate = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool, ulNewStartValue, 'DriF');
		if (!pKeyDate)
		{
			DbgPrint("[dbg]: ExAllocatePoolWithTag Kye memory Faild! STATUS=%x \n", nStatus);
			bStatus = FALSE;
			break;
		}
		memset(pKeyDate, 0, ulNewStartValue);
		// 再次读取注册表
		nStatus = ZwQueryValueKey(hKey, &usKeyValueName, KeyValuePartialInformation, (PVOID)pKeyDate, ulNewStartValue, &ulNewStartValue);
		if (!NT_SUCCESS(nStatus))
		{
			DbgPrint("[dbg]: ZwQueryValueKey Read Kye Faild! STATUS=%x \n", nStatus);
			bStatus = FALSE;
			break;
		}

		DbgPrint("[dbg]: Registry:%wZ, ValueName:%wZ, Value:%ws dataLen=%d\n", pRegistryPath, &usKeyValueName, (PWCHAR)pKeyDate->Data, pKeyDate->DataLength);
		bStatus = TRUE;
	} while (FALSE);

	if (pKeyDate)
		ExFreePoolWithTag(pKeyDate, 'DriF');
	if(hKey)
		ZwClose(hKey);
	return bStatus;
}

BOOLEAN FileOperationSample()
{
	HANDLE hFileHandle = NULL;

	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	BOOLEAN bStatus = FALSE;
	OBJECT_ATTRIBUTES ObjAttr = { 0 };
	IO_STATUS_BLOCK ioStatus = { 0 };
	UNICODE_STRING SouFileName = RTL_CONSTANT_STRING(L"\\??\\C:\\TEST\\a.txt");
	UNICODE_STRING TarFileName = RTL_CONSTANT_STRING(L"\\??\\C:\\TEST\\b.txt");
	UNICODE_STRING usFileValue = { 0 };
	LARGE_INTEGER offset = { 0 };


	InitializeObjectAttributes(&ObjAttr, &SouFileName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

	// 以 FILE_OPEN_IF的方式打开,存在直接打开,不存在创建
	nStatus = ZwCreateFile(&hFileHandle, GENERIC_ALL, &ObjAttr, &ioStatus, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS, NULL, 0);
	if (!NT_SUCCESS(nStatus) || !hFileHandle)
	{
		DbgPrint("[dbg]: OpenFile is failed, PATH=%wZ\n", SouFileName);
		return FALSE;
	}
		
	
	do
	{
		// 写文件
		RtlInitUnicodeString(&usFileValue, L"hasidhkajshdkjsahdjkashdkjsahdjksahdjkas\nABCDEFG");
		nStatus = ZwWriteFile(hFileHandle, NULL, NULL, NULL, &ioStatus, usFileValue.Buffer, usFileValue.Length, &offset, NULL);
		if (!NT_SUCCESS(nStatus))
		{
			DbgPrint("[dbg]: Write is failed, PATH=%wZ\n", SouFileName);
			break;
		}
		
		// 如果不释放,将会独占,后面不能使用.原因 ?
		if (hFileHandle)
		{
			ZwClose(hFileHandle);
			hFileHandle = NULL;
		}
			
		nStatus = CopyFile(&TarFileName, &SouFileName);
		if (NT_SUCCESS(nStatus))
			bStatus = TRUE;


	} while (FALSE);
	if(hFileHandle)
		ZwClose(hFileHandle);
	return bStatus;
}

NTSTATUS CopyFile(PUNICODE_STRING TargerPath, PUNICODE_STRING SourcePath)
{
	HANDLE TargerHandle = NULL;
	HANDLE SourceHandle = NULL;
	PVOID pBuffer = NULL;

	ULONG BufferSize = 1024 * 4;
	ULONG DataLength = 0;
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER offset = { 0 };
	IO_STATUS_BLOCK ioStatus = { 0 };
	OBJECT_ATTRIBUTES tarObjAttr = { 0 };
	OBJECT_ATTRIBUTES souObjAttr = { 0 };

	InitializeObjectAttributes(&tarObjAttr, TargerPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	InitializeObjectAttributes(&souObjAttr, SourcePath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

	do
	{
		// 打开源文件,目标文件,申请内存
		nStatus = ZwCreateFile(&TargerHandle, GENERIC_ALL, &tarObjAttr, &ioStatus, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ , FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS, NULL, 0);
		if (!NT_SUCCESS(nStatus) || !TargerHandle)
		{
			DbgPrint("[dbg]: COPY File -> Targer File Open Faild!  STATUS=%x \n", nStatus);
			break;
		}
		nStatus = ZwCreateFile(&SourceHandle, GENERIC_ALL, &souObjAttr, &ioStatus, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS, NULL, 0);
		if (!NT_SUCCESS(nStatus) || !SourceHandle)
		{
			DbgPrint("[dbg]: COPY File -> Source File Open Faild!  STATUS=%x \n", nStatus);
			break;
		}
		pBuffer = ExAllocatePoolWithTag(PagedPool, BufferSize, 'BUFF');
		if (!pBuffer)
		{
			DbgPrint("[dbg]: ExAllocatePool Faild!  STATUS=%x \n", nStatus);
			break;
		}

		// 使用一个循环来读取文件和写入文件
		while (TRUE)
		{
			// 读取文件
			nStatus = ZwReadFile(SourceHandle, NULL, NULL, NULL, &ioStatus, pBuffer, BufferSize, &offset, NULL);
			if (!NT_SUCCESS(nStatus))
			{
				if (STATUS_END_OF_FILE == nStatus)	// 如果状态为STATUS_END_OF_FILE说明文件已经读取完了
					nStatus = TRUE;
				break;
			}
			DataLength = (ULONG)ioStatus.Information;

			//写入文件
			nStatus = ZwWriteFile(TargerHandle, NULL, NULL, NULL, &ioStatus, pBuffer, DataLength, &offset, NULL);
			if (!NT_SUCCESS(nStatus))
				break;

			//移动文件指针,直到读取时读到STATUS_END_OF_FILE
			offset.QuadPart += BufferSize;
		}
	} while (0);

	if (pBuffer)
		ExFreePoolWithTag(pBuffer, 'BUFF');
	if (SourceHandle)
		ZwClose(SourceHandle);
	if (TargerHandle)
		ZwClose(TargerHandle);
	return nStatus;
}


VOID ThreadCallback(PVOID Context)
{
	PKEVENT pKevnt = (PKEVENT)Context;
	DbgPrint("[dbg]: Print In My Thread \n");
	KeSetEvent(pKevnt, 0, TRUE);

	PsTerminateSystemThread(STATUS_SUCCESS);
}

BOOLEAN SyetemThreadSample()
{
	PUNICODE_STRING pStr = NULL;
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	HANDLE hThread = NULL;
	KEVENT eEvent = { 0 };

	pStr = ExAllocatePoolWithTag(PagedPool, sizeof(UNICODE_STRING), 'thr');
	if (!pStr)
	{
		DbgPrint("[dbg]: Thread  ExAllocatePoolWithTag Faild! \n");
		return FALSE;
	}
	RtlInitUnicodeString(pStr, L"Hello Thread");

	// 事件初始化,设置为TRUE,执行到等待函数将会阻塞
	KeInitializeEvent(&eEvent, SynchronizationEvent, TRUE);

	// 创建线程
	nStatus = PsCreateSystemThread(&hThread, 0, NULL, NULL, NULL, ThreadCallback, (PVOID)&eEvent);
	if (!NT_SUCCESS(nStatus))
	{
		// error:
		return FALSE;
	}

	// 时间初始化后就可以使用了,在一个函数中,可以等待某个事件, 如果这个事件没有被设置，
	// 那么就会阻塞在这里继续等待
	KeWaitForSingleObject(&eEvent, Executive, KernelMode, 0, 0);

	DbgPrint("[dbg]: KEVENT set TRUE, Wait is exit! \n");

	// 设置某个事件的阻塞取消 KeSetEvent(&eEvent, 0, FALSE);
	if (NULL == hThread)
		ZwClose(hThread);
	return TRUE;
}

BOOLEAN DeviceControlSample()
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING cdo_name = RTL_CONSTANT_STRING(CWK_COD_DEVICE_NAME);
	UNICODE_STRING cod_syb = RTL_CONSTANT_STRING(CWK_COD_SYB_NAME);
	PDEVICE_OBJECT pDeviceObject = NULL;
	
	//// 生成一个控制设备,射程符号链接
	//UNICODE_STRING sddl = RTL_CONSTANT_STRING(L"D:P(A;;GAGGGWD)");
	//GUID guid;
	//CoCreateGuid(&guid);
	//nStatus = IoCreateDeviceSecure(g_poDriverObject, 0, &cdo_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &sddl, (LPCGUID)SLBKGUID_CLASS_MYCDO, &g_cdo);
	//if (!NT_SUCCESS(nStatus))
	//{
	//	return nStatus;
	//}

	nStatus = IoCreateDevice(g_poDriverObject, 0, &cdo_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);
	if (!NT_SUCCESS(nStatus))
	{
		DbgPrint("[dbg]: Create Kennel Device faild! STATUS=%x \n", nStatus);
		return FALSE;
	}

	// 在创建符号链接前,尝试先删除再创建新的符号链接
	IoDeleteSymbolicLink(&cod_syb);
	nStatus = IoCreateSymbolicLink(&cod_syb, &cdo_name);
	if (!NT_SUCCESS(nStatus))
	{
		DbgPrint("[dbg]: Create Kennel SymbolicLink faild! STATUS=%x \n", nStatus);
		IoDeleteDevice(pDeviceObject);
		return FALSE;
	}
	DbgPrint("[dbg]: Create Kennel SymbolicLink Success! sybName=%wZ  Device=%wZ\n", &cod_syb, &cdo_name);
	g_demo_cdo = pDeviceObject;
	return TRUE;
}

NTSTATUS cwkDispatchDemo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	PVOID pBuffer = NULL;
	ULONG ulInLen = 0;
	ULONG ulOutLen = 0;
	ULONG ulControlCode = 0;
	ULONG ulRetlen = 0;

	do
	{
		// 判断请求是否给之前生成的控制设备,如果不是,直接不处理
		if (DeviceObject != g_demo_cdo)
		{
			DbgPrint("[dbg]: Kennel Recv Device is no Demo\n");
			nStatus = STATUS_INVALID_DEVICE_OBJECT_PARAMETER;
			break;
		}
		// 判断请求种类是不是关闭和打开
		if (irpsp->MajorFunction == IRP_MJ_CREATE || irpsp->MajorFunction == IRP_MJ_CLOSE)
		{
			DbgPrint("[dbg]: Kennel Device Demo  is Open or Close \n");
			nStatus = STATUS_SUCCESS;
			break;
		}

		if (irpsp->MajorFunction == IRP_MJ_DEVICE_CONTROL)
		{
			pBuffer = Irp->AssociatedIrp.SystemBuffer;
			ulInLen = irpsp->Parameters.DeviceIoControl.InputBufferLength;
			ulOutLen = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
			ulControlCode = irpsp->Parameters.DeviceIoControl.IoControlCode;
			switch (ulControlCode)
			{
			case CWK_DVC_SEND_STR:
				ASSERT(pBuffer != NULL);
				ASSERT(ulInLen > 0 && ulInLen<254);		// 限定缓冲区长度
				ASSERT(ulOutLen == 0);
				DbgPrint("[dbg]: 3 huang Send string: %ws \n", (wchar_t*)pBuffer);
				nStatus = STATUS_SUCCESS;
				break;

			case CWK_DVC_RECV_STR:
			default:
				nStatus = STATUS_INVALID_PARAMETER;
				break;
			}
		}
	} while (FALSE);
	Irp->IoStatus.Information = ulRetlen;
	Irp->IoStatus.Status = nStatus;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return nStatus;
}

VOID ccpUnloadDemo()
{
	UNICODE_STRING cdo_syb = RTL_CONSTANT_STRING(CWK_COD_SYB_NAME);
	if (g_demo_cdo)
	{
		DbgPrint("[dbg:%ws]Driver Unload, Delete  Demo Device \n", __FUNCTIONW__);
		ASSERT(g_demo_cdo != NULL);
		IoDeleteSymbolicLink(&cdo_syb);
		IoDeleteDevice(g_demo_cdo);
	}

}
