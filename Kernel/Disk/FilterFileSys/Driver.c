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
*	第一步: 生成控制设备,并且给控制设备指定名称
*   第二步: 设置普通分发函数
*   第三步: 设置快速IO分发函数
*   第四步: 编写文件系统变动回调函数,并绑定刚激活的文件系统控制设备
*   第五步: 使用IoRegisterFsRegistrationChange函数注册系统变动回调函数
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

	// 控制设备生成失败
	if (!NT_SUCCESS(ntStatus))
	{	
		// 如果生成路径 \\FileSystem\\Filters\\SFilter 未找到说明可能是路径原因生成失败
		if (STATUS_OBJECT_PATH_NOT_FOUND != ntStatus)
		{
			KdPrint(("[dbg]![%ws]: Error creating control device object \"%wZ\", status =%08x\n ", __FUNCTIONW__, &nameString, ntStatus));
			return ntStatus;
		}

		// 低版本的操作系统没有\\FileSystem\\Filters这个目录, 尝试直接生成在\\FileSystem下
		RtlInitUnicodeString(&nameString, L"\\FileSystem\\SFilterCDO");
		ntStatus = IoCreateDevice(DriverObject, 0, &nameString, FILE_DEVICE_DISK_FILE_SYSTEM, FILE_DEVICE_SECURE_OPEN, FALSE, &g_SFilterControlDeviceObject);
		if (!NT_SUCCESS(ntStatus) || NULL == g_SFilterControlDeviceObject)
		{
			KdPrint(("[dbg]![%ws]: Error creating control device object \"%wZ\", status =%08x\n ", __FUNCTIONW__, &nameString, ntStatus));
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

#if WINVER >= 0x0501
	/* VERSION NOTE:
	* 有6个FastIO例程绕过了文件系统筛选器将请求直接传递到基本文件系统
	* AcquireFileForNtCreateSection, ReleaseFileForNtCreateSection, AcquireForModWrite, ReleaseForModWrite, AcquireForCcFlush, ReleaseForCcFlush
	* 在Windows XP及更高版本中,FsFilter回调函数被引入以允许过滤器以安全地挂接这些操作(请参阅IFS工具包文档有关这些新接口如何工作的更多详细信息)
	*/

	/*  MULTIVERSION NOTE:
	* 如果是为Windows XP及以上版本构建,则此驱动程序是为在上运行而构建的多个版本,
	* 将进行检测用于FsFilter回调注册API的存在,如果存在,将注册这些回调.否则不会
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

	// 注册IoRegisterFsRegistrationChange的回调函数"SfFsNotification",只有在文件系统被激活或者注销时才会调用
	/* 版本说明:
	* 在Windows XP及更高版本上，这也将枚举所有现有文件系统(RAW文件系统除外). 在Windows 2000上枚举在此筛选器之前加载的文件系统加载
	*/
	ntStatus = IoRegisterFsRegistrationChange(DriverObject, SfFsNotification);
	if (!NT_SUCCESS(ntStatus))
	{
		//  失败后前面分配的FastIO分发函数就没用了,直接释放掉
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

	// 尝试附加到RAW文件系统设备对象,因为它们不是由IoRegisterFsRegistrationChange枚举的
	RtlInitUnicodeString(&nameString, L"\\Device\\RawDisk");
	ntStatus = IoGetDeviceObjectPointer(&nameString, FILE_READ_ATTRIBUTES, &fileObject, &rawDeviceObject);
	if (NT_SUCCESS(ntStatus))
	{
		SfFsNotification(rawDeviceObject, TRUE);
		ObDereferenceObject(fileObject);
	}

	// 连接到RawCdRom设备
	RtlInitUnicodeString(&nameString, L"\\Device\\RawCdRom");
	ntStatus = IoGetDeviceObjectPointer(&nameString, FILE_READ_ATTRIBUTES, &fileObject, &rawDeviceObject);
	if (NT_SUCCESS(ntStatus))
	{
		SfFsNotification(rawDeviceObject, TRUE);
		ObDereferenceObject(fileObject);
	}

	// 初始化完成,清楚控制设备对象上的初始化标志
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
	// ASSERT宏只在DeBug版本中编译时才有意义,在Release版本中编译时不起任何作用
	// 在DeBug版本中,如果没有调试器,而且不满足ASSERT中的条件,则会出现蓝屏. 如果有调试器,则会弹出错误相关的信息
	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

	// 不做处理,直接下发
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp);
}

NTSTATUS NTAPI SfFsControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
* 文件系统控制IRP回调函数, DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = SfFsControl;
*/
{
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	// 当前运行IRQL级别
	PAGED_CODE();
	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

	switch (irpsp->MinorFunction)
	{
	case IRP_MN_MOUNT_VOLUME:		// 当卷被挂载,用SfFsControlMountVolume来绑定一个卷
		return SfFsControlMountVolume(DeviceObject, Irp);
	case IRP_MN_LOAD_FILE_SYSTEM:	
		// 当文件系统识别器决定加载真正的文件系统时的消息
		// 如果已经绑定了文件系统识别器，那么现在就应该解除绑定并销毁设备，同时生成新的设备去绑定真的文件系统
		return SfFsControlLoadFileSystem(DeviceObject, Irp);
	case IRP_MN_USER_FS_REQUEST:
	{
		if (FSCTL_DISMOUNT_VOLUME == irpsp->Parameters.FileSystemControl.FsControlCode)
		{	// 磁盘解挂载控制码,手动拔出U盘并不会导致这个请求
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
	*    如果是文件系统激活, 则绑定文件系统的控制设备
	*    如果是注销, 则解除绑定
	*/
	if (FsActive)
		SfAttachToFileSystemDevice(DeviceObject, &name);
	else
		SfDetachFromFileSystemDevice(DeviceObject);

	return;
}

VOID NTAPI SfGetObjectName(IN PVOID Object, IN OUT PUNICODE_STRING Name)
/*
	返回指定对象的名字,如果找不到名称将返回空字符串
	Object  - 想要获取名字的对象
	Name  -  已使用缓冲区并初始化的字符串对象,最小64个字节
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
	// 当编译的期望目标操作系统版本高于6x0501时, AttachDeviceToDeviceStackSafe可以调用,比IoAttachDeviceToDeviceStack更可靠
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

	// 检查设备类型
	if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType))
		return STATUS_SUCCESS;
	RtlInitEmptyUnicodeString(&fsName, tempNmaeBuffer, sizeof(tempNmaeBuffer));
	RtlInitUnicodeString(&fsrecName, L"\\FileSyastem\\Fs_Rec");
	SfGetObjectName(DeviceObject->DriverObject, &fsName);
	// 绑定文件系统识别器的全局变量标志为0,并且要绑定的目标设备所属驱动就是文件系统识别器,直接退出
	if (!FlagOn(g_SfDebug, SFDEBUG_ATTACH_TO_FSRECOGNIZER))
		if (0 == RtlCompareUnicodeString(&fsName, &fsrecName, TRUE))
			return STATUS_SUCCESS;

	// 生成新设备,绑定目标设备
	ntStatus = IoCreateDevice(g_SFilterDriverObject, sizeof(SFILTER_DEVICE_EXTENSION), NULL, \
							DeviceObject->DeviceType, 0, FALSE, &newDeviceObject);
	if (!NT_SUCCESS(ntStatus))
		return ntStatus;

	// 复制各种设备属性标志
	if (FlagOn(DeviceObject->Flags, DO_BUFFERED_IO))
		SetFlag(newDeviceObject->Flags, DO_BUFFERED_IO);
	if (FlagOn(DeviceObject->Flags, DO_DIRECT_IO))
		SetFlag(newDeviceObject->Flags, DO_DIRECT_IO);
	if (FlagOn(DeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN))
		SetFlag(newDeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN);
	devExt = newDeviceObject->DeviceExtension;

	// 绑定设备栈
	ntStatus = SfAttachDeviceToDeviceStack(newDeviceObject, DeviceObject, &devExt->AttachedToDeviceObject);
	if (!NT_SUCCESS(ntStatus))
		goto ErrorCleanupDevice;
	// 将设备名记录在设备扩展中
	RtlInitEmptyUnicodeString(&devExt->DeviceName, devExt->DeviceNameBuffer, sizeof(devExt->DeviceNameBuffer));
	RtlCopyUnicodeString(&devExt->DeviceName, DeviceName);
	ClearFlag(newDeviceObject->Flags, DO_DEVICE_INITIALIZING);

#if WINVER >= 0x0501
	/*
	* 当编译的期望目标操作系统版本高于6x0501时,
	*	Windows内核中一定有EnumerateDeviceObjectList函数组,这时可以枚举所有卷并逐个绑定
	* 如果期望系统比这个小,EnumerateDeviceObjectList函数组不存在,无法绑定已经加载的卷
	*/
	if (IS_WINDOWSXP_OR_LATER())
	{
		ASSERT(NULL != gSfDynamicFunctions.EnumerateDeviceObjectList &&
			NULL != gSfDynamicFunctions.GetDiskDeviceObject &&
			NULL != gSfDynamicFunctions.GetDeviceAttachmentBaseRef &&
			NULL != gSfDynamicFunctions.GetLowerDeviceObject);
		// 枚举文件系统上已有的所有卷
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

NTSTATUS NTAPI SfFsControlCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
/*
* 调用此例程是为了完成FsControl请求.向调度例程发送用于重新同步的事件信号
*/
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);

	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
	ASSERT(NULL != Context);

#if WINVER >= 0x0501
	// 在Windows XP或更高版本上，传入的上下文将是一个要发出信号的事件
	if (IS_WINDOWSXP_OR_LATER())
		KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
#else
	// 对于Windows 2000,如果处于高IRQL,应该使用Context中的工作项将此工作排队到工作线程.
	if (PASSIVE_LEVEL < KeGetCurrentIrql())
	{
		ExQueueWorkItem((PWORK_QUEUE_ITEM)Context, DelayedWorkQueue);
	}
	else
	{
		// IRQL处于PASSIVE_LEVEL,直接调用工作线程
		PWORK_QUEUE_ITEM workItem = Context;
		workItem->WorkerRoutine(workItem->Parameter);
	}
#endif // WINVER >= 0x0501

	return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS NTAPI SfFsControlLoadFileSystem(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*
* IRP IRP_MN_LOAD_FILE_SYSTEM 回调函数,数据包只是通过,直接下发,但是需要对设备栈进行处理
*/
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
	

	PAGED_CODE();

	SF_LOG_PRINT(SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("[dbg]![%ws]: Loading File System, Detaching from \"%wZ\"\n", \
		__FUNCTIONW__, &devExt->DeviceName));

#if WINVER >= 0x0501
	/* VERSION NOTE:
	* 在Windows2000上,我们不能简单地同步回调度例程来进行加载后的文件系统处理.我们需要在被动级别上完成这项工作,
	* 所以我们将把这项工作从完成例程排队到工作线程。
	* 
	* 对于Windows XP及更高版本，我们可以安全地同步回调度例程
	*/

	if (IS_WINDOWSXP_OR_LATER())
	{
		KEVENT waitEvent = { 0 };
		KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, SfFsControlCompletion, &waitEvent, TRUE, TRUE, TRUE);
		ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);

		// 等待操作完成
		if (STATUS_PENDING == ntStatus)
		{
			ntStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
			ASSERT(STATUS_SUCCESS == ntStatus);
		}

		// 验证是否调用IoCompleteRequest
		ASSERT(KeReadStateEvent(&waitEvent) || !NT_SUCCESS(Irp->IoStatus.Status));
		ntStatus = SfFsControlLoadFileSystemComplete(DeviceObject, Irp);
	}
#else
	// 设置一个完成例程，在加载完成时删除设备对象
	PFSCTRL_COMPLETION_CONTEXT completionContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(PFSCTRL_COMPLETION_CONTEXT), SFLT_POOL_TAG);
	if (NULL == completionContext)
	{
		IoSkipCurrentIrpStackLocation(Irp);
		ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
	}
	else
	{
		ExInitializeWorkItem(&completionContext->WorkItem, SfFsControlLoadFileSystemCompleteWorker, completionContext);
		completionContext->DeviceObject = DeviceObject;
		completionContext->Irp = Irp;
		completionContext->NewDeviceObject = NULL;
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, SfFsControlCompletion, completionContext, TRUE, TRUE, TRUE);
		IoDetachDevice(devExt->AttachedToDeviceObject);
		ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
	}
#endif // WINVER >= 0x0501


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

NTSTATUS NTAPI SfFsControlMountVolume(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	return STATUS_SUCCESS;
}