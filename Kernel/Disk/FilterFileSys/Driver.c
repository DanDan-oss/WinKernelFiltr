#include "SFilter.h"
#include "FastIo.h"
#include "Driver.h"

PDEVICE_OBJECT g_SFilterControlDeviceObject = 0;
PDRIVER_OBJECT g_SFilterDriverObject = NULL;
FAST_MUTEX g_SfilterAttachLock = {0};
ULONG g_SfDebug = 0;

static CONST PCHAR g_DeviceTypeNames[] = {
	"",
	"BEEP",
	"CD_ROM",
	"CD_ROM_FILE_SYSTEM",
	"CONTROLLER",
	"DATALINK",
	"DFS",
	"DISK",
	"DISK_FILE_SYSTEM",
	"FILE_SYSTEM",
	"INPORT_PORT",
	"KEYBOARD",
	"MAILSLOT",
	"MIDI_IN",
	"MIDI_OUT",
	"MOUSE",
	"MULTI_UNC_PROVIDER",
	"NAMED_PIPE",
	"NETWORK",
	"NETWORK_BROWSER",
	"NETWORK_FILE_SYSTEM",
	"NULL",
	"PARALLEL_PORT",
	"PHYSICAL_NETCARD",
	"PRINTER",
	"SCANNER",
	"SERIAL_MOUSE_PORT",
	"SERIAL_PORT",
	"SCREEN",
	"SOUND",
	"STREAMS",
	"TAPE",
	"TAPE_FILE_SYSTEM",
	"TRANSPORT",
	"UNKNOWN",
	"VIDEO",
	"VIRTUAL_DISK",
	"WAVE_IN",
	"WAVE_OUT",
	"8042_PORT",
	"NETWORK_REDIRECTOR",
	"BATTERY",
	"BUS_EXTENDER",
	"MODEM",
	"VDM",
	"MASS_STORAGE",
	"SMB",
	"KS",
	"CHANGER",
	"SMARTCARD",
	"ACPI",
	"DVD",
	"FULLSCREEN_VIDEO",
	"DFS_FILE_SYSTEM",
	"DFS_VOLUME",
	"SERENUM",
	"TERMSRV",
	"KSEC"
};

