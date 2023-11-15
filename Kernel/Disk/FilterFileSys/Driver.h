#ifndef _DIRVER_H
#define _DIRVER_H

#include <ntddk.h>

#define SFLT_POOL_TAG   'tlFS'
#define MAX_DEVNAME_LENGTH 64	// �����豸�������

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
