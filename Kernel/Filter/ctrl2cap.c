#include "ctrl2cap.h"
#include <ntddk.h>
#include <wdm.h>

extern POBJECT_TYPE IoDriverObjectType;	/* �ں�ȫ�ֱ���, ��������ֱ��ʹ��*/


NTSTATUS Ctrl2CaoMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	c2pAttachDevices(DriverObject, RegistryPath);
	return STATUS_SUCCESS;
}

NTSTATUS c2pAttachDevices(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING strNtNameString = { 0 };

	PC2P_DEV_EXT devExt = NULL;
	PDRIVER_OBJECT KbdDriverObject = NULL;
	PDEVICE_OBJECT pFileteDeviceObject = NULL;
	PDEVICE_OBJECT pTargetDeviceObject = NULL;
	PDEVICE_OBJECT pLowerDeviceObject = NULL;

	// // �򿪼�����������
	RtlInitUnicodeString(&strNtNameString, KBD_DRIVER_NAME);
	nStatus = ObReferenceObjectByName(&strNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);

	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("[dbg]: Couldn't get the  KbdDriver Object failed! DrverName=[%wZ]", strNtNameString));
		return(nStatus);
	}

	// ������������ü�������, ��ȡ������������ͷ�����
	ObDereferenceObject(KbdDriverObject);

	// �������������豸��
	pTargetDeviceObject = KbdDriverObject->DeviceObject;
	while (pTargetDeviceObject)
	{
		// ʹ�ü��������豸ջ�豸���Դ��������豸
		nStatus = IoCreateDevice(DriverObject, sizeof(PC2P_DEV_EXT), NULL, pTargetDeviceObject->DeviceType, pTargetDeviceObject->Characteristics,
			FALSE, OUT & pFileteDeviceObject);

		if (!NT_SUCCESS(nStatus))
		{
			KdPrint(("[dbg]: Couldn't Create the  Filter Device Object failed! STATUS=%x", nStatus));
			return (nStatus);
		}

		// �󶨹����豸
		pLowerDeviceObject = IoAttachDeviceToDeviceStack(pFileteDeviceObject, pTargetDeviceObject);
		if (!pLowerDeviceObject)
		{
			KdPrint(("[dbg]: Couldn't Attach to  Filter Device Object failed."));
			IoDeleteDevice(pFileteDeviceObject);
			pFileteDeviceObject = NULL;
			return nStatus;
		}

		// ��ʼ�������豸����չ����
		devExt = (PC2P_DEV_EXT)(pFileteDeviceObject->DeviceExtension);
		c2pFilterDevExtInit(devExt, pFileteDeviceObject, pTargetDeviceObject, pLowerDeviceObject);

		// �����豸��Ҫ��־
		pFileteDeviceObject->DeviceType = pLowerDeviceObject->DeviceType;
		pFileteDeviceObject->Characteristics = pLowerDeviceObject->Characteristics;
		pFileteDeviceObject->StackSize = pLowerDeviceObject->StackSize + 1;
		pFileteDeviceObject->Flags |= pLowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);

		// ָ�����,��һ�α���
		pTargetDeviceObject = pTargetDeviceObject->NextDevice;
	}
	return nStatus;
}

NTSTATUS c2pFilterDevExtInit(PC2P_DEV_EXT devExt, PDEVICE_OBJECT pFileteDeviceObject, PDEVICE_OBJECT pTargetDeviceObject, PDEVICE_OBJECT pLowerDeviceObject)
{
	memset(devExt, 0, sizeof(PC2P_DEV_EXT));
	devExt->NodeSize = sizeof(PC2P_DEV_EXT);
	devExt->FilterDeviceObject = pFileteDeviceObject;
	KeInitializeSpinLock(&(devExt->IoRequestsSpinLock));
	KeInitializeEvent(&(devExt->IoInProgressEvent), NotificationEvent, FALSE);
	devExt->TargetDeviceObject = pTargetDeviceObject;
	devExt->LowerDeviceObject = pLowerDeviceObject;
	return STATUS_SUCCESS;
}
