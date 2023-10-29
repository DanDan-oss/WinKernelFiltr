#include <ntifs.h>
#include <ntddvol.h>

#include "DPDriver.h"
#include "DPBitmap.h"


NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	if (RegistryPath)
		KdPrint(("[dbg][%ws] Driver RegistryPath:%wZ \n", __FUNCTIONW__, RegistryPath));
	if (DriverObject)
	{
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
			DriverObject->MajorFunction[i] = DPDispatchAny;
		DriverObject->MajorFunction[IRP_MJ_POWER] = DPDispatchPower;
		DriverObject->MajorFunction[IRP_MJ_PNP] = DPDispatchPnp;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DPDispatchDeviceControl;
		DriverObject->MajorFunction[IRP_MJ_READ] = DPDispatchReadWrite;
		DriverObject->MajorFunction[IRP_MJ_WRITE] = DPDispatchReadWrite;
		DriverObject->DriverExtension->AddDevice = DPAddDevice;
		DriverObject->DriverUnload = DpUnload;

		// 注册boot驱动结束回调函数
		IoRegisterBootDriverReinitialization(DriverObject, DPReinitializationRoutine, NULL);

		KdPrint(("[dbg][%ws]Driver Object Address:%p, Current IRQL=0x%u\n", __FUNCTIONW__, DriverObject, KeGetCurrentIrql()));
	}

	KdPrint(("[dbg][%ws] Built %s %s \n", __FUNCTIONW__, __DATE__, __TIME__));
	return STATUS_SUCCESS;
}

VOID NTAPI DpUnload(PDRIVER_OBJECT DriverObject)
{
	//这个驱动将会工作到系统关机,不用在驱动卸载的时候做任何清理动作
	UNREFERENCED_PARAMETER(DriverObject);
	return;
}

NTSTATUS NTAPI DPAddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PDP_FILTER_DEV_EXTENSION DevExt = NULL;
	PDEVICE_OBJECT LowerDeviceObject = NULL;
	PDEVICE_OBJECT FilterDeviceObject = NULL;
	HANDLE ThreadHandle = NULL;

	ntStatus = IoCreateDevice(DriverObject, sizeof(DP_FILTER_DEV_EXTENSION), NULL, FILE_DEVICE_DISK, FILE_DEVICE_SECURE_OPEN, FALSE, &FilterDeviceObject);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;
	LowerDeviceObject = IoAttachDeviceToDeviceStack(FilterDeviceObject, PhysicalDeviceObject);		// 设备绑定
	if (!LowerDeviceObject)
	{
		ntStatus = STATUS_NO_SUCH_DEVICE;
		goto ERROUT;
	}

	DevExt = FilterDeviceObject->DeviceExtension;
	RtlZeroMemory(DevExt, sizeof(DP_FILTER_DEV_EXTENSION));
	// 初始化过滤设备属性
	FilterDeviceObject->Flags = LowerDeviceObject->Flags;
	FilterDeviceObject->Flags |= DO_POWER_PAGABLE;	//电源可分页属性
	FilterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	DevExt->FilterDeviceObject = FilterDeviceObject;
	DevExt->PhyDeviceObject = PhysicalDeviceObject;
	DevExt->LowerDeviceObject = LowerDeviceObject;

	// 初始化事件
	KeInitializeEvent(&DevExt->PagingPathCountEvent, NotificationEvent, TRUE);
	InitializeListHead(&DevExt->RequestList);
	KeInitializeSpinLock(&DevExt->RequestLock);
	KeInitializeEvent(&DevExt->RequestEvent, SynchronizationEvent, FALSE);

	// 初始化处理线程,线程函数参数设置为设备扩展
	DevExt->ThreadTermFlag = FALSE;
	ntStatus = PsCreateSystemThread(&ThreadHandle, (ACCESS_MASK)0L, NULL, NULL, NULL, DPReadWriteThread, DevExt);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	// 获取处理线程对象句柄
	ntStatus = ObReferenceObjectByHandle(ThreadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, &DevExt->ThreadHandle, NULL);
	if (!NT_SUCCESS(ntStatus))
	{
		DevExt->ThreadTermFlag = TRUE;
		KeSetEvent(&DevExt->RequestEvent, (KPRIORITY)0, FALSE);
		goto ERROUT;
	}

ERROUT:

	if (ThreadHandle)
		ZwClose(ThreadHandle);

	if (!NT_SUCCESS(ntStatus))
	{
		if (LowerDeviceObject)
			IoDetachDevice(LowerDeviceObject);
		if (FilterDeviceObject)
			IoDeleteDevice(FilterDeviceObject);
	}
	return ntStatus;
}

