#ifndef _DEMO_H
#define _DEMO_H

#include <ntddk.h> 
void DemoMain();

void StringOperate(void);	/*	�ַ��� ����*/
void ListOperate(void);		/* �ں�ϵͳ����ṹ���� */
void SpinLockOperate(void);	/* �ں����������� */
void MemoryOperate(void);	/* �ں��ڴ�ز���*/
BOOLEAN EventOperationSample();	/* �ں�event����*/


#endif // !_DEMO_H
