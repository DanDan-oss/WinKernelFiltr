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

		// ע��boot���������ص�����
		IoRegisterBootDriverReinitialization(DriverObject, DPReinitializationRoutine, NULL);

		KdPrint(("[dbg][%ws]Driver Object Address:%p, Current IRQL=0x%u\n", __FUNCTIONW__, DriverObject, KeGetCurrentIrql()));
	}

	KdPrint(("[dbg][%ws] Built %s %s \n", __FUNCTIONW__, __DATE__, __TIME__));
	return STATUS_SUCCESS;
}

VOID NTAPI DpUnload(PDRIVER_OBJECT DriverObject)
{
	//����������Ṥ����ϵͳ�ػ�,����������ж�ص�ʱ�����κ�������
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
	LowerDeviceObject = IoAttachDeviceToDeviceStack(FilterDeviceObject, PhysicalDeviceObject);		// �豸��
	if (!LowerDeviceObject)
	{
		ntStatus = STATUS_NO_SUCH_DEVICE;
		goto ERROUT;
	}

	DevExt = FilterDeviceObject->DeviceExtension;
	RtlZeroMemory(DevExt, sizeof(DP_FILTER_DEV_EXTENSION));
	// ��ʼ�������豸����
	FilterDeviceObject->Flags = LowerDeviceObject->Flags;
	FilterDeviceObject->Flags |= DO_POWER_PAGABLE;	//��Դ�ɷ�ҳ����
	FilterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	DevExt->FilterDeviceObject = FilterDeviceObject;
	DevExt->PhyDeviceObject = PhysicalDeviceObject;
	DevExt->LowerDeviceObject = LowerDeviceObject;

	// ��ʼ���¼�
	KeInitializeEvent(&DevExt->PagingPathCountEvent, NotificationEvent, TRUE);
	InitializeListHead(&DevExt->RequestList);
	KeInitializeSpinLock(&DevExt->RequestLock);
	KeInitializeEvent(&DevExt->RequestEvent, SynchronizationEvent, FALSE);

	// ��ʼ�������߳�,�̺߳�����������Ϊ�豸��չ
	DevExt->ThreadTermFlag = FALSE;
	ntStatus = PsCreateSystemThread(&ThreadHandle, (ACCESS_MASK)0L, NULL, NULL, NULL, DPReadWriteThread, DevExt);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	// ��ȡ�����̶߳�����
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


	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);		// �����߳����ȼ�
	for (;;)
	{
		//�ȵȴ��������ͬ���¼������������û��irp��Ҫ�������ǵ��߳̾͵ȴ�������ó�cpuʱ��������߳�
		KeWaitForSingleObject(&DevExt->RequestList, Executive, KernelMode, FALSE, NULL);

		if (DevExt->ThreadTermFlag)	// �ж��߳̽�����־
		{
			PsTerminateSystemThread(STATUS_SUCCESS);
			return;
		}
		while (0 < (pRequestList = ExInterlockedRemoveHeadList(&DevExt->RequestList, &DevExt->RequestLock)))
		{	// ʹ��������ȥ���������
			Irp = CONTAINING_RECORD(pRequestList, IRP, Tail.Overlay.ListEntry);		//�Ӷ��е�������ҵ�ʵ�ʵ�irp�ĵ�ַ
			irpsp = IoGetCurrentIrpStackLocation(Irp);
			// //��ȡ���irp���а����Ļ����ַ�������ַ��������mdl��Ҳ���ܾ���ֱ�ӵĻ��壬��ȡ�������ǵ�ǰ�豸��io��ʽ��buffer����direct��ʽ
			if (!Irp->MdlAddress)
				sysBuf = (PBYTE)Irp->UserBuffer;
			else
				sysBuf = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

			switch (irpsp->MajorFunction)
			{
			case IRP_MJ_READ:	//����Ƕ���irp����������irp stack��ȡ����Ӧ�Ĳ�����Ϊoffset��length
				offset = irpsp->Parameters.Read.ByteOffset;
				length = irpsp->Parameters.Read.Length;
				break;
			case IRP_MJ_WRITE:	//�����д��irp����������irp stack��ȡ����Ӧ�Ĳ�����Ϊoffset��length
				offset = irpsp->Parameters.Write.ByteOffset;
				length = irpsp->Parameters.Write.Length;
				break;
			default:			//����֮�⣬offset��length����0
				offset.QuadPart = 0;
				length = 0;
				break;
			}

			if (!sysBuf || !length)
				goto ERRNEXT;

			// ������ת���Ĺ�����
			if (irpsp->MajorFunction == IRP_MJ_READ)
			{	// ������
				long testResult = DPBitmapTest(DevExt->Bitmap, offset, length);
				switch (testResult)
				{
				case BITMAP_RANGE_CLEAR:	// ��ȡ�Ĳ���ȫ���Ƕ�ȡδת���Ŀռ䣬Ҳ���������Ĵ����ϵ����ݣ�����ֱ�ӷ����²��豸ȥ����
					goto ERRNEXT;
				case BITMAP_RANGE_SET:		// ��ȡ�Ĳ���ȫ���Ƕ�ȡ�Ѿ�ת���Ŀռ䣬Ҳ���ǻ����ļ��ϵ����ݣ����Ǵ��ļ��ж�ȡ������Ȼ��ֱ��������irp
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
				case BITMAP_RANGE_BLEND:	// ��ȡ�Ĳ����ǻ�ϵģ�����Ҳ��Ҫ���²��豸�ж�����ͬʱ���ļ��ж�����Ȼ���ϲ�����
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
					//�����irp�����²��豸ȥ��ȡ��Ҫ���豸�϶�ȡ����Ϣ�洢��devBuf��
					ntStatus = DPForwardIrpSync(DevExt->LowerDeviceObject, Irp);
					if (!NT_SUCCESS(ntStatus))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					memcpy(devBuf, sysBuf, Irp->IoStatus.Information);
					// ���ļ���ȡ�������ݺʹ��豸��ȡ�������ݸ�����Ӧ��bitmapֵ�����кϲ����ϲ��Ľ������devBuf��
					ntStatus = DPBitmapGet(DevExt->Bitmap, offset, length, devBuf, fileBuf);
					if (!NT_SUCCESS(ntStatus))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					// �ϲ�������ݴ���ϵͳ���岢���irp
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
			{	// д����
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

	// ������ɺ��������ҽ��ȴ��¼�����Ϊ�����ʼ�����¼��������ɺ��������ã�����¼����ᱻ���ã�ͬʱҲ�ɴ˻�֪���irp���������
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
	// �����Windows Vista��ǰ�İ汾,ʹ��������²��豸IRPת������
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
	case IRP_MN_REMOVE_DEVICE: {	// PnP���������͵��Ƴ��豸IRP
		if (TRUE != DevExt->ThreadTermFlag && DevExt->ThreadHandle)
		{ // ����̻߳�������,�����߳�ֹͣ���б�־,�������¼���Ϣ
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
	case IRP_MN_DEVICE_USAGE_NOTIFICATION: {	// ͨ������, ������ɾ�������ļ�ʱ��洢�豸���͵�IRP����(ҳ���ļ��������ļ���dump�ļ�)
		BOOLEAN setPagable = FALSE;

		if (DeviceUsageTypePaging != irpsp->Parameters.UsageNotification.Type)
		{	// �������ѯ�ʷ�ҳ�ļ�,ֱ���·����²��豸ȥ����
			ntStatus = DPSendToNextDriver(DevExt->LowerDeviceObject, Irp);
			break;
		}

		// �ȴ���ҳ�����¼�
		ntStatus = KeWaitForSingleObject(&DevExt->PagingPathCountEvent, Executive, KernelMode, FALSE, NULL);

		// INFO: ������ҳ�ļ���ѯ���� irpsp->Parameters.UsageNotification.InPath��Ϊ��,����ɾ����ҳ�ļ�����
		if (!irpsp->Parameters.UsageNotification.InPath && DevExt->PagingPathCount == 1)
		{	// ɾ����ҳ�ļ�,����ֻʣ���һ����ҳ�ļ�ʱ
			if (!(DeviceObject->Flags & DO_POWER_INRUSH))
			{	// û�з�ҳ�ļ�������豸����,��Ҫ����DO_POWER_PAGABLE
				DeviceObject->Flags |= DO_POWER_PAGABLE;
				setPagable = TRUE;
			}
		}
		ntStatus = DPForwardIrpSync(DevExt->LowerDeviceObject, Irp);
		if (NT_SUCCESS(ntStatus))
		{	// �·����²��豸������ɹ�,˵���²��豸֧���������
			IoAdjustPagingPathCount(&DevExt->PagingPathCount, irpsp->Parameters.UsageNotification.InPath);
			if (irpsp->Parameters.UsageNotification.InPath && DevExt->PagingPathCount == 1)
				DeviceObject->Flags &= ~DO_POWER_PAGABLE;	// ��������ǽ�����ҳ�ļ���ѯ����,�����²��豸֧���������,�������ǵ�һ��������豸�ķ�ҳ�ļ����DO_POWER_PAGABLEλ
		}
		else
		{	// �·����²��豸������ʧ��,�²��豸��֧���������,��setPagable��ԭ
			if (setPagable == TRUE)
			{
				DeviceObject->Flags &= ~DO_POWER_PAGABLE;
				setPagable = FALSE;
			}
		}

		// ���÷�ҳ�����¼�
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
	case IOCTL_VOLUME_ONLINE: { // ���豸��ʼ������
		KeInitializeEvent(&Event, NotificationEvent, FALSE);
		VolumeContext.DevExt = DevExt;
		VolumeContext.Event = &Event;
		IoCopyCurrentIrpStackLocationToNext(Irp);
		// ����IRP��ɴ���ص������������豸����IRP
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
	{	// ��ǰ���ڱ���״̬��IRP����Ϊpending״̬,Ȼ�����irp�Ž���Ӧ�����������
		IoMarkIrpPending(Irp);
		ExInterlockedInsertTailList(&DevExt->RequestList, &Irp->Tail.Overlay.ListEntry, &DevExt->RequestLock);
		// ���ö��еȴ��¼�
		KeSetEvent(&DevExt->RequestEvent, (KPRIORITY)0, FALSE);
		return STATUS_PENDING;
	}

	// ��ǰ���ڱ���״̬��ֱ�ӽ����²��豸���д���
	return DPSendToNextDriver(DevExt->LowerDeviceObject, Irp);
}
