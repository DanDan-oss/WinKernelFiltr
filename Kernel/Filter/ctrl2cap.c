#include "ctrl2cap.h"
#include <ntddk.h>
#include <wdm.h>

#define DELAY_ONE_MICROSECOND (-10)
#define DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)


extern POBJECT_TYPE IoDriverObjectType;	/* �ں�ȫ�ֱ���, ��������ֱ��ʹ��*/
ULONG gC2pKeyCount = 0;

enum KeyBoardCode
{
	KBC_CAPS_LOCK = 0x3A,
	KBC_LCONTROL = 0x1D,
	KBC_P = 0x70,
	KBC_W = 0x77,
	KBC_w = 0x11,
	KBC_E = 0x65,
	KBC_e = 0x12
};

//  �Զ������ɨ�����ASCII�ַ�����
//  ������������1-10 ��ĸСд�ַ�(qwertyuiop)
char UnShift[] = {
    0, 0, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x2d, 0x3d, 0, 0,
    0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x6C, 0x6F, 0x70 };

//  �ϼ��̷��� ��ĸ��д(QWERTYUIOP)
char IsShfit[] = {
    0,0,0x21,0x40, 0x23, 0x24, 0x25, 0x5e, 0x26, 0x2a, 0x28, 0x29, 0x5f, 0x2b, 0, 0,
    0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4f, 0x50 };

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

	// ��ȡ��ǰ�߳̾���������߳����ȼ�Ϊʵʱ���ȼ�
	CurrentThread = KeGetCurrentThread();
	KeSetPriorityThread(CurrentThread, LOW_REALTIME_PRIORITY);

	UNREFERENCED_PARAMETER(DriverObject);
	KdPrint(("DriverEntry Ctrl2cap unLoading...\n"));

	// ѭ�������������豸ջ�����豸,��������豸�󶨲�ɾ��
	DeviceObject = DriverObject->DeviceObject;
	while (DeviceObject)
	{
		NewDeviceObject = DeviceObject->NextDevice;
		Ctrl2DetachDevices(DeviceObject);
		DeviceObject = NewDeviceObject;
	}
	ASSERT( DriverObject->DeviceObject ==  NULL);


	// Delay some time, 1����
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

	// �����豸��Դ���� IRP��������
	if (IRP_MJ_POWER == irpsp->MajorFunction)
		return Ctrl2CapDispatchPower(DeviceObject, Irp);

	// �����豸PNP(���弴��)����
	if (IRP_MJ_PNP == irpsp->MajorFunction)
		return Ctrl2CapDispatchPnP(DeviceObject, Irp);

	// ���̶�ȡ
	if (IRP_MJ_READ == irpsp->MajorFunction)
		return Ctrl2CapDispatchRead(DeviceObject, Irp);

	// �������󲻴���,ֱ���·�
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


	// ����ֱ���·�, Ȼ���ж��Ƿ���Ӳ���γ�,�����Ӳ���γ�����豸��,ɾ�����ɵĹ����豸, 
	IoSkipCurrentIrpStackLocation(Irp);
	nStatus = IoCallDriver(devExt->LowerDeviceObject, Irp);

	if (IRP_MN_REMOVE_DEVICE == irpsp->MinorFunction)
	{
		KdPrint(("[dbg]: IRP_MN_REMOVE_DEVICE\n"));

		// �����,ɾ�������豸
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


	//�жϵ�ǰIRP�Ƿ���IRPջ��׶�,����� ���Ǵ��������,����IRP����, ����IRP
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
	//����IRPջ��׶�,IRP��������1
	++gC2pKeyCount;

	//TODO: ��ȡ�������豸ָ��
	devExt = (PC2P_DEV_EXT)DeviceObject->DeviceExtension;
	irpsp = IoGetCurrentIrpStackLocation(Irp);

	// ����IRP��ɻص�����, ������ǰIRPջ�ռ�,����������̷��͸��²�����,Ȼ��ȴ�����������ȡ��ɨ�������IRP�󷵻�
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, Ctrl2CapReadComplete, DeviceObject, TRUE, TRUE, TRUE);
	return IoCallDriver(devExt->LowerDeviceObject, Irp);

	
}

NTSTATUS Ctrl2CapReadComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Context);

	// IRP��������1,
	--gC2pKeyCount;

	//�жϵ�ǰIRP�����Ƿ�ִ�гɹ�,���ִ�гɹ���ȡ�豸��������,��������û������
	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		Ctrl2CapDataAnalysis(Irp);
	}

	// �ж�IRP�ĵ��ȱ�ʶ�Ƿ�ʱ�ǹ���״̬, �����,�ֶ�����IRP���ȹ���״̬
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);

	return Irp->IoStatus.Status;
}

