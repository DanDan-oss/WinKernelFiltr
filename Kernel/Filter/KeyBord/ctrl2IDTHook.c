#include "ctrl2IDTHook.h"

void* g_p2c_old = NULL;

extern VOID* p2cGetIdt(int offset);	// ʹ��sidtָ���ȡP2C_IDTR�ṹ,������IDT��ַ
extern __declspec(naked) p2cInterrupProc(); // �ӹ�int 0x93�жϵĺ���
extern  _stdcall VOID p2cUserFilter();

void p2cHookInt93(BOOLEAN hook_or_unhook)
{

	P2C_IDTR idtr = { 0 };
	void* address = p2cGetIdt((int)((PCHAR)(&idtr + 1) - (PCHAR)&(idtr.base)  ));
	PP2C_IDT_ENTRY idt_addr = (PP2C_IDT_ENTRY)address;
	idt_addr += 0x93;

	KdPrint(("[dbg]: p2c The current address =%x\r\n", (void*)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high)));
	if (hook_or_unhook)
	{
		// ����HOOK
		KdPrint(("[dbg]: p2c try to hook intereupt.\r\n"));
		g_p2c_old = address;
		idt_addr->offset_low = P2C_LOW16_OF_32(p2cInterrupProc);
		idt_addr->offset_high = P2C_HIGH16_OF_32(p2cInterrupProc);
	}
	else
	{
		// ȡ��HOOK
		KdPrint(("[dbg]: p2c try  recovery intereupt.\r\n"));
		idt_addr->offset_low = P2C_LOW16_OF_32(g_p2c_old);
		idt_addr->offset_high = P2C_LOW16_OF_32(g_p2c_old);
		g_p2c_old = NULL;
	}
	KdPrint(("[dbg]: p2c The current address =%x\r\n", (void*)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high)));
	return;
}

void _stdcall p2cUserFilter()
{
	KdPrint(("[dbg]: ���Լ����ж�"));
}