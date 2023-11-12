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

	// �����豸����ʧ��
	if (!NT_SUCCESS(ntStatus))
	{	
		// �������·�� \\FileSystem\\Filters\\SFilter δ�ҵ�˵��������·��ԭ������ʧ��
		if (STATUS_OBJECT_PATH_NOT_FOUND != ntStatus)
		{
			KdPrint(("[dbg][%ws] SFilter: Error creating control device object \"%wZ\", status =%08x\n ", __FUNCTIONW__, &nameString, ntStatus));
			return ntStatus;
		}

		// �Ͱ汾�Ĳ���ϵͳû��\\FileSystem\\Filters���Ŀ¼, ����ֱ��������\\FileSystem��
		RtlInitUnicodeString(&nameString, L"\\FileSystem\\SFilterCDO");
		ntStatus = IoCreateDevice(DriverObject, 0, &nameString, FILE_DEVICE_DISK_FILE_SYSTEM, FILE_DEVICE_SECURE_OPEN, FALSE, &g_SFilterControlDeviceObject);
		if (!NT_SUCCESS(ntStatus) || NULL == g_SFilterControlDeviceObject)
		{
			KdPrint(("[dbg][%ws] SFilter: Error creating control device object \"%wZ\", status =%08x\n ", __FUNCTIONW__, &nameString, ntStatus));
			return ntStatus;
		}
	}

	// ��ʼ�������豸����ͨIRP�ַ�����
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		DriverObject->MajorFunction[i] = SfPassThrough;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = SfCreate;
	DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = SfCreate;
	DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] = SfCreate;
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = SfFsControl;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SfCleanupClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = SfCleanupClose;


	// ��ʼ�������豸�Ŀ���IO�ַ�����
	// ���麯����ָ����drjver->FastIoDispatc��,�������ﱾ����û�пռ��,����Ϊ�˱�����һ��ָ��,�����Լ�����ռ�,����IO�ַ�����������ڷǷ�ҳ�ڴ�NonPagedPool
	fastIoDispatch = ExAllocatePoolWithTag(NonPagedPool, sizeof(PFAST_IO_DISPATCH), SFLT_POOL_TAG);
	if (!fastIoDispatch)
	{	// �����ڴ�ʧ��
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
	// ASSERT��ֻ��DeBug�汾�б���ʱ��������,��Release�汾�б���ʱ�����κ�����
	// ��DeBug�汾��,���û�е�����,���Ҳ�����ASSERT�е�����,����������. ����е�����,��ᵯ��������ص���Ϣ
	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

	// ��������,ֱ���·�
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver((PSFILTER_DEVICE_EXTENSION)(DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp);
}

NTSTATUS NTAPI SfFsControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	
}


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
