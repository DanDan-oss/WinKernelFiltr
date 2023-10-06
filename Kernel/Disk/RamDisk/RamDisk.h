#ifndef _DISK_DIRVER_H
#define _DISK_DIRVER_H

#include <ntddk.h>
#include <wdf.h>
#include <ntdddisk.h>

#define NT_DEVICE_NAME                  L"\\Device\\Ramdisk"
#define DOS_DEVICE_NAME                 L"\\DosDevices\\"

#define RAMDISK_TAG                     'DanR'  // "DanR"
#define DOS_DEVNAME_LENGTH              (sizeof(DOS_DEVICE_NAME)+sizeof(WCHAR)*10)
#define DRIVE_LETTER_LENGTH             (sizeof(WCHAR)*10)

#define DRIVE_LETTER_BUFFER_SIZE        10
#define DOS_DEVNAME_BUFFER_SIZE         (sizeof(DOS_DEVICE_NAME) / 2) + 10

#define RAMDISK_MEDIA_TYPE              0xF8
#define DIR_ENTRIES_PER_SECTOR          16

#define DEFAULT_DISK_SIZE               (1024*1024)     // 1 MB
#define DEFAULT_ROOT_DIR_ENTRIES        512
#define DEFAULT_SECTORS_PER_CLUSTER     2
#define DEFAULT_DRIVE_LETTER            L"X:"

#define DIR_ATTR_READONLY   0x01
#define DIR_ATTR_HIDDEN     0x02
#define DIR_ATTR_SYSTEM     0x04
#define DIR_ATTR_VOLUME     0x08
#define DIR_ATTR_DIRECTORY  0x10
#define DIR_ATTR_ARCHIVE    0x20


typedef struct _DISK_INFO
{
	ULONG DiskSize;				// 磁盘大小,以字节计算,磁盘最大只有4GB
	ULONG RootDirEntries;		// 磁盘根文件系统的进入节点
	ULONG SectorsPerCluster;	// 磁盘每个簇由多少个扇区组成
	UNICODE_STRING DriveLetter;	// 磁盘盘符
}DISK_INFO, *PDISK_INFO;

typedef struct _DEVICE_EXTENSION 
{
	PUCHAR DiskImage;			// 指向内存区域,内存盘实际数据数据存储空间
	DISK_GEOMETRY DiskGeometry;	// 存储内存盘的磁盘Geometry扇区
	DISK_INFO DiskRegInfo;		// 磁盘信息结构,安装时存放在注册表中
	UNICODE_STRING SymbolicLink;	// 盘的符号链接
	WCHAR DriverLetterBuffer[DRIVE_LETTER_BUFFER_SIZE];		// DiskRegInfo中DriverLetter存储空间,用户在注册表中指定的盘符
	WCHAR DosDeviceNameBuffer[DOS_DEVNAME_BUFFER_SIZE];		// 符号链接的存储空间
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, DeviceGetExtension)

typedef struct _QUEUE_EXTENSION {
	PDEVICE_EXTENSION DeviceExtension;
} QUEUE_EXTENSION, * PQUEUE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_EXTENSION, QueueGetExtension)

#pragma pack(1)
typedef struct _BOOT_SECTOR
{
	UCHAR bsJump[3];		// 跳转指令,跳转代DBR引导程序
	CCHAR bsOemNamep[8];	// 这个卷OEM名称
	USHORT bsBytesPerSec;	// 每个扇区由多少字节
	UCHAR bsSecPerClus;		// 每个簇有多少个扇区
	USHORT bsResSectors;	// 保留扇区数目,指的是第一个FAT表开始之前的扇区数,包括DBR本身
	UCHAR bsFATs;			// 这个卷由多少个FAT表
	USHORT bsRootDirEnts;	// 这个卷的根目录入口点有几个
	USHORT bsSectors;		// 这个卷一共有多少个扇区,对大于65535个扇区的卷,字段为0
	UCHAR bsMedia;			// 这个卷的介质类型
	USHORT bsFATsecs;		// 每个FAT表占用多少个扇区
	USHORT bsSecPerTrack;	// 每个磁道有多少个扇区
	USHORT bsHeads;			// 有多少磁头,也就是每个柱面的磁道数
	ULONG bsHiddenSecs;		// 有多少隐藏扇区
	ULONG bsHugeSectors;	// 一个卷超过65535个扇区,会使用这个字段来说明总扇区数量
	UCHAR bsDriveBumber;	// 驱动器编号
	UCHAR bsReserved1;		// 保留字段
	UCHAR bsBootSignature;	// 磁盘扩展引导区标签,Windows要求这个标签为0x28或者0x29
	ULONG bsVolumeID;		// 磁盘卷ID
	CCHAR bsLable[11];		// 磁盘标卷
	CCHAR bsFileSystemType[8];	// 磁盘上文件系统类型
	CCHAR bsReserved2[448];		// 保留字段
	UCHAR bsSig2[2];		// DBR结束标签
}BOOT_SECTOR, *PBOOT_SECTOR;

typedef struct _DIR_ENTRY
{
	UCHAR deNmae[8];		// 文件名
	UCHAR deExtension[3];	// 文件扩展名
	UCHAR deAttributes;		// 文件属性
	UCHAR deReserved;		// 系统保留
	USHORT deTime;			// 文件建立时间
	USHORT deDate;			// 文件建立日期
	USHORT deStartCluster;	// 文件第一个簇的编号
	ULONG deFileSize;		// 文件大小
}DIR_ENTRY, *PDIR_ENTRY;
#pragma pack()

NTSTATUS RamDiskEvtDeviceAdd(WDFDRIVER DriverObject, PWDFDEVICE_INIT DeviceInit);	// 添加虚拟磁盘设备并初始化
VOID RamDiskEvtDeviceContextCleanup(IN WDFDEVICE Device);							// 清理创建的虚拟磁盘
VOID RamDiskQueryDiskRegParameters(PWCHAR RegistryPath, PDISK_INFO DiskRegInfo);	//为磁盘映像分配内存
NTSTATUS RamDiskFormatDisk(PDEVICE_EXTENSION devExt);	// 格式化虚拟磁盘
BOOLEAN RamDiskCheckParameters(PDEVICE_EXTENSION devExt, LARGE_INTEGER ByteOffset, size_t Length);

VOID RamDiskEvtIoRead(WDFQUEUE Queue, WDFREQUEST Request, size_t Length);
VOID RamDiskEvtIoWrite(WDFQUEUE Queue, WDFREQUEST Request, size_t Length);
VOID RamDiskEvtIoDeviceControl(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request, 
	IN size_t OutputBufferLength, 
	IN size_t InputBufferLength, 
	IN ULONG IoControlCode);
/*++
描述:
	当收到IRP_MJ_DEVICE_CONTROL请求时调用此回调函数
参数:
	Queue - 关联的队列对象句柄,带有I/O请求.
	Request - 关联队列对象句柄
	OutputBufferLength - 请求的输出缓冲区长度,如果输出缓冲区可用的话
	InputBufferLength - 请求的输入缓冲区长度,如果输入缓冲区可用的话
	IoControlCode - 自定义或系统定义的I/O控制码
返回值:
	VOID
--*/


#endif // !_DISK_DIRVER_H
