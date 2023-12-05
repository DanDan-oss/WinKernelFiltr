#include "SFilter.h"

#if WINVER >= 0x0501

SF_DYNAMIC_FUNCTION_POINTERS g_SfDynamicFunctions = { 0 };
ULONG g_SfOsMajorVersion = 0;
ULONG g_SfOsMinorVersion = 0;

NTSTATUS NTAPI SfPreFsFilterPassThrough(IN PFS_FILTER_CALLBACK_DATA Data, OUT PVOID* CompletionContext)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(CompletionContext);
	return STATUS_SUCCESS;
}

VOID NTAPI SfPostFsFilterPassThrough(IN PFS_FILTER_CALLBACK_DATA Data, IN NTSTATUS OperationStatus, IN PVOID CompletionContext)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(OperationStatus);
	UNREFERENCED_PARAMETER(CompletionContext);
}

NTSTATUS NTAPI SfEnumerateFileSystemVolumes(IN PDEVICE_OBJECT FSDeviceObject, IN PUNICODE_STRING FSName)
{
	UNREFERENCED_PARAMETER(FSDeviceObject);
	UNREFERENCED_PARAMETER(FSName);
	return STATUS_SUCCESS;
}

VOID SfLoadDynamicFunctions()
{

}

VOID SfGetCurrentVersion()
{

}

#endif

NTSTATUS NTAPI SfAttachDeviceToDeviceStack(IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, IN OUT PDEVICE_OBJECT* AttachedToDeviceObject)
{
	PAGED_CODE();

#if WINVER >= 0x0501
	// 当编译的期望目标操作系统版本高于6x0501时, AttachDeviceToDeviceStackSafe可以调用,比IoAttachDeviceToDeviceStack更可靠
	if (IS_WINDOWSXP_OR_LATER())
	{
		ASSERT(NULL != g_SfDynamicFunctions.AttachDeviceToDeviceStackSafe);
		return (g_SfDynamicFunctions.AttachDeviceToDeviceStackSafe)(SourceDevice, TargetDevice, AttachedToDeviceObject);
	}
	ASSERT(NULL == g_SfDynamicFunctions.AttachDeviceToDeviceStackSafe);
#endif // WINVER >= 0x0501

	* AttachedToDeviceObject = TargetDevice;
	*AttachedToDeviceObject = IoAttachDeviceToDeviceStack(SourceDevice, TargetDevice);
	if (NULL == *AttachedToDeviceObject)
		return STATUS_NO_SUCH_DEVICE;
	return STATUS_SUCCESS;
}

BOOLEAN NTAPI SfIsAttachedToDevice(PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT* AttachedDeviceObject OPTIONAL)
{
	PAGED_CODE();

#if WINVER >= 0x0501
	if (IS_WINDOWSXP_OR_LATER())
	{
		ASSERT(NULL != g_SfDynamicFunctions.GetLowerDeviceObject &&
			NULL != g_SfDynamicFunctions.GetDeviceAttachmentBaseRef);
		return SfIsAttachedToDeviceWXPAndLater(DeviceObject, AttachedDeviceObject);
	}
#endif  
	return SfIsAttachedToDeviceW2K(DeviceObject, AttachedDeviceObject);
}

BOOLEAN NTAPI SfIsAttachedToDeviceW2K(PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT* AttachedDeviceObject OPTIONAL)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(AttachedDeviceObject);
	return TRUE;
}
BOOLEAN NTAPI SfIsAttachedToDeviceWXPAndLater(PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT* AttachedDeviceObject OPTIONAL)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(AttachedDeviceObject);
	return TRUE;
}

VOID NTAPI SfReadDriverParameters(IN PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
}

NTSTATUS NTAPI OnSfilterDriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath, OUT PUNICODE_STRING userNameString, OUT PUNICODE_STRING syblnkString, OUT PULONG extensionSize)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	UNREFERENCED_PARAMETER(userNameString);
	UNREFERENCED_PARAMETER(syblnkString);
	UNREFERENCED_PARAMETER(extensionSize);
	return STATUS_SUCCESS;
}