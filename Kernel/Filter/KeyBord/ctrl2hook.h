// ���̹��ˣ�ͨ��HOOK������������ķַ�����ָ�����
/*
	KbdClass����Ϊ'����������'��Windows��,������ͨ����ָͳ��һ���豸����������.
������USB����,����PS��2���̾�������,���������������,�ܻ�úܺõ�ͨ����.
��������֮�º�ʵ�ʹ�����������������Ϊ"�˿�����". ���嵽����,j8042prt��PS/2���̵Ķ˿�����,USB��������Kbdhid
*/

#ifndef _CTRL2HOOK_H
#define _CTRL2HOOK_H

#include <ntddk.h>


typedef struct _KEYBOARD_INPUT_DATA
{
    USHORT UnitId;    // ����\\DEVICE\\KeyboardPort0���ֵ��0,\\DEVICE\\KeyboardPort1���ֵ��1,��������
    USHORT MakeCode;    // ɨ����
    USHORT Flags;        // ��־,��־һ�������»��ǵ���
    USHORT Reserved;        // ����
    ULONG ExtraInformation; // ��չ��Ϣ
}KEYBOARD_INPUT_DATA, * PKEYBOARD_INPUT_DATA;

typedef VOID(__stdcall* KeyBoardClassServiceCallBack)(IN PDEVICE_OBJECT DeviceObject, IN PKEYBOARD_INPUT_DATA InputDataStart, IN PKEYBOARD_INPUT_DATA InputDataEnd, IN OUT PULONG InputDataConsumed);
typedef struct _KBD_CALLBACK
{
    PDEVICE_OBJECT classDeviceObject;
    KeyBoardClassServiceCallBack ServiceCallBack;
    PVOID DevExtCallBackAddress;
}KBD_CALLBACK, *PKBD_CALLBACK;


/* ��ں��� */
NTSTATUS Ctrl2HookMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
NTSTATUS Ctrl2HookUnload(PDRIVER_OBJECT DriverObject);

// IRP�ַ�����
NTSTATUS Ctrl2FilterHookDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);


NTSTATUS Ctrl2IrpHook(PDRIVER_OBJECT DriverObject);		// HOOK ����������KdbClass IRP�ֺ���
NTSTATUS Ctrl2IrpUnHook(PDRIVER_OBJECT DriverObject);		// HOOK ����������KdbClass IRP�ֺ���

NTSTATUS KeyBoardServiceCallBackHook();
NTSTATUS KeyBoardServiceCallBackUnHook();


VOID __stdcall KeyboardServiceCallBackFunc(PDEVICE_OBJECT DeviceObject, PKEYBOARD_INPUT_DATA InputDataStart, PKEYBOARD_INPUT_DATA InputDataEnd, PULONG InputDataConsumed);


#endif // !_CTRL2HOOK_H