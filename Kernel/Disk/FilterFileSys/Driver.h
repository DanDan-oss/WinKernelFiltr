#ifndef _DIRVER_H
#define _DIRVER_H

#include <ntddk.h>

#define SFLT_POOL_TAG   'tlFS'
#define MAX_DEVNAME_LENGTH 64	// �����豸�������

#define SFDEBUG_DISPLAY_ATTACHMENT_NAMES    0x00000001      // ��ʾ���ӵ����豸���������
#define SFDEBUG_DISPLAY_CREATE_NAMES        0x00000002      // �ڴ��������л�ȡ����ʾ����
#define SFDEBUG_GET_CREATE_NAMES            0x00000004      // �����ڼ��ȡ����(����ʾ)
#define SFDEBUG_DO_CREATE_COMPLETION        0x00000008      // ������ɺ���,����ȡ����
#define SFDEBUG_ATTACH_TO_FSRECOGNIZER      0x00000010      // ���ӵ�FSRecognizer�豸����
#define SFDEBUG_ATTACH_TO_SHADOW_COPIES     0x00000020      // �ӵ�ShadowCopy Volume�豸����-- ���ǽ���Windows XP�����߰汾�ϴ���


extern PDEVICE_OBJECT g_SFilterControlDeviceObject;
extern PDRIVER_OBJECT g_SFilterDriverObject;
extern ULONG SfDebug;

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

/*

#define SF_LOG_PRINT( dbgLevel, string ) \
           (FlagOn(SfDebug,(dbgLevel)) ? DbgPrint string : ((void)0))
*/
#define SF_LOG_PRINT( dbgLevel, string ) \
    do { if (FlagOn(SfDebug, (dbgLevel))) DbgPrint string; } while(0)

#define GET_DEVICE_TYPE_NAME( type ) \
        ( (((type)>0) && ((type)< (sizeof(DeviceTypeNames) / sizeof(PCHAR)))) ? \
        (DeviceTypeNames[ (type) ]) : ("[Unknown]") )

typedef struct _SFILTER_DEVICE_EXTENSION {

    ULONG TypeFlag;
    PDEVICE_OBJECT AttachedToDeviceObject;			// ָ�򸽼ӵ��ļ�ϵͳ�豸�����ָ��
    PDEVICE_OBJECT StorageStackDeviceObject;		// ָ�����������ʵ������)�豸����,���ӵ����ļ�ϵͳ�豸����
    UNICODE_STRING DeviceName;						// ���豸������. ������ӵ����豸����,��Ϊ�������������������.������ӵ������豸����,��Ϊ�����豸���������
    WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH];		// �豸���ֻ�����
    UCHAR UserExtension[1];							// ������չ
} SFILTER_DEVICE_EXTENSION, * PSFILTER_DEVICE_EXTENSION;

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID NTAPI DriverUnload(PDRIVER_OBJECT DriverObject);

/*
	��ͨIRP�ַ�����
*/
NTSTATUS NTAPI SfCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfCleanupClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfPassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI SfFsControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
VOID NTAPI SfFsNotification(IN PDEVICE_OBJECT DeviceObject, IN BOOLEAN FsActive);
VOID NTAPI SfGetObjectName(IN PVOID Object, IN OUT PUNICODE_STRING Name);

NTSTATUS NTAPI SfAttachDeviceToDeviceStack(IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, IN OUT PDEVICE_OBJECT* AttachedToDeviceObject);
NTSTATUS NTAPI SfAttachToFileSystemDevice(IN PDEVICE_OBJECT DeviceObject, IN PUNICODE_STRING DeviceName);
VOID NTAPI SfDetachFromFileSystemDevice(IN PDEVICE_OBJECT DeviceObject);


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

#pragma alloc_text(PAGE, SfAttachToFileSystemDevice)
#pragma alloc_text(PAGE, SfAttachDeviceToDeviceStack)
#pragma alloc_text(PAGE, SfDetachFromFileSystemDevice)
#endif

#endif // !_DIRVER_H
