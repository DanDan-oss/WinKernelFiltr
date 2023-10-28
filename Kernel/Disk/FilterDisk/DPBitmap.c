#include <ntifs.h>
#include "DPBitmap.h"

#define _FileSystemNameLength	64
#define FAT16_SIG_OFFSET	54			//����FAT16�ļ�ϵͳǩ����ƫ����
#define FAT32_SIG_OFFSET	82			//����FAT32�ļ�ϵͳǩ����ƫ����
#define NTFS_SIG_OFFSET		3			//����NTFS�ļ�ϵͳǩ����ƫ����

PDP_FILTER_DEV_EXTENSION gProtectDevExt = NULL;
static tBitmap bitmapMask[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };	//bitmap��λ����

NTSTATUS NTAPI DPVolumeOnlineCompleteRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING DosName = { 0 };		// ���豸��dos��, C��D
	PVOLUME_ONLINE_CONTEXT VolumeContext = (PVOLUME_ONLINE_CONTEXT)Context;

	ASSERT(VolumeContext != NULL);
	ntStatus = IoVolumeDeviceToDosName(VolumeContext->DevExt->PhyDeviceObject, &DosName);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	// ������ת��Ϊ��д
	VolumeContext->DevExt->VolumeLetter = DosName.Buffer[0];
	if (VolumeContext->DevExt->VolumeLetter > TEXT('Z'))
		VolumeContext->DevExt->VolumeLetter -= (TEXT('a') - TEXT('A'));

	// ����C��, ��ȡC����Ϣ
	if (VolumeContext->DevExt->VolumeLetter == TEXT('C'))
	{
		// ��ȡ����Ϣ
		ntStatus = DPQueryVolumeInformation(VolumeContext->DevExt->PhyDeviceObject, &VolumeContext->DevExt->TotalSizeInByte, &VolumeContext->DevExt->ClusterSizeInByte, \
			&VolumeContext->DevExt->SectorSizeInByte);
		if(!NT_SUCCESS(ntStatus))
			goto ERROUT;
		// �������Ӧλͼ
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
	
	// ���õȴ�ͬ���¼�
	KeSetEvent(VolumeContext->Event, 0, FALSE);
	return ntStatus;
}

NTSTATUS NTAPI DPQueryVolumeInformationCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(DeviceObject);
	KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

PVOID DPBitmapAlloc(DWORD poolType, DWORD length)
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
	if (bitmap)
	{
		if (bitmap->BitMap)
		{
			for (int i = 0; i < bitmap->regionNumber; ++i)
				if (*(bitmap->BitMap + i))
					ExFreePool(*(bitmap->BitMap + i));
			ExFreePool(bitmap->BitMap);
			bitmap->BitMap = NULL;
		}
		ExFreePool(bitmap);
		bitmap = NULL;
	}
	return;
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

	//��������ļ�Ϊϡ���ļ�
	ntStatus = ZwFsControlFile(gProtectDevExt->TempFile, NULL, NULL, NULL, &ios, FSCTL_SET_SPARSE, NULL, 0, NULL, 0);
	if(!NT_SUCCESS(ntStatus))
		goto ERROUT;

	// ��������ļ��Ĵ�СΪD�̵Ĵ�С���ұ���10m�ı����ռ�
	fileEndInfo.EndOfFile.QuadPart = gProtectDevExt->TotalSizeInByte.QuadPart + 10 * 1024 * 1024;
	ntStatus = ZwSetInformationFile(gProtectDevExt->TempFile, &ios, &fileEndInfo, sizeof(FILE_END_OF_FILE_INFORMATION), FileEndOfFileInformation);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	//����ɹ���ʼ���ͽ������ı�����־����Ϊ�ڱ���״̬
	gProtectDevExt->Protect = TRUE;
	return;

ERROUT:
	KdPrint(("[dbg]: error create temp file!\n"));
	return;
}

NTSTATUS NTAPI DPBitmapInit(DP_BITMAP** bitmap, DWORD sectorSize, DWORD byteSize, DWORD regionSize, DWORD regionNumber)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	DP_BITMAP* myBitmap = NULL;

	if (!bitmap || !byteSize || !byteSize || !regionSize || !regionNumber)
		return STATUS_UNSUCCESSFUL;

	__try 
	{
		// ����bitmap�ṹ�ڴ�,����ʼ����ֵ
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

		// �����regionNumber��ָ��region��ָ��. ָ������
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
		// ������
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

		// ����Ҫ���õ�ƫ�����ͳ�����������Ҫʹ����Щregion, �����Ҫ�ͷ�������ָ����ڴ�ռ�
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

		// ��ʼ����bitmap�� ��Ҫ��Ҫ���õ�����װ�ֽڶ���(�������԰��ֽ����ö�����Ҫ��λ����), ����û���ֽڶ���������ֶ�����
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
		for (index = offset.QuadPart + (QWORD)length - bitmap->sectorSize; index >= offset.QuadPart; index -= bitmap->sectorSize)
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
		if (index < offset.QuadPart || setEnd.QuadPart == setBegin.QuadPart)
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}
		regionEnd = (ULONG)(setEnd.QuadPart / (QWORD)bitmap->regionReferSize);
		for (index = setBegin.QuadPart; index <= setEnd.QuadPart; )
		{
			regionBegin = (ULONG)(index / (QWORD)bitmap->regionReferSize);
			regionOffsetBegin = (ULONG)(index % (QWORD)bitmap->regionReferSize);
			byteOffsetBegin = regionOffsetBegin / bitmap->byteSize / bitmap->sectorSize;
			if (regionBegin == regionEnd)
			{	//����������õ�����û�п�����region��ֻ��Ҫʹ��memsetȥ����byte������Ȼ����������
				regionOffsetEnd = (ULONG)(setEnd.QuadPart % (QWORD)bitmap->regionReferSize);
				byteOffsetEnd = regionOffsetEnd / bitmap->byteSize / bitmap->sectorSize;
				memset(*(bitmap->BitMap + regionBegin) + byteOffsetBegin, 0xff, byteOffsetEnd - byteOffsetBegin + 1);
				break;
			}
			else
			{	//����������õ������������region����Ҫ����������
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
		// ������
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
		
		//������Ҫ��ȡ��λͼ��Χ�����������λ������Ϊ1������Ҫ��bufIn������ָ�����Ӧλ�õ����ݿ�����bufInOut��
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
		if (offset.QuadPart <0 || offset.QuadPart + length >bitmap->bitmapReferSize || !bitmap)
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