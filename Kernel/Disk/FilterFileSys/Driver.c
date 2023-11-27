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
	DriverObject->MajorFunction[IRP_MJ_READ] = SfRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = SfWrite;


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


/* Warning: 需要调试*/
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

	// 判断如果是对文件系统控制设备的操作,直接返回失败(控制设备的读写不做处理)
	if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject))
	{
		Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	// 对文件系统其他设备的操作,直接下发下层驱动
	if (devExt->StorageStackDeviceObject)
		return SfPassThrough(DeviceObject, Irp);

	// 对卷的文件操作
	// 获取文件读取位置偏移量、读取长度
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

	// 如果成功, 获取读到的内容
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
			return STATUS_INSUFFICIENT_RESOURCES;	// 返回资源不足
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

	// 判断如果是对文件系统控制设备的操作,直接返回失败(控制设备的读写不做处理)
	if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject))
	{
		Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	// 对文件系统其他设备的操作,直接下发下层驱动
	if (devExt->StorageStackDeviceObject)
		return SfPassThrough(DeviceObject, Irp);

	// 获取文件写入位置偏移量、写入长度
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
	//{	// 如果IRP请求成功了,把这个FileObject记录到集合里,这是一个刚刚打开或生成的目录
	//	// irpsp->Parameters.Create.Options & FILE_DIRECTORY_FILE和irpsp->Parameters.Create.Options & FILE_NONDIRECTORY_FILE均为0的情况下,Windows是否有打开一个文件对象的可能?
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
* 调用此例程是为了完成FsControl请求.向调度例程发送用于重新同步的事件信号
*/
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);

	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
	ASSERT(NULL != Context);

	// 高版本使用等待事件,低版本使用工作任务