VOID NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
/*  TODO:
*	��һ��: ���ɿ����豸,���Ҹ������豸ָ������
*   �ڶ���: ������ͨ�ַ�����
*   ������: ���ÿ���IO�ַ�����
*   ���Ĳ�: ��д�ļ�ϵͳ�䶯�ص�����,���󶨸ռ�����ļ�ϵͳ�����豸
*   ���岽: ʹ��IoRegisterFsRegistrationChange����ע��ϵͳ�䶯�ص�����
*/
{
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
	UNICODE_STRING nameString = { 0 };

	PDEVICE_OBJECT rawDeviceObject = NULL;
	PFILE_OBJECT fileObject = NULL;

	RtlInitUnicodeString(&nameString, L"\\FileSystem\\Filters\\SFilter");
	ntStatus = IoCreateDevice(DriverObject, 0, &nameString, FILE_DEVICE_DISK_FILE_SYSTEM, FILE_DEVICE_SECURE_OPEN, FALSE, &g_SFilterControlDeviceObject);

	// �����豸����ʧ��
	if (!NT_SUCCESS(ntStatus))
	{	
		// �������·�� \\FileSystem\\Filters\\SFilter δ�ҵ�˵��������·��ԭ������ʧ��
		if (STATUS_OBJECT_PATH_NOT_FOUND != ntStatus)
		{
			KdPrint(("[dbg]![%ws]: Error creating control device object \"%wZ\", status =%08x\n ", __FUNCTIONW__, &nameString, ntStatus));
			return ntStatus;
		}

		// �Ͱ汾�Ĳ���ϵͳû��\\FileSystem\\Filters���Ŀ¼, ����ֱ��������\\FileSystem��
		RtlInitUnicodeString(&nameString, L"\\FileSystem\\SFilterCDO");
		ntStatus = IoCreateDevice(DriverObject, 0, &nameString, FILE_DEVICE_DISK_FILE_SYSTEM, FILE_DEVICE_SECURE_OPEN, FALSE, &g_SFilterControlDeviceObject);
		if (!NT_SUCCESS(ntStatus) || NULL == g_SFilterControlDeviceObject)
		{
			KdPrint(("[dbg]![%ws]: Error creating control device object \"%wZ\", status =%08x\n ", __FUNCTIONW__, &nameString, ntStatus));
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
	DriverObject->MajorFunction[IRP_MJ_READ] = SfRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = SfWrite;


	/*	��ʼ�������豸�Ŀ���IO�ַ�����
	* ��֧��AcquireFileForNtCreateSection, ReleaseFileForNtCreateSection, AcquireForModWrite, ReleaseForModWrite, AcquireForCcFlush, ReleaseForCcFlush
	* ������ʷԭ��,��ЩFastIO��δ��ͨ��NT I/Oϵͳ���͵�������,����ֱ���͵��ļ�ϵͳ,
	* ��Windows XP�͸��߰汾�Ĳ���ϵͳ�Ͽ���ʹ��ϵͳ����"FsRtlRegisterFileSystemFilterCallbacks"��ȡ��Щ�ص�����
	*/

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

#if WINVER >= 0x0501
	/* VERSION NOTE:
	* ��6��FastIO�����ƹ����ļ�ϵͳɸѡ��������ֱ�Ӵ��ݵ������ļ�ϵͳ
	* AcquireFileForNtCreateSection, ReleaseFileForNtCreateSection, AcquireForModWrite, ReleaseForModWrite, AcquireForCcFlush, ReleaseForCcFlush
	* ��Windows XP�����߰汾��,FsFilter�ص�����������������������԰�ȫ�عҽ���Щ����(�����IFS���߰��ĵ��й���Щ�½ӿ���ι����ĸ�����ϸ��Ϣ)
	*/

	/*  MULTIVERSION NOTE:
	* �����ΪWindows XP�����ϰ汾����,�������������Ϊ�������ж������Ķ���汾,
	* �����м������FsFilter�ص�ע��API�Ĵ���,�������,��ע����Щ�ص�.���򲻻�
	*/
	FS_FILTER_CALLBACKS fsFilterCallbacks = { 0 };
	if (NULL != g_SfDynamicFunctions.RegisterFileSystemFilterCallbacks)
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

		ntStatus = (g_SfDynamicFunctions.RegisterFileSystemFilterCallbacks)(DriverObject, &fsFilterCallbacks);
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

	// ע��IoRegisterFsRegistrationChange�Ļص�����"SfFsNotification",ֻ�����ļ�ϵͳ���������ע��ʱ�Ż����
	/* �汾˵��:
	* ��Windows XP�����߰汾�ϣ���Ҳ��ö�����������ļ�ϵͳ(RAW�ļ�ϵͳ����). ��Windows 2000��ö���ڴ�ɸѡ��֮ǰ���ص��ļ�ϵͳ����
	*/
	ntStatus = IoRegisterFsRegistrationChange(DriverObject, SfFsNotification);
	if (!NT_SUCCESS(ntStatus))
	{
		//  ʧ�ܺ�ǰ������FastIO�ַ�������û����,ֱ���ͷŵ�
		KdPrint(("[dbg]![%ws]: Error registering FS change notification, status =%08x\n ", __FUNCTIONW__, ntStatus));
		if (fastIoDispatch)
		{
			ExFreePool(fastIoDispatch);
			fastIoDispatch = NULL;
			DriverObject->FastIoDispatch = NULL;
		}
		if (g_SFilterControlDeviceObject)
		{
			IoDeleteDevice(g_SFilterControlDeviceObject);
			g_SFilterControlDeviceObject = NULL;
		}
		return ntStatus;
	}

	// ���Ը��ӵ�RAW�ļ�ϵͳ�豸����,��Ϊ���ǲ�����IoRegisterFsRegistrationChangeö�ٵ�
	RtlInitUnicodeString(&nameString, L"\\Device\\RawDisk");
	ntStatus = IoGetDeviceObjectPointer(&nameString, FILE_READ_ATTRIBUTES, &fileObject, &rawDeviceObject);
	if (NT_SUCCESS(ntStatus))
	{
		SfFsNotification(rawDeviceObject, TRUE);
		ObDereferenceObject(fileObject);
	}

	// ���ӵ�RawCdRom�豸
	RtlInitUnicodeString(&nameString, L"\\Device\\RawCdRom");
	ntStatus = IoGetDeviceObjectPointer(&nameString, FILE_READ_ATTRIBUTES, &fileObject, &rawDeviceObject);
	if (NT_SUCCESS(ntStatus))
	{
		SfFsNotification(rawDeviceObject, TRUE);
		ObDereferenceObject(fileObject);
	}

	// ��ʼ�����,��������豸�����ϵĳ�ʼ����־
	if(g_SFilterControlDeviceObject)
		ClearFlag(g_SFilterControlDeviceObject->Flags, DO_DEVICE_INITIALIZING);
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
	// ASSERT��ֻ��DeBug�汾�б���ʱ��������,��Release�汾�б���ʱ�����κ�����
	// ��DeBug�汾��,���û�е�����,���Ҳ�����ASSERT�е�����,����������. ����е�����,��ᵯ��������ص���Ϣ
	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

	// ��������,ֱ���·�
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp);
}

NTSTATUS NTAPI SfFsControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
* �ļ�ϵͳ����IRP�ص�����, DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = SfFsControl;
*/
{
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	PAGED_CODE();
	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

	switch (irpsp->MinorFunction)
	{
	case IRP_MN_MOUNT_VOLUME:		// ��������,��SfFsControlMountVolume����һ����
		return SfFsControlMountVolume(DeviceObject, Irp);
	case IRP_MN_LOAD_FILE_SYSTEM:	
		// ���ļ�ϵͳʶ�������������������ļ�ϵͳʱ����Ϣ
		// ����Ѿ������ļ�ϵͳʶ��������ô���ھ�Ӧ�ý���󶨲������豸��ͬʱ�����µ��豸ȥ������ļ�ϵͳ
		return SfFsControlLoadFileSystem(DeviceObject, Irp);
	case IRP_MN_USER_FS_REQUEST:
	{
		if (FSCTL_DISMOUNT_VOLUME == irpsp->Parameters.FileSystemControl.FsControlCode)
		{	// ���̽���ؿ�����,�ֶ��γ�U�̲����ᵼ���������
			PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
			SF_LOG_PRINT(SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("[dbg]![%ws]: Dismounting volume %p \"%wZ\"\n",
				__FUNCTIONW__, devExt->AttachedToDeviceObject, &devExt->DeviceName));
		}
		break;
	}
	default:
		break;
	}
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp);
}


