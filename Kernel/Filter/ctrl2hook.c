#include "ctrl2hook.h"


#define KBD_DRIVER_NAME    L"\\Driver\\Kbdclass"            // 键盘类驱动
#define USBKBD_DRIVER_NAME L"\\Driver\\Kbdhid"            // USB键盘 端口驱动
#define PS2KBD_DRIVER_NAME L"\\Driver\\i8042prt"        // PS/2键盘 端口驱动

PDRIVER_DISPATCH OldDispatchFunction[IRP_MJ_MAXIMUM_FUNCTION] = { 0 };

NTSTATUS Ctrl2HookMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	// Ctrl2IrpHook(DriverObject);

	return STATUS_SUCCESS;
}

NTSTATUS Ctrl2HookUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	// Ctrl2IrpUnHook()
	return STATUS_SUCCESS;
}



NTSTATUS Ctrl2FilterHookDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);


	return STATUS_SUCCESS;
}


NTSTATUS Ctrl2IrpHook(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	PDRIVER_OBJECT KbdDriverObject = NULL;
	UNICODE_STRING uniNtNameString = { 0 };
	NTSTATUS Status = STATUS_UNSUCCESSFUL;


	RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME);
	Status = ObReferenceObjectByName(&uniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[dbg]: Couldn't get the  KbdDriver Object failed! DrverName=[%wZ]\n", uniNtNameString));
		return STATUS_UNSUCCESSFUL;
	}
	// 调用ObReferenceObjectByName会使驱动对象的引用计数增加,释放驱动对象引用
	ObDereferenceObject(KbdDriverObject);

	for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		OldDispatchFunction[i] = KbdDriverObject->MajorFunction[i];
		InterlockedExchangePointer((PVOID*)&KbdDriverObject->MajorFunction[i], (PVOID*)Ctrl2FilterHookDispatch);
	}
	return STATUS_SUCCESS;
}

NTSTATUS Ctrl2IrpUnHook(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	PDRIVER_OBJECT KbdDriverObject = NULL;
	UNICODE_STRING uniNtNameString = { 0 };
	NTSTATUS Status = STATUS_UNSUCCESSFUL;


	RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME);
	Status = ObReferenceObjectByName(&uniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[dbg]: Couldn't get the  KbdDriver Object failed! DrverName=[%wZ]\n", uniNtNameString));
		return STATUS_UNSUCCESSFUL;
	}
	// 调用ObReferenceObjectByName会使驱动对象的引用计数增加,释放驱动对象引用
	ObDereferenceObject(KbdDriverObject);

	for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		if(OldDispatchFunction[i])
			InterlockedExchangePointer((PVOID*)&KbdDriverObject->MajorFunction[i], (PVOID*)OldDispatchFunction[i]);

	KdPrint(("[dbg]: Unload [%wZ] IRP Hook OK!\n", uniNtNameString));
	return STATUS_SUCCESS;
}