#if WINVER >= 0x0501
	// 在Windows XP或更高版本上，传入的上下文将是一个要发出信号的事件
	if (IS_WINDOWSXP_OR_LATER())
	{
		KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
#endif // WINVER >= 0x0501

	// 对于Windows 2000,如果处于高IRQL,应该使用Context中的工作项将此工作排队到工作线程.
	// IRQL中断级别过高时,工作任务放到DelayedWorkQueue队列执行
	if (PASSIVE_LEVEL < KeGetCurrentIrql())
	{
		ExQueueWorkItem((PWORK_QUEUE_ITEM)Context, (WORK_QUEUE_TYPE)DelayedWorkQueue);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	// IRQL处于PASSIVE_LEVEL,直接调用工作线程
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
		ASSERT(NULL != g_SfDynamicFunctions.EnumerateDeviceObjectList &&
			NULL != g_SfDynamicFunctions.GetDiskDeviceObject &&
			NULL != g_SfDynamicFunctions.GetDeviceAttachmentBaseRef &&
			NULL != g_SfDynamicFunctions.GetLowerDeviceObject);
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

NTSTATUS NTAPI SfFsControlLoadFileSystem(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*
* IRP IRP_MN_LOAD_FILE_SYSTEM 回调函数,数据包只是通过,直接下发,但是需要对设备栈进行处理
*/
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
	PFSCTRL_COMPLETION_CONTEXT completionContext = NULL;
	

	PAGED_CODE();

	SF_LOG_PRINT(SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("[dbg]![%ws]: Loading File System, Detaching from \"%wZ\"\n", \
		__FUNCTIONW__, &devExt->DeviceName));

	// 高版本系统使用等待事件,低版本使用工作任务
#if WINVER >= 0x0501
	/* VERSION NOTE:
	* 在Windows2000上,我们不能简单地同步回调度例程来进行加载后的文件系统处理.我们需要在dispatch级别上完成这项工作,
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
		return ntStatus;
	}
#endif // WINVER >= 0x0501

	// 设置一个完成例程，在加载完成时删除设备对象
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
/* 绑定设备*/
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PSFILTER_DEVICE_EXTENSION newDevExt = SFilterDeviceObject->DeviceExtension;

	PAGED_CODE();
	ASSERT(IS_MY_DEVICE_OBJECT(SFilterDeviceObject));
#if WINVER >= 0x0501
	ASSERT(!SfIsAttachedToDevice(DeviceObject, NULL));
#endif // WINVER >= 0x501

	// 赋值设备属性标志
	if (FlagOn(DeviceObject->Flags, DO_BUFFERED_IO))
		SetFlag(SFilterDeviceObject->Flags, DO_BUFFERED_IO);
	if(FlagOn(DeviceObject->Flags, DO_DIRECT_IO))
		SetFlag(SFilterDeviceObject->Flags, DO_DIRECT_IO);

	// 循环尝试绑定,如果当前恰好在对这个磁盘做特殊操作(挂载或者解挂载操作有关),绑定可能失败.
	for (ULONG i = 0; i < 8; ++i)
	{
		LARGE_INTEGER interval = { 0 };
		ntStatus = SfAttachDeviceToDeviceStack(SFilterDeviceObject, DeviceObject, &newDevExt->AttachedToDeviceObject);
		if (NT_SUCCESS(ntStatus))
		{
			ClearFlag(SFilterDeviceObject->Flags, DO_DEVICE_INITIALIZING);
			return STATUS_SUCCESS;
		}

		// 延迟1sec
		interval.QuadPart = (1000 * DELAY_ONE_MILLISECOND);
		KeDelayExecutionThread(KernelMode, FALSE, &interval);
		continue;
	}
	//OnSfilterAttachPost(SFilterDeviceObject, DeviceObject, NULL, (PVOID)(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->UserExtension), ntStatus);
	return ntStatus;
}

NTSTATUS NTAPI SfFsControlMountVolume(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/* 处理 IRP_MN_MOUNT_VOLUME消息(当卷被挂载),用来绑定一个卷*/
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	PDEVICE_OBJECT storageStackDeviceObject = irpsp->Parameters.MountVolume.Vpb->RealDevice;	// 保存Vpb->RealDevice
	PDEVICE_OBJECT newDeviceObject = NULL;
	PSFILTER_DEVICE_EXTENSION newDevExt = NULL;
	BOOLEAN isShadowCopyVolume = 0;
	PFSCTRL_COMPLETION_CONTEXT completionContext = NULL;

	PAGED_CODE();
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType));

	// 判断是否是影卷,如果不打算绑定直接下发跳过
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

	// 生成过滤设备
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

	// 设置设备扩展
	newDevExt = newDeviceObject->DeviceExtension;
	newDevExt->StorageStackDeviceObject = storageStackDeviceObject;
	RtlInitEmptyUnicodeString(&newDevExt->DeviceName, newDevExt->DeviceNameBuffer, sizeof(newDevExt->DeviceNameBuffer));
	SfGetObjectName(storageStackDeviceObject, &newDevExt->DeviceName);

	// 高版本系统使用等待事件,低版本使用工作任务
#if WINVER >= 0x0501
	/* VERSION NOTE:
	*  在Windows2000上,不能简单地同步回调度例程来进行装载后处理. 需要在dispatch级别上完成，所以我们将把这项workerItem从完成例程排队到工作线程.
	* 因为绑定卷的过程中,sfilter使用了ExAcquireFastMutex来来等待这些不适宜在Dispatch级别使用的函数,可能在绑定设备时会发生死锁. 
	* 解决方法是在完成函数中把任务放到一个预先生成的系统线程中处理.系统线程执行的中断级别为Passive中断级,在这个线程中完成绑定是没有问题的
	* 
	*  Windows XP及更高版本，我们可以安全地同步回调度例程
	*/
	if (IS_WINDOWSXP_OR_LATER())
	{
		KEVENT waitEvent = { 0 };
		KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, SfFsControlCompletion, &waitEvent, TRUE, TRUE, TRUE);

		// 发送IRP并等待事件完成
		ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
		if (STATUS_PENDING == ntStatus)
		{
			ntStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
			ASSERT(STATUS_SUCCESS == ntStatus);
		}

		// 绑定卷
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
	// 初始化一个工作任务,写入上下文
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

		// 获取一个互斥体内核对象, 以'原子方式'判断是否绑定过该卷设备,防止同时对一个卷设备绑定两次
		ExAcquireFastMutex(&g_SfilterAttachLock);
		if (SfIsAttachedToDevice(vbp->DeviceObject, &attachedDeviceObject))
		{	// 说明已经绑定过了
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

		// 调用 SfAttachToMountedDevice完成绑定
		ntStatus = SfAttachToMountedDevice(vbp->DeviceObject, NewDeviceObject);
		if (!NT_SUCCESS(ntStatus))
		{
			SfCleanupMountedDevice(NewDeviceObject);
			IoDeleteDevice(NewDeviceObject);
		}
		ASSERT(NULL == attachedDeviceObject);
		ExReleaseFastMutex(&g_SfilterAttachLock);
	} while (FALSE);

	//完成请求
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

	// 如果打开成功,则获取文件名; 如果打开失败,则获取设备名
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
		// 设置已分配的缓冲区并重新获取名称
		nameInfo = (POBJECT_NAME_INFORMATION)NameControl->AllocatedBuffer;
		ntStatus = ObQueryNameString(FileObject, nameInfo, bufferSize, &size);
	}

	/*
	* 如果获取到一个名字并且在打开文件时错误,那么我们只收到了设备名称
	* 从FileObject中获取名称的其余部分(注意,只有在从Create中调用时才能执行此操作),只有当我们从创建中返回错误时,才会发生这种情况
	*/
	if (NT_SUCCESS(ntStatus) && !NT_SUCCESS(CreateStatus))
	{
		ULONG newSize = 0;
		PCHAR newBuffer = NULL;
		POBJECT_NAME_INFORMATION newNameInfo = NULL;

		// 计算名字缓冲区大小
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

			// 如果已申请缓冲区,就释放旧缓冲区. 并保存新分配的缓冲区地址
			if (NameControl->AllocatedBuffer)
				ExFreePool(NameControl->AllocatedBuffer);
			NameControl->AllocatedBuffer = newBuffer;
			bufferSize = newSize;
			nameInfo = newNameInfo;
		} else
		{
			nameInfo->Name.MaximumLength = (USHORT)(bufferSize - sizeof(OBJECT_NAME_INFORMATION));
		}

		// 如果存在相关的文件对象,先将该名称与分隔符一起附加到设备对象上
		if (FileObject->RelatedFileObject)
		{
			RtlAppendUnicodeStringToString(&nameInfo->Name, &FileObject->RelatedFileObject->FileName);
			RtlAppendUnicodeToString(&nameInfo->Name, L"\\");
		}

		// 附加对象名称
		RtlAppendUnicodeStringToString(&nameInfo->Name, &FileObject->FileName);
		ASSERT(nameInfo->Name.Length <= nameInfo->Name.MaximumLength);
	}
	return &nameInfo->Name;
}

// 分配MDL,缓冲区必须是预先分配好的
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

