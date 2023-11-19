#include "SFilter.h"
#include "FastIo.h"
#include "Driver.h"

PDEVICE_OBJECT g_SFilterControlDeviceObject = 0;
PDRIVER_OBJECT g_SFilterDriverObject = NULL;
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
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	return STATUS_SUCCESS;
}

VOID NTAPI SfFsNotification(IN PDEVICE_OBJECT DeviceObject, IN BOOLEAN FsActive)
{
	UNICODE_STRING name = { 0 };
	WCHAR nameBuffer[MAX_DEVNAME_LENGTH] = { 0 };
	PAGED_CODE();

	RtlInitEmptyUnicodeString(&name, nameBuffer, sizeof(nameBuffer));
	SfGetObjectName(DeviceObject, &name);

	SF_LOG_PRINT(SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("[dbg]!SfFsNotification: %s %p \"%wZ\" (%s)\n", \
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

NTSTATUS NTAPI SfAttachDeviceToDeviceStack(IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, IN OUT PDEVICE_OBJECT* AttachedToDeviceObject)
{
	PAGED_CODE();

#if WINVER >= 0x0501
	// �����������Ŀ�����ϵͳ�汾����6x0501ʱ, AttachDeviceToDeviceStackSafe���Ե���,��IoAttachDeviceToDeviceStack���ɿ�
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
		ASSERT(NULL != gSfDynamicFunctions.EnumerateDeviceObjectList &&
			NULL != gSfDynamicFunctions.GetDiskDeviceObject &&
			NULL != gSfDynamicFunctions.GetDeviceAttachmentBaseRef &&
			NULL != gSfDynamicFunctions.GetLowerDeviceObject);
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
