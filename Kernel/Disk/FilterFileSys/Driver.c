#include "SFilter.h"
#include "FastIo.h"
#include "Driver.h"

PDEVICE_OBJECT g_SFilterControlDeviceObject = 0;
PDRIVER_OBJECT g_SFilterDriverObject = NULL;

VOID NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	UNICODE_STRING nameString = { 0 };
	

	RtlInitUnicodeString(&nameString, L"\\FileSystem\\Filters\\SFilter");
	ntStatus = IoCreateDevice(DriverObject, 0, &nameString, FILE_DEVICE_DISK_FILE_SYSTEM, FILE_DEVICE_SECURE_OPEN, FALSE, &g_SFilterControlDeviceObject);

	// 控制设备生成失败
	if (!NT_SUCCESS(ntStatus))
	{	
		// 如果生成路径 \\FileSystem\\Filters\\SFilter 未找到说明可能是路径原因生成失败
		if (STATUS_OBJECT_PATH_NOT_FOUND != ntStatus)
		{
			KdPrint(("[dbg][%ws] SFilter: Error creating control device object \"%wZ\", status =%08x\n ", __FUNCTIONW__, &nameString, ntStatus));
			return ntStatus;
		}

		// 低版本的操作系统没有\\FileSystem\\Filters这个目录, 尝试直接生成在\\FileSystem下
		RtlInitUnicodeString(&nameString, L"\\FileSystem\\SFilterCDO");
		ntStatus = IoCreateDevice(DriverObject, 0, &nameString, FILE_DEVICE_DISK_FILE_SYSTEM, FILE_DEVICE_SECURE_OPEN, FALSE, &g_SFilterControlDeviceObject);
		if (!NT_SUCCESS(ntStatus) || NULL == g_SFilterControlDeviceObject)
		{
			KdPrint(("[dbg][%ws] SFilter: Error creating control device object \"%wZ\", status =%08x\n ", __FUNCTIONW__, &nameString, ntStatus));
			return ntStatus;
		}
	}

	// 初始化控制设备的普通IRP分发函数
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		DriverObject->MajorFunction[i] = SfPassThrough;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = SfCreate;
	DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = SfCreate;
	DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] = SfCreate;
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = SfFsControl;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SfCleanupClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = SfCleanupClose;


	/*	初始化控制设备的快速IO分发函数
	* 不支持AcquireFileForNtCreateSection, ReleaseFileForNtCreateSection, AcquireForModWrite, ReleaseForModWrite, AcquireForCcFlush, ReleaseForCcFlush
	* 由于历史原因,这些FastIO从未被通过NT I/O系统发送到过滤器,而是直接送到文件系统,
	* 在Windows XP和更高版本的操作系统上可以使用系统函数"FsRtlRegisterFileSystemFilterCallbacks"截取这些回调函数
	*/

	// 这组函数的指针在drjver->FastIoDispatc中,而且这里本来是没有空间的,所以为了保存这一组指针,必须自己分配空间,快速IO分发函数表必须在非分页内存NonPagedPool
	fastIoDispatch = ExAllocatePoolWithTag(NonPagedPool, sizeof(PFAST_IO_DISPATCH), SFLT_POOL_TAG);
	if (!fastIoDispatch)
	{	// 分配内存失败
		IoDeleteDevice(g_SFilterControlDeviceObject);
		g_SFilterControlDeviceObject = NULL;
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(fastIoDispatch, sizeof(PFAST_IO_DISPATCH));
	fastIoDispatch->SizeOfFastIoDispatch = sizeof(PFAST_IO_DISPATCH);
	
	fastIoDispatch->FastIoCheckIfPossible = SfFastIoCheckIfPossible;
	fastIoDispatch->FastIoRead = SfFastIoRead;
	fastIoDispatch->FastIoWrite = SfFastIoWrite;
	fastIoDispatch->FastIoQueryBasicInfo = SfFastIoQueryBasicInfo;
	fastIoDispatch->FastIoQueryStandardInfo = SfFastIoQueryStandardInfo;
	fastIoDispatch->FastIoLock = SfFastIoLock;
	fastIoDispatch->FastIoUnlockSingle = SfFastIoUnlockSingle;
	fastIoDispatch->FastIoUnlockAll = SfFastIoUnlockAll;
	fastIoDispatch->FastIoUnlockAllByKey = SfFastIoUnlockAllByKey;
	fastIoDispatch->FastIoDeviceControl = SfFastIoDeviceControl;
	fastIoDispatch->FastIoDetachDevice = SfFastIoDetachDevice;
	fastIoDispatch->FastIoQueryNetworkOpenInfo = SfFastIoQueryNetworkOpenInfo;
	fastIoDispatch->MdlRead = SfFastIoMdlRead;
	fastIoDispatch->MdlReadComplete = SfFastIoMdlReadComplete;
	fastIoDispatch->PrepareMdlWrite = SfFastIoPrepareMdlWrite;
	fastIoDispatch->MdlWriteComplete = SfFastIoMdlWriteComplete;
	fastIoDispatch->FastIoReadCompressed = SfFastIoReadCompressed;
	fastIoDispatch->FastIoWriteCompressed = SfFastIoWriteCompressed;
	fastIoDispatch->MdlReadCompleteCompressed = SfFastIoMdlReadCompleteCompressed;
	fastIoDispatch->MdlWriteCompleteCompressed = SfFastIoMdlWriteCompleteCompressed;
	fastIoDispatch->FastIoQueryOpen = SfFastIoQueryOpen;

	DriverObject->FastIoDispatch = fastIoDispatch;


	/* VERSION NOTE:
	* 有6个FastIO例程绕过了文件系统筛选器将请求直接传递到基本文件系统
	* AcquireFileForNtCreateSection, ReleaseFileForNtCreateSection, AcquireForModWrite, ReleaseForModWrite, AcquireForCcFlush, ReleaseForCcFlush
	* 在Windows XP及更高版本中,FsFilter回调函数被引入以允许过滤器以安全地挂接这些操作(请参阅IFS工具包文档有关这些新接口如何工作的更多详细信息)
	*/

	/*  MULTIVERSION NOTE:
	* 如果是为Windows XP及以上版本构建,则此驱动程序是为在上运行而构建的多个版本,
	* 将进行检测用于FsFilter回调注册API的存在,如果存在,将注册这些回调.否则不会
	*/
#if WINVER >= 0x0501
	FS_FILTER_CALLBACKS fsFilterCallbacks = { 0 };
	if (NULL != gSfDynamicFunctions.RegisterFileSystemFilterCallbacks)
	{
		fsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof(FS_FILTER_CALLBACKS);
		fsFilterCallbacks.PreAcquireForSectionSynchronization = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostAcquireForSectionSynchronization = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreReleaseForSectionSynchronization = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostReleaseForSectionSynchronization = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreAcquireForCcFlush = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostAcquireForCcFlush = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreReleaseForCcFlush = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostReleaseForCcFlush = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreAcquireForModifiedPageWriter = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostAcquireForModifiedPageWriter = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreReleaseForModifiedPageWriter = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostReleaseForModifiedPageWriter = SfPostFsFilterPassThrough;

		ntStatus = (gSfDynamicFunctions.RegisterFileSystemFilterCallbacks)(DriverObject, &fsFilterCallbacks);
		if (!NT_SUCCESS(ntStatus))
		{
			ExFreePool(fastIoDispatch);
			fastIoDispatch = NULL;
			DriverObject->FastIoDispatch = NULL;
			IoDeleteDevice(g_SFilterControlDeviceObject);
			g_SFilterControlDeviceObject = NULL;
		}
	}
#endif
	return ntStatus;
}

