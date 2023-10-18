#include <ntddk.h>
#include "FilterDP.h"

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID NTAPI DpUnload(PDRIVER_OBJECT DriverObject);

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	if (RegistryPath)
		KdPrint(("[dbg][%ws] Driver RegistryPath:%wZ \n", __FUNCTIONW__, RegistryPath));
	if (DriverObject)
	{
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
			DriverObject->MajorFunction[i] = DPDospatchAny;
		DriverObject->MajorFunction[IRP_MJ_POWER] = DPDispatchPower;
		DriverObject->MajorFunction[IRP_MJ_PNP] = DPDispatchPnp;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DPDispatchDeviceControl;
		DriverObject->MajorFunction[IRP_MJ_READ] = DPDispatchRead;
		DriverObject->MajorFunction[IRP_MJ_WRITE] = DPDispatchWrite;
		DriverObject->DriverExtension->AddDevice = DPAddDevice;
		DriverObject->DriverUnload = DpUnload;

		// 注册boot驱动结束回调函数
		IoRegisterBootDriverReinitialization(DriverObject, DPReinitializationRoutine);

		KdPrint(("[dbg][%ws]Driver Object Address:%p, Current IRQL=0x%u\n", __FUNCTIONW__, DriverObject, KeGetCurrentIrql()));
	}

	KdPrint(("[dbg][%ws] Built %s %s \n", __FUNCTIONW__, __DATE__, __TIME__));
	return STATUS_SUCCESS;
}

VOID NTAPI DpUnload(PDRIVER_OBJECT DriverObject)
{
	
}