#ifndef _DP_DIRVER_H
#define _DP_DIRVER_H

#include <ntddk.h>
#include <wdf.h>

typedef  unsigned char BYTE;
typedef  unsigned short int WORD;
typedef  unsigned long int DWORD;
typedef  unsigned long long int QWORD;
typedef  unsigned char tBitmap;


typedef struct _DP_BITMAP
{
	DWORD sectorSize;	// ����ÿ�������ж����ֽ�,ͬʱ˵����bitmap��һ��λ��Ӧ���ֽ���
	DWORD byteSize;		// ÿ���ֽ��м�λ,Ĭ���������8
	DWORD regionSize;	// ÿ�������ֽ�
	DWORD regionNumber;	// ���bitmap�ܹ��ж��ٿ�
	DWORD regionReferSize;	// ���Ӧ��ʵ���ֽ�, ���ֵӦ���� sectorSize * byteSize * regionSize
	QWORD bitmapReferSize;	// ���bitmap��Ӧ��ʵ���ֽ���, ���ֵӦ���� sectorSize * byteSize * regionSize * regionNumber
	tBitmap** BitMap;		// ָ��bitmap�ռ�
	PVOID lockBitmap;
}DP_BITMAP, *PDP_BITMAP;

typedef struct _DP_FILTER_DEV_EXTENSION
{
	WCHAR VolumeLetter;					// ������� "C: , D:"
	BYTE Protect;					// ���Ƿ��ڱ���״̬
	LARGE_INTEGER TotalSizeInByte;		// ���С,��λbyte
	DWORD ClusterSizeInByte;			// �����ļ�ϵͳ��ÿ�ش�С
	DWORD SectorSizeInByte;				// ��ÿ�������Ĵ�С
	BOOLEAN InitializeCompleted;		// ��־λ,����ṹ�Ƿ��ʼ�����

	PDEVICE_OBJECT FilterDeviceObject;	// ���豸��Ӧ�����豸���豸����
	PDEVICE_OBJECT LowerDeviceObject;	// ���豸��Ӧ�Ĺ����豸���²��豸����
	PDEVICE_OBJECT PhyDeviceObject;		// ���豸��Ӧ�������豸����

	HANDLE TempFile;				// ����ת�����ļ����
	PDP_BITMAP Bitmap;				// ���ϱ���ϵͳʹ�õ�ͼλ���
	LIST_ENTRY RequestList;			// ���ϱ���ϵͳʹ�õ��������
	KSPIN_LOCK RequestLock;			// ���ϱ���ϵͳʹ�õ�������е���
	KEVENT RequestEvent;			// ���ϱ���ϵͳʹ�õ�������е�ͬ���¼�
	PVOID ThreadHandle;				// ���ϱ���ϵͳʹ�õ�������д����̵߳��߳̾��
	BOOLEAN ThreadTermFlag;			// ���ϱ���ϵͳʹ�õ�������еĴ����߳̽�����־
	KEVENT PagingPathCountEvent;	// ���ϱ���ϵͳ�ػ���ҳ��Դ����ļ����¼�
	LONG PagingPathCount;			// ���ϱ���ϵͳ�ػ���ҳ��Դ����ļ���
}DP_FILTER_DEV_EXTENSION, *PDP_FILTER_DEV_EXTENSION;

typedef struct _VOLUME_ONLINE_CONTEXT
{
	PDP_FILTER_DEV_EXTENSION DevExt;	//��volume_online��DeviceIoControl�д�����ɺ������豸��չ
	PKEVENT Event;						//��volume_online��DeviceIoControl�д�����ɺ�����ͬ���¼�
}VOLUME_ONLINE_CONTEXT, *PVOLUME_ONLINE_CONTEXT;

NTSTATUS NTAPI DPAddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject);

NTSTATUS NTAPI DPDispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI DPDispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI DPDispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);


NTSTATUS DPVolumeOnlineCompleteRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, _In_reads_opt_(_Inexpressible_("varies")) PVOID Context);
#endif // !_DISK_DIRVER_H

