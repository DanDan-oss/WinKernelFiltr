#include "FastIo.h"
#include "Driver.h"

BOOLEAN NTAPI SfFastIoCheckIfPossible(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, \
	IN ULONG LockKey, IN BOOLEAN CheckForReadOperation, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject)
	/*++
		����:
				�����������ڼ��Ŀ���I/O"ֱͨ"����,�Ƿ���ԶԴ��ļ����п���I/O
				�˺���ֻ�ǵ����ļ�ϵͳ����Ӧ����,��������ļ�ϵͳδʵ�ָú���,�򷵻�FALSE
		����:
			FileObject				- ָ��Ҫ�������ļ������ָ��
			FileOffset				- �������ļ��е��ֽ�ƫ����
			Length					- Ҫִ�еĲ����ĳ���
			Wait					- ���÷��Ƿ�Ը��ȴ�,�޷���ȡ�ʵ�����ʱ
			LockKey					- Ϊ�ļ����ṩ���÷�����Կ
			CheckForReadOperation	- ���÷��Ƿ����ڼ���ȡ��TRUE����д�����
			IoStatus				- ָ��һ��������ָ�룬���ڽ���IO����״̬
			DeviceObject			- ָ�������������豸�����ָ�룬���豸λ�ڸò���������
		����ֵ:
			���ݶ��ڸ��ļ��Ƿ����I/O����������ֵΪTRUE��FALSE
	--*/
{
	PDEVICE_OBJECT nextDeviceObject = NULL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	PAGED_CODE();

	if (DeviceObject->DeviceExtension)
	{
		ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoCheckIfPossible))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoRead))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoWrite))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoQueryBasicInfo))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoQueryStandardInfo))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoLock))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoUnlockSingle))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoUnlockAll))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoUnlockAllByKey))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoDeviceControl))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoQueryNetworkOpenInfo))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, MdlRead))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, MdlReadComplete))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, PrepareMdlWrite))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, MdlWriteComplete))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoReadCompressed))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoWriteCompressed))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, MdlReadCompleteCompressed))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, MdlWriteCompleteCompressed))	// �жϺ�����Ч��
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
		// ��ȡ�������󶨵��豸��FastIo����
		nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject;
		ASSERT(nextDeviceObject);
		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
		if (VALID_FAST_IO_DISPATCH_HANDLER(fastIoDispatch, FastIoQueryOpen))	// �жϺ�����Ч��
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