VOID NTAPI DPReadWriteThread(PVOID Context)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PDP_FILTER_DEV_EXTENSION DevExt = (PDP_FILTER_DEV_EXTENSION)Context;
	PLIST_ENTRY pRequestList = NULL;
	PIRP Irp = NULL;
	PIO_STACK_LOCATION irpsp = NULL;
	PBYTE sysBuf = NULL;
	ULONG length = 0;
	LARGE_INTEGER offset = { 0 };
	PBYTE fileBuf = NULL;
	PBYTE devBuf = NULL;
	IO_STATUS_BLOCK iso = { 0 };


	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);		// 设置线程优先级
	for (;;)
	{
		//先等待请求队列同步事件，如果队列中没有irp需要处理，我们的线程就等待在这里，让出cpu时间给其它线程
		KeWaitForSingleObject(&DevExt->RequestList, Executive, KernelMode, FALSE, NULL);

		if (DevExt->ThreadTermFlag)	// 判断线程结束标志
		{
			PsTerminateSystemThread(STATUS_SUCCESS);
			return;
		}
		while (0 < (pRequestList = ExInterlockedRemoveHeadList(&DevExt->RequestList, &DevExt->RequestLock)))
		{	// 使用自旋锁去除请求队列
			Irp = CONTAINING_RECORD(pRequestList, IRP, Tail.Overlay.ListEntry);		//从队列的入口里找到实际的irp的地址
			irpsp = IoGetCurrentIrpStackLocation(Irp);
			// //获取这个irp其中包含的缓存地址，这个地址可能来自mdl，也可能就是直接的缓冲，这取决于我们当前设备的io方式是buffer还是direct方式
			if (!Irp->MdlAddress)
				sysBuf = (PBYTE)Irp->UserBuffer;
			else
				sysBuf = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

			switch (irpsp->MajorFunction)
			{
			case IRP_MJ_READ:	//如果是读的irp请求，我们在irp stack中取得相应的参数作为offset和length
				offset = irpsp->Parameters.Read.ByteOffset;
				length = irpsp->Parameters.Read.Length;
				break;
			case IRP_MJ_WRITE:	//如果是写的irp请求，我们在irp stack中取得相应的参数作为offset和length
				offset = irpsp->Parameters.Write.ByteOffset;
				length = irpsp->Parameters.Write.Length;
				break;
			default:			//除此之外，offset和length都是0
				offset.QuadPart = 0;
				length = 0;
				break;
			}

			if (!sysBuf || !length)
				goto ERRNEXT;

			// 下面是转储的过程了
			if (irpsp->MajorFunction == IRP_MJ_READ)
			{	// 读过程
				long testResult = DPBitmapTest(DevExt->Bitmap, offset, length);
				switch (testResult)
				{
				case BITMAP_RANGE_CLEAR:	// 读取的操作全部是读取未转储的空间，也就是真正的磁盘上的内容，我们直接发给下层设备去处理
					goto ERRNEXT;
				case BITMAP_RANGE_SET:		// 读取的操作全部是读取已经转储的空间，也就是缓冲文件上的内容，我们从文件中读取出来，然后直接完成这个irp
					fileBuf = (PBYTE)ExAllocatePoolWithTag(NonPagedPool, length, 'xypZ');
					if (!fileBuf)
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					RtlZeroMemory(fileBuf, length);
					ntStatus = ZwReadFile(DevExt->TempFile, NULL, NULL, NULL, &iso, fileBuf, length, &offset, NULL);
					if (!NT_SUCCESS(ntStatus))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					Irp->IoStatus.Information = length;
					RtlCopyMemory(sysBuf, fileBuf, Irp->IoStatus.Information);
					goto ERRCMPLT;
					break;
				case BITMAP_RANGE_BLEND:	// 读取的操作是混合的，我们也需要从下层设备中读出，同时从文件中读出，然后混合并返回
					fileBuf = (PBYTE)ExAllocatePoolWithTag(NonPagedPool, length, 'xypZ');
					if (!fileBuf)
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					RtlZeroMemory(fileBuf, length);
					devBuf = ExAllocatePoolWithTag(NonPagedPool, length, 'xypZ');
					if (!devBuf)
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					RtlZeroMemory(devBuf, length);
					ntStatus = ZwReadFile(DevExt->TempFile, NULL, NULL, NULL, &iso, fileBuf, length, &offset, NULL);
					if (!NT_SUCCESS(ntStatus))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					//把这个irp发给下层设备去获取需要从设备上读取的信息存储到devBuf中
					ntStatus = DPForwardIrpSync(DevExt->LowerDeviceObject, Irp);
					if (!NT_SUCCESS(ntStatus))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					memcpy(devBuf, sysBuf, Irp->IoStatus.Information);
					// 从文件获取到的数据和从设备获取到的数据根据相应的bitmap值来进行合并，合并的结果放在devBuf中
					ntStatus = DPBitmapGet(DevExt->Bitmap, offset, length, devBuf, fileBuf);
					if (!NT_SUCCESS(ntStatus))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					// 合并完的数据存入系统缓冲并完成irp
					memcpy(sysBuf, devBuf, Irp->IoStatus.Information);
					goto ERRCMPLT;
					break;
				default:
					ntStatus = STATUS_INSUFFICIENT_RESOURCES;
					goto ERRERR;
					break;
				}
			}
			else
			{	// 写过程
				ntStatus = ZwWriteFile(DevExt->TempFile, NULL, NULL, NULL, &iso, sysBuf, length, &offset,  NULL);
				if (!NT_SUCCESS(ntStatus))
				{
					ntStatus = STATUS_INSUFFICIENT_RESOURCES;
					goto ERRERR;
				}
				ntStatus = DPBitmapSet(DevExt->Bitmap, offset, length);
				if (!NT_SUCCESS(ntStatus))
				{
					ntStatus = STATUS_INSUFFICIENT_RESOURCES;
					goto ERRERR;
				}
				goto ERRCMPLT;
			}
ERRERR:
			if (fileBuf)
			{
				ExFreePool(fileBuf);
				fileBuf = NULL;
			}
			if (devBuf)
			{
				ExFreePool(devBuf);
				devBuf = NULL;
			}
			DPCompleteRequest(Irp, ntStatus, IO_NO_INCREMENT);
			continue;

ERRNEXT:
			if (fileBuf)
			{
				ExFreePool(fileBuf);
				fileBuf = NULL;
			}
			if (devBuf)
			{
				ExFreePool(devBuf);
				devBuf = NULL;
			}
			DPSendToNextDriver(DevExt->LowerDeviceObject, Irp);
			continue;

ERRCMPLT:
			if (fileBuf)
			{
				ExFreePool(fileBuf);
				fileBuf = NULL;
			}
			if (devBuf)
			{
				ExFreePool(devBuf);
				devBuf = NULL;
			}
			DPCompleteRequest(Irp, STATUS_SUCCESS, IO_DISK_INCREMENT);
			continue;
		}
	}
}

