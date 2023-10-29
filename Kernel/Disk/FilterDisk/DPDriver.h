#ifndef _DP_DIRVER_H
#define _DP_DIRVER_H
#include <ntddk.h>

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID NTAPI DpUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS NTAPI DPAddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject);
VOID NTAPI DPReadWriteThread(PVOID Context);
NTSTATUS NTAPI DPSendToNextDriver(PDEVICE_OBJECT TargetDeviceObject, PIRP Irp);

NTSTATUS NTAPI DPForwardIrpSync(PDEVICE_OBJECT TargetDeviceObject, PIRP Irp);
NTSTATUS NTAPI DPIrpCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);
NTSTATUS NTAPI DPCompleteRequest(PIRP Irp, NTSTATUS Status, CCHAR Priority);

NTSTATUS DPDispatchAny(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI DPDispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI DPDispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI DPDispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI DPDispatchReadWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);


#endif // !_DP_DIRVER_H