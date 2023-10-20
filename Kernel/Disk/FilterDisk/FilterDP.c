#include "FilterDP.h"
#include <ntddvol.h>

PDP_FILTER_DEV_EXTENSION gProtectDevExt = NULL;
static tBitmap bitmapMask[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };

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
	if(!NT_SUCCESS(ntStatus))
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

NTSTATUS NTAPI DPDispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PDP_FILTER_DEV_EXTENSION DevExt = DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);

	switch (irpsp->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE: {	// PnP管理器发送的移除设备IRP
		if (TRUE != DevExt->ThreadTermFlag && NULL != DevExt->ThreadHandle)
		{ // 如果线程还在运行,设置线程停止运行标志,并发送事件信息
			DevExt->ThreadTermFlag = TRUE;
			KeSetEvent(&DevExt->RequestEvent, (KPRIORITY)0, FALSE);
			KeWaitForSingleObject(DevExt->ThreadHandle, Executive, KernelMode, FALSE, NULL);
		}
		ObDereferenceObject(DevExt->ThreadHandle);
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
		ntStatus = DPforwardIrpSync(DevExt->LowerDeviceObject, Irp);
		if (NT_SUCCESS(ntStatus))
		{	// 下发给下层设备的请求成功,说明下层设备支持这个操作
			IoAdjustPagingPathCount(&DevExt->PagingPathCount, irpsp->Parameters.UsageNotification.InPath);
			if (irpsp->Parameters.UsageNotification.InPath && DevExt->PagingPathCount == 1 )
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

NTSTATUS NTAPI DPDispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PDP_FILTER_DEV_EXTENSION DevExt = DeviceObject->DeviceExtension;

#if (NTDDI_VERSION < NTDDI_VISTA)
	// 如果是Windows Vista以前的版本,使用特殊的下层设备IRP转发函数
	PoStartNextPowerIrp(Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver(DevExt->LowerDeviceObject, Irp);
#else
	return DPSendToNextDriver(DevExt->LowerDeviceObject);
#endif // (NTDDI_VERSION <NTDDI_VISTA)
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

NTSTATUS DPVolumeOnlineCompleteRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING DosName = { 0 };		// 卷设备的dos名, C、D
	PVOLUME_ONLINE_CONTEXT VolumeContext = (PVOLUME_ONLINE_CONTEXT)Context;

	ASSERT(VolumeContext != NULL);
	ntStatus = IoVolumeDeviceToDosName(VolumeContext->DevExt->PhyDeviceObject, &DosName);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	// 将名字转换为大写
	VolumeContext->DevExt->VolumeLetter = DosName.Buffer[0];
	if (VolumeContext->DevExt->VolumeLetter > TEXT('Z'))
		VolumeContext->DevExt->VolumeLetter -= (TEXT('a') - TEXT('A'));

	// 保护C盘, 获取C盘信息
	if (VolumeContext->DevExt->VolumeLetter == TEXT('C'))
	{
		// 获取卷信息
		ntStatus = DPQueryVolumeInformation(VolumeContext->DevExt->PhyDeviceObject, &VolumeContext->DevExt->TotalSizeInByte, &VolumeContext->DevExt->ClusterSizeInByte, \
			&VolumeContext->DevExt->SectorSizeInByte);
		if(!NT_SUCCESS(ntStatus))
			goto ERROUT;
		// 建立卷对应位图
		ntStatus = DPBitmapInit(&VolumeContext->DevExt->Bitmap, VolumeContext->DevExt->SectorSizeInByte, 8, 25600, \
			(DWORD)(VolumeContext->DevExt->TotalSizeInByte.QuadPart / (QWORD)(25600 * 8 * VolumeContext->DevExt->SectorSizeInByte)) + 1);
		if (!NT_SUCCESS(ntStatus))
			goto ERROUT;
		gProtectDevExt = VolumeContext->DevExt;
	}

ERROUT:
	if (!NT_SUCCESS(ntStatus))
	{
		if (VolumeContext->DevExt->Bitmap)
			DPBitmapFree(VolumeContext->DevExt->Bitmap);
		if (VolumeContext->DevExt->TempFile)
			ZwClose(VolumeContext->DevExt->TempFile);
	}
	if (DosName.Buffer)
		ExFreePool(DosName.Buffer);
	
	// 设置等待同步事件
	KeSetEvent(VolumeContext->Event, 0, FALSE);
	return ntStatus;
}

NTSTATUS DPBitmapInit(DP_BITMAP** bitmap, unsigned long sectorSize, unsigned long byteSize, unsigned long regionSize, unsigned long regionNumber)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	DP_BITMAP* myBitmap = NULL;

	if (NULL == bitmap || 0 == byteSize || 0 == byteSize || 0 == regionSize || 0 == regionNumber)
		return STATUS_UNSUCCESSFUL;

	__try 
	{
		// 分配bitmap结构内存,并初始化赋值
		myBitmap = (DP_BITMAP*)DPBitmapAlloc(0, sizeof(DP_BITMAP));
		if (NULL == myBitmap)
		{
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		RtlIsZeroMemory(myBitmap, sizeof(DP_BITMAP));
		myBitmap->sectorSize = sectorSize;
		myBitmap->byteSize = byteSize;
		myBitmap->regionSize = regionSize;
		myBitmap->regionNumber = regionNumber;
		myBitmap->regionReferSize = sectorSize * byteSize * regionSize;
		myBitmap->bitmapReferSize = (QWORD)sectorSize * (QWORD)byteSize * (QWORD)regionSize * (QWORD)regionNumber;

		// 分配出regionNumber个指向region的指针. 指针数组
		myBitmap->BitMap = (tBitmap**)DPBitmapAlloc(0, sizeof(tBitmap*) * regionNumber);
		if (NULL == myBitmap->BitMap)
		{
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		RtlIsZeroMemory(myBitmap->BitMap, sizeof(tBitmap*) * regionNumber);
		ntStatus = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (!NT_SUCCESS(ntStatus))
	{
		if (myBitmap)
			DPBitmapFree(myBitmap);
		*bitmap = NULL;
	}
	return ntStatus;
}

NTSTATUS DPBitmapSet(DP_BITMAP* bitmap, LARGE_INTEGER offset, unsigned long length)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	DWORD bitPos = 0;
	DWORD regionBegin = 0, regionEnd = 0;
	DWORD regionOffsetBegin = 0, regionOffsetEnd = 0;
	DWORD byteOffsetBegin = 0, byteOffsetEnd = 0;
	LARGE_INTEGER setBegin = { 0 }, setEnd = { 0 };
	QWORD index = 0;
	__try
	{
		// 检查变量
		if (NULL == bitmap || offset.QuadPart < 0 || \
			0 != offset.QuadPart % bitmap->sectorSize || 0 != length % bitmap->sectorSize )
		{
			ntStatus = STATUS_INVALID_PARAMETER;
			__leave;
		}

		// 根据要设置的偏移量和长度来计算需要使用哪些region, 如果需要就分配它们指向的内存空间
		regionBegin = (DWORD)(offset.QuadPart / (QWORD)bitmap->regionReferSize);
		regionEnd = (DWORD)((offset.QuadPart + (QWORD)length) / (QWORD)bitmap->regionReferSize);
		for (index = 0; index < regionEnd; ++index)
			if (NULL == *(bitmap->BitMap + index))
			{
				*(bitmap->BitMap + index) = (tBitmap*)DPBitmapAlloc(0, sizeof(tBitmap) * bitmap->regionSize);
				if ( NULL == *(bitmap->BitMap + index))
				{
					ntStatus = STATUS_INSUFFICIENT_RESOURCES;
					__leave;
				}
				else
				{
					RtlZeroMemory(*(bitmap->BitMap + index), sizeof(tBitmap) * bitmap->regionSize);
				}
			}

		// 开始设置bitmap， 需要将要设置的区域安装字节对齐(这样可以按字节设置而不需要按位设置), 对于没有字节对齐的区域手动设置
		for (QWORD index = offset.QuadPart; index < offset.QuadPart + (QWORD)length; index += bitmap->sectorSize)
		{
			regionBegin = (DWORD)(index / (QWORD)bitmap->regionReferSize);
			regionOffsetBegin = (DWORD)(index % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize % bitmap->sectorSize;
			bitPos = (byteOffsetBegin / bitmap->sectorSize) % bitmap->byteSize;
			if (!bitPos)
			{
				setBegin.QuadPart = index;
				break;
			}
			*(*(bitmap->BitMap + regionBegin) + regionOffsetBegin) |= bitmapMask[bitPos];
		}
		if (index >= offset.QuadPart + (QWORD)length)
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}
		for (index = offset.QuadPart + (QWORD)length - bitmap->sectorSize; index >= offset.QuadPart; index -= bitmap->sectorSize)
		{
			regionBegin = (DWORD)(index / bitmap->regionReferSize);
			regionOffsetBegin = (DWORD)(index % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize / bitmap->sectorSize;
			bitPos = (regionOffsetBegin / bitmap->sectorSize) % bitmap->byteSize;
			if (7 == bitPos)
			{
				setEnd.QuadPart = index;
				break;
			}
			*(*(bitmap->BitMap + regionBegin) + byteOffsetBegin) |= bitmapMask[bitPos];
		}
		if (index < offset.QuadPart || setEnd.QuadPart == setBegin.QuadPart)
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}
		regionEnd = (DWORD)(setEnd.QuadPart / (QWORD)bitmap->regionReferSize);
		for (index = setBegin.QuadPart; index <= setEnd.QuadPart; )
		{
			regionBegin = (DWORD)(index / (QWORD)bitmap->regionReferSize);
			regionOffsetBegin = (DWORD)(index % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize / bitmap->sectorSize;
			if (regionBegin == regionEnd)
			{	//如果我们设置的区域没有跨两个region，只需要使用memset去做按byte的设置然后跳出即可
				regionOffsetEnd = (DWORD)(setEnd.QuadPart % (QWORD)bitmap->regionReferSize);
				byteOffsetEnd = regionOffsetEnd / bitmap->byteSize / bitmap->sectorSize;
				memset(*(bitmap->BitMap + regionBegin) + byteOffsetBegin, 0xff, byteOffsetEnd - byteOffsetBegin + 1);
				break;
			}
			else
			{	//如果我们设置的区域跨了两个region，需要设置完后递增
				regionOffsetEnd = bitmap->regionReferSize;
				byteOffsetEnd = regionOffsetEnd / bitmap->byteSize / bitmap->sectorSize;
				memset(*(bitmap->BitMap + regionBegin) + byteOffsetBegin, 0xff, byteOffsetEnd - byteOffsetBegin);
				index += (byteOffsetEnd - byteOffsetBegin) * bitmap->byteSize * bitmap->sectorSize;
			}
		}
		ntStatus = STATUS_SUCCESS;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ntStatus = STATUS_UNSUCCESSFUL;
	}

	if (!NT_SUCCESS(ntStatus))
	{

	}
	return ntStatus;
}
