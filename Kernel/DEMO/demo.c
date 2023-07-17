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

	FileOperationSample();

	KdBreakPoint();
}


/*	UNICODE_STRING ����*/
void StringOperationSample(void)
{
	/*
	typedef struct _UNICODE_STRING {
		USHORT Length;   //��Ч�ַ����ĳ��ȣ��ֽ�����
		USHORT MaximumLength; //�ַ�������󳤶ȣ��ֽ�����
		PWSTR Buffer; //ָ���ַ�����ָ��
	}UNICODE_STRING,*PUNICODE_STRING;
	
	typedef STRING
	{
		USHORT Length;   //��Ч�ַ����ĳ��ȣ��ֽ�����
		USHORT MaximumLength; //�ַ�������󳤶ȣ��ֽ�����
		PSTR Buffer; //ָ���ַ�����ָ��
	}ANSI_STRING

	
	VOID RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);	// ��ʼ���ַ����ṹ,��û��ΪDestinationString.Buffer�����ڴ�,��ֱ�ӳ���	
	VOID RtlInitEmptyUnicodeString(PUNICODE_STRING UnicodeString, PWCHAR Buffer, USHORT BufferSize);	//// ��ʼ��һ���յ�UnicodeString,����ָ��Buffer

	VOID RtlUnicodeStringCopyString(PUNICODE_STRING DestinationString, NTSTRSAFE pszSrc);		// ����
	NTSTATUS RtlAppendUnicodeToString(PUNICODE_STRING DestinationString, PCWSTR SourceString);	// ��UNICODE_STRING�ַ���׷�ӿ�����Buffer,Buffer����ָ��NULL

	RtlCompareString(const PANSI_STRING String1, const  PANSI_STRING String2, BOOLEAN CaseInSensitive);			// ANSI_STRING �ַ����Ƚ�
	RtlCompareUnicodeString(PUNICODE_STRING String1,PUNICODE_STRING String2, BOOLEAN CaseInsensitive);			// UNICODE_STRING�ַ����Ƚ�

	NTSTRSAFEDDI RtlStringCchPrintfA(NTSTRSAFE_PSTR pszDest, cchDest, NTSTRSAFE_PCSTR pszFormat, ...);			// ��ʽ���ַ��������� :sprintf()

	*/
	
	// �ַ�����ʼ��
	UNICODE_STRING uFirstString1 = { 0 };
	RtlInitUnicodeString(&uFirstString1, L"Hello Kernel String struct UNICODE_STRING\n");	// ��û��ΪuFirstString1.Buffer�����ַ���,ֱ������ָ����
	DbgPrint("[dbg]: Length=%d MaximumLength=%d uFirstString1=\"%wZ \" 1" , uFirstString1.Length, uFirstString1.MaximumLength, &uFirstString1);

	WCHAR strBuffer2[] = L"Hello Kernel String\n";
	UNICODE_STRING uFirstString2 = { 0 };
	RtlInitUnicodeString(&uFirstString2, strBuffer2);			// uFirstString2ָ����strBuffer2,���ʱ��ı�strBuffer2�ַ������ֵ,uFirstString2Ҳ����Ÿı�
	strBuffer2[0] = L'W';
	DbgPrint("[dbg]: uFirstString2=\"%wZ \" 2", &uFirstString2);

	
	// �ַ�������
	WCHAR strBuffer3[128] = { 0 };
	UNICODE_STRING uFirstString3 = { 0 };
	RtlInitEmptyUnicodeString(&uFirstString3, strBuffer3, sizeof(strBuffer3));
		// ���������Ҫʹ��#include <Ntstrsafe.h> TARGETLIBS=$(DDK_LIB_PATH)\ntstrsafe.lib, �������ֻ����IRQL PASSIVELEVEL��ʹ��
	RtlUnicodeStringCopyString(&uFirstString3, L"Hello Kernel String struct 3\n");
	DbgPrint("[dbg]: uFirstString3=\"%wZ \" ", &uFirstString3);

}



