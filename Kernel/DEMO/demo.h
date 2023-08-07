#ifndef _DEMO_H
#define _DEMO_H

#include <ntddk.h>

#define CWK_COD_SYB_NAME L"\\??\\slbkcdo_1187480520"
#define CWK_COD_DEVICE_NAME L"\\Device\\slbkcdo_1187480520"

#define CWK_DVC_SEND_STR (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x911, METHOD_BUFFERED, FILE_WRITE_DATA)
#define CWK_DVC_RECV_STR (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x912, METHOD_BUFFERED, FILE_READ_DATA)

void DemoMain();

NTSTATUS CopyFile(PUNICODE_STRING TargerPath, PUNICODE_STRING SourcePath);	// 两个文件互相拷贝


void StringOperationSample(void);	/*	字符串 操作*/
void ListOperationSample(void);		/* 内核系统链表结构操作 */
void SpinLockOperationSample(void);	/* 内核自旋锁操作 */
void MemoryOperationSample(void);	/* 内核内存池操作*/
BOOLEAN EventOperationSample();		/* 内核event对象*/
BOOLEAN RegistryKeyOperationSample();	// 内核操作注册表
BOOLEAN FileOperationSample();		/* 文件操作*/
BOOLEAN SyetemThreadSample();	/* 线程操作 */

BOOLEAN DeviceControlSample();	/* 应用层和驱动层通信*/
NTSTATUS cwkDispatchDemo(PDEVICE_OBJECT DeviceObject, PIRP Irp);	// 请求分发函数



#endif // !_DEMO_H
