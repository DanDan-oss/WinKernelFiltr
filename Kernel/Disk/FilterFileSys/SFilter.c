#include "SFilter.h"

#if WINVER >= 0x0501

SF_DYNAMIC_FUNCTION_POINTERS gSfDynamicFunctions = { 0 };
ULONG gSfOsMajorVersion = 0;
ULONG gSfOsMinorVersion = 0;

#endif


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