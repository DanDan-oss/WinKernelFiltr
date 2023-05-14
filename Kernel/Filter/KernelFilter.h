#ifndef _KERNEL_FILTR_H
#define _KERNEL_FILTR_H

#include <ntddk.h>
#include <wdm.h>

#define KBD_DRIVER_NAME    L"\\Driver\\Kbdclass"            // 键盘类驱动
#define USBKBD_DRIVER_NAME L"\\Driver\\Kbdhid"            // USB键盘 端口驱动
#define PS2KBD_DRIVER_NAME L"\\Driver\\i8042prt"        // PS/2键盘 端口驱动


// 键盘按下Shift CapsLock  NumLock标志
#define K_SHFIT 1
#define K_CAPS    2
#define K_NUM    4

extern BOOLEAN IsAttachDevUp;                    // 是否添加新设备到键盘设备栈栈顶
extern BOOLEAN IsHookKbdIRP;                   // 是否HOOK Kbdclass IRP例程

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
}C2P_DEV_EXT, *PC2P_DEV_EXT;

/* 键盘扫描码传递结构*/
typedef struct _KEYBOARD_INPUT_DATA
{
    USHORT UnitId;    // 对于\\DEVICE\\KeyboardPort0这个值是0,\\DEVICE\\KeyboardPort1这个值是1,依此类推
    USHORT MakeCode;    // 扫描码
    USHORT Flags;        // 标志,标志一个键按下还是弹起
    USHORT Reserved;        // 保留
    ULONG ExtraInformation; // 扩展信息
}KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

/* 键盘类驱动服务回调例程结构*/
typedef VOID(_stdcall* KeyBoardClassServiceCallback)(PDEVICE_OBJECT DeviceObject, PKEYBOARD_INPUT_DATA InputDataStart,
    PKEYBOARD_INPUT_DATA InputDataEnd, PULONG InputDataConsumed);


// 通过名字获得驱动对象指针函数声明，微软文档未公开,声明直接使用
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectName, ULONG Attributes, PACCESS_STATE AccessState,
    ACCESS_MASK DesiredAccess, POBJECT_TYPE ObjectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);


NTSTATUS cwkDispatch(PDEVICE_OBJECT dev, PIRP Irp);            // 主要驱动分发函数(不过滤处理的消息)
NTSTATUS c2pDispatchPower(PDEVICE_OBJECT dev, PIRP Irp);    //  电源操作分发函数
NTSTATUS c2pDispatchPNP(PDEVICE_OBJECT dev, PIRP Irp);        // 设备插拔
NTSTATUS c2pDispatchRead(PDEVICE_OBJECT dev, PIRP Irp);        // 驱动读分发函数
NTSTATUS c2pReadCompletion(PDEVICE_OBJECT dev, PIRP Irp, PVOID Context);    // 读操作的完成回调函数
void c2pDataAnalysis(PIRP Irp);                                // 获取的数据解析键扫描码
void __stdcall c2pKeyboardDown(UCHAR uch);                    // 当键盘按下,进行数据过滤
UCHAR __stdcall KeyboarFindAssic(UCHAR uch);                // 扫描码转字符串

// 获得键盘设备绑定键盘过滤
NTSTATUS c2pAttachDevices(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

// 初始化设备扩展
NTSTATUS c2pDevExtInit(IN PC2P_DEV_EXT devExt, IN PDEVICE_OBJECT pFilterDeviceObject, IN PDEVICE_OBJECT pTargetDeviceObject, IN PDEVICE_OBJECT pLowerDeviceObject);

VOID c2pUnload(IN PDRIVER_OBJECT DriverObject);    // 卸载驱动的过滤设备
VOID c2pDetach(IN PDEVICE_OBJECT DeviceObject);    // 过滤设备绑定并删除


/* Hook顶层键盘类驱动的IRP分发函数*/
NTSTATUS c2pSetHookIrp(PDRIVER_OBJECT DriverObject);
NTSTATUS c2pHookDispatch(PDEVICE_OBJECT dev, PIRP Irp);
NTSTATUS c2pHookReadDisp(PDEVICE_OBJECT dev, PIRP Irp);        // Hook消息分发函数
NTSTATUS c2pHookUnload(PDRIVER_OBJECT DriverObject);


/* HOOK键盘 端口驱动 */
NTSTATUS FindDriverObject(PDRIVER_OBJECT DriverObject);        // 获取中间层端口驱动并查找设备扩展里类驱动回调例程函数地址
BOOLEAN searchServiceCallback(PDEVICE_OBJECT DeviceObject, PVOID DeviceExt);        // 搜索保存在设备扩展里的类驱动设备队形和回调处理函数

#endif
