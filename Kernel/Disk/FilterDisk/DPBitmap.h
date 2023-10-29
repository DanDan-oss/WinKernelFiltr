
#ifndef _DP_BITMAP_H
#define _DP_BITMAP_H

#include <ntddk.h>
#include <wdf.h>

#define BITMAP_ERR_OUTOFRANGE	-1
#define BITMAP_ERR_ALLOCMEMORY	-2
#define BITMAP_SUCCESS			0
#define BITMAP_BIT_SET			1
#define BITMAP_BIT_CLEAR		2
#define BITMAP_BIT_UNKNOW		3
#define BITMAP_RANGE_SET		4
#define BITMAP_RANGE_CLEAR		5
#define BITMAP_RANGE_BLEND		6
#define BITMAP_RANGE_SIZE		25600
#define BITMAP_RANGE_SIZE_SMALL 256
#define BITMAP_RANGE_SIZE_MAX	51684
#define BITMAP_RANGE_AMOUNT		16*1024

#define SPARSE_FILE_NAME L"\\??\\Z:\\temp.dat"

typedef  unsigned __int8 BYTE, *PBYTE;
typedef  unsigned __int16 WORD, *PWORD;
typedef  unsigned __int32 DWORD, *PDWORD;
typedef  unsigned __int64 QWORD, *PQWORD;
typedef  unsigned char tBitmap;


#pragma pack(1)

typedef struct _DP_NTFS_BOOT_SECTOR
{
	UCHAR		Jump[3];				// offset+0
	UCHAR		FSID[8];				// offset+3
	USHORT		BytesPerSector;			// offset+11
	UCHAR		SectorsPerCluster;		// offset+13
	USHORT		ReservedSectors;		// offset+14
	UCHAR		Mbz1;					// offset+16		
	USHORT		Mbz2;					// offset+17
	USHORT		Reserved1;				// offset+19
	UCHAR		MediaDesc;				// offset+21
	USHORT		Mbz3;					// offset+22
	USHORT		SectorsPerTrack;		// offset+24
	USHORT		Heads;					// offset+26
	ULONG		HiddenSectors;			// offset+28
	ULONG		Reserved2[2];			// offset+32
	ULONGLONG	TotalSectors;			// offset+40
	ULONGLONG	MftStartLcn;			// offset+48
	ULONGLONG	Mft2StartLcn;			// offset+56
}DP_NTFS_BOOT_SECTOR, * PDP_NTFS_BOOT_SECTOR;

typedef struct _DP_FAT16_BOOT_SECTOR
{
	UCHAR		JMPInstruction[3];
	UCHAR		OEM[8];
	USHORT		BytesPerSector;
	UCHAR		SectorsPerCluster;
	USHORT		ReservedSectors;
	UCHAR		NumberOfFATs;
	USHORT		RootEntries;
	USHORT		Sectors;
	UCHAR		MediaDescriptor;
	USHORT		SectorsPerFAT;
	USHORT		SectorsPerTrack;
	USHORT		Heads;
	DWORD		HiddenSectors;
	DWORD		LargeSectors;
	UCHAR		PhysicalDriveNumber;
	UCHAR		CurrentHead;
} DP_FAT16_BOOT_SECTOR, * PDP_FAT16_BOOT_SECTOR;

typedef struct _DP_FAT32_BOOT_SECTOR
{
	UCHAR		JMPInstruction[3];
	UCHAR		OEM[8];
	USHORT		BytesPerSector;
	UCHAR		SectorsPerCluster;
	USHORT		ReservedSectors;
	UCHAR		NumberOfFATs;
	USHORT		RootEntries;
	USHORT		Sectors;
	UCHAR		MediaDescriptor;
	USHORT		SectorsPerFAT;
	USHORT		SectorsPerTrack;
	USHORT		Heads;
	DWORD		HiddenSectors;
	DWORD		LargeSectors;
	DWORD		LargeSectorsPerFAT;
	UCHAR		Data[24];
	UCHAR		PhysicalDriveNumber;
	UCHAR		CurrentHead;
} DP_FAT32_BOOT_SECTOR, * PDP_FAT32_BOOT_SECTOR;

#pragma pack()


