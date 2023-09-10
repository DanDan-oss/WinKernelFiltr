// 键盘过滤，通过HOOK键盘驱动对象的分发函数指针过滤
/*
	KbdClass被称为'键盘类驱动'在Windows中,类驱动通常是指统管一类设备的驱动程序.
不管是USB键盘,还是PS／2键盘均经过它,所以在这层做拦截,能获得很好的通用性.
在类驱动之下和实际埂件交互的驱动被称为"端口驱动". 具体到键盘,j8042prt是PS/2键盘的端口驱动,USB键盘则是Kbdhid
*/

#ifndef _CTRL2HOOK_H
#define _CTRL2HOOK_H

#include <ntddk.h>


typedef struct _KEYBOARD_INPUT_DATA
{
    USHORT UnitId;    // 对于\\DEVICE\\KeyboardPort0这个值是0,\\DEVICE\\KeyboardPort1这个值是1,依此类推
    USHORT MakeCode;    // 扫描码
    USHORT Flags;        // 标志,标志一个键按下还是弹起
    USHORT Reserved;        // 保留
    ULONG ExtraInformation; // 扩展信息
}KEYBOARD_INPUT_DATA, * PKEYBOARD_INPUT_DATA;

typedef VOID(__stdcall* KeyBoardClassServiceCallBack)(IN PDEVICE_OBJECT DeviceObject, IN PKEYBOARD_INPUT_DATA InputDataStart, IN PKEYBOARD_INPUT_DATA InputDataEnd, IN OUT PULONG InputDataConsumed);
typedef struct _KBD_CALLBACK
{
    PDEVICE_OBJECT classDeviceObject;
    KeyBoardClassServiceCallBack ServiceCallBack;
    PVOID DevExtCallBackAddress;
}KBD_CALLBACK, *PKBD_CALLBACK;


/* 入口函数 */
NTSTATUS Ctrl2HookMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
NTSTATUS Ctrl2HookUnload(PDRIVER_OBJECT DriverObject);

// IRP分发函数
NTSTATUS Ctrl2FilterHookDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);


NTSTATUS Ctrl2IrpHook(PDRIVER_OBJECT DriverObject);		// HOOK 键盘类驱动KdbClass IRP分函数
NTSTATUS Ctrl2IrpUnHook(PDRIVER_OBJECT DriverObject);		// HOOK 键盘类驱动KdbClass IRP分函数

NTSTATUS KeyBoardServiceCallBackHook();
NTSTATUS KeyBoardServiceCallBackUnHook();


VOID __stdcall KeyboardServiceCallBackFunc(PDEVICE_OBJECT DeviceObject, PKEYBOARD_INPUT_DATA InputDataStart, PKEYBOARD_INPUT_DATA InputDataEnd, PULONG InputDataConsumed);


#endif // !_CTRL2HOOK_H