/* Warning: ��Ҫ����*/
NTSTATUS NTAPI SfRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT fileObject = irpsp->FileObject;
	PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
	LARGE_INTEGER offset = { 0 };
	KEVENT	waitEvent = { 0 };
	ULONG ulLength = 0;
	PVOID bufferAddress = NULL;

	UNREFERENCED_PARAMETER(fileObject);

	// �ж�����Ƕ��ļ�ϵͳ�����豸�Ĳ���,ֱ�ӷ���ʧ��(�����豸�Ķ�д��������)
	if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject))
	{
		Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	// ���ļ�ϵͳ�����豸�Ĳ���,ֱ���·��²�����
	if (devExt->StorageStackDeviceObject)
		return SfPassThrough(DeviceObject, Irp);

	// �Ծ���ļ�����
	// ��ȡ�ļ���ȡλ��ƫ��������ȡ����
	offset.QuadPart = irpsp->Parameters.Read.ByteOffset.QuadPart;
	ulLength = irpsp->Parameters.Read.Length;

	KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, SfReadCompletion, &waitEvent, TRUE, TRUE, TRUE);
	ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
	if (STATUS_PENDING == ntStatus)
	{
		ntStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
		ASSERT(STATUS_SUCCESS == ntStatus);
	}
	if (!NT_SUCCESS(Irp->IoStatus.Status))
		return Irp->IoStatus.Status;

	// ����ɹ�, ��ȡ����������
	switch (irpsp->MinorFunction)
	{
	case IRP_MN_NORMAL:
	{
		if (Irp->MdlAddress)
			bufferAddress = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
		else
			bufferAddress = Irp->UserBuffer;

		Irp->IoStatus.Information = ulLength;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		//Irp->FileObject->CurrentByteOffset.QuadPart = offset.QuadPart + ulLength;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	case IRP_MN_MDL:
	{
		PMDL mdl = MyMdlMemoryAllocate(ulLength);
		if (!mdl)
			return STATUS_INSUFFICIENT_RESOURCES;	// ������Դ����
		Irp->MdlAddress = mdl;
		Irp->IoStatus.Information = ulLength;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		//Irp->FileObject->CurrentByteOffset.QuadPart = offset.QuadPart + ulLength;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	default:
		break;
	}
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp);
}

NTSTATUS NTAPI SfWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT fileObject = irpsp->FileObject;
	PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
	LARGE_INTEGER offset = { 0 };
	ULONG ulLength = 0;

	UNREFERENCED_PARAMETER(fileObject);

	// �ж�����Ƕ��ļ�ϵͳ�����豸�Ĳ���,ֱ�ӷ���ʧ��(�����豸�Ķ�д��������)
	if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject))
	{
		Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	// ���ļ�ϵͳ�����豸�Ĳ���,ֱ���·��²�����
	if (devExt->StorageStackDeviceObject)
		return SfPassThrough(DeviceObject, Irp);

	// ��ȡ�ļ�д��λ��ƫ������д�볤��
	offset.QuadPart = irpsp->Parameters.Write.ByteOffset.QuadPart;
	ulLength = irpsp->Parameters.Write.Length;


	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp);
}

NTSTATUS NTAPI SfCreateComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(Context);
	//PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	//PFILE_OBJECT fileObject = irpsp->FileObject;
	//if (NT_SUCCESS(Irp->IoStatus.Status))
	//{	// ���IRP����ɹ���,�����FileObject��¼��������,����һ���ոմ򿪻����ɵ�Ŀ¼
	//	// irpsp->Parameters.Create.Options & FILE_DIRECTORY_FILE��irpsp->Parameters.Create.Options & FILE_NONDIRECTORY_FILE��Ϊ0�������,Windows�Ƿ��д�һ���ļ�����Ŀ���?
	//	if ((fileObject) && (irpsp->Parameters.Create.Options & FILE_DIRECTORY_FILE))
	//		MyAddObjectToSet(fileObject);
	//}
	//return Irp->IoStatus.Status;

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI SfCleanupCloseComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(Context);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI SfFsControlCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
/*
* ���ô�������Ϊ�����FsControl����.��������̷�����������ͬ�����¼��ź�
*/
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);

	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
	ASSERT(NULL != Context);

	// �߰汾ʹ�õȴ��¼�,�Ͱ汾ʹ�ù�������
