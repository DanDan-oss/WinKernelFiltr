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


/*	UNICODE_STRING 操作*/
void StringOperate(void)
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
void ListOperate(void)
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

void SpinLockOperate(void)
{
	
	return;
}