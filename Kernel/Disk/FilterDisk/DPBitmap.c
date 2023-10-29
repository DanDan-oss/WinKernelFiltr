#include <ntifs.h>
#include "DPBitmap.h"

#define _FileSystemNameLength	64
#define FAT16_SIG_OFFSET	54			//定义FAT16文件系统签名的偏移量
#define FAT32_SIG_OFFSET	82			//定义FAT32文件系统签名的偏移量
#define NTFS_SIG_OFFSET		3			//定义NTFS文件系统签名的偏移量

PDP_FILTER_DEV_EXTENSION gProtectDevExt = NULL;
static tBitmap bitmapMask[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };	//bitmap的位掩码

NTSTATUS NTAPI DPVolumeOnlineCompleteRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
	UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING DosName = { 0 };		// 卷设备的dos名, C、D
	PVOLUME_ONLINE_CONTEXT VolumeContext = (PVOLUME_ONLINE_CONTEXT)Context;

	ASSERT(VolumeContext != NULL);
	ntStatus = IoVolumeDeviceToDosName(VolumeContext->DevExt->PhyDeviceObject, &DosName);
	if (!NT_SUCCESS(ntStatus) || !DosName.Buffer)
		goto ERROUT;

	// 将名字转换为大写
	VolumeContext->DevExt->VolumeLetter = DosName.Buffer[0];
	if (VolumeContext->DevExt->VolumeLetter > TEXT('Z'))
		VolumeContext->DevExt->VolumeLetter -= (TEXT('a') - TEXT('A'));

	// 保护z盘, 获取z盘信息
	if (VolumeContext->DevExt->VolumeLetter == TEXT('Z'))
	{
		// 获取卷信息
		ntStatus = DPQueryVolumeInformation(VolumeContext->DevExt->PhyDeviceObject, &VolumeContext->DevExt->TotalSizeInByte, &VolumeContext->DevExt->ClusterSizeInByte, \
			&VolumeContext->DevExt->SectorSizeInByte);
		if(!NT_SUCCESS(ntStatus))
			goto ERROUT;
		// 建立卷对应位图
		ntStatus = DPBitmapInit(&VolumeContext->DevExt->Bitmap, VolumeContext->DevExt->SectorSizeInByte, 8, 25600, \
			(DWORD)(VolumeContext->DevExt->TotalSizeInByte.QuadPart / (LONGLONG)(25600 * 8 * (LONGLONG)VolumeContext->DevExt->SectorSizeInByte)) + 1);
		if (!NT_SUCCESS(ntStatus))
			goto ERROUT;
		gProtectDevExt = VolumeContext->DevExt;
	}

ERROUT:
	if (!NT_SUCCESS(ntStatus))
	{
		if (VolumeContext->DevExt->Bitmap)
			DPBitmapFree(VolumeContext->DevExt->Bitmap);
		if (VolumeContext->DevExt->TempFile)
			ZwClose(VolumeContext->DevExt->TempFile);
	}
	if (DosName.Buffer)
		ExFreePool(DosName.Buffer);
	
	// 设置等待同步事件
	KeSetEvent(VolumeContext->Event, 0, FALSE);
	return ntStatus;
}

