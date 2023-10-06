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
	ULONG DiskSize;				// ���̴�С,���ֽڼ���,�������ֻ��4GB
	ULONG RootDirEntries;		// ���̸��ļ�ϵͳ�Ľ���ڵ�
	ULONG SectorsPerCluster;	// ����ÿ�����ɶ��ٸ��������
	UNICODE_STRING DriveLetter;	// �����̷�
}DISK_INFO, *PDISK_INFO;

typedef struct _DEVICE_EXTENSION 
{
	PUCHAR DiskImage;			// ָ���ڴ�����,�ڴ���ʵ���������ݴ洢�ռ�
	DISK_GEOMETRY DiskGeometry;	// �洢�ڴ��̵Ĵ���Geometry����
	DISK_INFO DiskRegInfo;		// ������Ϣ�ṹ,��װʱ�����ע�����
	UNICODE_STRING SymbolicLink;	// �̵ķ�������
	WCHAR DriverLetterBuffer[DRIVE_LETTER_BUFFER_SIZE];		// DiskRegInfo��DriverLetter�洢�ռ�,�û���ע�����ָ�����̷�
	WCHAR DosDeviceNameBuffer[DOS_DEVNAME_BUFFER_SIZE];		// �������ӵĴ洢�ռ�
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, DeviceGetExtension)

typedef struct _QUEUE_EXTENSION {
	PDEVICE_EXTENSION DeviceExtension;
} QUEUE_EXTENSION, * PQUEUE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_EXTENSION, QueueGetExtension)

#pragma pack(1)
typedef struct _BOOT_SECTOR
{
	UCHAR bsJump[3];		// ��תָ��,��ת��DBR��������
	CCHAR bsOemNamep[8];	// �����OEM����
	USHORT bsBytesPerSec;	// ÿ�������ɶ����ֽ�
	UCHAR bsSecPerClus;		// ÿ�����ж��ٸ�����
	USHORT bsResSectors;	// ����������Ŀ,ָ���ǵ�һ��FAT��ʼ֮ǰ��������,����DBR����
	UCHAR bsFATs;			// ������ɶ��ٸ�FAT��
	USHORT bsRootDirEnts;	// �����ĸ�Ŀ¼��ڵ��м���
	USHORT bsSectors;		// �����һ���ж��ٸ�����,�Դ���65535�������ľ�,�ֶ�Ϊ0
	UCHAR bsMedia;			// �����Ľ�������
	USHORT bsFATsecs;		// ÿ��FAT��ռ�ö��ٸ�����
	USHORT bsSecPerTrack;	// ÿ���ŵ��ж��ٸ�����
	USHORT bsHeads;			// �ж��ٴ�ͷ,Ҳ����ÿ������Ĵŵ���
	ULONG bsHiddenSecs;		// �ж�����������
	ULONG bsHugeSectors;	// һ������65535������,��ʹ������ֶ���˵������������
	UCHAR bsDriveBumber;	// ���������
	UCHAR bsReserved1;		// �����ֶ�
	UCHAR bsBootSignature;	// ������չ��������ǩ,WindowsҪ�������ǩΪ0x28����0x29
	ULONG bsVolumeID;		// ���̾�ID
	CCHAR bsLable[11];		// ���̱��
	CCHAR bsFileSystemType[8];	// �������ļ�ϵͳ����
	CCHAR bsReserved2[448];		// �����ֶ�
	UCHAR bsSig2[2];		// DBR������ǩ
}BOOT_SECTOR, *PBOOT_SECTOR;

typedef struct _DIR_ENTRY
{
	UCHAR deNmae[8];		// �ļ���
	UCHAR deExtension[3];	// �ļ���չ��
	UCHAR deAttributes;		// �ļ�����
	UCHAR deReserved;		// ϵͳ����
	USHORT deTime;			// �ļ�����ʱ��
	USHORT deDate;			// �ļ���������
	USHORT deStartCluster;	// �ļ���һ���صı��
	ULONG deFileSize;		// �ļ���С
}DIR_ENTRY, *PDIR_ENTRY;
#pragma pack()

NTSTATUS RamDiskEvtDeviceAdd(WDFDRIVER DriverObject, PWDFDEVICE_INIT DeviceInit);	// �����������豸����ʼ��
VOID RamDiskEvtDeviceContextCleanup(IN WDFDEVICE Device);							// ���������������
VOID RamDiskQueryDiskRegParameters(PWCHAR RegistryPath, PDISK_INFO DiskRegInfo);	//Ϊ����ӳ������ڴ�
NTSTATUS RamDiskFormatDisk(PDEVICE_EXTENSION devExt);	// ��ʽ���������
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
����:
	���յ�IRP_MJ_DEVICE_CONTROL����ʱ���ô˻ص�����
����:
	Queue - �����Ķ��ж�����,����I/O����.
	Request - �������ж�����
	OutputBufferLength - ������������������,���������������õĻ�
	InputBufferLength - ��������뻺��������,������뻺�������õĻ�
	IoControlCode - �Զ����ϵͳ�����I/O������
����ֵ:
	VOID
--*/


#endif // !_DISK_DIRVER_H
