#ifndef _SERIAL_PORT_H
#define _SERIAL_PORT_H

#include <ntddk.h>

void SerialPortMain(PDRIVER_OBJECT DriverObject);

void ccpAttachAllComs(PDRIVER_OBJECT DriverObject);

// ͨ��IDλ��׺,��ȡ�����豸����
PDEVICE_OBJECT ccpOpenCom(ULONG ID, NTSTATUS* Status);

// ���ɹ����豸����
NTSTATUS ccpAttachDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT* FiltDeviceObject, PDEVICE_OBJECT* Next);

NTSTATUS ccpDispatchCom(PDEVICE_OBJECT DeviceObject, PIRP IRP);


// ж���豸
void ccpUnloadSerialPort(PDRIVER_OBJECT DriverObject);


#endif // !_SERIAL_PORT_H
