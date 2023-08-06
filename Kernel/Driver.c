#include <ntddk.h>
#include <Ntstrsafe.h>
#include "DEMO/demo.h"

extern PDRIVER_OBJECT g_poDriverObject = NULL;
extern PUNICODE_STRING g_psRegistryPath = NULL;
extern PDEVICE_OBJECT g_demo_cdo;

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING cdo_syb = RTL_CONSTANT_STRING(CWK_COD_SYB_NAME);
	if (g_poDriverObject)
		DbgPrint("[dbg:%ws]Driver Unload, Driver Object Address:%p, Current Process ID=%p\n", __FUNCTIONW__, DriverObject, PsGetCurrentProcessId());
	if (g_psRegistryPath->Buffer && g_psRegistryPath)
	{

		DbgPrint("[dbg:%ws]Driver Unload, Driver RegistryPath:%wZ\n", __FUNCTIONW__, g_psRegistryPath);

		ExFreePoolWithTag(g_psRegistryPath->Buffer, 'Path');
		g_psRegistryPath = NULL;
	}
	if (g_demo_cdo)
	{
		DbgPrint("[dbg:%ws]Driver Unload, Delete Device \n", __FUNCTIONW__);
		ASSERT(g_demo_cdo != NULL);
		IoDeleteSymbolicLink(&cdo_syb);
		IoDeleteDevice(g_demo_cdo);
	}
		
	return;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	static UNICODE_STRING usRegistryPath = { 0 };
	DbgPrint("[dbg:%ws] Hello Kernel World\n", __FUNCTIONW__);
	if (RegistryPath)
	{
		PVOID pBuffer = ExAllocatePoolWithTag(PagedPool, RegistryPath->MaximumLength, 'Path');
		if (NULL == pBuffer)
		{
			DbgPrint("[dbg:%ws]Driver  Init RegistryPath Failed: Path=%wZ\n", __FUNCTIONW__, RegistryPath);
			return STATUS_UNSUCCESSFUL;
		}
		usRegistryPath.Buffer = pBuffer;
		usRegistryPath.Length = RegistryPath->Length;
		usRegistryPath.MaximumLength = RegistryPath->MaximumLength;
		RtlCopyMemory(pBuffer, RegistryPath->Buffer, RegistryPath->Length);

		g_psRegistryPath = &usRegistryPath;
		DbgPrint("[dbg:%ws]Driver RegistryPath:%wZ \n", __FUNCTIONW__, g_psRegistryPath);
	}
		
	if (DriverObject)
	{
		g_poDriverObject = DriverObject;
		DriverObject->DriverUnload = DriverUnload;
		DbgPrint("[dbg:%ws]Driver Object Address:%p, Current IRQL=0x%u\n", __FUNCTIONW__, DriverObject, KeGetCurrentIrql());
	}


	DemoMain();
	return STATUS_SUCCESS;
}