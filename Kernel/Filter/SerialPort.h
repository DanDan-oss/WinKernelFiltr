#ifndef _SERIAL_PORT_H
#define _SERIAL_PORT_H

#include <ntddk.h>

void SerialPortMain(PDRIVER_OBJECT DriverObject);

void ccpAttachAllComs(PDRIVER_OBJECT DriverObject);

// 通过ID位后缀,获取串口设备对象
PDEVICE_OBJECT ccpOpenCom(ULONG ID, NTSTATUS* Status);

// 生成过滤设备并绑定
NTSTATUS ccpAttachDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT* FiltDeviceObject, PDEVICE_OBJECT* Next);

NTSTATUS ccpDispatchCom(PDEVICE_OBJECT DeviceObject, PIRP IRP);


// 卸载设备
void ccpUnloadSerialPort(PDRIVER_OBJECT DriverObject);


#endif // !_SERIAL_PORT_H
