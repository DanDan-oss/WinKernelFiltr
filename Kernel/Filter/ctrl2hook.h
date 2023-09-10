// ���̹��ˣ�ͨ��HOOK������������ķַ�����ָ�����

#ifndef _CTRL2HOOK_H
#define _CTRL2HOOK_H

#include <ntddk.h>

// ͨ��һ�����������һ�������ָ��(��Դ����,�ĵ�û�й���,����ֱ��ʹ����)
// ����ObReferenceObjectByName��ö������û�ʹ������������ü�������,ObDereferenceObject�ͷ�������������
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectName, ULONG Attribuites, PACCESS_STATE AccessState, ACCESS_MASK DesiredAccess,
	POBJECT_TYPE ObjectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);

extern POBJECT_TYPE* IoDriverObjectType;	/* �ں�ȫ�ֱ���, ��������ֱ��ʹ��. ͨ�ü����������������*/

/* ��ں��� */
NTSTATUS Ctrl2HookMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
NTSTATUS Ctrl2HookUnload(PDRIVER_OBJECT DriverObject);

// IRP�ַ�����
NTSTATUS Ctrl2FilterHookDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);


NTSTATUS Ctrl2IrpHook(PDRIVER_OBJECT DriverObject);		// HOOK ����������KdbClass IRP�ֺ���
NTSTATUS Ctrl2IrpUnHook(PDRIVER_OBJECT DriverObject);		// HOOK ����������KdbClass IRP�ֺ���


#endif // !_CTRL2HOOK_H