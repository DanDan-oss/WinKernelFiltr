#include "Driver.h"

PDEVICE_OBJECT g_SFilterControlDeviceObject = 0;

VOID NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
	
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
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


	// 初始化控制设备的快速IO分发函数
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
	return ntStatus;
}

NTSTATUS NTAPI SfCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	
}

NTSTATUS NTAPI SfCleanupClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{

}

NTSTATUS NTAPI SfPassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	// ASSERT宏只在DeBug版本中编译时才有意义,在Release版本中编译时不起任何作用
	// 在DeBug版本中,如果没有调试器,而且不满足ASSERT中的条件,则会出现蓝屏. 如果有调试器,则会弹出错误相关的信息
	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

	// 不做处理,直接下发
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver((PSFILTER_DEVICE_EXTENSION)(DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp);
}

NTSTATUS NTAPI SfFsControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	
}


BOOLEAN NTAPI SfFastIoCheckIfPossible(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, \
	IN ULONG LockKey, IN BOOLEAN CheckForReadOperation, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
/*++
	描述:
			此例程是用于检查的快速I/O"直通"函数,是否可以对此文件进行快速I/O
			此函数只是调用文件系统的相应函数,或者如果文件系统未实现该函数,则返回FALSE
	参数:
		FileObject				- 指向要操作的文件对象的指针
		FileOffset				- 操作的文件中的字节偏移量
		Length					- 要执行的操作的长度
		Wait					- 调用方是否愿意等待,无法获取适当的锁时
		LockKey					- 为文件锁提供调用方的密钥
		CheckForReadOperation	- 调用方是否正在检查读取（TRUE）或写入操作
		IoStatus				- 指向一个变量的指针，用于接收IO操作状态
		DeviceObject			- 指向此驱动程序的设备对象的指针，该设备位于该操作将发生
	返回值:
		根据对于该文件是否快速I/O，函数返回值为TRUE或FALSE
--*/
{

}

BOOLEAN
SfFastIoUnlockAll(
	IN PFILE_OBJECT FileObject,
	PEPROCESS ProcessId,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
)
{
	
}
