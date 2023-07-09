#ifndef _DEMO_H
#define _DEMO_H

#include <ntddk.h>

void DemoMain();

void StringOperationSample(void);	/*	字符串 操作*/
void ListOperationSample(void);		/* 内核系统链表结构操作 */
void SpinLockOperationSample(void);	/* 内核自旋锁操作 */
void MemoryOperationSample(void);	/* 内核内存池操作*/
BOOLEAN EventOperationSample();		/* 内核event对象*/
BOOLEAN RegistryKeyOperationSample();	// 内核操作注册表


#endif // !_DEMO_H