#if WINVER >= 0x0501
	// ��Windows XP����߰汾�ϣ�����������Ľ���һ��Ҫ�����źŵ��¼�
	if (IS_WINDOWSXP_OR_LATER())
	{
		KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
#endif // WINVER >= 0x0501

	// ����Windows 2000,������ڸ�IRQL,Ӧ��ʹ��Context�еĹ�����˹����Ŷӵ������߳�.
	// IRQL�жϼ������ʱ,��������ŵ�DelayedWorkQueue����ִ��
	if (PASSIVE_LEVEL < KeGetCurrentIrql())
	{
		ExQueueWorkItem((PWORK_QUEUE_ITEM)Context, (WORK_QUEUE_TYPE)DelayedWorkQueue);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	// IRQL����PASSIVE_LEVEL,ֱ�ӵ��ù����߳�
	PWORK_QUEUE_ITEM workItem = Context;
	workItem->WorkerRoutine(workItem->Parameter);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS NTAPI SfReadCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(Context);
	return STATUS_SUCCESS;
}

VOID NTAPI SfFsNotification(IN PDEVICE_OBJECT DeviceObject, IN BOOLEAN FsActive)
{
	UNICODE_STRING name = { 0 };
	WCHAR nameBuffer[MAX_DEVNAME_LENGTH] = { 0 };
	PAGED_CODE();

	RtlInitEmptyUnicodeString(&name, nameBuffer, sizeof(nameBuffer));
	SfGetObjectName(DeviceObject, &name);

	SF_LOG_PRINT(SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("[dbg]![%ws]: %s %p \"%wZ\" (%s)\n", __FUNCTIONW__, \
		((FsActive) ? "Activating file system" : "Deactivating file system"), \
		DeviceObject, &name, GET_DEVICE_TYPE_NAME(DeviceObject->DeviceType)) );

	/*
	*    ������ļ�ϵͳ����, ����ļ�ϵͳ�Ŀ����豸
	*    �����ע��, ������
	*/
	if (FsActive)
		SfAttachToFileSystemDevice(DeviceObject, &name);
	else
		SfDetachFromFileSystemDevice(DeviceObject);

	return;
}

VOID NTAPI SfGetObjectName(IN PVOID Object, IN OUT PUNICODE_STRING Name)
/*
	����ָ�����������,����Ҳ������ƽ����ؿ��ַ���
	Object  - ��Ҫ��ȡ���ֵĶ���
	Name  -  ��ʹ�û���������ʼ�����ַ�������,��С64���ֽ�
*/
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	CHAR niBuf[512] = { 0 };	// name
	POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)niBuf;
	ULONG ulLength = 0;
	Name->Length = 0;

	ntStatus = ObQueryNameString(Object, nameInfo, sizeof(niBuf), &ulLength);
	if (NT_SUCCESS(ntStatus))
	{
		RtlCopyUnicodeString(Name, &nameInfo->Name);
	}
	return;
}

NTSTATUS NTAPI SfAttachToFileSystemDevice(IN PDEVICE_OBJECT DeviceObject, IN PUNICODE_STRING DeviceName)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PDEVICE_OBJECT newDeviceObject = NULL;
	PSFILTER_DEVICE_EXTENSION devExt = NULL;
	UNICODE_STRING fsrecName = { 0 };
	UNICODE_STRING fsName = { 0 };
	WCHAR tempNmaeBuffer[MAX_DEVNAME_LENGTH] = { 0 };
	PAGED_CODE();

	// ����豸����
	if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType))
		return STATUS_SUCCESS;
	RtlInitEmptyUnicodeString(&fsName, tempNmaeBuffer, sizeof(tempNmaeBuffer));
	RtlInitUnicodeString(&fsrecName, L"\\FileSyastem\\Fs_Rec");
	SfGetObjectName(DeviceObject->DriverObject, &fsName);
	// ���ļ�ϵͳʶ������ȫ�ֱ�����־Ϊ0,����Ҫ�󶨵�Ŀ���豸�������������ļ�ϵͳʶ����,ֱ���˳�
	if (!FlagOn(g_SfDebug, SFDEBUG_ATTACH_TO_FSRECOGNIZER))
		if (0 == RtlCompareUnicodeString(&fsName, &fsrecName, TRUE))
			return STATUS_SUCCESS;

	// �������豸,��Ŀ���豸
	ntStatus = IoCreateDevice(g_SFilterDriverObject, sizeof(SFILTER_DEVICE_EXTENSION), NULL, \
							DeviceObject->DeviceType, 0, FALSE, &newDeviceObject);
	if (!NT_SUCCESS(ntStatus))
		return ntStatus;

	// ���Ƹ����豸���Ա�־
	if (FlagOn(DeviceObject->Flags, DO_BUFFERED_IO))
		SetFlag(newDeviceObject->Flags, DO_BUFFERED_IO);
	if (FlagOn(DeviceObject->Flags, DO_DIRECT_IO))
		SetFlag(newDeviceObject->Flags, DO_DIRECT_IO);
	if (FlagOn(DeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN))
		SetFlag(newDeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN);
	devExt = newDeviceObject->DeviceExtension;

	// ���豸ջ
	ntStatus = SfAttachDeviceToDeviceStack(newDeviceObject, DeviceObject, &devExt->AttachedToDeviceObject);
	if (!NT_SUCCESS(ntStatus))
		goto ErrorCleanupDevice;
	// ���豸����¼���豸��չ��
	RtlInitEmptyUnicodeString(&devExt->DeviceName, devExt->DeviceNameBuffer, sizeof(devExt->DeviceNameBuffer));
	RtlCopyUnicodeString(&devExt->DeviceName, DeviceName);
	ClearFlag(newDeviceObject->Flags, DO_DEVICE_INITIALIZING);