NTSTATUS NTAPI DPSendToNextDriver(PDEVICE_OBJECT TargetDeviceObject, PIRP Irp)
{
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(TargetDeviceObject, Irp);
}

NTSTATUS NTAPI DPForwardIrpSync(PDEVICE_OBJECT TargetDeviceObject, PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	KEVENT event = { 0 };

	KeInitializeEvent(&event, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(Irp);

	// 设置完成函数，并且将等待事件设置为上面初始化的事件，如果完成函数被调用，这个事件将会被设置，同时也由此获知这个irp处理完成了
	IoSetCompletionRoutine(Irp, DPIrpCompletionRoutine, &event, TRUE, TRUE, TRUE);
	ntStatus = IoCallDriver(TargetDeviceObject, Irp);
	if (STATUS_PENDING == ntStatus)
	{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		ntStatus = Irp->IoStatus.Status;
	}
	return ntStatus;
}

NTSTATUS NTAPI DPIrpCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	PKEVENT Event = (PKEVENT)Context;

	KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS NTAPI DPCompleteRequest(PIRP Irp, NTSTATUS Status, CCHAR Priority)
{
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, Priority);
	return STATUS_SUCCESS;
}

NTSTATUS DPDispatchAny(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PDP_FILTER_DEV_EXTENSION DevExt = DeviceObject->DeviceExtension;
	return DPSendToNextDriver(DevExt->LowerDeviceObject, Irp);
}

NTSTATUS NTAPI DPDispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PDP_FILTER_DEV_EXTENSION DevExt = DeviceObject->DeviceExtension;

#if (NTDDI_VERSION < NTDDI_VISTA)
	// 如果是Windows Vista以前的版本,使用特殊的下层设备IRP转发函数
	PoStartNextPowerIrp(Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver(DevExt->LowerDeviceObject, Irp);
#else
	return DPSendToNextDriver(DevExt->LowerDeviceObject, Irp);
#endif // (NTDDI_VERSION <NTDDI_VISTA)
}