NTSTATUS NTAPI SfCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI SfCleanupClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI SfPassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	// ASSERT宏只在DeBug版本中编译时才有意义,在Release版本中编译时不起任何作用
	// 在DeBug版本中,如果没有调试器,而且不满足ASSERT中的条件,则会出现蓝屏. 如果有调试器,则会弹出错误相关的信息
	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

	// 不做处理,直接下发
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp);
}

NTSTATUS NTAPI SfFsControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI SfAttachDeviceToDeviceStack(IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, IN OUT PDEVICE_OBJECT* AttachedToDeviceObject)
{
	PAGED_CODE();

#if WINVER >= 0x0501
	// 当编译的期望目标操作系统版本高于6x0561时, AttachDeviceToDeviceStackSafe可以调用,比IoAttachDeviceToDeviceStack更可靠
	if (IS_WINDOWSXP_OR_LATER())
	{
		ASSERT(NULL != gSfDynamicFunctions.AttachDeviceToDeviceStackSafe);
		return (gSfDynamicFunctions.AttachDeviceToDeviceStackSafe)(SourceDevice, TargetDevice, AttachedToDeviceObject);
	}
	ASSERT(NULL == gSfDynamicFunctions.AttachDeviceToDeviceStackSafe);
#endif // WINVER >= 0x0501

	*AttachedToDeviceObject = TargetDevice;
	*AttachedToDeviceObject = IoAttachDeviceToDeviceStack(SourceDevice, TargetDevice);
	if (NULL == *AttachedToDeviceObject)
		return STATUS_NO_SUCH_DEVICE;
	return STATUS_SUCCESS;

}
