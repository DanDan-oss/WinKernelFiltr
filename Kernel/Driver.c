#include <ntddk.h>
#include <Ntstrsafe.h>
#include "DEMO/demo.h"
#include "Filter/SerialPort.h"
#include "Filter/ctrl2cap.h"

extern PDRIVER_OBJECT g_poDriverObject = NULL;
extern PUNICODE_STRING g_psRegistryPath = NULL;
extern PDEVICE_OBJECT g_demo_cdo;					// demo设备对象


VOID DriverUnload(PDRIVER_OBJECT DriverObject);					// 驱动卸载回调函数
NTSTATUS cwkDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);	// IRP请求分发函数


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
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
			DriverObject->MajorFunction[i] = cwkDispatch;
	}

	// DemoMain();
	// SerialPortMain(DriverObject);		// 串口过滤

	// Ctrl2CaoMain(DriverObject, RegistryPath);		// 键盘过滤

	return STATUS_SUCCESS;
}


VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	if (g_poDriverObject)
		DbgPrint("[dbg:%ws]Driver Unload, Driver Object Address:%p, Current Process ID=%p\n", __FUNCTIONW__, DriverObject, PsGetCurrentProcessId());
	if (g_psRegistryPath->Buffer && g_psRegistryPath)
	{

		DbgPrint("[dbg:%ws]Driver Unload, Driver RegistryPath:%wZ\n", __FUNCTIONW__, g_psRegistryPath);

		ExFreePoolWithTag(g_psRegistryPath->Buffer, 'Path');
		g_psRegistryPath = NULL;
	}
	if (g_demo_cdo)
		ccpUnloadDemo();


	return;
}


NTSTATUS cwkDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION irpsp = NULL;
	NTSTATUS nStatus = STATUS_INVALID_DEVICE_OBJECT_PARAMETER;

	if (DeviceObject == g_demo_cdo && DeviceObject != NULL)
		return cwkDispatchDemo(DeviceObject, Irp);

	nStatus = ccpDispatchCom(DeviceObject, Irp);
	if (nStatus != STATUS_INVALID_DEVICE_OBJECT_PARAMETER)
		return nStatus;

	// TODO: 键盘的IRP分发函数由于没有去遍历过滤设备判断,只是简单判断扩展结构大小,可能兼容很差
	nStatus = Ctrl2CapDispatchGeneral(DeviceObject, Irp);
	if (nStatus != STATUS_INVALID_DEVICE_OBJECT_PARAMETER)
		return nStatus;

	irpsp = IoGetCurrentIrpStackLocation(Irp);
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = nStatus;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return nStatus;
}