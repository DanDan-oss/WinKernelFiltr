#include "ctrl2cap.h"
#include <ntddk.h>
#include <wdm.h>

#define DELAY_ONE_MICROSECOND (-10)
#define DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)


extern POBJECT_TYPE IoDriverObjectType;	/* 内核全局变量, 声明可以直接使用*/
ULONG gC2pKeyCount = 0;

NTSTATUS Ctrl2CapMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	nStatus = Ctrl2CapAttachDevices(DriverObject, RegistryPath);

	return nStatus;
}

NTSTATUS Ctrl2CapUnload(PDRIVER_OBJECT DriverObject)
{
	PDEVICE_OBJECT DeviceObject = NULL;
	PDEVICE_OBJECT NewDeviceObject = NULL;

	LARGE_INTEGER Interval = { 0 };
	PRKTHREAD CurrentThread = NULL;

	//TODO: 获取当前线程句柄并设置线程优先级为实时优先级
	CurrentThread = KeGetCurrentThread();
	KeSetPriorityThread(CurrentThread, LOW_REALTIME_PRIORITY);

	UNREFERENCED_PARAMETER(DriverObject);
	KdPrint(("DriverEntry Ctrl2cap unLoading...\n"));

	//TODO:    循环遍历本驱动设备栈所有设备,解除过滤设备绑定并删除
	DeviceObject = DriverObject->DeviceObject;
	while (DeviceObject)
	{
		NewDeviceObject = DeviceObject->NextDevice;
		Ctrl2DetachDevices(DeviceObject);
		DeviceObject = NewDeviceObject;
	}
	ASSERT( DriverObject->DeviceObject ==  NULL);


	// Delay some time, 1秒钟
	//Interval = RtlConvertLongToLargeInteger(1000 * DELAY_ONE_MILLISECOND);
	Interval.QuadPart = 1000 * DELAY_ONE_MILLISECOND;
	while (gC2pKeyCount)
		KeDelayExecutionThread(KernelMode, FALSE, &Interval);

	KdPrint(("DriverEntry Ctrl2cap unLoad OK!\n"));
	return (STATUS_SUCCESS);
}

NTSTATUS Ctrl2CapDispatchGeneral(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	PC2P_DEV_EXT devExt = (PC2P_DEV_EXT)DeviceObject->DeviceExtension;

	if (devExt->NodeSize !=  sizeof(PC2P_DEV_EXT) || devExt->LowerDeviceObject == NULL)
		return STATUS_INVALID_DEVICE_OBJECT_PARAMETER;

	// 被绑定设备电源操作 IRP单独处理
	if (IRP_MJ_POWER == irpsp->MajorFunction)
		return Ctrl2CapDispatchPower(DeviceObject, Irp);

	// 被绑定设备PNP(即插即用)操作
	if (IRP_MJ_PNP == irpsp->MajorFunction)
		return Ctrl2CapDispatchPnP(DeviceObject, Irp);

	// 键盘读取
	if (IRP_MJ_READ == irpsp->MajorFunction)
		return Ctrl2CapDispatchRead(DeviceObject, Irp);

	// 其他请求不处理,直接下发
	KdPrint(("Other Diapatch!\n"));
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PC2P_DEV_EXT)DeviceObject->DeviceExtension)->LowerDeviceObject, Irp);
}

NTSTATUS Ctrl2CapDispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PC2P_DEV_EXT devExt = (PC2P_DEV_EXT)DeviceObject->DeviceExtension;
	PoStartNextPowerIrp(Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver(devExt->LowerDeviceObject, Irp);
}

NTSTATUS Ctrl2CapDispatchPnP(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PC2P_DEV_EXT devExt = (PC2P_DEV_EXT)DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;


	// TODO; 请求直接下发, 然后判断是否是硬件拔出,如果是硬件拔出解除设备绑定,删除生成的过滤设备, 
	IoSkipCurrentIrpStackLocation(Irp);
	nStatus = IoCallDriver(devExt->LowerDeviceObject, Irp);

	if (IRP_MN_REMOVE_DEVICE == irpsp->MinorFunction)
	{
		KdPrint(("[dbg]: IRP_MN_REMOVE_DEVICE\n"));

		// 解除绑定,删除过滤设备
		Ctrl2DetachDevices(DeviceObject);
		nStatus = STATUS_SUCCESS;
	}
	return  nStatus;
}

NTSTATUS Ctrl2CapDispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	ULONG ulReturnInformation = 0;
	PC2P_DEV_EXT devExt =NULL;
	PIO_STACK_LOCATION irpsp = NULL;


	//TODO: 判断当前IRP是否在IRP栈最底端,如果是 这是错误的请求,结束IRP传递, 返回IRP
	if (1 == Irp->CurrentLocation)
	{
		KdPrint(("[dbg]: Ctrl2CapDispatchRead encountered bogus current localtion\n"));
		ulReturnInformation = 0;
		nStatus = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Status = nStatus;
		Irp->IoStatus.Information = ulReturnInformation;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return nStatus;
	}
	//TODO: 不在IRP栈最底端,IRP计数器加1
	++gC2pKeyCount;

	//TODO: 获取被过滤设备指针
	devExt = (PC2P_DEV_EXT)DeviceObject->DeviceExtension;
	irpsp = IoGetCurrentIrpStackLocation(Irp);

	// 设置IRP完成回调函数, 拷贝当前IRP栈空间,设置完成例程发送给下层驱动,然后等待键盘驱动读取键扫描码完成IRP后返回
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, Ctrl2CapReadComplete, DeviceObject, TRUE, TRUE, TRUE);
	return IoCallDriver(devExt->LowerDeviceObject, Irp);

	
}

