// 键盘过滤，通过往设备栈中添加新设备的方式过滤数据
#ifndef _CTRL2CAP_H
#define _CTRL2CAP_H

#include <ntddk.h>

#define KEY_MAKE 0
#define KEY_BREAK 1
#define KEY_E0 2
#define KEY_E1 4
#define KEY_TERMSRV_SET_LED 8
#define KEY_TERMSRV_SHADOW 0x10
#define KEY_TERMSRV_VKPACKET 0x20



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

typedef struct _KEYBOARD_INPUT_DATA
{
    USHORT UnitId;    // 对于\\DEVICE\\KeyboardPort0这个值是0,\\DEVICE\\KeyboardPort1这个值是1,依此类推
    USHORT MakeCode;    // 扫描码
    USHORT Flags;        // 标志,标志一个键按下还是弹起
    USHORT Reserved;        // 保留
    ULONG ExtraInformation; // 扩展信息
}KEYBOARD_INPUT_DATA, * PKEYBOARD_INPUT_DATA;

// 通过一个名字来获得一个对象得指针(闭源函数,文档没有公开,声明直接使用了)
// 调用ObReferenceObjectByName获得对象引用会使驱动对象的引用计数增加,ObDereferenceObject释放驱动对象引用
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectName, ULONG Attribuites, PACCESS_STATE AccessState, ACCESS_MASK DesiredAccess, 
	POBJECT_TYPE ObjectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);


/* 入口函数 */
NTSTATUS Ctrl2CapMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
NTSTATUS Ctrl2CapUnload(PDRIVER_OBJECT DriverObject);

// IRP分发函数
NTSTATUS Ctrl2CapDispatchGeneral(PDEVICE_OBJECT DeviceObject, PIRP Irp);
// 电源管理IRP
NTSTATUS Ctrl2CapDispatchPower(PDEVICE_OBJECT DeviceObject, PIRP Irp);
// 设备PNP操作
NTSTATUS Ctrl2CapDispatchPnP(PDEVICE_OBJECT DeviceObject, PIRP Irp);
// 设备读取操作
NTSTATUS Ctrl2CapDispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// 读操作IRP完成回调函数
NTSTATUS Ctrl2CapReadComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

/* 打开驱动对象 KbdClass 绑定它下面所有的设备*/
NTSTATUS Ctrl2CapAttachDevices(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
// 初始化过滤设备 设备扩展信息
NTSTATUS Ctrl2CapFilterDevExtInit(PC2P_DEV_EXT devExt, PDEVICE_OBJECT pFileteDeviceObject, PDEVICE_OBJECT pTargetDeviceObject, PDEVICE_OBJECT pLowerDeviceObject);

NTSTATUS Ctrl2DetachDevices(PDEVICE_OBJECT DeviceObject);

/* 解析键盘操作码 */
NTSTATUS Ctrl2CapDataAnalysis(PIRP Irp);

int __stdcall  MakeCodeToAscii(UCHAR sch, const int Kb_Status);		// 将键盘扫描码转换成ASCII码
#endif // !_CTRL2CAP_H
