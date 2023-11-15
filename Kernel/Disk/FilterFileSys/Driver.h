#ifndef _DIRVER_H
#define _DIRVER_H

#include <ntddk.h>

#define SFLT_POOL_TAG   'tlFS'
#define MAX_DEVNAME_LENGTH 64	// 本地设备名称最大

typedef struct _SFILTER_DEVICE_EXTENSION {

    ULONG TypeFlag;
    PDEVICE_OBJECT AttachedToDeviceObject;			// 指向附加的文件系统设备对象的指针
    PDEVICE_OBJECT StorageStackDeviceObject;		// 指向与关联的真实（磁盘)设备对象,附加到的文件系统设备对象
    UNICODE_STRING DeviceName;						// 此设备的名称. 如果连接到卷设备对象,则为物理磁盘驱动器的名称.如果附加到控制设备对象,则为控制设备对象的名称
    WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH];		// 设备名字缓冲区
    UCHAR UserExtension[1];							// 其他扩展
} SFILTER_DEVICE_EXTENSION, * PSFILTER_DEVICE_EXTENSION;



NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID NTAPI DriverUnload(PDRIVER_OBJECT DriverObject);

/*
	普通IRP分发函数
*/
NTSTATUS NTAPI SfCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfCleanupClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfPassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfFsControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI SfAttachDeviceToDeviceStack(IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, IN OUT PDEVICE_OBJECT* AttachedToDeviceObject);

extern PDEVICE_OBJECT g_SFilterControlDeviceObject;
extern PDRIVER_OBJECT g_SFilterDriverObject;


#define IS_MY_DEVICE_OBJECT(DeviceObject) \
    (((DeviceObject) != NULL) && \
     ((DeviceObject)->DriverObject == g_SFilterDriverObject) && \
      ((DeviceObject)->DeviceExtension != NULL) && \
	  ((*(ULONGLONG *)(DeviceObject)->DeviceExtension) == SFLT_POOL_TAG))

#define IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject) \
    ( ((DeviceObject) == g_SFilterControlDeviceObject) ? \
        (ASSERT(((DeviceObject)->DriverObject == g_SFilterDriverObject) && \
        ((DeviceObject)->DeviceExtension == NULL)), TRUE) : \
     FALSE)

#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)


#if DBG && WINVER >= 0x0501
#pragma alloc_text(PAGE, DriverUnload)
#endif

#pragma alloc_text(PAGE, SfCreate)
#pragma alloc_text(PAGE, SfCleanupClose)
#pragma alloc_text(PAGE, SfFsControl)
#pragma alloc_text(PAGE, SfAttachDeviceToDeviceStack)
#endif

#endif // !_DIRVER_H
