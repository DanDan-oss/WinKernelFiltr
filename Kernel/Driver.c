#include <ntddk.h>

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	if (DriverObject)
		DbgPrint("[dbg:%ws]Driver Unload, Driver Object Address:%p, Current Process ID=%p\n", __FUNCTIONW__, DriverObject, PsGetCurrentProcessId());

	return;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	KdBreakPoint();

	DbgPrint("[dbg:%ws] Hello Kernel World\n", __FUNCTIONW__);
	if (RegistryPath)
		DbgPrint("[dbg:%ws]Driver RegistryPath:%wZ\n", __FUNCTIONW__, RegistryPath);
	if (DriverObject)
	{
		DriverObject->DriverUnload = DriverUnload;

		DbgPrint("[dbg:%ws]Driver Object Address:%p, Current IRQL=0x%u\n", __FUNCTIONW__, DriverObject, KeGetCurrentIrql());
	}

	return STATUS_SUCCESS;
}