#if WINVER >= 0x0501
	/*
	* �����������Ŀ�����ϵͳ�汾����6x0501ʱ,
	*	Windows�ں���һ����EnumerateDeviceObjectList������,��ʱ����ö�����о������
	* �������ϵͳ�����С,EnumerateDeviceObjectList�����鲻����,�޷����Ѿ����صľ�
	*/
	if (IS_WINDOWSXP_OR_LATER())
	{
		ASSERT(NULL != g_SfDynamicFunctions.EnumerateDeviceObjectList &&
			NULL != g_SfDynamicFunctions.GetDiskDeviceObject &&
			NULL != g_SfDynamicFunctions.GetDeviceAttachmentBaseRef &&
			NULL != g_SfDynamicFunctions.GetLowerDeviceObject);
		// ö���ļ�ϵͳ�����е����о�
		ntStatus = SfEnumerateFileSystemVolumes(DeviceObject, &fsName);
		if (!NT_SUCCESS(ntStatus))
		{
			IoDetachDevice(devExt->AttachedToDeviceObject);
			goto ErrorCleanupDevice;
		}
	}
#endif // WINVER >= 0x0501

	return STATUS_SUCCESS;

ErrorCleanupDevice:
	SfCleanupMountedDevice(newDeviceObject);
	IoDeleteDevice(newDeviceObject);
	return ntStatus;
}

VOID NTAPI SfDetachFromFileSystemDevice(IN PDEVICE_OBJECT DeviceObject)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	return;
}

VOID NTAPI SfCleanupMountedDevice(IN PDEVICE_OBJECT DeviceObject)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
	return;
}

NTSTATUS NTAPI SfFsControlLoadFileSystem(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*
* IRP IRP_MN_LOAD_FILE_SYSTEM �ص�����,���ݰ�ֻ��ͨ��,ֱ���·�,������Ҫ���豸ջ���д���
*/
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
	PFSCTRL_COMPLETION_CONTEXT completionContext = NULL;
	

	PAGED_CODE();

	SF_LOG_PRINT(SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("[dbg]![%ws]: Loading File System, Detaching from \"%wZ\"\n", \
		__FUNCTIONW__, &devExt->DeviceName));

	// �߰汾ϵͳʹ�õȴ��¼�,�Ͱ汾ʹ�ù�������
#if WINVER >= 0x0501
	/* VERSION NOTE:
	* ��Windows2000��,���ǲ��ܼ򵥵�ͬ���ص������������м��غ���ļ�ϵͳ����.������Ҫ��dispatch��������������,
	* �������ǽ������������������Ŷӵ������̡߳�
	* 
	* ����Windows XP�����߰汾�����ǿ��԰�ȫ��ͬ���ص�������
	*/

	if (IS_WINDOWSXP_OR_LATER())
	{
		KEVENT waitEvent = { 0 };
		KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, SfFsControlCompletion, &waitEvent, TRUE, TRUE, TRUE);
		ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);

		// �ȴ��������
		if (STATUS_PENDING == ntStatus)
		{
			ntStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
			ASSERT(STATUS_SUCCESS == ntStatus);
		}

		// ��֤�Ƿ����IoCompleteRequest
		ASSERT(KeReadStateEvent(&waitEvent) || !NT_SUCCESS(Irp->IoStatus.Status));
		ntStatus = SfFsControlLoadFileSystemComplete(DeviceObject, Irp);
		return ntStatus;
	}
#endif // WINVER >= 0x0501

	// ����һ��������̣��ڼ������ʱɾ���豸����
	completionContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(PFSCTRL_COMPLETION_CONTEXT), SFLT_POOL_TAG);
	if (NULL == completionContext)
	{
		IoSkipCurrentIrpStackLocation(Irp);
		ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
		return ntStatus;
	}
	ExInitializeWorkItem(&completionContext->WorkItem, SfFsControlLoadFileSystemCompleteWorker, completionContext);
	completionContext->DeviceObject = DeviceObject;
	completionContext->Irp = Irp;
	completionContext->NewDeviceObject = NULL;
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, SfFsControlCompletion, completionContext, TRUE, TRUE, TRUE);
	IoDetachDevice(devExt->AttachedToDeviceObject);
	ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
	return ntStatus;
}

NTSTATUS NTAPI SfFsControlLoadFileSystemComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	return STATUS_SUCCESS;
}
VOID NTAPI SfFsControlLoadFileSystemCompleteWorker(IN PFSCTRL_COMPLETION_CONTEXT Context)
{
	UNREFERENCED_PARAMETER(Context);
	return;
}

NTSTATUS NTAPI SfAttachToMountedDevice(IN PDEVICE_OBJECT DeviceObject, IN PDEVICE_OBJECT SFilterDeviceObject)
/* ���豸*/
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PSFILTER_DEVICE_EXTENSION newDevExt = SFilterDeviceObject->DeviceExtension;

	PAGED_CODE();
	ASSERT(IS_MY_DEVICE_OBJECT(SFilterDeviceObject));
#if WINVER >= 0x0501
	ASSERT(!SfIsAttachedToDevice(DeviceObject, NULL));
