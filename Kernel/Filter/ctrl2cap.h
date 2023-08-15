// 键盘过滤
#ifndef _CTRL2CAP_H
#define _CTRL2CAP_H

#include <ntddk.h>

#define KBD_DRIVER_NAME    L"\\Driver\\Kbdclass"            // 键盘类驱动
#define USBKBD_DRIVER_NAME L"\\Driver\\Kbdhid"            // USB键盘 端口驱动
#define PS2KBD_DRIVER_NAME L"\\Driver\\i8042prt"        // PS/2键盘 端口驱动

/* 自定义设备扩展*/
typedef struct _C2P_DEV_EXT
{
	ULONG NodeSize;                            // 此结构的大小
	KSPIN_LOCK IoRequestsSpinLock;            // 同时调用保护锁
	KEVENT IoInProgressEvent;                // 进程间同步处理
	PDEVICE_OBJECT FilterDeviceObject;        // 生成的过滤设备对象
	PDEVICE_OBJECT TargetDeviceObject;        // 成功被绑定的设备对象指针
	PDEVICE_OBJECT LowerDeviceObject;        // 获取键盘驱动得到的设备对象
	// TargetDeviceObject与TargetDeviceObject是同一个指针,一个是通过获取驱动对象时,指向驱动的设备得到的,一个是绑定设备时获取的
}C2P_DEV_EXT, * PC2P_DEV_EXT;

// 通过一个名字来获得一个对象得指针(闭源函数,文档没有公开,声明直接使用了)
// 调用ObReferenceObjectByName获得对象引用会使驱动对象的引用计数增加,ObDereferenceObject释放驱动对象引用
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectName, ULONG Attribuites, PACCESS_STATE AccessState, ACCESS_MASK DesiredAccess, 
	POBJECT_TYPE ObjectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);

/* 入口函数 */
NTSTATUS Ctrl2CaoMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);


/* 打开驱动对象 KbdClass 绑定它下面所有的设备*/
NTSTATUS c2pAttachDevices(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

// 初始化过滤设备 设备扩展信息
NTSTATUS c2pFilterDevExtInit(PC2P_DEV_EXT devExt, PDEVICE_OBJECT pFileteDeviceObject, PDEVICE_OBJECT pTargetDeviceObject, PDEVICE_OBJECT pLowerDeviceObject);


#endif // !_CTRL2CAP_H