NTSTATUS Ctrl2CapReadComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Context);
	ULONGLONG ulBuflen = 0;
	PUCHAR pBuffer = NULL;

	//TODO:    IRP计数器减1,
	--gC2pKeyCount;

	//TODO: 判断当前IRP请求是否执行成功,如果执行成功读取设备流过数据,否则数据没有意义
	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		ulBuflen = Irp->IoStatus.Information;
		pBuffer = Irp->AssociatedIrp.SystemBuffer;

		for (int i = 0; i < ulBuflen; ++i)
			DbgPrint("ctrl2cap read: %2x\r\n", pBuffer[i]);
	}

	// 判断IRP的调度标识是否时是挂起状态, 如果否,手动设置IRP调度挂起状态
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);

	return Irp->IoStatus.Status;
}

NTSTATUS Ctrl2CapAttachDevices(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING strNtNameString = { 0 };

	PC2P_DEV_EXT devExt = NULL;		//  自定义设备扩展结构 指针
	PDRIVER_OBJECT KbdDriverObject = NULL;		// 键盘驱动对象
	PDEVICE_OBJECT pFileteDeviceObject = NULL;	// 生成过滤设备对象指针
	PDEVICE_OBJECT pTargetDeviceObject = NULL;	// 绑定成功后的真实设备对象指针,函数返回赋值为0表示绑定失败
	PDEVICE_OBJECT pLowerDeviceObject = NULL;	// 键盘驱动设备栈的设备对象

	//TODO: 通过名字获取驱动对象指针, 获取完毕释放对象,保存指针
	RtlInitUnicodeString(&strNtNameString, KBD_DRIVER_NAME);
	nStatus = ObReferenceObjectByName(&strNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);

	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("[dbg]: Couldn't get the  KbdDriver Object failed! DrverName=[%wZ]", strNtNameString));
		return(nStatus);
	}

	// 调用ObReferenceObjectByName会使驱动对象的引用计数增加,释放驱动对象引用
	ObDereferenceObject(KbdDriverObject);

	// 遍历键盘驱动设备链
	pTargetDeviceObject = KbdDriverObject->DeviceObject;
	while (pTargetDeviceObject)
	{
		// 生成过滤设备
		nStatus = IoCreateDevice(DriverObject, sizeof(PC2P_DEV_EXT), NULL, pTargetDeviceObject->DeviceType, pTargetDeviceObject->Characteristics,
			FALSE, OUT & pFileteDeviceObject);

		if (!NT_SUCCESS(nStatus))
		{
			KdPrint(("[dbg]: Couldn't Create the  Filter Device Object failed! STATUS=%x", nStatus));
			return (nStatus);
		}

		//  绑定过滤设备
		pLowerDeviceObject = IoAttachDeviceToDeviceStack(pFileteDeviceObject, pTargetDeviceObject);
		if (!pLowerDeviceObject)
		{
			KdPrint(("[dbg]: Couldn't Attach to  Filter Device Object failed."));
			IoDeleteDevice(pFileteDeviceObject);
			pFileteDeviceObject = NULL;
			return nStatus;
		}

		// 初始化过滤设备的扩展属性(生成设备对象时指定的大小)
		devExt = (PC2P_DEV_EXT)(pFileteDeviceObject->DeviceExtension);
		Ctrl2CapFilterDevExtInit(devExt, pFileteDeviceObject, pTargetDeviceObject, pLowerDeviceObject);

		// 拷贝设备重要标志标志
		pFileteDeviceObject->DeviceType = pLowerDeviceObject->DeviceType;
		pFileteDeviceObject->Characteristics = pLowerDeviceObject->Characteristics;
		pFileteDeviceObject->StackSize = pLowerDeviceObject->StackSize + 1;
		pFileteDeviceObject->Flags |= pLowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);

		// 指针后移,下一次遍历
		pTargetDeviceObject = pTargetDeviceObject->NextDevice;
	}
	nStatus = STATUS_SUCCESS;
	return nStatus;
}


NTSTATUS Ctrl2CapFilterDevExtInit(PC2P_DEV_EXT devExt, PDEVICE_OBJECT pFileteDeviceObject, PDEVICE_OBJECT pTargetDeviceObject, PDEVICE_OBJECT pLowerDeviceObject)
{
	memset(devExt, 0, sizeof(PC2P_DEV_EXT));
	devExt->NodeSize = sizeof(PC2P_DEV_EXT);
	devExt->FilterDeviceObject = pFileteDeviceObject;
	devExt->TargetDeviceObject = pTargetDeviceObject;
	devExt->LowerDeviceObject = pLowerDeviceObject;

	// 初始化结构里的自旋锁
	KeInitializeSpinLock(&(devExt->IoRequestsSpinLock));
	KeInitializeEvent(&(devExt->IoInProgressEvent), NotificationEvent, FALSE);

	return STATUS_SUCCESS;
}


NTSTATUS Ctrl2DetachDevices(PDEVICE_OBJECT DeviceObject)
{
    PC2P_DEV_EXT devExt = NULL;
    //TODO: 获取设备对象的扩展

     devExt = DeviceObject->DeviceExtension;
	 if (!devExt->FilterDeviceObject || DeviceObject != devExt->FilterDeviceObject)
	 {
		 KdPrint(("[dbg]: Couldn't Detach to  Filter Device Object failed. %p", devExt->FilterDeviceObject));
		 return STATUS_DEVICE_ENUMERATION_ERROR;
	 }
         

     //TODO: 解除设备扩展里面绑定的设备对象的绑定
    IoDetachDevice(devExt->TargetDeviceObject);
    devExt->FilterDeviceObject = NULL;
    devExt->TargetDeviceObject = NULL;
    devExt->LowerDeviceObject = NULL;
    IoDeleteDevice(DeviceObject);
    return STATUS_SUCCESS;
}
