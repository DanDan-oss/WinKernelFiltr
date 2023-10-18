#include "FilterDP.h"
#include <ntddvol.h>

PDP_FILTER_DEV_EXTENSION gProtectDevExt = NULL;
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
	if(!NT_SUCCESS(ntStatus))
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

NTSTATUS NTAPI DPDispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PDP_FILTER_DEV_EXTENSION DevExt = DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);

	switch (irpsp->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE: {	// PnP���������͵��Ƴ��豸IRP
		if (TRUE != DevExt->ThreadTermFlag && NULL != DevExt->ThreadHandle)
		{ // ����̻߳�������,�����߳�ֹͣ���б�־,�������¼���Ϣ
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
		ntStatus = DPforwardIrpSync(DevExt->LowerDeviceObject, Irp);
		if (NT_SUCCESS(ntStatus))
		{	// �·����²��豸������ɹ�,˵���²��豸֧���������
			IoAdjustPagingPathCount(&DevExt->PagingPathCount, irpsp->Parameters.UsageNotification.InPath);
			if (irpsp->Parameters.UsageNotification.InPath && DevExt->PagingPathCount == 1 )
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

NTSTATUS NTAPI DPDispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PDP_FILTER_DEV_EXTENSION DevExt = DeviceObject->DeviceExtension;

#if (NTDDI_VERSION < NTDDI_VISTA)
	// �����Windows Vista��ǰ�İ汾,ʹ��������²��豸IRPת������
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

NTSTATUS DPVolumeOnlineCompleteRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING DosName = { 0 };		// ���豸��dos��, C��D
	PVOLUME_ONLINE_CONTEXT VolumeContext = (PVOLUME_ONLINE_CONTEXT)Context;

	ASSERT(VolumeContext != NULL);
	ntStatus = IoVolumeDeviceToDosName(VolumeContext->DevExt->PhyDeviceObject, &DosName);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	// ������ת��Ϊ��д
	VolumeContext->DevExt->VolumeLetter = DosName.Buffer[0];
	if (VolumeContext->DevExt->VolumeLetter > TEXT('Z'))
		VolumeContext->DevExt->VolumeLetter -= (TEXT('a') - TEXT('A'));

	// ����C��, ��ȡC����Ϣ
	if (VolumeContext->DevExt->VolumeLetter == TEXT('C'))
	{
		// ��ȡ����Ϣ
		ntStatus = DPQueryVolumeInformation(VolumeContext->DevExt->PhyDeviceObject, &VolumeContext->DevExt->TotalSizeInByte, &VolumeContext->DevExt->ClusterSizeInByte, \
			&VolumeContext->DevExt->SectorSizeInByte);
		if(!NT_SUCCESS(ntStatus))
			goto ERROUT;
		// �������Ӧλͼ
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
	
	// ���õȴ�ͬ���¼�
	KeSetEvent(VolumeContext->Event, 0, FALSE);
	return ntStatus;
}
