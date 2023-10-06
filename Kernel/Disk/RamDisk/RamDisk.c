#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <ntdddisk.h>
#include "RamDisk.h"


NTSTATUS RamDiskEvtDeviceAdd(WDFDRIVER DriverObject, PWDFDEVICE_INIT DeviceInit)
{
	UNREFERENCED_PARAMETER(DriverObject);


	KdPrint(("[dbg]:�����������豸\n"));
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	WDFDEVICE wdfDevice = NULL;						// �½��豸
	WDFQUEUE wdfQueue = NULL;;						// �½�����
	WDF_IO_QUEUE_CONFIG ioQueueConfig = { 0 };		// Ҫ�����Ķ�������
	WDF_OBJECT_ATTRIBUTES deviceAttributes = { 0 };	// Ҫ�������豸�������������
	WDF_OBJECT_ATTRIBUTES queueAttributes = { 0 };	// Ҫ�����Ķ��ж������������
	PDEVICE_EXTENSION wdfDeviceExtension = NULL;	// �豸��չ
	PQUEUE_EXTENSION wdfQueueContext = NULL;		// ������չ

	UNICODE_STRING deviceName = { 0 };
	UNICODE_STRING win32Name = { 0 };


	DECLARE_CONST_UNICODE_STRING(ntDeviceName, NT_DEVICE_NAME);
	PAGED_CODE();

	nStatus = WdfDeviceInitAssignName(DeviceInit, &ntDeviceName);
	if (!NT_SUCCESS(nStatus))
		return nStatus;

	// �����豸����,�ص�����
	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_DISK);
	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);
	WdfDeviceInitSetExclusive(DeviceInit, FALSE);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_EXTENSION);
	deviceAttributes.EvtCleanupCallback = RamDiskEvtDeviceContextCleanup;

	// �����豸
	nStatus = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &wdfDevice);
	if (!NT_SUCCESS(nStatus))
		return nStatus;

	wdfDeviceExtension = DeviceGetExtension(wdfDevice);


	// ��ʼ�������豸�������������,�����豸���ơ���дIRP����ص�����
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);
	ioQueueConfig.EvtIoDeviceControl = RamDiskEvtIoDeviceControl;
	ioQueueConfig.EvtIoRead = RamDiskEvtIoRead;
	ioQueueConfig.EvtIoWrite = RamDiskEvtIoWrite;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, QUEUE_EXTENSION);

	// ��������
	nStatus = WdfIoQueueCreate(wdfDevice, &ioQueueConfig, &queueAttributes, &wdfQueue);
	if (!NT_SUCCESS(nStatus))
		return nStatus;


	//��ʼ��������չ
	wdfQueueContext = QueueGetExtension(wdfQueue);
	wdfQueueContext->DeviceExtension = wdfDeviceExtension;

	wdfDeviceExtension->DiskRegInfo.DriveLetter.Buffer = (PWCHAR)&wdfDeviceExtension->DriverLetterBuffer;
	wdfDeviceExtension->DiskRegInfo.DriveLetter.MaximumLength = sizeof(wdfDeviceExtension->DriverLetterBuffer);
	RamDiskQueryDiskRegParameters(WdfDriverGetRegistryPath(WdfDeviceGetDriver(wdfDevice)), &wdfDeviceExtension->DiskRegInfo);

	// ��ʼ���������ڴ�
	wdfDeviceExtension->DiskImage = ExAllocatePoolWithTag(PagedPool, wdfDeviceExtension->DiskRegInfo.DiskSize, RAMDISK_TAG);
	if (!wdfDeviceExtension->DiskImage)
		return STATUS_UNSUCCESSFUL;
	RamDiskFormatDisk(wdfDeviceExtension);	// ��ʼ������
	nStatus = STATUS_SUCCESS;

	// ��ʼ����������
	RtlInitUnicodeString(&win32Name, DOS_DEVICE_NAME);
	RtlInitUnicodeString(&deviceName, NT_DEVICE_NAME);
	wdfDeviceExtension->SymbolicLink.Buffer = (PWCHAR)&wdfDeviceExtension->DosDeviceNameBuffer;
	wdfDeviceExtension->SymbolicLink.MaximumLength = sizeof(wdfDeviceExtension->DosDeviceNameBuffer);
	wdfDeviceExtension->SymbolicLink.Length = win32Name.Length;
	RtlCopyUnicodeString(&wdfDeviceExtension->SymbolicLink, &win32Name);
	RtlAppendUnicodeStringToString(&wdfDeviceExtension->SymbolicLink, &wdfDeviceExtension->DiskRegInfo.DriveLetter);
	nStatus = WdfDeviceCreateSymbolicLink(wdfDevice, &wdfDeviceExtension->SymbolicLink);
	return nStatus;
}