NTSTATUS NTAPI DPDispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PDP_FILTER_DEV_EXTENSION DevExt = DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);

	switch (irpsp->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE: {	// PnP管理器发送的移除设备IRP
		if (TRUE != DevExt->ThreadTermFlag && DevExt->ThreadHandle)
		{ // 如果线程还在运行,设置线程停止运行标志,并发送事件信息
			DevExt->ThreadTermFlag = TRUE;
			KeSetEvent(&DevExt->RequestEvent, (KPRIORITY)0, FALSE);
			KeWaitForSingleObject(DevExt->ThreadHandle, Executive, KernelMode, FALSE, NULL);
			ObDereferenceObject(DevExt->ThreadHandle);
		}
		if (DevExt->Bitmap)
			DPBitmapFree(DevExt->Bitmap);
		if (DevExt->LowerDeviceObject)
			IoDetachDevice(DevExt->LowerDeviceObject);
		if (DevExt->FilterDeviceObject)
			IoDeleteDevice(DevExt->FilterDeviceObject);
	}break;
	case IRP_MN_DEVICE_USAGE_NOTIFICATION: {	// 通告请求, 建立或删除特殊文件时想存储设备发送的IRP请求(页面文件、休眠文件、dump文件)
		BOOLEAN setPagable = FALSE;

		if (DeviceUsageTypePaging != irpsp->Parameters.UsageNotification.Type)
		{	// 如果不是询问分页文件,直接下发给下层设备去处理
			ntStatus = DPSendToNextDriver(DevExt->LowerDeviceObject, Irp);
			break;
		}

		// 等待分页计数事件
		ntStatus = KeWaitForSingleObject(&DevExt->PagingPathCountEvent, Executive, KernelMode, FALSE, NULL);

		// INFO: 建立分页文件查询请求 irpsp->Parameters.UsageNotification.InPath不为空,否则删除分页文件请求
		if (!irpsp->Parameters.UsageNotification.InPath && DevExt->PagingPathCount == 1)
		{	// 删除分页文件,并且只剩最后一个分页文件时
			if (!(DeviceObject->Flags & DO_POWER_INRUSH))
			{	// 没有分页文件在这个设备上了,需要设置DO_POWER_PAGABLE
				DeviceObject->Flags |= DO_POWER_PAGABLE;
				setPagable = TRUE;
			}
		}
		ntStatus = DPForwardIrpSync(DevExt->LowerDeviceObject, Irp);
		if (NT_SUCCESS(ntStatus))
		{	// 下发给下层设备的请求成功,说明下层设备支持这个操作
			IoAdjustPagingPathCount(&DevExt->PagingPathCount, irpsp->Parameters.UsageNotification.InPath);
			if (irpsp->Parameters.UsageNotification.InPath && DevExt->PagingPathCount == 1)
				DeviceObject->Flags &= ~DO_POWER_PAGABLE;	// 这个请求是建立分页文件查询请求,并且下层设备支持这个请求,并且这是第一个在这个设备的分页文件清除DO_POWER_PAGABLE位
		}
		else
		{	// 下发给下层设备的请求失败,下层设备不支持这个操作,将setPagable还原
			if (setPagable == TRUE)
			{
				DeviceObject->Flags &= ~DO_POWER_PAGABLE;
				setPagable = FALSE;
			}
		}

		// 设置分页计数事件
		KeSetEvent(&DevExt->PagingPathCountEvent, IO_NO_INCREMENT, FALSE);
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}break;
	default:
		ntStatus = DPSendToNextDriver(DevExt->LowerDeviceObject, Irp);
		break;
	}
	return ntStatus;
}

NTSTATUS NTAPI DPDispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PDP_FILTER_DEV_EXTENSION DevExt = DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	KEVENT Event = { 0 };
	VOLUME_ONLINE_CONTEXT VolumeContext;

	switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_VOLUME_ONLINE: { // 卷设备初始化请求
		KeInitializeEvent(&Event, NotificationEvent, FALSE);
		VolumeContext.DevExt = DevExt;
		VolumeContext.Event = &Event;
		IoCopyCurrentIrpStackLocationToNext(Irp);
		// 设置IRP完成处理回调函数并向下设备传递IRP
		IoSetCompletionRoutine(Irp, DPVolumeOnlineCompleteRoutine, &VolumeContext, TRUE, TRUE, TRUE);
		ntStatus = IoCallDriver(DevExt->LowerDeviceObject, Irp);
		KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
		return ntStatus;
	}break;
	default:
		break;
	}
	return DPSendToNextDriver(DevExt->LowerDeviceObject, Irp);
}

NTSTATUS NTAPI DPDispatchReadWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PDP_FILTER_DEV_EXTENSION DevExt = (PDP_FILTER_DEV_EXTENSION)DeviceObject->DeviceExtension;

	if (DevExt->Protect)
	{	// 当前卷在保护状态将IRP设置为pending状态,然后将这个irp放进相应的请求队列里
		IoMarkIrpPending(Irp);
		ExInterlockedInsertTailList(&DevExt->RequestList, &Irp->Tail.Overlay.ListEntry, &DevExt->RequestLock);
		// 设置队列等待事件
		KeSetEvent(&DevExt->RequestEvent, (KPRIORITY)0, FALSE);
		return STATUS_PENDING;
	}

	// 当前卷不在保护状态，直接交给下层设备进行处理
	return DPSendToNextDriver(DevExt->LowerDeviceObject, Irp);
}