NTSTATUS NTAPI DPQueryVolumeInformation(PDEVICE_OBJECT DeviceObject, PLARGE_INTEGER TotalSize, PDWORD ClusterSize, PDWORD SectorSize)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER readOffset = { 0 };	//读取的偏移量,对于DBR来说是卷的起始位置,所以偏移量为0
	IO_STATUS_BLOCK ios = { 0 };
	KEVENT event = { 0 };
	PIRP irp = NULL;
	BYTE DBR[512] = { 0 };			// 用来读取卷DBR扇区的数据缓冲区
	ULONG DBRLength = 512;			// DBR扇区有512个bytes大小
	CONST UCHAR FAT16FLG[4] = { 'F','A','T','1' };	// 这是FAT16文件系统的标志
	CONST UCHAR FAT32FLG[4] = { 'F','A','T','3' };	// 这是FAT32文件系统的标志
	CONST UCHAR NTFSFLG[4] = { 'N','T','F','S' };	// 这是NTFS文件系统的标志

	// 三个指针的类型分别代表FAT16,FAT32和NTFS类型文件系统的DBR数据结构,统一指向读取的DBR数据
	PDP_NTFS_BOOT_SECTOR pdpNtfsBootSector = (PDP_NTFS_BOOT_SECTOR)DBR;
	PDP_FAT16_BOOT_SECTOR pdpFat16BootSector = (PDP_FAT16_BOOT_SECTOR)DBR;
	PDP_FAT32_BOOT_SECTOR pdpFat32BootSector = (PDP_FAT32_BOOT_SECTOR)DBR;

	KeInitializeEvent(&event, NotificationEvent, FALSE);
	irp = IoBuildAsynchronousFsdRequest(IRP_MJ_READ, DeviceObject, DBR, DBRLength, &readOffset, &ios);
	if (!irp)
		goto ERROUT;
	IoSetCompletionRoutine(irp, DPQueryVolumeInformationCompletionRoutine, &event, TRUE, TRUE, TRUE);
	ntStatus = IoCallDriver(DeviceObject, irp);
	if (STATUS_PENDING == ntStatus)
	{
		// 下层设备还未完成irp请求,等待
		ntStatus = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		ntStatus = irp->IoStatus.Status;
		if (!NT_SUCCESS(ntStatus))
			goto ERROUT;
	}

	// 通过标志判断系统卷类型
	if (*(PDWORD)&DBR[NTFS_SIG_OFFSET] == *(PDWORD)NTFSFLG)
	{	// 这是一个ntfs文件系统的卷,下面根据ntfs卷的DBR定义来对各种需要获取的值进行赋值操作
		*SectorSize = (DWORD)(pdpNtfsBootSector->BytesPerSector);
		*ClusterSize = (*SectorSize) * (DWORD)(pdpNtfsBootSector->SectorsPerCluster);
		TotalSize->QuadPart = (LONGLONG)(*SectorSize) * (LONGLONG)pdpNtfsBootSector->TotalSectors;
	}
	else if (*(PDWORD)&DBR[FAT16_SIG_OFFSET] == *(DWORD*)FAT16FLG)
	{	// 这是一个fat16文件系统的卷,下面根据fat16卷的DBR定义来对各种需要获取的值进行赋值操作
		*SectorSize = (DWORD)(pdpFat16BootSector->BytesPerSector);
		*ClusterSize = (*SectorSize) * (DWORD)(pdpFat16BootSector->SectorsPerCluster);
		TotalSize->QuadPart = (LONGLONG)(*SectorSize) * ((LONGLONG)pdpFat16BootSector->LargeSectors + pdpFat16BootSector->Sectors);
	}
	else if (*(DWORD*)&DBR[FAT32_SIG_OFFSET] == *(DWORD*)FAT32FLG)
	{	// 这是一个fat32文件系统的卷,下面根据fat32卷的DBR定义来对各种需要获取的值进行赋值操作
		*SectorSize = (DWORD)(pdpFat32BootSector->BytesPerSector);
		*ClusterSize = (*SectorSize) * (DWORD)(pdpFat32BootSector->SectorsPerCluster);
		TotalSize->QuadPart = (LONGLONG)(*SectorSize) * ((LONGLONG)pdpFat32BootSector->LargeSectors + pdpFat32BootSector->Sectors);
	}
	else
	{	// 其他文件系统卷
		ntStatus = STATUS_UNSUCCESSFUL;
	}

ERROUT:
	if (irp)
		IoFreeIrp(irp);
	return ntStatus;
}

NTSTATUS NTAPI DPQueryVolumeInformationCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(DeviceObject);
	KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

PVOID DPBitmapAlloc(ULONG poolType, ULONG length)
{
	switch (poolType)
	{
	case 0:
		return ExAllocatePoolWithTag(NonPagedPool, length, 'mbpZ');
	case 1:
		return ExAllocatePoolWithTag(PagedPool, length, 'mbpZ');
	default:
		break;
	}
	return NULL;
}

VOID NTAPI DPBitmapFree(DP_BITMAP* bitmap)
{

	if (!bitmap)
		return;

	if (!bitmap->BitMap)
		goto FREE_BITMAP;

	for (DWORD i = 0; i < bitmap->regionNumber; ++i)
		if (NULL != *(bitmap->BitMap + i))
			ExFreePool(*(bitmap->BitMap + i));

FREE_BITMAP:
	if (bitmap->BitMap) 
	{
		ExFreePool(bitmap->BitMap);
		bitmap->BitMap = NULL;
	}
	ExFreePool(bitmap);
	bitmap = NULL;
	return;
}