#endif // WINVER >= 0x501

	// ��ֵ�豸���Ա�־
	if (FlagOn(DeviceObject->Flags, DO_BUFFERED_IO))
		SetFlag(SFilterDeviceObject->Flags, DO_BUFFERED_IO);
	if(FlagOn(DeviceObject->Flags, DO_DIRECT_IO))
		SetFlag(SFilterDeviceObject->Flags, DO_DIRECT_IO);

	// ѭ�����԰�,�����ǰǡ���ڶ�����������������(���ػ��߽���ز����й�),�󶨿���ʧ��.
	for (ULONG i = 0; i < 8; ++i)
	{
		LARGE_INTEGER interval = { 0 };
		ntStatus = SfAttachDeviceToDeviceStack(SFilterDeviceObject, DeviceObject, &newDevExt->AttachedToDeviceObject);
		if (NT_SUCCESS(ntStatus))
		{
			ClearFlag(SFilterDeviceObject->Flags, DO_DEVICE_INITIALIZING);
			return STATUS_SUCCESS;
		}

		// �ӳ�1sec
		interval.QuadPart = (1000 * DELAY_ONE_MILLISECOND);
		KeDelayExecutionThread(KernelMode, FALSE, &interval);
		continue;
	}
	//OnSfilterAttachPost(SFilterDeviceObject, DeviceObject, NULL, (PVOID)(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->UserExtension), ntStatus);
	return ntStatus;
}

NTSTATUS NTAPI SfFsControlMountVolume(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/* ���� IRP_MN_MOUNT_VOLUME��Ϣ(��������),������һ����*/
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	PDEVICE_OBJECT storageStackDeviceObject = irpsp->Parameters.MountVolume.Vpb->RealDevice;	// ����Vpb->RealDevice
	PDEVICE_OBJECT newDeviceObject = NULL;
	PSFILTER_DEVICE_EXTENSION newDevExt = NULL;
	BOOLEAN isShadowCopyVolume = 0;
	PFSCTRL_COMPLETION_CONTEXT completionContext = NULL;

	PAGED_CODE();
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType));

	// �ж��Ƿ���Ӱ��,����������ֱ���·�����
	ntStatus = SfIsShadowCopyVolume(storageStackDeviceObject, &isShadowCopyVolume);
	if (NT_SUCCESS(ntStatus) && isShadowCopyVolume && !FlagOn(g_SfDebug, SFDEBUG_ATTACH_TO_SHADOW_COPIES))
	{
		UNICODE_STRING shadowDeviceName = { 0 };
		WCHAR shadowNameBuffer[MAX_DEVNAME_LENGTH] = { 0 };
		RtlInitEmptyUnicodeString(&shadowDeviceName, shadowNameBuffer, sizeof(shadowNameBuffer));
		SfGetObjectName(storageStackDeviceObject, &shadowDeviceName);
		SF_LOG_PRINT(SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("[dbg]![%ws]: Not attaching to volume %p \"%wZ\", shadow copy volume\n", \
			__FUNCTIONW__, storageStackDeviceObject, &shadowDeviceName));

		IoSkipCurrentIrpStackLocation(Irp);
		return IoCallDriver(devExt->AttachedToDeviceObject, Irp);
	}

	// ���ɹ����豸
	ntStatus = IoCreateDevice(g_SFilterDriverObject, sizeof(SFILTER_DEVICE_EXTENSION), NULL, DeviceObject->DeviceType, \
		0, FALSE, &newDeviceObject);
	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("[dbg]![%ws]: Error creating volume device object,, status =%08x\n ", __FUNCTIONW__, ntStatus));
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = ntStatus;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return ntStatus;
	}

	// �����豸��չ
	newDevExt = newDeviceObject->DeviceExtension;
	newDevExt->StorageStackDeviceObject = storageStackDeviceObject;
	RtlInitEmptyUnicodeString(&newDevExt->DeviceName, newDevExt->DeviceNameBuffer, sizeof(newDevExt->DeviceNameBuffer));
	SfGetObjectName(storageStackDeviceObject, &newDevExt->DeviceName);

	// �߰汾ϵͳʹ�õȴ��¼�,�Ͱ汾ʹ�ù�������
#if WINVER >= 0x0501
	/* VERSION NOTE:
	*  ��Windows2000��,���ܼ򵥵�ͬ���ص�������������װ�غ���. ��Ҫ��dispatch��������ɣ��������ǽ�������workerItem����������Ŷӵ������߳�.
	* ��Ϊ�󶨾�Ĺ�����,sfilterʹ����ExAcquireFastMutex�����ȴ���Щ��������Dispatch����ʹ�õĺ���,�����ڰ��豸ʱ�ᷢ������. 
	* �������������ɺ����а�����ŵ�һ��Ԥ�����ɵ�ϵͳ�߳��д���.ϵͳ�߳�ִ�е��жϼ���ΪPassive�жϼ�,������߳�����ɰ���û�������
	* 
	*  Windows XP�����߰汾�����ǿ��԰�ȫ��ͬ���ص�������
	*/
	if (IS_WINDOWSXP_OR_LATER())
	{
		KEVENT waitEvent = { 0 };
		KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, SfFsControlCompletion, &waitEvent, TRUE, TRUE, TRUE);

		// ����IRP���ȴ��¼����
		ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
		if (STATUS_PENDING == ntStatus)
		{
			ntStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
			ASSERT(STATUS_SUCCESS == ntStatus);
		}

		// �󶨾�
		ASSERT(KeReadStateEvent(&waitEvent) || !NT_SUCCESS(Irp->IoStatus.Status));
		ntStatus = SfFsControlMountVolumeComplete(DeviceObject, Irp, newDeviceObject);
		return ntStatus;
	}