NTSTATUS Ctrl2CapAttachDevices(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING strNtNameString = { 0 };

	PC2P_DEV_EXT devExt = NULL;		//  �Զ����豸��չ�ṹ ָ��
	PDRIVER_OBJECT KbdDriverObject = NULL;		// ������������
	PDEVICE_OBJECT pFileteDeviceObject = NULL;	// ���ɹ����豸����ָ��
	PDEVICE_OBJECT pTargetDeviceObject = NULL;	// �󶨳ɹ������ʵ�豸����ָ��,�������ظ�ֵΪ0��ʾ��ʧ��
	PDEVICE_OBJECT pLowerDeviceObject = NULL;	// ���������豸ջ���豸����

	// ͨ�����ֻ�ȡ��������ָ��, ��ȡ����ͷŶ���,����ָ��
	RtlInitUnicodeString(&strNtNameString, KBD_DRIVER_NAME);
	nStatus = ObReferenceObjectByName(&strNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("[dbg]: Couldn't get the  KbdDriver Object failed! DrverName=[%wZ]", strNtNameString));
		return(nStatus);
	}

	// ����ObReferenceObjectByName��ʹ������������ü�������,�ͷ�������������
	ObDereferenceObject(KbdDriverObject);

	// �������������豸��
	pTargetDeviceObject = KbdDriverObject->DeviceObject;
	while (pTargetDeviceObject)
	{
		// ���ɹ����豸
		nStatus = IoCreateDevice(DriverObject, sizeof(PC2P_DEV_EXT), NULL, pTargetDeviceObject->DeviceType, pTargetDeviceObject->Characteristics,
			FALSE, OUT & pFileteDeviceObject);

		if (!NT_SUCCESS(nStatus))
		{
			KdPrint(("[dbg]: Couldn't Create the  Filter Device Object failed! STATUS=%x", nStatus));
			return (nStatus);
		}

		//  �󶨹����豸
		pLowerDeviceObject = IoAttachDeviceToDeviceStack(pFileteDeviceObject, pTargetDeviceObject);
		if (!pLowerDeviceObject)
		{
			KdPrint(("[dbg]: Couldn't Attach to  Filter Device Object failed."));
			IoDeleteDevice(pFileteDeviceObject);
			pFileteDeviceObject = NULL;
			return nStatus;
		}

		// ��ʼ�������豸����չ����(�����豸����ʱָ���Ĵ�С)
		devExt = (PC2P_DEV_EXT)(pFileteDeviceObject->DeviceExtension);
		Ctrl2CapFilterDevExtInit(devExt, pFileteDeviceObject, pTargetDeviceObject, pLowerDeviceObject);

		// �����豸��Ҫ��־��־
		pFileteDeviceObject->DeviceType = pLowerDeviceObject->DeviceType;
		pFileteDeviceObject->Characteristics = pLowerDeviceObject->Characteristics;
		pFileteDeviceObject->StackSize = pLowerDeviceObject->StackSize + 1;
		pFileteDeviceObject->Flags |= pLowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);

		// ָ�����,��һ�α���
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

	// ��ʼ���ṹ���������
	KeInitializeSpinLock(&(devExt->IoRequestsSpinLock));
	KeInitializeEvent(&(devExt->IoInProgressEvent), NotificationEvent, FALSE);

	return STATUS_SUCCESS;
}


NTSTATUS Ctrl2DetachDevices(PDEVICE_OBJECT DeviceObject)
{
    PC2P_DEV_EXT devExt = DeviceObject->DeviceExtension;

	 if (!devExt->FilterDeviceObject || DeviceObject != devExt->FilterDeviceObject)
	 {
		 KdPrint(("[dbg]: Couldn't Detach to  Filter Device Object failed. %p", devExt->FilterDeviceObject));
		 return STATUS_DEVICE_ENUMERATION_ERROR;
	 }
         
     //����豸��չ����󶨵��豸����İ�
    IoDetachDevice(devExt->TargetDeviceObject);
    devExt->FilterDeviceObject = NULL;
    devExt->TargetDeviceObject = NULL;
    devExt->LowerDeviceObject = NULL;
    IoDeleteDevice(DeviceObject);
    return STATUS_SUCCESS;
}

NTSTATUS Ctrl2CapDataAnalysis(PIRP Irp)
{
	PKEYBOARD_INPUT_DATA pKeyData = NULL;
	LONGLONG bufLen = 0;	 /* ���ݳ���*/
	ULONGLONG KeyBoardNum = 0;            // KEYBOARD_INPUT_DATA�ṹ����

	pKeyData = Irp->AssociatedIrp.SystemBuffer;
	bufLen = Irp->IoStatus.Information;
	KeyBoardNum = bufLen / sizeof(KEYBOARD_INPUT_DATA);

	for (int i = 0; i < KeyBoardNum; ++i)
	{
		KdPrint(("[dbg]:Ctrl2CapDataAnalysis numKeys:%d\n", KeyBoardNum));
		KdPrint(("[dbg]:Ctrl2CapDataAnalysis scanCode:%x\n", pKeyData->MakeCode));
		KdPrint(("[dbg]:Ctrl2CapDataAnalysis %s\n", pKeyData->Flags ? "Up" : "Down"));


		// ��������: ��Caps Lock������Ctrl��. W������E��
		if (pKeyData->MakeCode == KBC_CAPS_LOCK)
			pKeyData->MakeCode = KBC_LCONTROL;
		if (pKeyData->MakeCode == KBC_W)
			pKeyData->MakeCode = KBC_E;

	}

	return STATUS_SUCCESS;
}
