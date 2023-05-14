#include <ntddk.h>
VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	UNREFERENCED_PARAMETER(pDriver);
	DbgPrint("Goodbye~\n");
}
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegPath)
{
	DbgPrint("Hello Driver!\n");
	UNREFERENCED_PARAMETER(pRegPath);
	pDriver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}