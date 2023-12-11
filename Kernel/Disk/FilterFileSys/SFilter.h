#ifndef _SFILTER_H
#define _SFILTER_H

#include <ntifs.h>
#include <ntddk.h>

typedef enum
{
    SF_IRP_GO_ON = 0,       // 本驱动要继续处理这个IRP,下发后,完成处理回调函数OnSfilterIrpPost会被调用,
    SF_IRP_COMPLETED = 1,   // 驱动已经完成了该请求,下面不会再继续发送该请求
    SF_IRP_PASS = 2,        // 表示这个IRP将下发,并且不再调用完成后的处理函数
}SF_RET;

#if WINVER == 0x0500
#ifndef FlagOn
#define FlagOn(_F,_SF)       ((_F) & (_SF))
#endif

#ifndef BooleanFlagOn
#define BooleanFlagOn(_F, _SF)        ((BOOLEAN)(((_F)&(_SF)) != 0))
#endif //

#ifndef SetFlag
#define SetFlag(_F,_SF)     ((_F) |= (_SF))
#endif

#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif

#define RtlInitEmptyUnicodeString(_ucStr,_buf,_bufSize) \
    ( (_ucStr)->Buffer = (_buf), \
      (_ucStr)->Length =0, \
      (_ucStr)->MaximumLength = (USHORT)(_bufSize) )

#ifndef min
#define min(_a,_b)    (((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef max
#define max(_a,_b)     (((_a) > (_b)) ? (_a) : (_b))
#endif