#endif // WINVER >= 0x0501

	completionContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(FSCTRL_COMPLETION_CONTEXT), SFLT_POOL_TAG);
	if (NULL == completionContext)
	{
		IoSkipCurrentIrpStackLocation(Irp);
		ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
		return ntStatus;
	}
	// ��ʼ��һ����������,д��������
	ExInitializeWorkItem(&completionContext->WorkItem, SfFsControlMountVolumeCompleteWorker, completionContext);
	completionContext->DeviceObject = DeviceObject;
	completionContext->Irp = Irp;
	completionContext->NewDeviceObject = newDeviceObject;
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, SfFsControlCompletion, &completionContext->WorkItem, TRUE, TRUE, TRUE);
	ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
	return ntStatus;
}

NTSTATUS NTAPI SfIsShadowCopyVolume(IN PDEVICE_OBJECT StorageStackDeviceObject, OUT PBOOLEAN IsShadowCopy)
{
	UNREFERENCED_PARAMETER(StorageStackDeviceObject);
	UNREFERENCED_PARAMETER(IsShadowCopy);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI SfFsControlMountVolumeComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PDEVICE_OBJECT NewDeviceObject)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PSFILTER_DEVICE_EXTENSION newDevExt = NewDeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	PDEVICE_OBJECT attachedDeviceObject = NULL;
	PVPB vbp = { 0 };

	PAGED_CODE();
	vbp = newDevExt->StorageStackDeviceObject->Vpb;
	if (vbp != irpsp->Parameters.MountVolume.Vpb)
	{
		SF_LOG_PRINT(SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("[dbg]![%ws]: VPB in IRP stack changed %p IRPVPB=%p VPB=%p\n",
						__FUNCTIONW__, vbp->DeviceObject, irpsp->Parameters.MountVolume.Vpb, vbp));
	}

	do
	{
		if (!NT_SUCCESS(Irp->IoStatus.Status))
		{
			SF_LOG_PRINT(SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("[dbg]![%ws]: Mount volume failure for %p \"%wZ\", status=%08x\n",
							__FUNCTIONW__, DeviceObject, &newDevExt->DeviceName, Irp->IoStatus.Status));
			SfCleanupMountedDevice(NewDeviceObject);
			IoDeleteDevice(NewDeviceObject);
			break;
		}

		// ��ȡһ���������ں˶���, ��'ԭ�ӷ�ʽ'�ж��Ƿ�󶨹��þ��豸,��ֹͬʱ��һ�����豸������
		ExAcquireFastMutex(&g_SfilterAttachLock);
		if (SfIsAttachedToDevice(vbp->DeviceObject, &attachedDeviceObject))
		{	// ˵���Ѿ��󶨹���
			if(attachedDeviceObject)
				SF_LOG_PRINT(SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("[dbg]![%ws]: Mount volume failure for %p \"%wZ\", already attached\n",
						__FUNCTIONW__, ((PSFILTER_DEVICE_EXTENSION)attachedDeviceObject->DeviceExtension)->AttachedToDeviceObject,
						&newDevExt->DeviceName));
			SfCleanupMountedDevice(NewDeviceObject);
			IoDeleteDevice(NewDeviceObject);
			if (attachedDeviceObject)
				ObDereferenceObject(attachedDeviceObject);
			ExReleaseFastMutex(&g_SfilterAttachLock);
			break;
		}

		// ���� SfAttachToMountedDevice��ɰ�
		ntStatus = SfAttachToMountedDevice(vbp->DeviceObject, NewDeviceObject);
		if (!NT_SUCCESS(ntStatus))
		{
			SfCleanupMountedDevice(NewDeviceObject);
			IoDeleteDevice(NewDeviceObject);
		}
		ASSERT(NULL == attachedDeviceObject);
		ExReleaseFastMutex(&g_SfilterAttachLock);
	} while (FALSE);

	//�������
	ntStatus = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return ntStatus;
}

VOID NTAPI SfFsControlMountVolumeCompleteWorker(IN PFSCTRL_COMPLETION_CONTEXT Context)
{
	ASSERT(NULL != Context);
	SfFsControlMountVolumeComplete(Context->DeviceObject, Context->Irp, Context->NewDeviceObject);
	ExFreePoolWithTag(Context, SFLT_POOL_TAG);
}

