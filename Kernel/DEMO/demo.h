#ifndef _DEMO_H
#define _DEMO_H

#include <ntddk.h>

#define CWK_COD_SYB_NAME L"\\??\\slbkcdo_1187480520"
#define CWK_COD_DEVICE_NAME L"\\Device\\slbkcdo_1187480520"

#define CWK_DVC_SEND_STR (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x911, METHOD_BUFFERED, FILE_WRITE_DATA)
#define CWK_DVC_RECV_STR (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x912, METHOD_BUFFERED, FILE_READ_DATA)

void DemoMain();

NTSTATUS CopyFile(PUNICODE_STRING TargerPath, PUNICODE_STRING SourcePath);	// �����ļ����࿽��


void StringOperationSample(void);	/*	�ַ��� ����*/
void ListOperationSample(void);		/* �ں�ϵͳ����ṹ���� */
void SpinLockOperationSample(void);	/* �ں����������� */
void MemoryOperationSample(void);	/* �ں��ڴ�ز���*/
BOOLEAN EventOperationSample();		/* �ں�event����*/
BOOLEAN RegistryKeyOperationSample();	// �ں˲���ע���
BOOLEAN FileOperationSample();		/* �ļ�����*/
BOOLEAN SyetemThreadSample();	/* �̲߳��� */

BOOLEAN DeviceControlSample();	/* Ӧ�ò��������ͨ��*/
NTSTATUS cwkDispatchDemo(PDEVICE_OBJECT DeviceObject, PIRP Irp);	// ����ַ�����



#endif // !_DEMO_H
