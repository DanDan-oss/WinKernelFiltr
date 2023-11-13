#include "FastIo.h"
#include "Driver.h"

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
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoCheckIfPossible))	// 判断函数有效性
			return fastIoDispatch->FastIoCheckIfPossible(FileObject, FileOffset, Length, Wait, LockKey, CheckForReadOperation, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoRead(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, \
	IN ULONG LockKey, OUT PVOID Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoRead))	// 判断函数有效性
			return fastIoDispatch->FastIoRead(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoWrite(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, \
	IN ULONG LockKey, IN PVOID Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoWrite))	// 判断函数有效性
			return fastIoDispatch->FastIoWrite(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoQueryBasicInfo(IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_BASIC_INFORMATION Buffer, \
	OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoQueryBasicInfo))	// 判断函数有效性
			return fastIoDispatch->FastIoQueryBasicInfo(FileObject, Wait, Buffer, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoQueryStandardInfo(IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_STANDARD_INFORMATION Buffer, \
	OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoQueryStandardInfo))	// 判断函数有效性
			return fastIoDispatch->FastIoQueryStandardInfo(FileObject, Wait, Buffer, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoLock(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PLARGE_INTEGER Length, PEPROCESS ProcessId, \
	ULONG Key, BOOLEAN FailImmediately, BOOLEAN ExclusiveLock, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoLock))	// 判断函数有效性
			return fastIoDispatch->FastIoLock(FileObject, FileOffset, Length, ProcessId, Key, FailImmediately, ExclusiveLock, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoUnlockSingle(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PLARGE_INTEGER Length, \
	PEPROCESS ProcessId, ULONG Key, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoUnlockSingle))	// 判断函数有效性
			return fastIoDispatch->FastIoUnlockSingle(FileObject, FileOffset, Length, ProcessId, Key, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoUnlockAll(IN PFILE_OBJECT FileObject, PEPROCESS ProcessId, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoUnlockAll))	// 判断函数有效性
			return fastIoDispatch->FastIoUnlockAll(FileObject, ProcessId, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoUnlockAllByKey(IN PFILE_OBJECT FileObject, PVOID ProcessId, ULONG Key, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoUnlockAllByKey))	// 判断函数有效性
			return fastIoDispatch->FastIoUnlockAllByKey(FileObject, ProcessId, Key, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoDeviceControl(IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, \
	OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, IN ULONG IoControlCode, OUT PIO_STATUS_BLOCK IoStatus, \
	IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoDeviceControl))	// 判断函数有效性
			return fastIoDispatch->FastIoDeviceControl(FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, \
				IoControlCode, IoStatus, nextDeviceObject);
	}
	return FALSE;
}


BOOLEAN NTAPI SfFastIoQueryNetworkOpenInfo(IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_NETWORK_OPEN_INFORMATION Buffer, \
	OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoQueryNetworkOpenInfo))	// 判断函数有效性
			return fastIoDispatch->FastIoQueryNetworkOpenInfo(FileObject, Wait, Buffer, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoMdlRead(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, OUT PMDL* MdlChain, \
	OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, MdlRead))	// 判断函数有效性
			return fastIoDispatch->MdlRead(FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoMdlReadComplete(IN PFILE_OBJECT FileObject, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, MdlReadComplete))	// 判断函数有效性
			return fastIoDispatch->MdlReadComplete(FileObject, MdlChain, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoPrepareMdlWrite(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, \
	OUT PMDL* MdlChain, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, PrepareMdlWrite))	// 判断函数有效性
			return fastIoDispatch->PrepareMdlWrite(FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoMdlWriteComplete(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, MdlWriteComplete))	// 判断函数有效性
			return fastIoDispatch->MdlWriteComplete(FileObject, FileOffset, MdlChain, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoReadCompressed(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, OUT PVOID Buffer, \
	OUT PMDL* MdlChain, OUT PIO_STATUS_BLOCK IoStatus, OUT struct _COMPRESSED_DATA_INFO* CompressedDataInfo, \
	IN ULONG CompressedDataInfoLength, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoReadCompressed))	// 判断函数有效性
			return fastIoDispatch->FastIoReadCompressed(FileObject, FileOffset, Length, LockKey, Buffer, MdlChain, IoStatus, CompressedDataInfo, \
				CompressedDataInfoLength, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoWriteCompressed(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, IN PVOID Buffer, \
	OUT PMDL* MdlChain, OUT PIO_STATUS_BLOCK IoStatus, IN struct _COMPRESSED_DATA_INFO* CompressedDataInfo, \
	IN ULONG CompressedDataInfoLength, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoWriteCompressed))	// 判断函数有效性
			return fastIoDispatch->FastIoWriteCompressed(FileObject, FileOffset, Length, LockKey, Buffer, MdlChain, IoStatus, CompressedDataInfo, \
				CompressedDataInfoLength, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoMdlReadCompleteCompressed(IN PFILE_OBJECT FileObject, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, MdlReadCompleteCompressed))	// 判断函数有效性
			return fastIoDispatch->MdlReadCompleteCompressed(FileObject, MdlChain, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoMdlWriteCompleteCompressed(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, MdlWriteCompleteCompressed))	// 判断函数有效性
			return fastIoDispatch->MdlWriteCompleteCompressed(FileObject, FileOffset, MdlChain, nextDeviceObject);
	}
	return FALSE;
}

BOOLEAN NTAPI SfFastIoQueryOpen(IN PIRP Irp, OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation, IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// 获取本驱动绑定的设备和FastIo函数
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoQueryOpen))	// 判断函数有效性
			return fastIoDispatch->FastIoQueryOpen(Irp, NetworkInformation, nextDeviceObject);
	}
	return FALSE;
}

VOID NTAPI SfFastIoDetachDevice(IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice)
{
	UNREFERENCED_PARAMETER(SourceDevice);
	UNREFERENCED_PARAMETER(TargetDevice);
	return;
}
