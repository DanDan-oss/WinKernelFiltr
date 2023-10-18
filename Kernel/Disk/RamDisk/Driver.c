#include <ntddk.h>
#include <wdf.h>

#include "RamDisk.h"

DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);					// 驱动卸载回调函数

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	WDF_DRIVER_CONFIG config;

	if(RegistryPath)
		KdPrint(("[dbg][%ws] Driver RegistryPath:%wZ \n", __FUNCTIONW__, RegistryPath));
	if (DriverObject)
	{
		DriverObject->DriverUnload = DriverUnload;
		KdPrint(("[dbg][%ws]Driver Object Address:%p, Current IRQL=0x%u\n", __FUNCTIONW__, DriverObject, KeGetCurrentIrql()));
	}
	KdPrint(("[dbg][%ws] Windows Ramdisk Driver - Driver Framework Edition \n", __FUNCTIONW__));
	KdPrint(("[dbg][%ws] Built %s %s \n", __FUNCTIONW__, __DATE__, __TIME__));

	if (RegistryPath && DriverObject)
	{
		WDF_DRIVER_CONFIG_INIT(&config, RamDiskEvtDeviceAdd);
		return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
	}
	return STATUS_UNSUCCESSFUL;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	KdPrint(("[dbg][%ws] Driver Unload, Driver Object Address:%p, Current Process ID=%p\n", __FUNCTIONW__, DriverObject, PsGetCurrentProcessId()));
}
