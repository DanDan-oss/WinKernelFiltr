#include <ntifs.h>
#include <ntddk.h>
#include <Ntstrsafe.h>
#include "demo.h"

// TARGETLIBS=$(DDK_LIB_PATH)\ntstrsafe.lib

extern PDRIVER_OBJECT g_poDriverObject;
extern PUNICODE_STRING g_psRegistryPath;

void DemoMain()
{
	StringOperationSample();
	ListOperationSample();
	SpinLockOperationSample();
	MemoryOperationSample();
	EventOperationSample();
	RegistryKeyOperationSample();

	KdBreakPoint();
}


/*	UNICODE_STRING 操作*/
void StringOperationSample(void)
{
	/*
	typedef struct _UNICODE_STRING {
		USHORT Length;   //有效字符串的长度（字节数）
		USHORT MaximumLength; //字符串的最大长度（字节数）
		PWSTR Buffer; //指向字符串的指针
	}UNICODE_STRING,*PUNICODE_STRING;
	
	typedef STRING
	{
		USHORT Length;   //有效字符串的长度（字节数）
		USHORT MaximumLength; //字符串的最大长度（字节数）
		PSTR Buffer; //指向字符串的指针
	}ANSI_STRING

	
	VOID RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);	// 初始化字符串结构,并没有为DestinationString.Buffer分配内存,会直接常量	
	VOID RtlInitEmptyUnicodeString(PUNICODE_STRING UnicodeString, PWCHAR Buffer, USHORT BufferSize);	//// 初始化一个空的UnicodeString,将他指向Buffer

	VOID RtlUnicodeStringCopyString(PUNICODE_STRING DestinationString, NTSTRSAFE pszSrc);		// 拷贝
	NTSTATUS RtlAppendUnicodeToString(PUNICODE_STRING DestinationString, PCWSTR SourceString);	// 将UNICODE_STRING字符串追加拷贝到Buffer,Buffer不能指向NULL

	RtlCompareString(const PANSI_STRING String1, const  PANSI_STRING String2, BOOLEAN CaseInSensitive);			// ANSI_STRING 字符串比较
	RtlCompareUnicodeString(PUNICODE_STRING String1,PUNICODE_STRING String2, BOOLEAN CaseInsensitive);			// UNICODE_STRING字符串比较

	NTSTRSAFEDDI RtlStringCchPrintfA(NTSTRSAFE_PSTR pszDest, cchDest, NTSTRSAFE_PCSTR pszFormat, ...);			// 格式化字符串类似于 :sprintf()

	*/
	
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
	
	/*
	typedef struct _LIST_ENTRY
	{
		struct _LIST_ENTRY* Flink;	// 指向后一个节点
		struct _LIST_ENTRY* Blink;	// 指向前一个节点
	}LIST_ENTRY, *PLIST_ENTRY;

	CONTAINING_RECORD(Address, type, Field);	// 宏函数,由于 Entry->Flink并不是直接指向下一个节点的结构起始位置,而是LIST_ENTRY的位置,需要使用宏函数转换
	BOOLEAN IsListEmpty(const PLIST_ENTRY ListHead);		// 判断是不是空链表,TRUE表示空链表,FALSE表示链表非空
	VOID InitializeListHead(_Out_ PLIST_ENTRY ListHand);	// 初始化头节点, 原型 ListHeader->Flink = ListHeader->Blink = PListHeader;

	VOID InsertHeadList(PLIST_ENTRY ListHand, PLIST_ENTRY Entry);	// 头插法,头节点的后一个节点
	VOID InsertTailList(PLIST_ENTRY ListHand, PLIST_ENTRY Entry);	// 尾插法

	PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHand);				// 移除头指针指向的第一个元素,移除成功返回移除的节点的指针,移除失败返回NULL
	PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHand);				// 移除最后一个元素,移除成功返回移除的节点的指针,移除失败返回NULL
	BOOLEAN RemoveEntryList(PLIST_ENTRY Entry);						// 移除某个特定的节点
	*/


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
	/* 
	typedef ULONG_PTR KSPIN_LOCK;		// 自旋锁类型, 本质指针类型
	
	void KeInitializeSpinLock(PKSPIN_LOCK SpinLock);		// 初始化自旋锁
	void KeAcquireSpinLock(SpinLock,OldIrql);				// 宏函数,通过引发IRQL使自旋锁上锁
		// 注意KSPIN_LOCK变量不能写成局部变量,因为这样每个线程都会有一个自己的KSPIN_LOCK，这个就会变的没有意义,应该写成全局使大家共用这个函数

	VOID KeReleaseSpinLock (PKSPIN_LOCK SpinLock, KIRQL NewIrql);	// 释放自旋锁,还原正在运行的原始IRQL

	PLIST_ENTRY ExInterlockedInsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY ListEntry,PKSPIN_LOCK Lock);		// 使用自旋锁的方式往链表中添加节点,返回插入成功节点的指针
	PLIST_ENTRY ExInterlockedRemoveHeadList(PLIST_ENTRY ListHead, PKSPIN_LOCK Lock);	// 用自旋锁的方式移除链表中第一个节点,成功将返回移除的节点指针返回

	// 队列自旋锁
	typedef struct _KLOCK_QUEUE_HANDLE {
	KSPIN_LOCK_QUEUE LockQueue;
	KIRQL OldIrql;
	} KLOCK_QUEUE_HANDLE, *PKLOCK_QUEUE_HANDLE;

	void KeAcquireInStackQueuedSpinLock(PKSPIN_LOCK SpinLock, PKLOCK_QUEUE_HANDLE LockHandle.);	// 获取队列自旋锁
	void KeReleaseInStackQueuedSpinLock(PKLOCK_QUEUE_HANDLE LockHandle);	// 释放由KeAcquireInStackQueuedSpinLock获取的队列自旋锁
	
	*/

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
	/*
	typedef enum _POOL_TYPE
	{
		NonPagedPool,
		NonPagedPoolExecute,
		PagedPool, 
		NonPagedPoolNx,
		...
	}POOL_TYPE;


	PVOID ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfButes, ULONG Tag);	//分配指定类型的池内存，并返回指向已分配块的指针

	PVOID (*PALLOCATE_FUNCTION)(POLL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);	// 旁视列表申请内存回调函数原型
	VOID (*PFREE_FUNCTION)(PVOID Buffer);					// 旁视列表释放内存回调函数原型
	void ExInitializeNPagedLookasideListP(PNPAGED_LOOKASIDE_LIST Lookaside, PALLOCATE_FUNCTION Allocate, PFREE_FUNCTION Free, ULONG Flags, SIZE_T Size, ULONG Tag, USHORT Depth);	// 初始化旁视列表内存管理对象,Allocate和Free设置为NULL时,操作系统会使用默认的

	NTSTATUS ExInitializeLookasideListEx(PLOOKASIDE_LIST_EX lookaside, PALLOCATE_FUNCTION Allocate, PFREE_FUNCTION_EX Free, POOL_TYPE PoolType, ULONG Flags, SIZE_T Size, ULONG Tag, USHORT Depath); 
	*/

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

		DbgPrint("[dbg]: ReAlloc First: %p, Second:%p\n", pFirstMemory);

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