#ifdef ASSERT
#undef ASSERT
#if DBG
#define ASSERT( exp ) \
    (!(exp)) ? \
    (RtlAssert(#exp, __FILE__, __LINE__, NULL), FALSE) : \
    TRUE;
#else
#define ASSERT( exp )   ((void) 0)
#endif
#endif // DBG

#define ExFreePoolWithTag( _a, _b ) ExFreePool( (_a) )
#endif WINVER == 0x0500

#if WINVER >= 0x0501

typedef NTSTATUS(NTAPI *PSF_REGISTER_FILE_SYSTEM_FILTER_CALLBACKS)(IN PDRIVER_OBJECT DriverObject, IN PFS_FILTER_CALLBACKS Callbacks);
typedef NTSTATUS(NTAPI *PSF_ATTACH_DEVICE_TO_DEVICE_STACK_SAFE)(IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, OUT PDEVICE_OBJECT* AttachedToDeviceObject);
typedef NTSTATUS(NTAPI *PSF_ENUMERATE_DEVICE_OBJECT_LIST)(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT* DeviceObjectList, IN ULONG DeviceObjectListSize,\
															OUT PULONG ActualNumberDeviceObjects);
typedef NTSTATUS(NTAPI *PSF_GET_LOWER_DEVICE_OBJECT)(IN PDEVICE_OBJECT DeviceObject);
typedef NTSTATUS(NTAPI *PSF_GET_DEVICE_ATTACHMENT_BASE_REF)(IN PDEVICE_OBJECT DeviceObject);
typedef NTSTATUS(NTAPI *PSF_GET_DISK_DEVICE_OBJECT)(IN PDEVICE_OBJECT FileSystemDeviceObject, OUT PDEVICE_OBJECT* DiskDeviceObject);
typedef NTSTATUS(NTAPI *PSF_GET_ATTACHED_DEVICE_REFERENCE)(IN PDEVICE_OBJECT DeviceObject);
typedef NTSTATUS(NTAPI *PSF_GET_VERSION)(IN OUT PRTL_OSVERSIONINFOW VersionInformation);

typedef struct _SF_DYNAMIC_FUNCTION_POINTERS
{
	/* 以下回调函数在Windows XP(5.1)及以上版本可用*/
    PSF_REGISTER_FILE_SYSTEM_FILTER_CALLBACKS RegisterFileSystemFilterCallbacks;
    PSF_ATTACH_DEVICE_TO_DEVICE_STACK_SAFE AttachDeviceToDeviceStackSafe;
    PSF_ENUMERATE_DEVICE_OBJECT_LIST EnumerateDeviceObjectList;
    PSF_GET_LOWER_DEVICE_OBJECT GetLowerDeviceObject;
    PSF_GET_DEVICE_ATTACHMENT_BASE_REF GetDeviceAttachmentBaseRef;
    PSF_GET_DISK_DEVICE_OBJECT GetDiskDeviceObject;
    PSF_GET_ATTACHED_DEVICE_REFERENCE GetAttachedDeviceReference;
    PSF_GET_VERSION GetVersion;
}SF_DYNAMIC_FUNCTION_POINTERS, *PSF_DYNAMIC_FUNCTION_POINTERS;

extern  SF_DYNAMIC_FUNCTION_POINTERS g_SfDynamicFunctions;
extern  ULONG g_SfOsMajorVersion;
extern  ULONG g_SfOsMinorVersion;


#define IS_WINDOWS2000()		((g_SfOsMajorVersion==5) && (g_SfOsMinorVersion==0))
#define IS_WINDOWSXP()			((g_SfOsMajorVersion==5) && (g_SfOsMinorVersion ==1))
#define IS_WINDOWSXP_OR_LATER()	(((g_SfOsMajorVersion==5) && (g_SfOsMinorVersion>=1)) || (g_SfOsMajorVersion>5))
#define IS_WINDOWSSRV2003_OR_LATER	(((g_SfOsMajorVersion==5) && (g_SfOsMinorVersion>=2)) || (g_SfOsMajorVersion>5))
#endif


NTSTATUS NTAPI SfAttachDeviceToDeviceStack(IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, \
    IN OUT PDEVICE_OBJECT* AttachedToDeviceObject);

BOOLEAN NTAPI SfIsAttachedToDevice(PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT* AttachedDeviceObject OPTIONAL);
BOOLEAN NTAPI SfIsAttachedToDeviceW2K(PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT* AttachedDeviceObject OPTIONAL);
BOOLEAN NTAPI SfIsAttachedToDeviceWXPAndLater(PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT* AttachedDeviceObject OPTIONAL);
NTSTATUS NTAPI OnSfilterDriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath, OUT PUNICODE_STRING userNameString, OUT PUNICODE_STRING syblnkString, OUT PULONG extensionSize);

#if WINVER >= 0x0501
NTSTATUS NTAPI SfPreFsFilterPassThrough(IN PFS_FILTER_CALLBACK_DATA Data, OUT PVOID* CompletionContext);
VOID NTAPI SfPostFsFilterPassThrough(IN PFS_FILTER_CALLBACK_DATA Data, IN NTSTATUS OperationStatus, IN PVOID CompletionContext);
NTSTATUS NTAPI SfEnumerateFileSystemVolumes(IN PDEVICE_OBJECT FSDeviceObject, IN PUNICODE_STRING FSName);
VOID SfLoadDynamicFunctions();
VOID SfGetCurrentVersion();
#endif

VOID NTAPI NTAPI SfReadDriverParameters(IN PUNICODE_STRING RegistryPath);
BOOLEAN NTAPI OnSfilterAttachPre(IN PDEVICE_OBJECT OurDevice, IN PDEVICE_OBJECT TheDeviceToAttach, IN PUNICODE_STRING DeviceName, IN PVOID Extension);
VOID NTAPI OnSfilterAttachPost(IN PDEVICE_OBJECT OurDevice, IN PDEVICE_OBJECT TheDeviceToAttach, IN PDEVICE_OBJECT TheDeviceToAttached, IN PVOID Extension, IN NTSTATUS Status);
SF_RET NTAPI OnSfilterIrpPre(IN PDEVICE_OBJECT DeviceObject, IN PDEVICE_OBJECT NextObject, IN PVOID Externsion, IN PIRP Irp, OUT NTSTATUS* Status, PVOID* Context);
VOID NTAPI OnSfilterIrpPost(IN PDEVICE_OBJECT DeviceObject, IN PDEVICE_OBJECT NextObject, IN PVOID Extension, IN PIRP Irp, IN NTSTATUS Status, PVOID Context);


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SfAttachDeviceToDeviceStack)
#pragma alloc_text(PAGE, SfIsAttachedToDevice)
#pragma alloc_text(PAGE, SfIsAttachedToDeviceW2K)
#if WINVER >= 0x0501
#pragma alloc_text(PAGE, SfIsAttachedToDeviceWXPAndLater)
#pragma alloc_text(PAGE, SfEnumerateFileSystemVolumes)
#endif
#endif // !ALLOC_PRAGMA
#endif // !_SFILTER_H