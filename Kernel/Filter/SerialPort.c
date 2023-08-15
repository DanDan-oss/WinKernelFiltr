#include "SerialPort.h"
#include <ntstrsafe.h>

#define CCP_MAX_COM_ID 32		// ������32������
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

	// ��������豸�İ�
	for (int i = 0; i < CCP_MAX_COM_ID; ++i)
		if (s_nextObject[i])
			IoDetachDevice(s_nextObject[i]);

	// ����󶨺�ȴ�5��,���������ڴ����IRP����
	laInterVal.QuadPart = (5 * 1000 * DELAY_ONE_MILLISECOND);
	KeDelayExecutionThread(KernelMode, FALSE, &laInterVal);

	// ɾ�������豸
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
		// ��ȡ���ڶ�������
		comObject = ccpOpenCom(i, &nStatus);
		if (!comObject)
			continue;

		// �������Ƿ�󶨳ɹ�
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

	// ����idת���ɴ��ڵĺ�׺��
	RtlStringCchPrintfW(wcName, 32, L"Device\\Serial%d", ID);
	RtlInitUnicodeString(&nameStr, wcName);

	// ���豸����
	*Status = IoGetDeviceObjectPointer(&nameStr, FILE_ALL_ACCESS, &fileObject, &devObject);
	if (!NT_SUCCESS(*Status))
	{
		KdPrint(("[dbg]: open Serial Port Device failed: %wZ \n", nameStr));
		return NULL;
	}
		

	// ������ļ����������
	ObDereferenceObject(fileObject);
	return devObject;
}


NTSTATUS ccpAttachDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT OldDeviceObject, PDEVICE_OBJECT* FiltDeviceObject, PDEVICE_OBJECT* Next)
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;

	PDEVICE_OBJECT topDev = NULL;

	// �����豸
	nStatus = IoCreateDevice(DriverObject, 0, NULL, OldDeviceObject->DeviceType, 0, FALSE, OUT FiltDeviceObject);
	if (!NT_SUCCESS(nStatus))
		return nStatus;

	// �����豸��Ҫ��־
	if (OldDeviceObject->Flags & DO_BUFFERED_IO)
		(*FiltDeviceObject)->Flags |= DO_BUFFERED_IO;
	if (OldDeviceObject->Flags & DO_DIRECT_IO)
		(*FiltDeviceObject)->Flags |= DO_DIRECT_IO;
	if (OldDeviceObject->Characteristics & FILE_DEVICE_SECURE_OPEN)
		(*FiltDeviceObject)->Characteristics |= FILE_DEVICE_SECURE_OPEN;
	(*FiltDeviceObject)->Flags |= DO_POWER_PAGABLE;

	// ���豸�󶨵���һ���豸��
	topDev = IoAttachDeviceToDeviceStack(*FiltDeviceObject, OldDeviceObject);
	if (!topDev)
	{
		IoDeleteDevice(*FiltDeviceObject);
		*FiltDeviceObject = NULL;
		return STATUS_UNSUCCESSFUL;
	}

	// �����豸�Ѿ�����
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
			// ��Դ����
			if (irpsp->MajorFunction == IRP_MJ_POWER)
			{
				PoStartNextPowerIrp(Irp);
				IoSkipCurrentIrpStackLocation(Irp);
				return PoCallDriver(s_nextObject[i], Irp);
			}

			// д����
			if (irpsp->MajorFunction == IRP_MJ_WRITE)
			{
				bufferLen = irpsp->Parameters.Write.Length;
				if (Irp->MdlAddress)
					pBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
				else if (Irp->UserBuffer)
					pBuffer = Irp->UserBuffer;
				else
					pBuffer = Irp->AssociatedIrp.SystemBuffer;

				// ��ӡ����
				if(pBuffer)
					for (UINT32 j = 0; j < bufferLen; ++j)
						KdPrint(("[dbg]: comcap: Send Data: %2x \n", ((PUCHAR)pBuffer)[j]));
			}

			// ������ֱ���·�
			IoSkipCurrentIrpStackLocation(Irp);
			return IoCallDriver(s_nextObject[i], Irp);
		}
	}

	// ����������ڱ��󶨵��豸��,����Ϣ�ߵ���ǰ�������������,ֱ�ӷ��ز�������
	return STATUS_INVALID_DEVICE_OBJECT_PARAMETER;
}

