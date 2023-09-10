// 键盘过滤，通过HOOK键盘驱动对象的分发函数指针过滤

#ifndef _CTRL2HOOK_H
#define _CTRL2HOOK_H

#include <ntddk.h>

// 通过一个名字来获得一个对象得指针(闭源函数,文档没有公开,声明直接使用了)
// 调用ObReferenceObjectByName获得对象引用会使驱动对象的引用计数增加,ObDereferenceObject释放驱动对象引用
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectName, ULONG Attribuites, PACCESS_STATE AccessState, ACCESS_MASK DesiredAccess,
	POBJECT_TYPE ObjectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);

extern POBJECT_TYPE* IoDriverObjectType;	/* 内核全局变量, 声明可以直接使用. 通用键盘驱动对象的类型*/

/* 入口函数 */
NTSTATUS Ctrl2HookMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
NTSTATUS Ctrl2HookUnload(PDRIVER_OBJECT DriverObject);

// IRP分发函数
NTSTATUS Ctrl2FilterHookDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);


NTSTATUS Ctrl2IrpHook(PDRIVER_OBJECT DriverObject);		// HOOK 键盘类驱动KdbClass IRP分函数
NTSTATUS Ctrl2IrpUnHook(PDRIVER_OBJECT DriverObject);		// HOOK 键盘类驱动KdbClass IRP分函数


#endif // !_CTRL2HOOK_H