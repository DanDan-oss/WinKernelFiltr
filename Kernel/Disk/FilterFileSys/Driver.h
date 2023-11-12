#ifndef _DIRVER_H
#define _DIRVER_H

#include <ntddk.h>

#define SFLT_POOL_TAG   'tlFS'

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID NTAPI DriverUnload(PDRIVER_OBJECT DriverObject);

/*
	普通IRP分发函数
*/
NTSTATUS NTAPI SfCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfCleanupClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfPassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfFsControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// 快速IO分发函数

BOOLEAN NTAPI SfFastIoCheckIfPossible(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, \
										IN ULONG LockKey,IN BOOLEAN CheckForReadOperation, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoRead(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, \
							IN ULONG LockKey, OUT PVOID Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoWrite(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, \
							IN ULONG LockKey, IN PVOID Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoQueryBasicInfo(IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_BASIC_INFORMATION Buffer, \
									OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoQueryStandardInfo(IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_STANDARD_INFORMATION Buffer, \
										OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoLock(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PLARGE_INTEGER Length, PEPROCESS ProcessId, \
							ULONG Key, BOOLEAN FailImmediately, BOOLEAN ExclusiveLock, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoUnlockSingle( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PLARGE_INTEGER Length, \
									PEPROCESS ProcessId, ULONG Key, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoUnlockAll(IN PFILE_OBJECT FileObject, PEPROCESS ProcessId, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoUnlockAllByKey(IN PFILE_OBJECT FileObject, PVOID ProcessId, ULONG Key, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoDeviceControl(IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, \
									OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, IN ULONG IoControlCode, OUT PIO_STATUS_BLOCK IoStatus, \
									IN PDEVICE_OBJECT DeviceObject);

VOID NTAPI SfFastIoDetachDevice(IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice);

BOOLEAN NTAPI SfFastIoQueryNetworkOpenInfo(IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_NETWORK_OPEN_INFORMATION Buffer, \
											OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);
BOOLEAN NTAPI SfFastIoMdlRead(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, OUT PMDL* MdlChain, \
								OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoMdlReadComplete(IN PFILE_OBJECT FileObject, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoPrepareMdlWrite(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, \
                                        OUT PMDL* MdlChain, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoMdlWriteComplete(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoReadCompressed(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey,  OUT PVOID Buffer, \
                                    OUT PMDL* MdlChain, OUT PIO_STATUS_BLOCK IoStatus, OUT struct _COMPRESSED_DATA_INFO* CompressedDataInfo, \
                                    IN ULONG CompressedDataInfoLength, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoWriteCompressed(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, IN PVOID Buffer, \
                                    OUT PMDL* MdlChain, OUT PIO_STATUS_BLOCK IoStatus, IN struct _COMPRESSED_DATA_INFO* CompressedDataInfo, \
                                    IN ULONG CompressedDataInfoLength, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoMdlReadCompleteCompressed(IN PFILE_OBJECT FileObject, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoMdlWriteCompleteCompressed(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject);

BOOLEAN NTAPI SfFastIoQueryOpen(IN PIRP Irp, OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation, IN PDEVICE_OBJECT DeviceObject);
#endif // !_DIRVER_H
