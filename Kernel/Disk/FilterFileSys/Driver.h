#ifndef _DIRVER_H
#define _DIRVER_H

#include <ntddk.h>

#define SFLT_POOL_TAG   'tlFS'
#define MAX_DEVNAME_LENGTH 64	// 本地设备名称最大

#define SFDEBUG_DISPLAY_ATTACHMENT_NAMES    0x00000001      // 显示附加到的设备对象的名称
#define SFDEBUG_DISPLAY_CREATE_NAMES        0x00000002      // 在创建过程中获取和显示名称
#define SFDEBUG_GET_CREATE_NAMES            0x00000004      // 创建期间获取名称(不显示)
#define SFDEBUG_DO_CREATE_COMPLETION        0x00000008      // 创建完成函数,不获取名称
#define SFDEBUG_ATTACH_TO_FSRECOGNIZER      0x00000010      // 附加到FSRecognizer设备对象
#define SFDEBUG_ATTACH_TO_SHADOW_COPIES     0x00000020      // 接到ShadowCopy Volume设备对象-- 它们仅在Windows XP及更高版本上存在


extern PDEVICE_OBJECT g_SFilterControlDeviceObject;
extern PDRIVER_OBJECT g_SFilterDriverObject;
extern ULONG g_SfDebug;

#define IS_MY_DEVICE_OBJECT(_DeviceObject) \
    (((_DeviceObject) != NULL) && \
     ((_DeviceObject)->DriverObject == g_SFilterDriverObject) && \
      ((_DeviceObject)->DeviceExtension != NULL) && \
	  ((*(ULONGLONG *)(_DeviceObject)->DeviceExtension) == SFLT_POOL_TAG))

#define IS_MY_CONTROL_DEVICE_OBJECT(_DeviceObject) \
    ( ((_DeviceObject) == g_SFilterControlDeviceObject) ? \
        (ASSERT(((_DeviceObject)->DriverObject == g_SFilterDriverObject) && \
        ((_DeviceObject)->DeviceExtension == NULL)), TRUE) : \
     FALSE)

#define IS_DESIRED_DEVICE_TYPE(_type) \
        (((_type) == FILE_DEVICE_DISK_FILE_SYSTEM) || \
          ((_type) == FILE_DEVICE_CD_ROM_FILE_SYSTEM) || \
          ((_type) == FILE_DEVICE_NETWORK_FILE_SYSTEM))

/*
#define SF_LOG_PRINT( dbgLevel, string ) \
           (FlagOn(SfDebug,(dbgLevel)) ? DbgPrint string : ((void)0))
*/
#define SF_LOG_PRINT( _dbgLevel, _string ) \
    do { if (FlagOn(g_SfDebug, (_dbgLevel))) DbgPrint _string; } while(0)


#define GET_DEVICE_TYPE_NAME( _type ) \
        ( (((_type)>0) && ((_type)< (sizeof(g_DeviceTypeNames) / sizeof(PCHAR)))) ? \
        (g_DeviceTypeNames[ (_type) ]) : ("[Unknown]") )

typedef struct _SFILTER_DEVICE_EXTENSION {

    ULONG TypeFlag;
    PDEVICE_OBJECT AttachedToDeviceObject;			// 指向附加的文件系统设备对象的指针
    PDEVICE_OBJECT StorageStackDeviceObject;		// 指向与关联的真实（磁盘)设备对象,附加到的文件系统设备对象
    UNICODE_STRING DeviceName;						// 此设备的名称. 如果连接到卷设备对象,则为物理磁盘驱动器的名称.如果附加到控制设备对象,则为控制设备对象的名称
    WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH];		// 设备名字缓冲区
    UCHAR UserExtension[1];							// 其他扩展
} SFILTER_DEVICE_EXTENSION, * PSFILTER_DEVICE_EXTENSION;

typedef struct _FSCTRL_COMPLETION_CONTEXT
{
    WORK_QUEUE_ITEM WorkItem;                   // 如果此完成处理需要在工作线程中完成，则将使用上下文和工作例程初始化的工作项
    PDEVICE_OBJECT DeviceObject;                // 此设备当前指向的设备对象
    PIRP Irp;                                   // 此FSCTRL操作的IRP消息
    PDEVICE_OBJECT NewDeviceObject;             // 我们已经分配并部分初始化的新设备对象，如果装载成功，我们将附加到已装载的卷
}FSCTRL_COMPLETION_CONTEXT, *PFSCTRL_COMPLETION_CONTEXT;

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID NTAPI DriverUnload(PDRIVER_OBJECT DriverObject);

/*
	普通IRP分发函数
*/
NTSTATUS NTAPI SfCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfCleanupClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfPassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfFsControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
VOID NTAPI SfFsNotification(IN PDEVICE_OBJECT DeviceObject, IN BOOLEAN FsActive);
VOID NTAPI SfGetObjectName(IN PVOID Object, IN OUT PUNICODE_STRING Name);

NTSTATUS NTAPI SfAttachDeviceToDeviceStack(IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, \
                    IN OUT PDEVICE_OBJECT* AttachedToDeviceObject);
NTSTATUS NTAPI SfAttachToFileSystemDevice(IN PDEVICE_OBJECT DeviceObject, IN PUNICODE_STRING DeviceName);
VOID NTAPI SfDetachFromFileSystemDevice(IN PDEVICE_OBJECT DeviceObject);
VOID NTAPI SfCleanupMountedDevice(IN PDEVICE_OBJECT DeviceObject);  // 释放设备扩展中的内存
NTSTATUS NTAPI SfFsControlCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS NTAPI SfFsControlLoadFileSystem(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI SfFsControlLoadFileSystemComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID NTAPI SfFsControlLoadFileSystemCompleteWorker(IN PFSCTRL_COMPLETION_CONTEXT Context);

NTSTATUS NTAPI SfFsControlMountVolume(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);


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
#pragma alloc_text(PAGE, SfFsNotification)
#pragma alloc_text(PAGE, SfFsControlMountVolume)

#pragma alloc_text(PAGE, SfAttachToFileSystemDevice)
#pragma alloc_text(PAGE, SfAttachDeviceToDeviceStack)
#pragma alloc_text(PAGE, SfDetachFromFileSystemDevice)
#pragma alloc_text(PAGE, SfFsControlLoadFileSystem)
#pragma alloc_text(PAGE, SfFsControlLoadFileSystemComplete)
#endif

#endif // !_DIRVER_H