typedef struct _DP_BITMAP
{
	ULONG		sectorSize;			// ����ÿ�������ж����ֽ�,ͬʱ˵����bitmap��һ��λ��Ӧ���ֽ���
	ULONG		byteSize;			// ÿ���ֽ��м�λ,Ĭ���������8
	ULONG		regionSize;			// ÿ�������ֽ�
	ULONG		regionNumber;		// ���bitmap�ܹ��ж��ٿ�
	ULONG		regionReferSize;	// ���Ӧ��ʵ���ֽ�, ���ֵӦ���� sectorSize * byteSize * regionSize
	QWORD		bitmapReferSize;	// ���bitmap��Ӧ��ʵ���ֽ���, ���ֵӦ���� sectorSize * byteSize * regionSize * regionNumber
	tBitmap**	BitMap;				// ָ��bitmap�ռ�.ָ������[]
	PVOID		lockBitmap;
}DP_BITMAP, *PDP_BITMAP;

typedef struct _DP_FILTER_DEV_EXTENSION
{
	WCHAR			VolumeLetter;			// ������� "C: , D:"
	BYTE			Protect;				// ���Ƿ��ڱ���״̬
	LARGE_INTEGER	TotalSizeInByte;		// ���С,��λbyte
	DWORD			ClusterSizeInByte;		// �����ļ�ϵͳ��ÿ�ش�С
	DWORD			SectorSizeInByte;		// ��ÿ�������Ĵ�С
	BOOLEAN			InitializeCompleted;	// ��־λ,����ṹ�Ƿ��ʼ�����

	PDEVICE_OBJECT	FilterDeviceObject;		// ���豸��Ӧ�����豸���豸����
	PDEVICE_OBJECT	LowerDeviceObject;		// ���豸��Ӧ�Ĺ����豸���²��豸����
	PDEVICE_OBJECT	PhyDeviceObject;		// ���豸��Ӧ�������豸����

	HANDLE			TempFile;				// ����ת�����ļ����
	PDP_BITMAP		Bitmap;					// ���ϱ���ϵͳʹ�õ�ͼλ���
	LIST_ENTRY		RequestList;			// ���ϱ���ϵͳʹ�õ��������
	KSPIN_LOCK		RequestLock;			// ���ϱ���ϵͳʹ�õ�������е���
	KEVENT			RequestEvent;			// ���ϱ���ϵͳʹ�õ�������е�ͬ���¼�
	PVOID			ThreadHandle;			// ���ϱ���ϵͳʹ�õ�������д����̵߳��߳̾��
	BOOLEAN			ThreadTermFlag;			// ���ϱ���ϵͳʹ�õ�������еĴ����߳̽�����־
	KEVENT			PagingPathCountEvent;	// ���ϱ���ϵͳ�ػ���ҳ��Դ����ļ����¼�
	LONG			PagingPathCount;		// ���ϱ���ϵͳ�ػ���ҳ��Դ����ļ���
}DP_FILTER_DEV_EXTENSION, *PDP_FILTER_DEV_EXTENSION;

typedef struct _VOLUME_ONLINE_CONTEXT
{
	PDP_FILTER_DEV_EXTENSION DevExt;	//��volume_online��DeviceIoControl�д�����ɺ������豸��չ
	PKEVENT Event;						//��volume_online��DeviceIoControl�д�����ɺ�����ͬ���¼�
}VOLUME_ONLINE_CONTEXT, *PVOLUME_ONLINE_CONTEXT;


VOID NTAPI DPReinitializationRoutine(IN	PDRIVER_OBJECT DriverObject,IN PVOID Context,IN ULONG Count);
NTSTATUS NTAPI DPVolumeOnlineCompleteRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, _In_reads_opt_(_Inexpressible_("varies")) PVOID Context);
NTSTATUS NTAPI DPQueryVolumeInformation(PDEVICE_OBJECT DeviceObject, PLARGE_INTEGER TotalSize, PDWORD ClusterSize, PDWORD SectorSize);
NTSTATUS NTAPI DPQueryVolumeInformationCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

PVOID DPBitmapAlloc(ULONG poolType, ULONG length);
VOID NTAPI DPBitmapFree(DP_BITMAP* bitmap);

NTSTATUS NTAPI DPBitmapInit(DP_BITMAP** bitmap, ULONG sectorSize, ULONG byteSize, ULONG regionSize, ULONG regionNumber);
NTSTATUS NTAPI DPBitmapSet(DP_BITMAP* bitmap, LARGE_INTEGER offset, ULONG length);
NTSTATUS NTAPI DPBitmapGet(DP_BITMAP* bitmap, LARGE_INTEGER offset, ULONG length, PVOID bufInOut, PVOID bufIn);

long DPBitmapTest(DP_BITMAP* bitmap, LARGE_INTEGER offset, ULONG length);

#endif // !_DP_BITMAP_H
