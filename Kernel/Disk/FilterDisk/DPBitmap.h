
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
	ULONG		sectorSize;			// 卷中每个扇区有多少字节,同时说明了bitmap中一个位对应的字节数
	ULONG		byteSize;			// 每个字节有几位,默认情况下是8
	ULONG		regionSize;			// 每个块多大字节
	ULONG		regionNumber;		// 这个bitmap总共有多少块
	ULONG		regionReferSize;	// 块对应的实际字节, 这个值应该是 sectorSize * byteSize * regionSize
	QWORD		bitmapReferSize;	// 这个bitmap对应的实际字节数, 这个值应该是 sectorSize * byteSize * regionSize * regionNumber
	tBitmap**	BitMap;				// 指向bitmap空间.指针数组[]
	PVOID		lockBitmap;
}DP_BITMAP, *PDP_BITMAP;

typedef struct _DP_FILTER_DEV_EXTENSION
{
	WCHAR			VolumeLetter;			// 卷的名字 "C: , D:"
	BYTE			Protect;				// 卷是否处于保护状态
	LARGE_INTEGER	TotalSizeInByte;		// 卷大小,单位byte
	DWORD			ClusterSizeInByte;		// 卷上文件系统的每簇大小
	DWORD			SectorSizeInByte;		// 卷每个扇区的大小
	BOOLEAN			InitializeCompleted;	// 标志位,这个结构是否初始化完毕

	PDEVICE_OBJECT	FilterDeviceObject;		// 卷设备对应过滤设备的设备对象
	PDEVICE_OBJECT	LowerDeviceObject;		// 卷设备对应的过滤设备的下层设备对象
	PDEVICE_OBJECT	PhyDeviceObject;		// 卷设备对应的物理设备对象

	HANDLE			TempFile;				// 用来转储的文件句柄
	PDP_BITMAP		Bitmap;					// 卷上保护系统使用的图位句柄
	LIST_ENTRY		RequestList;			// 卷上保护系统使用的请求队列
	KSPIN_LOCK		RequestLock;			// 卷上保护系统使用的请求队列的锁
	KEVENT			RequestEvent;			// 卷上保护系统使用的请求队列的同步事件
	PVOID			ThreadHandle;			// 卷上保护系统使用的请求队列处理线程的线程句柄
	BOOLEAN			ThreadTermFlag;			// 卷上保护系统使用的请求队列的处理线程结束标志
	KEVENT			PagingPathCountEvent;	// 卷上保护系统关机分页电源请求的计数事件
	LONG			PagingPathCount;		// 卷上保护系统关机分页电源请求的计数
}DP_FILTER_DEV_EXTENSION, *PDP_FILTER_DEV_EXTENSION;

typedef struct _VOLUME_ONLINE_CONTEXT
{
	PDP_FILTER_DEV_EXTENSION DevExt;	//在volume_online的DeviceIoControl中传给完成函数的设备扩展
	PKEVENT Event;						//在volume_online的DeviceIoControl中传给完成函数的同步事件
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
