#include "SerialPort.h"
#include <ntstrsafe.h>

#define CCP_MAX_COM_ID 32		// 最大绑定至32个串口
#define DELAY_ONE_MICROSECOND (-10)
#define DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)

PDEVICE_OBJECT s_filterObject[CCP_MAX_COM_ID] = { 0 };
PDEVICE_OBJECT s_nextObject[CCP_MAX_COM_ID] = { 0 };

void SerialPortMain(PDRIVER_OBJECT DriverObject)
{
	ccpAttachAllComs(DriverObject);
}

void ccpUnloadSerialPort(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);


	LARGE_INTEGER laInterVal;

	// 解除过滤设备的绑定
	for (int i = 0; i < CCP_MAX_COM_ID; ++i)
		if (s_nextObject[i])
			IoDetachDevice(s_nextObject[i]);

	// 解除绑定后等待5秒,等所有正在处理的IRP结束
	laInterVal.QuadPart = (5 * 1000 * DELAY_ONE_MILLISECOND);
	KeDelayExecutionThread(KernelMode, FALSE, &laInterVal);

	// 删除过滤设备
	for (int i = 0; i < CCP_MAX_COM_ID; ++i)
		if (s_filterObject[i])
			IoDeleteDevice(s_filterObject[i]);
	return;
}

void ccpAttachAllComs(PDRIVER_OBJECT DriverObject)
{
	PDEVICE_OBJECT comObject;
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;

	for (int i = 0; i < CCP_MAX_COM_ID; ++i)
	{
		// 获取串口对象引用
		comObject = ccpOpenCom(i, &nStatus);
		if (!comObject)
			continue;

		// 不处理是否绑定成功
		ccpAttachDevice(DriverObject, comObject, &s_filterObject[i], &s_nextObject[i]);
	}
}

PDEVICE_OBJECT ccpOpenCom(ULONG ID, NTSTATUS* Status)
{
	UNICODE_STRING nameStr = { 0 };
	static WCHAR wcName[32] = { 0 };
	PFILE_OBJECT fileObject = NULL;
	PDEVICE_OBJECT devObject = NULL;

	memset(wcName, 0, sizeof(WCHAR) * 2);

	// 根据id转换成串口的后缀名
	RtlStringCchPrintfW(wcName, 32, L"Device\\Serial%d", ID);
	RtlInitUnicodeString(&nameStr, wcName);

	// 打开设备对象
	*Status = IoGetDeviceObjectPointer(&nameStr, FILE_ALL_ACCESS, &fileObject, &devObject);
	if (!NT_SUCCESS(*Status))
	{
		KdPrint(("[dbg]: open Serial Port Device failed: %wZ \n", nameStr));
		return NULL;
	}
		

	// 解除对文件对象的引用
	ObDereferenceObject(fileObject);
	return devObject;
}


NTSTATUS ccpAttachDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT OldDeviceObject, PDEVICE_OBJECT* FiltDeviceObject, PDEVICE_OBJECT* Next)
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;

	PDEVICE_OBJECT topDev = NULL;

	// 生成设备
	nStatus = IoCreateDevice(DriverObject, 0, NULL, OldDeviceObject->DeviceType, 0, FALSE, OUT FiltDeviceObject);
	if (!NT_SUCCESS(nStatus))
		return nStatus;

	// 拷贝设备重要标志
	if (OldDeviceObject->Flags & DO_BUFFERED_IO)
		(*FiltDeviceObject)->Flags |= DO_BUFFERED_IO;
	if (OldDeviceObject->Flags & DO_DIRECT_IO)
		(*FiltDeviceObject)->Flags |= DO_DIRECT_IO;
	if (OldDeviceObject->Characteristics & FILE_DEVICE_SECURE_OPEN)
		(*FiltDeviceObject)->Characteristics |= FILE_DEVICE_SECURE_OPEN;
	(*FiltDeviceObject)->Flags |= DO_POWER_PAGABLE;

	// 将设备绑定到另一个设备上
	topDev = IoAttachDeviceToDeviceStack(*FiltDeviceObject, OldDeviceObject);
	if (!topDev)
	{
		IoDeleteDevice(*FiltDeviceObject);
		*FiltDeviceObject = NULL;
		return STATUS_UNSUCCESSFUL;
	}

	// 设置设备已经启动
	(*FiltDeviceObject)->Flags = (*FiltDeviceObject)->Flags & ~DO_DEVICE_INITIALIZING;
	*Next = topDev;
	return STATUS_SUCCESS;
}

NTSTATUS ccpDispatchCom(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	//NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG bufferLen = 0;
	PVOID pBuffer = NULL;

	for (UINT32 i = 0; i < CCP_MAX_COM_ID; ++i)
	{
		if (s_filterObject[i] == DeviceObject)
		{
			// 电源操作
			if (irpsp->MajorFunction == IRP_MJ_POWER)
			{
				PoStartNextPowerIrp(Irp);
				IoSkipCurrentIrpStackLocation(Irp);
				return PoCallDriver(s_nextObject[i], Irp);
			}

			// 写请求
			if (irpsp->MajorFunction == IRP_MJ_WRITE)
			{
				bufferLen = irpsp->Parameters.Write.Length;
				if (Irp->MdlAddress)
					pBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
				else if (Irp->UserBuffer)
					pBuffer = Irp->UserBuffer;
				else
					pBuffer = Irp->AssociatedIrp.SystemBuffer;

				// 打印内容
				if(pBuffer)
					for (UINT32 j = 0; j < bufferLen; ++j)
						KdPrint(("[dbg]: comcap: Send Data: %2x \n", ((PUCHAR)pBuffer)[j]));
			}

			// 将请求直接下发
			IoSkipCurrentIrpStackLocation(Irp);
			return IoCallDriver(s_nextObject[i], Irp);
		}
	}

	// 如果根本不在被绑定的设备中,那消息走到当前驱动是有问题的,直接返回参数错误
	return STATUS_INVALID_DEVICE_OBJECT_PARAMETER;
}

