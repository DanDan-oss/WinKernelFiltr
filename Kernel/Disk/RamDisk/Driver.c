#include <ntddk.h>
#include <wdf.h>

#define NT_DEVICE_NAME L"\\Device\\Ramdisk"

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);					// 驱动卸载回调函数
NTSTATUS RamDiskEvtDeviceAdd(WDFDRIVER DriverObject, PWDFDEVICE_INIT DeviceInit);

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	WDF_DRIVER_CONFIG config;

	if(RegistryPath)
		KdPrint(("[dbg:%ws]Driver RegistryPath:%wZ \n", __FUNCTIONW__, RegistryPath));
	if (DriverObject)
	{
		DriverObject->DriverUnload = DriverUnload;
		KdPrint(("[dbg:%ws]Driver Object Address:%p, Current IRQL=0x%u\n", __FUNCTIONW__, DriverObject, KeGetCurrentIrql()));
	}
	KdPrint(("[dbg:%ws] Windows Ramdisk Driver - Driver Framework Edition \n", __FUNCTIONW__));
	KdPrint(("[dbg:%ws] Built %s %s \n", __FUNCTIONW__, __DATE__, __TIME__));

	
	WDF_DRIVER_CONFIG_INIT(&config, RamDiskEvtDeviceAdd);
	return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	KdPrint(("[dbg:%ws]Driver Unload, Driver Object Address:%p, Current Process ID=%p\n", __FUNCTIONW__, DriverObject, PsGetCurrentProcessId()));
}

NTSTATUS RamDiskEvtDeviceAdd(WDFDRIVER DriverObject, PWDFDEVICE_INIT DeviceInit)
{
	UNREFERENCED_PARAMETER(DriverObject);

	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	WDFDEVICE wdfDevice = NULL;						// 新建设备
	WDFQUEUE wdfQueue = NULL;;						// 新建队列
	WDF_IO_QUEUE_CONFIG ioQueueConfig = { 0 };		// 要建立的队列配置
	WDF_OBJECT_ATTRIBUTES deviceAttributes = { 0 };	// 要建立的设备对象的属性描述
	WDF_OBJECT_ATTRIBUTES queueAttributes = { 0 };	// 要建立的队列对象的属性描述
	PDEVOBJ_EXTENSION wdfDeviceExtension = NULL;	// 设备扩展
	PQUEUE_EXTENSION wdfQueueContext = NULL;		// 队列扩展

	UNICODE_STRING ntDeviceName = { 0 };

	DECLARE_CONST_UNICODE_STRING(ntDeviceName, NT_DEVICE_NAME);
	PAGED_CODE();

	return STATUS_SUCCESS;
}