VOID RamDiskEvtDeviceContextCleanup(IN WDFDEVICE Device)
{
	KdPrint(("[dbg]:������������豸\n"));
	PDEVICE_EXTENSION devExt = DeviceGetExtension(Device);
	PAGED_CODE();
	if (devExt->DiskImage)
	{
		ExFreePoolWithTag(devExt->DiskImage, RAMDISK_TAG);
		devExt->DiskImage = NULL;
	}
}

VOID RamDiskQueryDiskRegParameters(PWCHAR RegistryPath, PDISK_INFO DiskRegInfo)
{
	RTL_QUERY_REGISTRY_TABLE rtlQueryRegTabl[5 + 1] = { 0 };
	DISK_INFO defDiskRegInfo = { 0 };
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;

	PAGED_CODE();
	ASSERT(NULL != RegistryPath);

	defDiskRegInfo.DiskSize = DEFAULT_DISK_SIZE;
	defDiskRegInfo.RootDirEntries = DEFAULT_ROOT_DIR_ENTRIES;
	defDiskRegInfo.SectorsPerCluster = DEFAULT_SECTORS_PER_CLUSTER;

	RtlInitUnicodeString(&defDiskRegInfo.DriveLetter, DEFAULT_DRIVE_LETTER);
	RtlZeroMemory(rtlQueryRegTabl, sizeof(rtlQueryRegTabl));

	// ���ò�ѯ��
	rtlQueryRegTabl[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
	rtlQueryRegTabl[0].Name = L"Parameters";
	rtlQueryRegTabl[0].EntryContext = NULL;
	rtlQueryRegTabl[0].DefaultType = (ULONG_PTR)NULL;
	rtlQueryRegTabl[0].DefaultData = NULL;
	rtlQueryRegTabl[0].DefaultLength = (ULONG_PTR)NULL;

	// ���̲���
	rtlQueryRegTabl[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
	rtlQueryRegTabl[1].Name = L"DiskSize";
	rtlQueryRegTabl[1].EntryContext = &DiskRegInfo->DiskSize;
	rtlQueryRegTabl[1].DefaultType = REG_DWORD;
	rtlQueryRegTabl[1].DefaultData = &defDiskRegInfo.DiskSize;
	rtlQueryRegTabl[1].DefaultLength = sizeof(ULONG);

	rtlQueryRegTabl[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
	rtlQueryRegTabl[2].Name = L"RootDirEntries";
	rtlQueryRegTabl[2].EntryContext = &DiskRegInfo->RootDirEntries;
	rtlQueryRegTabl[2].DefaultType = REG_DWORD;
	rtlQueryRegTabl[2].DefaultData = &defDiskRegInfo.RootDirEntries;
	rtlQueryRegTabl[2].DefaultLength = sizeof(ULONG);

	rtlQueryRegTabl[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
	rtlQueryRegTabl[3].Name = L"SectorsPerCluster";
	rtlQueryRegTabl[3].EntryContext = &DiskRegInfo->SectorsPerCluster;
	rtlQueryRegTabl[3].DefaultType = REG_DWORD;
	rtlQueryRegTabl[3].DefaultData = &defDiskRegInfo.SectorsPerCluster;
	rtlQueryRegTabl[3].DefaultLength = sizeof(ULONG);

	rtlQueryRegTabl[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
	rtlQueryRegTabl[4].Name = L"DriveLetter";
	rtlQueryRegTabl[4].EntryContext = &DiskRegInfo->DriveLetter;
	rtlQueryRegTabl[4].DefaultType = REG_SZ;
	rtlQueryRegTabl[4].DefaultData = &defDiskRegInfo.DriveLetter.Buffer;
	rtlQueryRegTabl[4].DefaultLength = 0;

	nStatus = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL, RegistryPath, rtlQueryRegTabl, NULL, NULL);
	if (!NT_SUCCESS(nStatus))
	{
		DiskRegInfo->DiskSize = defDiskRegInfo.DiskSize;
		DiskRegInfo->RootDirEntries = defDiskRegInfo.RootDirEntries;
		DiskRegInfo->SectorsPerCluster = defDiskRegInfo.SectorsPerCluster;
		RtlCopyUnicodeString(&DiskRegInfo->DriveLetter, &defDiskRegInfo.DriveLetter);
	}

	KdPrint(("[dbg]: DiskSize          = 0x%lx\n", DiskRegInfo->DiskSize));
	KdPrint(("[dbg]: RootDirEntries    = 0x%lx\n", DiskRegInfo->RootDirEntries));
	KdPrint(("[dbg]: SectorsPerCluster = 0x%lx\n", DiskRegInfo->SectorsPerCluster));
	KdPrint(("[dbg]: DriveLetter       = %wZ\n", &(DiskRegInfo->DriveLetter)));
	return;
}

NTSTATUS RamDiskFormatDisk(PDEVICE_EXTENSION devExt)
{
	PBOOT_SECTOR bootSector = (PBOOT_SECTOR)devExt->DiskImage;	// ������������

	PUCHAR firstFatSector = NULL;	// ָ���һ��FAT��
	ULONG rootDirEntries = 0;		// ��Ŀ¼��ڵ�����
	ULONG sectorsPerCluster = 0;	// ÿ�����ɶ����������
	USHORT fatType = 0;				// FAT�ļ�ϵͳ����, FAT12/FAT16
	USHORT fatEntries = 0;			// FAT���ж��ٱ���
	USHORT fatSectorCnt = 0;		// FAT����Ҫռ�ö��������洢
	PDIR_ENTRY rootDir = NULL;		// ��һ����Ŀ¼��ڵ�


	PAGED_CODE();
	ASSERT(512 == sizeof(BOOT_SECTOR));
	ASSERT(NULL != devExt->DiskImage);
 
	RtlZeroMemory(devExt->DiskImage, devExt->DiskRegInfo.DiskSize);
	// TODO: ����ÿ��������512�ֽ�,ÿ���ŵ���32������,ÿ�������������ŵ�.
	devExt->DiskGeometry.BytesPerSector = 512;
	devExt->DiskGeometry.SectorsPerTrack = 32;
	devExt->DiskGeometry.TracksPerCylinder = 2;
	// TODO: ���������ɴ�������������õ�,���̽�������RAMDISK_MEDIA_TYPE
	devExt->DiskGeometry.Cylinders.QuadPart = devExt->DiskRegInfo.DiskSize / 512 / 32 / 2;
	devExt->DiskGeometry.MediaType = RAMDISK_MEDIA_TYPE;

	KdPrint(("[dbg]: Cylinders: %lld\n TracksPerCylinder: %ld\n SectorsPerTrack: %ld\n BytesPerSector: %ld\n",
		devExt->DiskGeometry.Cylinders.QuadPart, devExt->DiskGeometry.TracksPerCylinder,
		devExt->DiskGeometry.SectorsPerTrack, devExt->DiskGeometry.BytesPerSector
		));

	// ��ʼ����Ŀ¼���
	rootDirEntries = devExt->DiskRegInfo.RootDirEntries;
	sectorsPerCluster = devExt->DiskRegInfo.SectorsPerCluster;
	if (rootDirEntries & (DIR_ENTRIES_PER_SECTOR - 1))
		rootDirEntries = (rootDirEntries + (DIR_ENTRIES_PER_SECTOR - 1)) & ~(DIR_ENTRIES_PER_SECTOR - 1);

	// ��ʼ��DBR�ṹ
	/* TODO: ��תָ��, Windowsϵͳָ���Ľṹ��С,�����ṹ��ַ*/
	bootSector->bsJump[0] = 0xeb;
	bootSector->bsJump[1] = 0x3c;
	bootSector->bsJump[2] = 0x90;
	bootSector->bsOemNamep[0] = 'D';
	bootSector->bsOemNamep[1] = 'a';
	bootSector->bsOemNamep[2] = 'n';
	bootSector->bsOemNamep[3] = 'R';
	bootSector->bsOemNamep[4] = 'a';
	bootSector->bsOemNamep[5] = 'm';
	bootSector->bsOemNamep[6] = ' ';
	bootSector->bsOemNamep[7] = ' ';
	bootSector->bsBytesPerSec = (USHORT)devExt->DiskGeometry.BytesPerSector;
	bootSector->bsResSectors = 1;	// �����ֻ��һ����������,��DBR����
	bootSector->bsFATs = 1;			// �������ľ�ͬ,Ϊ�˽�ʡ�ռ�,����ֻ���һ��FAT��,������ͨ��������
	bootSector->bsRootDirEnts = (USHORT)rootDirEntries;
	bootSector->bsSectors = (USHORT)(devExt->DiskRegInfo.DiskSize / devExt->DiskGeometry.BytesPerSector);
	bootSector->bsMedia = (UCHAR)devExt->DiskGeometry.MediaType;
	bootSector->bsSecPerClus = (UCHAR)(devExt->DiskRegInfo.SectorsPerCluster);
	// TODO: FAT������Ŀ=(��������-����������-��Ŀ¼��ڵ�ռ��������)/ÿ��������+2  ��ΪFAT���е�0��͵�һ���Ǳ�����,���Լ�2
	fatEntries = (bootSector->bsSectors - bootSector->bsResSectors - bootSector->bsRootDirEnts / DIR_ENTRIES_PER_SECTOR) / bootSector->bsSecPerClus + 2;
	// TODO: ����������4087ʹ��FAT16�ļ�ϵͳ,��֮ʹ��FAT12�ļ�ϵͳ
	if (fatEntries > 4087)
	{
		fatType = 16;
		fatSectorCnt = (fatEntries * 2 + 511) / 512;
		fatEntries = fatEntries + fatSectorCnt;
		fatSectorCnt = (fatEntries * 2 + 511) / 512;
	}
	else
	{
		fatType = 12;
		fatSectorCnt = (((fatEntries * 3 + 1) / 2) + 511) / 512;
		fatEntries += fatSectorCnt;
		fatSectorCnt = (((fatEntries * 3 + 1) / 2) + 511) / 512;
	}
	bootSector->bsFATsecs = fatSectorCnt;
	bootSector->bsSecPerTrack = (USHORT)devExt->DiskGeometry.SectorsPerTrack;
	bootSector->bsHeads = (USHORT)devExt->DiskGeometry.TracksPerCylinder;
	bootSector->bsBootSignature = 0x29;
	bootSector->bsVolumeID = 0x12344321;
	bootSector->bsLable[0] = 'R';
	bootSector->bsLable[1] = 'a';
	bootSector->bsLable[2] = 'm';
	bootSector->bsLable[3] = 'D';
	bootSector->bsLable[4] = 'i';
	bootSector->bsLable[5] = 's';
	bootSector->bsLable[6] = 'k';
	bootSector->bsLable[7] = ' ';
	bootSector->bsLable[8] = ' ';
	bootSector->bsLable[9] = ' ';
	bootSector->bsLable[10] = ' ';
	bootSector->bsFileSystemType[0] = 'F';
	bootSector->bsFileSystemType[1] = 'A';
	bootSector->bsFileSystemType[2] = 'T';
	bootSector->bsFileSystemType[3] = '1';
	bootSector->bsFileSystemType[4] = (fatType == 16) ? '6' : '2';
	bootSector->bsFileSystemType[5] = ' ';
	bootSector->bsFileSystemType[6] = ' ';
	bootSector->bsFileSystemType[7] = ' ';
	bootSector->bsSig2[0] = 0x55;
	bootSector->bsSig2[0] = 0xAA;

	// ��ʼ��FAT��
	firstFatSector = (PUCHAR)(bootSector + 1);
	firstFatSector[0] = (UCHAR)devExt->DiskGeometry.MediaType;
	firstFatSector[1] = 0XFF;
	firstFatSector[2] = 0XFF;
	if (fatType == 16)
		firstFatSector[3] = 0xFF;

	// ��ʼ����һ��Ŀ¼��ڵ�, ͨ����һ����Ŀ¼��ڵ�洢��һ�����ձ���Ϊ����Ŀ¼�����
	rootDir = (PDIR_ENTRY)(bootSector + 1 + fatSectorCnt);
	// ��ʼ�����
	rootDir->deNmae[0] = 'M';
	rootDir->deNmae[1] = 'S';
	rootDir->deNmae[2] = '-';
	rootDir->deNmae[3] = 'R';
	rootDir->deNmae[4] = 'A';
	rootDir->deNmae[5] = 'M';
	rootDir->deNmae[6] = 'D';
	rootDir->deNmae[7] = 'R';
	rootDir->deExtension[0] = 'I';
	rootDir->deExtension[1] = 'V';
	rootDir->deExtension[2] = 'E';
	rootDir->deAttributes = DIR_ATTR_VOLUME;

	return STATUS_SUCCESS;
}

BOOLEAN RamDiskCheckParameters(PDEVICE_EXTENSION devExt, LARGE_INTEGER ByteOffset, size_t Length)
{
	// TODO: ����Ƿ������Ч����. ��ʼƫ����+���ȳ�������������,���߳��Ȳ���������С���ʵ�����,����һ������
	if (devExt->DiskRegInfo.DiskSize < Length || ByteOffset.QuadPart < 0 ||
		((ULONGLONG)ByteOffset.QuadPart > (ULONGLONG)devExt->DiskRegInfo.DiskSize) ||
		(Length & (devExt->DiskGeometry.BytesPerSector - 1)))
	{
		KdPrint(("[dbg]: Error invalid parameter, ByteOffset: %lld Length: %lld\n",ByteOffset.QuadPart, Length));
		return FALSE;
	}
	return TRUE;
}

VOID RamDiskEvtIoRead(WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
	PDEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;	
	WDF_REQUEST_PARAMETERS  wdfParameters = { 0 };
	LARGE_INTEGER byteOffset = { 0 };
	WDFMEMORY hMemory = NULL;
	NTSTATUS nStatus = STATUS_INVALID_PARAMETER;

	WDF_REQUEST_PARAMETERS_INIT(&wdfParameters);
	WdfRequestGetParameters(Request, &wdfParameters);
	byteOffset.QuadPart = wdfParameters.Parameters.Read.DeviceOffset;
	// LOG: �ж϶�ȡ��Χ���ܳ������̾����С�������������
	if (RamDiskCheckParameters(devExt, byteOffset, Length))
	{
		nStatus = WdfRequestRetrieveOutputMemory(Request, &hMemory);
		if (NT_SUCCESS(nStatus))
			nStatus = WdfMemoryCopyFromBuffer(hMemory, 0, devExt->DiskImage + byteOffset.LowPart, Length);
	}
	WdfRequestCompleteWithInformation(Request, nStatus, (ULONG_PTR)Length);
}

VOID RamDiskEvtIoWrite(WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
	PDEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;
	WDF_REQUEST_PARAMETERS  wdfParameters = { 0 };
	LARGE_INTEGER byteOffset = { 0 };
	WDFMEMORY hMemory = NULL;
	NTSTATUS nStatus = STATUS_INVALID_PARAMETER;

	WDF_REQUEST_PARAMETERS_INIT(&wdfParameters);
	WdfRequestGetParameters(Request, &wdfParameters);
	byteOffset.QuadPart = wdfParameters.Parameters.Read.DeviceOffset;
	// LOG: �ж϶�ȡ��Χ���ܳ������̾����С�������������
	if (RamDiskCheckParameters(devExt, byteOffset, Length))
	{
		nStatus = WdfRequestRetrieveOutputMemory(Request, &hMemory);
		if (NT_SUCCESS(nStatus))
			nStatus = WdfMemoryCopyToBuffer(hMemory, 0, devExt->DiskImage + byteOffset.LowPart, Length);
	}
	WdfRequestCompleteWithInformation(Request, nStatus, (ULONG_PTR)Length);
}

VOID RamDiskEvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode)
{
	PDEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;
	ULONG_PTR information = 0;
	size_t bufSize = 0;
	NTSTATUS nStatus = STATUS_INVALID_DEVICE_REQUEST;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);
	switch (IoControlCode)
	{
	case IOCTL_DISK_GET_PARTITION_INFO: {	// ��ȡ������Ϣ
		PPARTITION_INFORMATION outputBuffer = NULL;
		PBOOT_SECTOR bootSector = (PBOOT_SECTOR)devExt->DiskImage;
		information = sizeof(PARTITION_INFORMATION);
		nStatus = WdfRequestRetrieveOutputBuffer(Request, information, &outputBuffer, &bufSize);
		if (NT_SUCCESS(nStatus))
		{
			outputBuffer->PartitionType = (bootSector->bsFileSystemType[4] == '6') ? PARTITION_FAT_16 : PARTITION_FAT_12;
			outputBuffer->BootIndicator = FALSE;
			outputBuffer->RecognizedPartition = TRUE;
			outputBuffer->RewritePartition = FALSE;
			outputBuffer->StartingOffset.QuadPart = 0;
			outputBuffer->PartitionLength.QuadPart = devExt->DiskRegInfo.DiskSize;
			outputBuffer->HiddenSectors = (ULONG)(1L);
			outputBuffer->PartitionNumber = (ULONG)(-1L);
			nStatus = STATUS_SUCCESS;
		}
	}
	break;

	case IOCTL_DISK_GET_DRIVE_GEOMETRY: {	// ��ȡ��������
		PDISK_GEOMETRY outputBuffer = NULL;
		information = sizeof(DISK_GEOMETRY);
		nStatus = WdfRequestRetrieveOutputBuffer(Request, information, &outputBuffer, &bufSize);
		if (NT_SUCCESS(nStatus) && bufSize >= information)
		{
			RtlCopyMemory(outputBuffer, &(devExt->DiskGeometry), information);
			nStatus = STATUS_SUCCESS;
		}
	}
	break;

	case IOCTL_DISK_CHECK_VERIFY:
	case IOCTL_DISK_IS_WRITABLE:
		nStatus = STATUS_SUCCESS;
	break;

	default:
	break;
	}
	WdfRequestCompleteWithInformation(Request, nStatus, information);
}