PUNICODE_STRING SfGetFileName(IN PFILE_OBJECT FileObject, IN NTSTATUS CreateStatus, IN OUT PGET_NAME_CONTROL NameControl)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	POBJECT_NAME_INFORMATION nameInfo = NULL;
	ULONG size = 0;
	ULONG bufferSize = 0;
	nameInfo = (POBJECT_NAME_INFORMATION)NameControl->SmallBuffer;
	bufferSize = sizeof(NameControl->SmallBuffer);

	// ����򿪳ɹ�,���ȡ�ļ���; �����ʧ��,���ȡ�豸��
	ntStatus = ObQueryNameString((NT_SUCCESS(CreateStatus) ? (PVOID)FileObject : (PVOID)FileObject->DeviceObject), nameInfo, bufferSize, &size);
	if (STATUS_BUFFER_OVERFLOW == ntStatus)
	{
		bufferSize = size + sizeof(WCHAR);
		NameControl->AllocatedBuffer = ExAllocatePoolWithTag(NonPagedPool, bufferSize, SFLT_POOL_TAG);
		if (!NameControl->AllocatedBuffer)
		{
			RtlInitEmptyUnicodeString((PUNICODE_STRING)&NameControl->SmallBuffer, (PWCHAR)(NameControl->SmallBuffer + sizeof(UNICODE_STRING)), (USHORT)(sizeof(NameControl->SmallBuffer) - sizeof(UNICODE_STRING)));
			return (PUNICODE_STRING)&NameControl->SmallBuffer;
		}
		// �����ѷ���Ļ����������»�ȡ����
		nameInfo = (POBJECT_NAME_INFORMATION)NameControl->AllocatedBuffer;
		ntStatus = ObQueryNameString(FileObject, nameInfo, bufferSize, &size);
	}

	/*
	* �����ȡ��һ�����ֲ����ڴ��ļ�ʱ����,��ô����ֻ�յ����豸����
	* ��FileObject�л�ȡ���Ƶ����ಿ��(ע��,ֻ���ڴ�Create�е���ʱ����ִ�д˲���),ֻ�е����ǴӴ����з��ش���ʱ,�Żᷢ���������
	*/
	if (NT_SUCCESS(ntStatus) && !NT_SUCCESS(CreateStatus))
	{
		ULONG newSize = 0;
		PCHAR newBuffer = NULL;
		POBJECT_NAME_INFORMATION newNameInfo = NULL;

		// �������ֻ�������С
		newSize = size + FileObject->FileName.Length;
		if (FileObject->RelatedFileObject)
			newSize += FileObject->RelatedFileObject->FileName.Length + sizeof(WCHAR);
		if (newSize > bufferSize)
		{
			newBuffer = ExAllocatePoolWithTag(NonPagedPool, newSize, SFLT_POOL_TAG);
			if (!newBuffer)
			{
				RtlInitEmptyUnicodeString((PUNICODE_STRING)&NameControl->SmallBuffer, (PWCHAR)(NameControl->SmallBuffer + sizeof(UNICODE_STRING)), (USHORT)(sizeof(NameControl->SmallBuffer) - sizeof(UNICODE_STRING)));
				return (PUNICODE_STRING)&NameControl->SmallBuffer;
			}
			newNameInfo = (POBJECT_NAME_INFORMATION)newBuffer;
			RtlInitEmptyUnicodeString(&newNameInfo->Name, (PWCHAR)(newBuffer + sizeof(OBJECT_NAME_INFORMATION)), (USHORT)(newSize - sizeof(OBJECT_NAME_INFORMATION)));
			RtlCopyUnicodeString(&newNameInfo->Name, &nameInfo->Name);

			// ��������뻺����,���ͷžɻ�����. �������·���Ļ�������ַ
			if (NameControl->AllocatedBuffer)
				ExFreePool(NameControl->AllocatedBuffer);
			NameControl->AllocatedBuffer = newBuffer;
			bufferSize = newSize;
			nameInfo = newNameInfo;
		} else
		{
			nameInfo->Name.MaximumLength = (USHORT)(bufferSize - sizeof(OBJECT_NAME_INFORMATION));
		}

		// ���������ص��ļ�����,�Ƚ���������ָ���һ�𸽼ӵ��豸������
		if (FileObject->RelatedFileObject)
		{
			RtlAppendUnicodeStringToString(&nameInfo->Name, &FileObject->RelatedFileObject->FileName);
			RtlAppendUnicodeToString(&nameInfo->Name, L"\\");
		}

		// ���Ӷ�������
		RtlAppendUnicodeStringToString(&nameInfo->Name, &FileObject->FileName);
		ASSERT(nameInfo->Name.Length <= nameInfo->Name.MaximumLength);
	}
	return &nameInfo->Name;
}

// ����MDL,������������Ԥ�ȷ���õ�
_inline PMDL MyMdlAllocate(PVOID Buffer, ULONG Length)
{
	PMDL mdl = IoAllocateMdl(Buffer, Length, FALSE, FALSE, NULL);
	if (!mdl)
		return NULL;
	MmBuildMdlForNonPagedPool(mdl);
	return mdl;
}

_inline PMDL MyMdlMemoryAllocate(ULONG Length)
{
	PMDL mdl = NULL;
	PVOID buffer = ExAllocatePool(NonPagedPool, Length);
	if (!buffer)
		return NULL;
	mdl = MyMdlAllocate(buffer, Length);
	if (!mdl)
	{
		ExFreePool(buffer);
		return NULL;
	}
	return mdl;
}

_inline VOID MyMdlMemoryFree(PMDL Mdl)
{
	PVOID buffer = NULL;
	if (!Mdl)
		return;
	buffer = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);
	if (buffer)
		ExFreePool(Mdl);
	IoFreeMdl(Mdl);
}

