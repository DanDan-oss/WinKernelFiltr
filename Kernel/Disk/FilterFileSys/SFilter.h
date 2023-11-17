#ifndef _SFILTER_H
#define _SFILTER_H

#include <ntifs.h>
#include <ntddk.h>

#if WINVER == 0x0500
#ifndef FlagOn
#define FlagOn(F,SF)       ((F) & (SF))
#endif

#ifndef BooleanFlagOn
#define BooleanFlagOn(F, SF)        ((BOOLEAN)(((F)&(SF)) != 0))
#endif //

#ifndef SetFlag
#define SetFlag(F,SF)     ((F) |= (SF))
#endif

#ifndef ClearFlag
#define ClearFlag(F,SF)     ((F) &= ~(SF))
#endif

#define RtlInitEmptyUnicodeString(ucStr,buf,bufSize) \
    ( (ucStr)->Buffer = (buf), \
      (ucStr)->Length =0, \
      (ucStr)->MaximumLength = (USHORT)(bufSize) )

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)     (((a) > (b)) ? (a) : (b))
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

#define ExFreePoolWithTag( a, b ) ExFreePool( (a) )
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

extern  SF_DYNAMIC_FUNCTION_POINTERS gSfDynamicFunctions;
extern  ULONG gSfOsMajorVersion;
extern  ULONG gSfOsMinorVersion;


#define IS_WINDOWS2000()		((gSfOsMajorVersion==5) && (gSfOsMinorVersion==0))
#define IS_WINDOWSXP()			((gSfOsMajorVersion==5) && (gSfOsMinorVersion ==1))
#define IS_WINDOWSXP_OR_LATER()	(((gSfOsMajorVersion==5) && (gSfOsMinorVersion>=1)) || (gSfOsMajorVersion>5))
#define IS_WINDOWSSRV2003_OR_LATER	(((gSfOsMajorVersion==5) && (gSfOsMinorVersion>=2)) || (gSfOsMajorVersion>5))

NTSTATUS  SfPreFsFilterPassThrough(IN PFS_FILTER_CALLBACK_DATA Data, OUT PVOID* CompletionContext);
VOID NTAPI SfPostFsFilterPassThrough(IN PFS_FILTER_CALLBACK_DATA Data, IN NTSTATUS OperationStatus, IN PVOID CompletionContext);
#endif
#endif // !_SFILTER_H