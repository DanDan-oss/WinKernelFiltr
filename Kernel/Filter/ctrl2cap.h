// ���̹���
#ifndef _CTRL2CAP_H
#define _CTRL2CAP_H

#include <ntddk.h>

#define KBD_DRIVER_NAME    L"\\Driver\\Kbdclass"            // ����������
#define USBKBD_DRIVER_NAME L"\\Driver\\Kbdhid"            // USB���� �˿�����
#define PS2KBD_DRIVER_NAME L"\\Driver\\i8042prt"        // PS/2���� �˿�����


/* �Զ����豸��չ*/
typedef struct _C2P_DEV_EXT
{
	ULONG NodeSize;                            // �˽ṹ�Ĵ�С
	KSPIN_LOCK IoRequestsSpinLock;            // ͬʱ���ñ�����
	KEVENT IoInProgressEvent;                // ���̼�ͬ������
	PDEVICE_OBJECT FilterDeviceObject;        // ���ɵĹ����豸����
	PDEVICE_OBJECT TargetDeviceObject;        // �ɹ����󶨵��豸����ָ��
	PDEVICE_OBJECT LowerDeviceObject;        // ��ȡ���������õ����豸����
	// TargetDeviceObject��TargetDeviceObject��ͬһ��ָ��,һ����ͨ����ȡ��������ʱ,ָ���������豸�õ���,һ���ǰ��豸ʱ��ȡ��
}C2P_DEV_EXT, * PC2P_DEV_EXT;

typedef struct _KEYBOARD_INPUT_DATA
{
    USHORT UnitId;    // ����\\DEVICE\\KeyboardPort0���ֵ��0,\\DEVICE\\KeyboardPort1���ֵ��1,��������
    USHORT MakeCode;    // ɨ����
    USHORT Flags;        // ��־,��־һ�������»��ǵ���
    USHORT Reserved;        // ����
    ULONG ExtraInformation; // ��չ��Ϣ
}KEYBOARD_INPUT_DATA, * PKEYBOARD_INPUT_DATA;

// ͨ��һ�����������һ�������ָ��(��Դ����,�ĵ�û�й���,����ֱ��ʹ����)
// ����ObReferenceObjectByName��ö������û�ʹ������������ü�������,ObDereferenceObject�ͷ�������������
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectName, ULONG Attribuites, PACCESS_STATE AccessState, ACCESS_MASK DesiredAccess, 
	POBJECT_TYPE ObjectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);


/* ��ں��� */
NTSTATUS Ctrl2CapMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
NTSTATUS Ctrl2CapUnload(PDRIVER_OBJECT DriverObject);

// IRP�ַ�����
NTSTATUS Ctrl2CapDispatchGeneral(PDEVICE_OBJECT DeviceObject, PIRP Irp);
// ��Դ����IRP
NTSTATUS Ctrl2CapDispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp);
// �豸PNP����
NTSTATUS Ctrl2CapDispatchPnP(PDEVICE_OBJECT DeviceObject, PIRP Irp);
// �豸��ȡ����
NTSTATUS Ctrl2CapDispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// ������IRP��ɻص�����
NTSTATUS Ctrl2CapReadComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

/* ���������� KbdClass �����������е��豸*/
NTSTATUS Ctrl2CapAttachDevices(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
// ��ʼ�������豸 �豸��չ��Ϣ
NTSTATUS Ctrl2CapFilterDevExtInit(PC2P_DEV_EXT devExt, PDEVICE_OBJECT pFileteDeviceObject, PDEVICE_OBJECT pTargetDeviceObject, PDEVICE_OBJECT pLowerDeviceObject);

NTSTATUS Ctrl2DetachDevices(PDEVICE_OBJECT DeviceObject);

/* �������̲����� */
NTSTATUS Ctrl2CapDataAnalysis(PIRP Irp);


#endif // !_CTRL2CAP_H
