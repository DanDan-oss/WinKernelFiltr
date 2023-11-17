#include "SFilter.h"

#if WINVER >= 0x0501

SF_DYNAMIC_FUNCTION_POINTERS gSfDynamicFunctions = { 0 };
ULONG gSfOsMajorVersion = 0;
ULONG gSfOsMinorVersion = 0;

#endif


NTSTATUS  SfPreFsFilterPassThrough(IN PFS_FILTER_CALLBACK_DATA Data, OUT PVOID* CompletionContext)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(CompletionContext);
	return STATUS_SUCCESS;
}

VOID  SfPostFsFilterPassThrough(IN PFS_FILTER_CALLBACK_DATA Data, IN NTSTATUS OperationStatus, IN PVOID CompletionContext)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(OperationStatus);
	UNREFERENCED_PARAMETER(CompletionContext);
}