/* LIST_ENTRY ����ṹ���� */
void ListOperationSample(void)
{
	
	/*
	typedef struct _LIST_ENTRY
	{
		struct _LIST_ENTRY* Flink;	// ָ���һ���ڵ�
		struct _LIST_ENTRY* Blink;	// ָ��ǰһ���ڵ�
	}LIST_ENTRY, *PLIST_ENTRY;

	CONTAINING_RECORD(Address, type, Field);	// �꺯��,���� Entry->Flink������ֱ��ָ����һ���ڵ�Ľṹ��ʼλ��,����LIST_ENTRY��λ��,��Ҫʹ�ú꺯��ת��
	BOOLEAN IsListEmpty(const PLIST_ENTRY ListHead);		// �ж��ǲ��ǿ�����,TRUE��ʾ������,FALSE��ʾ����ǿ�
	VOID InitializeListHead(_Out_ PLIST_ENTRY ListHand);	// ��ʼ��ͷ�ڵ�, ԭ�� ListHeader->Flink = ListHeader->Blink = PListHeader;

	VOID InsertHeadList(PLIST_ENTRY ListHand, PLIST_ENTRY Entry);	// ͷ�巨,ͷ�ڵ�ĺ�һ���ڵ�
	VOID InsertTailList(PLIST_ENTRY ListHand, PLIST_ENTRY Entry);	// β�巨

	PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHand);				// �Ƴ�ͷָ��ָ��ĵ�һ��Ԫ��,�Ƴ��ɹ������Ƴ��Ľڵ��ָ��,�Ƴ�ʧ�ܷ���NULL
	PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHand);				// �Ƴ����һ��Ԫ��,�Ƴ��ɹ������Ƴ��Ľڵ��ָ��,�Ƴ�ʧ�ܷ���NULL
	BOOLEAN RemoveEntryList(PLIST_ENTRY Entry);						// �Ƴ�ĳ���ض��Ľڵ�
	*/


	// ʹ���Զ���ڵ�ṹǶ��ϵͳLIST_ENTRY
	typedef struct _TestListEntery
	{
		ULONG m_ulDataA;
		ULONG m_ulDataB;
		LIST_ENTRY m_ListEntry;		// ����Blinkָ��ǰһ���ڵ��LIST_ENTRY�ṹ,Flinkָ���һ���ڵ�LIST_ENTRY
		ULONG m_ulDataC;
		ULONG m_ulDataD;
	}TestListEntery, * PTestListEntery;


	LIST_ENTRY ListHeader = { 0 };		// ͷ�ڵ�
	TestListEntery EnteryA = { 0 };
	TestListEntery EnteryB = { 0 };
	TestListEntery EnteryC = { 0 };

	EnteryA.m_ulDataA = 'A';
	EnteryB.m_ulDataA = 'B';
	EnteryC.m_ulDataA = 'C';

	InitializeListHead(&ListHeader);	

	// �ڵ����
	InsertHeadList(&ListHeader, &EnteryA.m_ListEntry);	
	InsertHeadList(&ListHeader, &EnteryB.m_ListEntry);
	InsertTailList(&ListHeader, &EnteryC.m_ListEntry);

	// �ڵ��Ƴ�
	RemoveHeadList(&ListHeader);

	// �������
	DbgPrint("[dbg]: start Ergodic List\n");
	PLIST_ENTRY pListEntery = ListHeader.Flink;
	while (pListEntery != &ListHeader)
	{
	/* ���� pListEntery->Flink������ֱ��ָ����һ���ڵ�Ľṹͷ,����LIST_ENTRY��λ��,��Ҫʹ�� CONTAINING_RECORD(Address, type, Field) ת����pTestEntryָ��ṹͷ��ַ */
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
	typedef ULONG_PTR KSPIN_LOCK;		// ����������, ����ָ������
	
	void KeInitializeSpinLock(PKSPIN_LOCK SpinLock);		// ��ʼ��������
	void KeAcquireSpinLock(SpinLock,OldIrql);				// �꺯��,ͨ������IRQLʹ����������
		// ע��KSPIN_LOCK��������д�ɾֲ�����,��Ϊ����ÿ���̶߳�����һ���Լ���KSPIN_LOCK������ͻ���û������,Ӧ��д��ȫ��ʹ��ҹ����������

	VOID KeReleaseSpinLock (PKSPIN_LOCK SpinLock, KIRQL NewIrql);	// �ͷ�������,��ԭ�������е�ԭʼIRQL

	PLIST_ENTRY ExInterlockedInsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY ListEntry,PKSPIN_LOCK Lock);		// ʹ���������ķ�ʽ����������ӽڵ�,���ز���ɹ��ڵ��ָ��
	PLIST_ENTRY ExInterlockedRemoveHeadList(PLIST_ENTRY ListHead, PKSPIN_LOCK Lock);	// ���������ķ�ʽ�Ƴ������е�һ���ڵ�,�ɹ��������Ƴ��Ľڵ�ָ�뷵��

	// ����������
	typedef struct _KLOCK_QUEUE_HANDLE {
	KSPIN_LOCK_QUEUE LockQueue;
	KIRQL OldIrql;
	} KLOCK_QUEUE_HANDLE, *PKLOCK_QUEUE_HANDLE;

	void KeAcquireInStackQueuedSpinLock(PKSPIN_LOCK SpinLock, PKLOCK_QUEUE_HANDLE LockHandle.);	// ��ȡ����������
	void KeReleaseInStackQueuedSpinLock(PKLOCK_QUEUE_HANDLE LockHandle);	// �ͷ���KeAcquireInStackQueuedSpinLock��ȡ�Ķ���������
	
	*/

	// TODO: �������������ʹ��
	typedef struct _FILE_INFO
	{
		LIST_ENTRY m_ListEntry;
		WCHAR m_strFileNmae[260];
	}FILE_INFO, *PFILE_INFO;

	KIRQL irql = 0;
	UNREFERENCED_PARAMETER(irql);

	LIST_ENTRY my_list_head = { 0 };			// ����ͷ
	FILE_INFO my_file_info = { 0 };
	KSPIN_LOCK my_list_lock = { 0 };		//�������,д��ȫ�ֱ������߾�̬����

	InitializeListHead(&my_list_head);
	ExInterlockedInsertHeadList(&my_list_head, (PLIST_ENTRY)&my_file_info, &my_list_lock);

	// �������
	PLIST_ENTRY pListEntery = my_list_head.Flink;
	while (pListEntery != &my_list_head)
	{
		/* ���� pListEntery->Flink������ֱ��ָ����һ���ڵ�Ľṹͷ,����LIST_ENTRY��λ��,��Ҫʹ�� CONTAINING_RECORD(Address, type, Field) ת����pTestEntryָ��ṹͷ��ַ */
		PFILE_INFO pTestEntry = CONTAINING_RECORD(pListEntery, FILE_INFO, m_ListEntry);
		if (!pTestEntry)
			break;
		DbgPrint("[dbg]: FileName= %ws \n", pTestEntry->m_strFileNmae);
		pListEntery = pListEntery->Flink;
	}

	ExInterlockedRemoveHeadList(&my_list_head, &my_list_lock);
	// ����������
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


	PVOID ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfButes, ULONG Tag);	//����ָ�����͵ĳ��ڴ棬������ָ���ѷ�����ָ��

	PVOID (*PALLOCATE_FUNCTION)(POLL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);	// �����б������ڴ�ص�����ԭ��
	VOID (*PFREE_FUNCTION)(PVOID Buffer);					// �����б��ͷ��ڴ�ص�����ԭ��
	void ExInitializeNPagedLookasideListP(PNPAGED_LOOKASIDE_LIST Lookaside, PALLOCATE_FUNCTION Allocate, PFREE_FUNCTION Free, ULONG Flags, SIZE_T Size, ULONG Tag, USHORT Depth);	// ��ʼ�������б��ڴ�������,Allocate��Free����ΪNULLʱ,����ϵͳ��ʹ��Ĭ�ϵ�

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

		// ��ʼ�������б����
		ExInitializeNPagedLookasideList(pLookAsideList, NULL, NULL, 0, 128, 'a', 0);
		bInit = TRUE;

		if(NULL == (pFirstMemory = ExAllocateFromNPagedLookasideList(pLookAsideList)))
			break;
		if (NULL == (pSecondMemory = ExAllocateFromNPagedLookasideList(pLookAsideList)))
			break;

		DbgPrint("[dbg]: First: %p, Second:%p\n", pFirstMemory, pSecondMemory);

		// �ͷ�pFirstMemory
		ExFreeToNPagedLookasideList(pLookAsideList, pFirstMemory);

		// �ٴη���,�鿴�ڴ��Ƿ��ǴӸ�ͨ�������б��ͷŵ��ڴ�
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
	}ACCESS_MASK; // EVENT��Ȩ��

	typedef enum _EVENT_TYPE {
		SynchronizationEvent, NotificationEvent, ...
	}EVENT_TYPE; // EVENT����

	typedef enum _ObjectType	//ָ��������͵�ָ�� 
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
		// ��� ObjectType ���� NULL�������ϵͳ����֤�ṩ�Ķ��������Ƿ��� Handle ָ���Ķ���Ķ�������ƥ�� 
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

		//TODO: ����Event����
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

		//TODO: ��Event����
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
		// ��ע���
		nStatus = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &ObjAttr, 0, NULL, REG_OPTION_VOLATILE, &ulDisposition);
		if (!NT_SUCCESS(nStatus))
		{
			DbgPrint("[dbg]: Create RegistryKey Faild! STATUS=%x \n", nStatus);
			return FALSE;
		}

		// �޸�ע���ֵ
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

		// ��ȡע���,
		// ���ڲ�֪����ֵ�Ĵ�С,��ʹ��ZwQueryValueKey(,,KeyValuePartialInformation,NULL,0,&datasize) ��ȡ��ֵ��Ϣ
		// ZwQueryValueKey()�����᷵��STATUS_BUFFER_TOO_SMALL�ռ䲻��,����datasize�ᱻ��ֵ��ֵvaule����Ҫ���ڴ��С
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
		// �ٴζ�ȡע���
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

	// �� FILE_OPEN_IF�ķ�ʽ��,����ֱ�Ӵ�,�����ڴ���
	nStatus = ZwCreateFile(&hFileHandle, GENERIC_ALL, &ObjAttr, &ioStatus, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS, NULL, 0);
	if (!NT_SUCCESS(nStatus) || !hFileHandle)
	{
		DbgPrint("[dbg]: OpenFile is failed, PATH=%wZ\n", SouFileName);
		return FALSE;
	}
		
	
	do
	{
		// д�ļ�
		RtlInitUnicodeString(&usFileValue, L"hasidhkajshdkjsahdjkashdkjsahdjksahdjkas\nABCDEFG");
		nStatus = ZwWriteFile(hFileHandle, NULL, NULL, NULL, &ioStatus, usFileValue.Buffer, usFileValue.Length, &offset, NULL);
		if (!NT_SUCCESS(nStatus))
		{
			DbgPrint("[dbg]: Write is failed, PATH=%wZ\n", SouFileName);
			break;
		}
		
		// ������ͷ�,�����ռ,���溯������ʹ��.ԭ�� ?
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
		// ��Դ�ļ�,Ŀ���ļ�,�����ڴ�
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

		// ʹ��һ��ѭ������ȡ�ļ���д���ļ�
		while (TRUE)
		{
			// ��ȡ�ļ�
			nStatus = ZwReadFile(SourceHandle, NULL, NULL, NULL, &ioStatus, pBuffer, BufferSize, &offset, NULL);
			if (!NT_SUCCESS(nStatus))
			{
				if (STATUS_END_OF_FILE == nStatus)	// ���״̬ΪSTATUS_END_OF_FILE˵���ļ��Ѿ���ȡ����
					nStatus = TRUE;
				break;
			}
			DataLength = (ULONG)ioStatus.Information;

			//д���ļ�
			nStatus = ZwWriteFile(TargerHandle, NULL, NULL, NULL, &ioStatus, pBuffer, DataLength, &offset, NULL);
			if (!NT_SUCCESS(nStatus))
				break;

			//�ƶ��ļ�ָ��,ֱ����ȡʱ����STATUS_END_OF_FILE
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

