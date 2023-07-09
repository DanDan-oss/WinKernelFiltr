#ifndef _DEMO_H
#define _DEMO_H

#include <ntddk.h> 
void DemoMain();

void StringOperate(void);	/*	字符串 操作*/
void ListOperate(void);		/* 内核系统链表结构操作 */
void SpinLockOperate(void);	/* 内核自旋锁操作 */
void MemoryOperate(void);	/* 内核内存池操作*/
BOOLEAN EventOperationSample();	/* 内核event对象*/


#endif // !_DEMO_H
