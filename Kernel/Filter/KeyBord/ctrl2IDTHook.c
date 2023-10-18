#include "ctrl2IDTHook.h"

void* g_p2c_old = NULL;

extern VOID* GetIdtBase(PP2C_IDTR64 idtr);	// 使用sidt指令读取P2C_IDTR结构,并返回IDT地址
extern p2cInterrupProc(); // 接管int 0x93中断的函数

void*  GetIdtIsr(unsigned int Index)
{
	P2C_IDTR idtr = {0};
	PP2C_IDT_ENTRY idt_addr = NULL;
	PP2C_IDTR pIdtr = GetIdtBase(&idtr);
	VOID* isr_func = NULL;
	idt_addr = (PP2C_IDT_ENTRY)P2C_MAKELONG(*((PUINT64)((char*)pIdtr + 2)), *((PUINT64)((char*)pIdtr + 6)));
	//idt_addr = (PP2C_IDT_ENTRY)(((unsigned long long int)idt_addr) + (Index * sizeof(P2C_IDT_ENTRY)));
	idt_addr += Index;
	isr_func = (VOID*)P2C_IDTR_OF_ISR(idt_addr->offset_high, idt_addr->OffsetMiddle, idt_addr->offset_low);

	KdPrint(("[dbg]: idt %x  address=%p ,isr function address=%p\n", Index, idt_addr, isr_func));
	return NULL;
}

void _stdcall p2cHookInt93(BOOLEAN hook_or_unhook)
{
	P2C_IDTR idtr = { 0 };
	PP2C_IDT_ENTRY idt_addr = NULL;
	PP2C_IDTR pIdtr = GetIdtBase(&idtr);

	UNREFERENCED_PARAMETER(hook_or_unhook);

	idt_addr = (PP2C_IDT_ENTRY)P2C_MAKELONG(*((PUINT64)((char*)pIdtr + 2)), *((PUINT64)((char*)pIdtr + 6)));
	idt_addr += 0x93;
	KdPrint(("[dbg]: p2c The current address =%p.\r\n", (VOID*)P2C_IDTR_OF_ISR(idt_addr->offset_high, idt_addr->OffsetMiddle, idt_addr->offset_low) ));
	//if (hook_or_unhook)
	//{
	//	// 进行HOOK
	//	KdPrint(("[dbg]: p2c try to hook intereupt.\r\n"));
	//	g_p2c_old = (void*)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high);
	//	idt_addr->offset_low = P2C_LOW16_OF_32(p2cInterrupProc);
	//	idt_addr->offset_high = P2C_HIGH16_OF_32(p2cInterrupProc);
	//}
	//else
	//{
	//	// 取消HOOK
	//	KdPrint(("[dbg]: p2c try  recovery intereupt.\r\n"));
	//	idt_addr->offset_low = P2C_LOW16_OF_32(g_p2c_old);
	//	idt_addr->offset_high = P2C_LOW16_OF_32(g_p2c_old);
	//	g_p2c_old = NULL;
	//}
	KdPrint(("[dbg]: p2c The current address =%p\r\n", (void*)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high)));
	return;

}
void _stdcall p2cUserFilter()
{
	// 首先读取端口获得按键扫描码打印出来,然后将这个扫描码写回端口. 以使其他的应用程序能正确接收到按键
	// 如采不想让其他的程序截获按键,则可以写回一个任意的数据

	//static P2C_U8 sch_pre = 0;
	//P2C_U8 sch = 0;
	//p2cWaitForkbRead();
	//__asm in al, 0x60
	//__asm mov sch, al
	//KdPrint(("[dbg]: p2c scan code = %2x\n", sch));
	// 把数据写回端口,以便让其他程序可以正确读取

	//if (sch_pre != sch)
	//{
	//	sch_pre = sch;
	//	__asm mov al, 0xd2
	//	__asm out 0x64, al
	//	p2cWaitForkbWrite();
	//	__asm mov al, sch
	//	__asm out 0x60, al
	//}
}

ULONG _stdcall p2cWaitForkbRead()
{
	//int i = 0x64;
	//P2C_U8 chBuf = 0;

	//do
	//{
	//	__asm in al, 0x64
	//	__asm mov chBuf, al
	//	KeStallExecutionProcessor(50);
	//	if (!(chBuf & OBUFFER_FULL))
	//		break;
	//} while (--i);
	//if (i)
	//	return TRUE;
	//return FALSE;


	return TRUE;
}

ULONG _stdcall p2cWaitForkbWrite()
{
	//int i = 0x64;
	//P2C_U8 chBuf = 0;

	//do
	//{
	//	__asm in al, 0x64
	//	__asm mov chBuf, al
	//	KeStallExecutionProcessor(50);
	//	if (!(chBuf & IBUFFER_FULL))
	//		break;
	//} while (--i);
	//if (i)
	//	return TRUE;
	//return FALSE;

	return TRUE;
}