VOID NTAPI DPReinitializationRoutine(IN	PDRIVER_OBJECT DriverObject, IN PVOID Context, IN ULONG Count)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(Count);
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	static PWCHAR wcSparseFileName = SPARSE_FILE_NAME;		// //Z盘的缓冲文件名
	UNICODE_STRING uniSparseFileName = { 0 };
	IO_STATUS_BLOCK ios = { 0 };
	OBJECT_ATTRIBUTES ObjectAttr = { 0 };
	FILE_END_OF_FILE_INFORMATION fileEndInfo = { 0 };		//设置文件大小的时候使用的文件结尾描述符

	// 打开我们将要用来做转储的文件
	RtlInitUnicodeString(&uniSparseFileName, wcSparseFileName);
	InitializeObjectAttributes(&ObjectAttr, &uniSparseFileName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	// 建立文件,加入FILE_NO_INTERMEDIATE_BUFFERING选项,避免文件系统再缓存这个文件
	ntStatus = ZwCreateFile(&gProtectDevExt->TempFile, GENERIC_READ | GENERIC_WRITE, &ObjectAttr, &ios, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OVERWRITE_IF, \
		FILE_NON_DIRECTORY_FILE | FILE_RANDOM_ACCESS | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING, NULL, 0);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	//设置这个文件为稀疏文件
	ntStatus = ZwFsControlFile(gProtectDevExt->TempFile, NULL, NULL, NULL, &ios, FSCTL_SET_SPARSE, NULL, 0, NULL, 0);
	if(!NT_SUCCESS(ntStatus))
		goto ERROUT;

	// 设置这个文件的大小为z盘的大小并且保留10m的保护空间
	fileEndInfo.EndOfFile.QuadPart = gProtectDevExt->TotalSizeInByte.QuadPart + 10 * 1024 * 1024;
	ntStatus = ZwSetInformationFile(gProtectDevExt->TempFile, &ios, &fileEndInfo, sizeof(FILE_END_OF_FILE_INFORMATION), FileEndOfFileInformation);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	//如果成功初始化就将这个卷的保护标志设置为在保护状态
	gProtectDevExt->Protect = TRUE;
	return;

ERROUT:
	KdPrint(("[dbg]: error create temp file!\n"));
	return;
}

