#ifndef _DEMO_H
#define _DEMO_H

#include <ntddk.h>

void DemoMain();

void StringOperationSample(void);	/*	�ַ��� ����*/
void ListOperationSample(void);		/* �ں�ϵͳ����ṹ���� */
void SpinLockOperationSample(void);	/* �ں����������� */
void MemoryOperationSample(void);	/* �ں��ڴ�ز���*/
BOOLEAN EventOperationSample();		/* �ں�event����*/
BOOLEAN RegistryKeyOperationSample();	// �ں˲���ע���


#endif // !_DEMO_H
