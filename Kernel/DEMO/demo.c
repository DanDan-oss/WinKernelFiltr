#include "demo.h"
#include <ntddk.h>
#include <Ntstrsafe.h>

// TARGETLIBS=$(DDK_LIB_PATH)\ntstrsafe.lib

void DemoMain()
{
	StringOperate();
	ListOperate();

	KdBreakPoint();
}


/*	UNICODE_STRING ����*/
void StringOperate(void)
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
void ListOperate(void)
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



void SpinLockOperate(void)
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

	static KSPIN_LOCK my_list_lock = { 0 };		//�������,д��ȫ�ֱ������߾�̬����
	KIRQL irql = 0;
	UNREFERENCED_PARAMETER(irql);

	LIST_ENTRY my_list_head = { 0 };			// ����ͷ
	FILE_INFO my_file_info = { 0 };


	KeInitializeSpinLock(&my_list_lock);
	ExInterlockedInsertHeadList(&my_list_head, &my_file_info.m_ListEntry, &my_list_lock);

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

void MemoryOperate(void)
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
		ExFreeToNPagedLookasideList(pLookAsideList, pFirstMemory);
		pFirstMemory = NULL;
	}
	if (bInit && pLookAsideList)
	{
		ExDeleteNPagedLookasideList(pLookAsideList);
		bInit = FALSE;
	}
	if (pLookAsideList)
	{
		ExFreePoolWithTag(pLookAsideList, 'a');
		pLookAsideList = NULL;
	}
	return;
}