NTSTATUS NTAPI DPBitmapInit(DP_BITMAP** bitmap, ULONG sectorSize, ULONG byteSize, ULONG regionSize, ULONG regionNumber)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	DP_BITMAP* myBitmap = NULL;

	if (!bitmap || !byteSize || !byteSize || !regionSize || !regionNumber)
		return STATUS_UNSUCCESSFUL;

	__try 
	{
		// 分配bitmap结构内存,并初始化赋值
		myBitmap = (DP_BITMAP*)DPBitmapAlloc(0, sizeof(DP_BITMAP));
		if (!myBitmap)
		{
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		RtlIsZeroMemory(myBitmap, sizeof(DP_BITMAP));
		myBitmap->sectorSize = sectorSize;
		myBitmap->byteSize = byteSize;
		myBitmap->regionSize = regionSize;
		myBitmap->regionNumber = regionNumber;
		myBitmap->regionReferSize = sectorSize * byteSize * regionSize;
		myBitmap->bitmapReferSize = (QWORD)sectorSize * (QWORD)byteSize * (QWORD)regionSize * (QWORD)regionNumber;

		// 分配出regionNumber个指向region的指针. 指针数组
		myBitmap->BitMap = (tBitmap**)DPBitmapAlloc(0, sizeof(tBitmap*) * regionNumber);
		if (!myBitmap->BitMap)
		{
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		RtlIsZeroMemory(myBitmap->BitMap, sizeof(tBitmap*) * regionNumber);
		ntStatus = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (!NT_SUCCESS(ntStatus))
	{
		if (myBitmap)
			DPBitmapFree(myBitmap);
		*bitmap = NULL;
	}
	return ntStatus;
}

NTSTATUS NTAPI DPBitmapSet(DP_BITMAP* bitmap, LARGE_INTEGER offset, ULONG length)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	ULONG bitPos = 0;
	ULONG regionBegin = 0, regionEnd = 0;
	ULONG regionOffsetBegin = 0, regionOffsetEnd = 0;
	ULONG byteOffsetBegin = 0, byteOffsetEnd = 0;
	LARGE_INTEGER setBegin = { 0 }, setEnd = { 0 };
	QWORD index = 0;
	__try
	{
		// 检查变量
		if (!bitmap || offset.QuadPart < 0)
		{
			ntStatus = STATUS_INVALID_PARAMETER;
			__leave;
		}
		if (offset.QuadPart % bitmap->sectorSize || length % bitmap->sectorSize )
		{
			ntStatus = STATUS_INVALID_PARAMETER;
			__leave;
		}

		// 根据要设置的偏移量和长度来计算需要使用哪些region, 如果需要就分配它们指向的内存空间
		regionBegin = (ULONG)(offset.QuadPart / (QWORD)bitmap->regionReferSize);
		regionEnd = (ULONG)((offset.QuadPart + (QWORD)length) / (QWORD)bitmap->regionReferSize);
		for (index = 0; index < regionEnd; ++index)
			if (NULL == *(bitmap->BitMap + index))
			{
				*(bitmap->BitMap + index) = (tBitmap*)DPBitmapAlloc(0, sizeof(tBitmap) * bitmap->regionSize);
				if ( NULL == *(bitmap->BitMap + index))
				{
					ntStatus = STATUS_INSUFFICIENT_RESOURCES;
					__leave;
				}
				else
				{
					RtlZeroMemory(*(bitmap->BitMap + index), sizeof(tBitmap) * bitmap->regionSize);
				}
			}

		// 开始设置bitmap， 需要将要设置的区域安装字节对齐(这样可以按字节设置而不需要按位设置), 对于没有字节对齐的区域手动设置
		for (index = offset.QuadPart; index < offset.QuadPart + (QWORD)length; index += bitmap->sectorSize)
		{
			regionBegin = (ULONG)(index / (QWORD)bitmap->regionReferSize);
			regionOffsetBegin = (ULONG)(index % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize % bitmap->sectorSize;
			bitPos = (byteOffsetBegin / bitmap->sectorSize) % bitmap->byteSize;
			if (!bitPos)
			{
				setBegin.QuadPart = index;
				break;
			}
			*(*(bitmap->BitMap + regionBegin) + regionOffsetBegin) |= bitmapMask[bitPos];
		}
		if (index >= offset.QuadPart + (QWORD)length)
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}
		for (index = offset.QuadPart + (QWORD)length - bitmap->sectorSize; index >= (QWORD)offset.QuadPart; index -= bitmap->sectorSize)
		{
			regionBegin = (ULONG)(index / bitmap->regionReferSize);
			regionOffsetBegin = (ULONG)(index % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize / bitmap->sectorSize;
			bitPos = (regionOffsetBegin / bitmap->sectorSize) % bitmap->byteSize;
			if (7 == bitPos)
			{
				setEnd.QuadPart = index;
				break;
			}
			*(*(bitmap->BitMap + regionBegin) + byteOffsetBegin) |= bitmapMask[bitPos];
		}
		if (index < (QWORD)offset.QuadPart || setEnd.QuadPart == setBegin.QuadPart)
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}
		regionEnd = (ULONG)(setEnd.QuadPart / (QWORD)bitmap->regionReferSize);
		for (index = setBegin.QuadPart; index <= (QWORD)setEnd.QuadPart; )
		{
			regionBegin = (ULONG)(index / (QWORD)bitmap->regionReferSize);
			regionOffsetBegin = (ULONG)(index % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize / bitmap->sectorSize;
			if (regionBegin == regionEnd)
			{	//如果我们设置的区域没有跨两个region，只需要使用memset去做按byte的设置然后跳出即可
				regionOffsetEnd = (ULONG)(setEnd.QuadPart % (QWORD)bitmap->regionReferSize);
				byteOffsetEnd = regionOffsetEnd / bitmap->byteSize / bitmap->sectorSize;
				memset(*(bitmap->BitMap + regionBegin) + byteOffsetBegin, 0xff, byteOffsetEnd - byteOffsetBegin + 1);
				break;
			}
			else
			{	//如果我们设置的区域跨了两个region，需要设置完后递增
				regionOffsetEnd = bitmap->regionReferSize;
				byteOffsetEnd = regionOffsetEnd / bitmap->byteSize / bitmap->sectorSize;
				memset(*(bitmap->BitMap + regionBegin) + byteOffsetBegin, 0xff, byteOffsetEnd - byteOffsetBegin);
				index += (byteOffsetEnd - byteOffsetBegin) * bitmap->byteSize * bitmap->sectorSize;
			}
		}
		ntStatus = STATUS_SUCCESS;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ntStatus = STATUS_UNSUCCESSFUL;
	}

	if (!NT_SUCCESS(ntStatus))
	{

	}
	return ntStatus;
}

NTSTATUS NTAPI DPBitmapGet(DP_BITMAP* bitmap, LARGE_INTEGER offset, ULONG length, PVOID bufInOut, PVOID bufIn)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	ULONG bitPos = 0;
	ULONG regionBegin = 0;
	ULONG regionOffsetBegin = 0;
	ULONG byteOffsetBegin = 0;
	QWORD index = 0;

	__try
	{
		// 检查参数
		if (!bitmap || !bufInOut || !bufIn || offset.QuadPart < 0)
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}
		if (offset.QuadPart % bitmap->sectorSize || length % bitmap->sectorSize)
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}
		
		//遍历需要获取的位图范围，如果出现了位被设置为1，就需要用bufIn参数中指向的相应位置的数据拷贝到bufInOut中
		for (index = 0; index < length; index += bitmap->sectorSize)
		{
			regionBegin = (ULONG)((offset.QuadPart + (QWORD)index) / (QWORD)bitmap->regionReferSize);
			regionOffsetBegin = (ULONG)((offset.QuadPart + (QWORD)index) % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize / bitmap->sectorSize;
			bitPos = (regionOffsetBegin / bitmap->sectorSize) % bitmap->byteSize;
			if (*(bitmap->BitMap + regionBegin) && (*(*(bitmap->BitMap + regionBegin) + byteOffsetBegin) & bitmapMask[bitPos]))
				memcpy((tBitmap*)bufInOut + index, (tBitmap*)bufIn + index, bitmap->sectorSize);

		}
		ntStatus = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		ntStatus = STATUS_UNSUCCESSFUL;
	}
	return ntStatus;
}

