#include <ntifs.h>
#include "DPBitmap.h"

PDP_FILTER_DEV_EXTENSION gProtectDevExt = NULL;
static tBitmap bitmapMask[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };	//bitmap的位掩码

NTSTATUS NTAPI DPVolumeOnlineCompleteRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING DosName = { 0 };		// 卷设备的dos名, C、D
	PVOLUME_ONLINE_CONTEXT VolumeContext = (PVOLUME_ONLINE_CONTEXT)Context;

	ASSERT(VolumeContext != NULL);
	ntStatus = IoVolumeDeviceToDosName(VolumeContext->DevExt->PhyDeviceObject, &DosName);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	// 将名字转换为大写
	VolumeContext->DevExt->VolumeLetter = DosName.Buffer[0];
	if (VolumeContext->DevExt->VolumeLetter > TEXT('Z'))
		VolumeContext->DevExt->VolumeLetter -= (TEXT('a') - TEXT('A'));

	// 保护C盘, 获取C盘信息
	if (VolumeContext->DevExt->VolumeLetter == TEXT('C'))
	{
		// 获取卷信息
		ntStatus = DPQueryVolumeInformation(VolumeContext->DevExt->PhyDeviceObject, &VolumeContext->DevExt->TotalSizeInByte, &VolumeContext->DevExt->ClusterSizeInByte, \
			&VolumeContext->DevExt->SectorSizeInByte);
		if(!NT_SUCCESS(ntStatus))
			goto ERROUT;
		// 建立卷对应位图
		ntStatus = DPBitmapInit(&VolumeContext->DevExt->Bitmap, VolumeContext->DevExt->SectorSizeInByte, 8, 25600, \
			(DWORD)(VolumeContext->DevExt->TotalSizeInByte.QuadPart / (QWORD)(25600 * 8 * VolumeContext->DevExt->SectorSizeInByte)) + 1);
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

VOID NTAPI DPReinitializationRoutine(IN	PDRIVER_OBJECT DriverObject, IN PVOID Context, IN ULONG Count)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	static PWCHAR wcSparseFileName = SPARSE_FILE_NAME;
	UNICODE_STRING uniSparseFileName = { 0 };
	IO_STATUS_BLOCK ios = { 0 };
	OBJECT_ATTRIBUTES ObjectAttr = { 0 };
	FILE_END_OF_FILE_INFORMATION fileEndInfo = { 0 };

	RtlInitUnicodeString(&uniSparseFileName, wcSparseFileName);
	InitializeObjectAttributes(&ObjectAttr, &uniSparseFileName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	ntStatus = ZwCreateFile(&gProtectDevExt->TempFile, GENERIC_READ | GENERIC_WRITE, &ObjectAttr, &ios, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OVERWRITE_IF, \
		FILE_NON_DIRECTORY_FILE | FILE_RANDOM_ACCESS | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING, NULL, 0);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	//设置这个文件为稀疏文件
	ntStatus = ZwFsControlFile(gProtectDevExt->TempFile, NULL, NULL, NULL, &ios, FSCTL_SET_SPARSE, NULL, 0, NULL, 0);
	if(!NT_SUCCESS(ntStatus))
		goto ERROUT;

	// 设置这个文件的大小为D盘的大小并且保留10m的保护空间
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

NTSTATUS NTAPI DPBitmapInit(DP_BITMAP** bitmap, unsigned long sectorSize, unsigned long byteSize, unsigned long regionSize, unsigned long regionNumber)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	DP_BITMAP* myBitmap = NULL;

	if (!bitmap || !byteSize || !byteSize || !regionSize || !regionNumber)
		return STATUS_UNSUCCESSFUL;

	__try 
	{
		// 分配bitmap结构内存,并初始化赋值
		myBitmap = (DP_BITMAP*)DPBitmapAlloc(0, sizeof(DP_BITMAP));
		if (NULL == myBitmap)
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
		if (NULL == myBitmap->BitMap)
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

NTSTATUS NTAPI DPBitmapSet(DP_BITMAP* bitmap, LARGE_INTEGER offset, unsigned long length)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	DWORD bitPos = 0;
	DWORD regionBegin = 0, regionEnd = 0;
	DWORD regionOffsetBegin = 0, regionOffsetEnd = 0;
	DWORD byteOffsetBegin = 0, byteOffsetEnd = 0;
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
		regionBegin = (DWORD)(offset.QuadPart / (QWORD)bitmap->regionReferSize);
		regionEnd = (DWORD)((offset.QuadPart + (QWORD)length) / (QWORD)bitmap->regionReferSize);
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
		for (QWORD index = offset.QuadPart; index < offset.QuadPart + (QWORD)length; index += bitmap->sectorSize)
		{
			regionBegin = (DWORD)(index / (QWORD)bitmap->regionReferSize);
			regionOffsetBegin = (DWORD)(index % (QWORD)bitmap->regionReferSize);
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
		for (index = offset.QuadPart + (QWORD)length - bitmap->sectorSize; index >= offset.QuadPart; index -= bitmap->sectorSize)
		{
			regionBegin = (DWORD)(index / bitmap->regionReferSize);
			regionOffsetBegin = (DWORD)(index % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize / bitmap->sectorSize;
			bitPos = (regionOffsetBegin / bitmap->sectorSize) % bitmap->byteSize;
			if (7 == bitPos)
			{
				setEnd.QuadPart = index;
				break;
			}
			*(*(bitmap->BitMap + regionBegin) + byteOffsetBegin) |= bitmapMask[bitPos];
		}
		if (index < offset.QuadPart || setEnd.QuadPart == setBegin.QuadPart)
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}
		regionEnd = (DWORD)(setEnd.QuadPart / (QWORD)bitmap->regionReferSize);
		for (index = setBegin.QuadPart; index <= setEnd.QuadPart; )
		{
			regionBegin = (DWORD)(index / (QWORD)bitmap->regionReferSize);
			regionOffsetBegin = (DWORD)(index % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize / bitmap->sectorSize;
			if (regionBegin == regionEnd)
			{	//如果我们设置的区域没有跨两个region，只需要使用memset去做按byte的设置然后跳出即可
				regionOffsetEnd = (DWORD)(setEnd.QuadPart % (QWORD)bitmap->regionReferSize);
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

NTSTATUS NTAPI DPBitmapGet(DP_BITMAP* bitmap, LARGE_INTEGER offset, unsigned long length, void* bufInOut, void* bufIn)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	DWORD bitPos = 0;
	DWORD regionBegin = 0;
	DWORD regionOffsetBegin = 0;
	DWORD byteOffsetBegin = 0;
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
			regionBegin = (DWORD)((offset.QuadPart + (QWORD)index) / (QWORD)bitmap->regionReferSize);
			regionOffsetBegin = (DWORD)((offset.QuadPart + (QWORD)index) % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize / bitmap->sectorSize;
			bitPos = (regionOffsetBegin / bitmap->sectorSize) % bitmap->byteSize;
			if (NULL != *(bitmap->BitMap + regionBegin) && (*(*(bitmap->BitMap + regionBegin) + byteOffsetBegin) & bitmapMask[bitPos]))
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