long DPBitmapTest(DP_BITMAP* bitmap, LARGE_INTEGER offset, ULONG length)
{
	CHAR flag = 0;
	QWORD index = 0;
	ULONG bitPos = 0;
	ULONG regionBegin = 0;
	ULONG regionOffsetBegin = 0;
	ULONG byteOffsetBegin = 0;
	LONG ret = BITMAP_BIT_UNKNOW;
	__try
	{
		if (offset.QuadPart <0 || (QWORD)offset.QuadPart + length >bitmap->bitmapReferSize || !bitmap)
		{
			ret = BITMAP_BIT_UNKNOW;
			__leave;
		}

		for (index = 0; index < length; length += bitmap->sectorSize)
		{
			regionBegin = (ULONG)((offset.QuadPart + (QWORD)index) / (QWORD)bitmap->regionReferSize);
			regionOffsetBegin = (ULONG)((offset.QuadPart + (QWORD)index) % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize / bitmap->sectorSize;
			bitPos = (regionOffsetBegin / bitmap->sectorSize) % bitmap->byteSize;
			if (*(bitmap->BitMap + regionBegin) && *(*(bitmap->BitMap + regionBegin) + byteOffsetBegin) & bitmapMask[bitPos])
				flag |= 0x2;
			else
				flag |= 0x1;

			if (0x3 == flag)
				break;
		}

		switch (flag)
		{
		case 0x1:
			ret = BITMAP_RANGE_CLEAR;
			break;
		case 0x2:
			ret = BITMAP_RANGE_SET;
			break;
		case 0x3:
			ret = BITMAP_RANGE_BLEND;
			break;
		default:
			break;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		ret = BITMAP_BIT_UNKNOW;
	}

	return ret;
}