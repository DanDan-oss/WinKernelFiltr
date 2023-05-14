#include <immintrin.h>
#include "KernelFilter.h"

//  自定义键盘扫描码的ASCII字符数组
//  主键键盘数字1-10 字母小写字符(qwertyuiop)
char UnShift[] = {
    0, 0, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x2d, 0x3d, 0, 0,
    0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x6C, 0x6F, 0x70 };

//  上键盘符号 字母大写(QWERTYUIOP)
char IsShfit[] = {
    0,0,0x21,0x40, 0x23, 0x24, 0x25, 0x5e, 0x26, 0x2a, 0x28, 0x29, 0x5f, 0x2b, 0, 0,
    0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4f, 0x50 };

extern POBJECT_TYPE* IoDriverObjectType;        // 通过键盘驱动对象的类型

static int Kb_Status = K_NUM;                // 键盘键位按下标志
static int downKeyNum = 5;                    // HOOK按键驱动KbdclassIRP处理消息的次数
ULONG gC2pKeyCount = 0;                            // IRP请求计数器

BOOLEAN IsAttachDevUp = 0;                    // 是否添加新设备到键盘设备栈栈顶
BOOLEAN IsHookKbdIRP = 0;                    // 是否HOOK Kbdclass IRP例程

PDRIVER_DISPATCH oldDriverDispatch[IRP_MJ_MAXIMUM_FUNCTION + 1] = {0};

/* 自定义键盘 类驱动设备指针和回调函数例程结构*/
typedef struct _KBD_CALLBACK
{
    PDEVICE_OBJECT DeviceObject;
    KeyBoardClassServiceCallback ServiceCallback;
} KBD_CALLBACK, * PKBD_CALLBACK;

KBD_CALLBACK  gKbdCallBack={ 0 };            // 设备对象指针和类驱动回调函数地址


// 获得键盘设备 生成虚拟设备过滤键盘
NTSTATUS c2pAttachDevices(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
    UNICODE_STRING UniNtNameString = { 0 };
    PC2P_DEV_EXT devExt;                            /* 自定义设备扩展结构 指针*/
    PDEVICE_OBJECT pFilterDeviceObject = NULL;        // 生成过滤设备对象指针
    PDEVICE_OBJECT pTargetDeviceObject = NULL;        // 绑定成功后的真实设备对象指针,函数返回赋值为0表示绑定失败
    PDEVICE_OBJECT pLowerDeviceObject = NULL;        // 键盘驱动设备栈的设备对象
    PDRIVER_OBJECT KbdDriverObject = NULL;            // 键盘驱动对象
    ULONG iNum = 0;                                    /* 设备栈绑定设备对象数量*/

    //TODO: 通过名字获取驱动对象指针, 获取完毕释放对象,保存指针
    RtlInitUnicodeString(&UniNtNameString, KBD_DRIVER_NAME);

    nStatus = ObReferenceObjectByName(&UniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType,
        KernelMode, NULL, &KbdDriverObject);

    // 判断获取驱动对象是否成功
    if (!NT_SUCCESS(nStatus))
    {
        DbgPrint("[Message]: Could't get the MyTest Device Object, ErrorCode:%x\n", nStatus);
        return nStatus;
    }
    DbgPrint(("[Message]: Get the MyTest Device Object OK\n"));
    pTargetDeviceObject = KbdDriverObject->DeviceObject;

     // 调用ObReferenceObjectByName会使驱动对象的引用计数增加,释放驱动对象引用
    ObDereferenceObject(KbdDriverObject);

    while (pTargetDeviceObject)
    {
        //TODO: 生成过滤设备对象并绑定
        // 生成过滤设备
        nStatus = IoCreateDevice(DriverObject, sizeof(C2P_DEV_EXT), NULL, pTargetDeviceObject->DeviceType,
            pTargetDeviceObject->Characteristics, FALSE, &pFilterDeviceObject);
        if (!NT_SUCCESS(nStatus))
        {
            DbgPrint("[Message]: Could't Create the New MyFilter Device Object, ErrorCode:%x\n", nStatus);
            return nStatus;
        }

        // 绑定设备过滤设备
        pLowerDeviceObject = IoAttachDeviceToDeviceStack(pFilterDeviceObject, pTargetDeviceObject);
        if (!pLowerDeviceObject)
        {
            DbgPrint(("[Message]: Could't attach to MyTest Device Object\n"));
            IoDeleteDevice(pFilterDeviceObject);
            pFilterDeviceObject = NULL;
            return (nStatus);
        }

        //TODO:  初始化设备对象扩展(生成设备对象时指定的大小)
        devExt = (PC2P_DEV_EXT)(pFilterDeviceObject->DeviceExtension);
        c2pDevExtInit(devExt, pFilterDeviceObject, pTargetDeviceObject, pLowerDeviceObject);

        DbgPrint("[Message]: DeviceObject=[%p] ,pFilterDevObj=[%p], pTargetDevObj=[%p], pLowerDevObj=[%p]\n", pFilterDeviceObject,
            devExt->FilterDeviceObject, pTargetDeviceObject, pLowerDeviceObject);

        //TODO:  拷贝复制重要标志和属性
        pFilterDeviceObject->DeviceType = pLowerDeviceObject->DeviceType;
        pFilterDeviceObject->Characteristics = pLowerDeviceObject->Characteristics;
        pFilterDeviceObject->StackSize = pLowerDeviceObject->StackSize;
        pFilterDeviceObject->Flags |= pLowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
        DbgPrint("[Message]：The Device Object is Nunber %u", iNum++);
        pTargetDeviceObject = pTargetDeviceObject->NextDevice;
    }
    DbgPrint("[Message]：The Driver Object's have Device Object %u\n", iNum);

    IsAttachDevUp = 1;
    return nStatus;
}

NTSTATUS c2pDevExtInit(PC2P_DEV_EXT devExt, PDEVICE_OBJECT pFilterDeviceObject,
            PDEVICE_OBJECT pTargetDeviceObject, PDEVICE_OBJECT pLowerDeviceObject)
{
    //TODO: 重置PC2P_DEV_EXT设备结构
    memset(devExt, 0, sizeof(C2P_DEV_EXT));
    devExt->NodeSize = sizeof(C2P_DEV_EXT);

    //TODO: 初始化结构里的自旋锁
    KeInitializeSpinLock(&(devExt->IoRequestsSpinLock));
    KeInitializeEvent(&(devExt->IoInProgressEvent), NotificationEvent, FALSE);

    //TODO: 结构体 设备对象指针赋值
    devExt->FilterDeviceObject = pFilterDeviceObject;
    devExt->TargetDeviceObject = pTargetDeviceObject;
    devExt->LowerDeviceObject = pLowerDeviceObject;
    return STATUS_SUCCESS;
}

// 删除绑定的过滤设备
VOID c2pUnload(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT DeviceObject= NULL;
    PKTHREAD CurrentThread = NULL;            // 当前线程
    LARGE_INTEGER Interval = {0};            // 线程等待时间

    //TODO: 获取当前线程句柄并设置线程优先级为实时优先级
    CurrentThread = KeGetCurrentThread();
    KeSetPriorityThread(CurrentThread, LOW_REALTIME_PRIORITY);

    //TODO:    循环遍历本驱动设备栈所有设备,解除设备绑定并删除
    DeviceObject = DriverObject->DeviceObject;

    while (DeviceObject)
    {
        c2pDetach(DeviceObject);        // 解除设备绑定
        DeviceObject = DeviceObject->NextDevice;
    }
    ASSERT(DriverObject->DeviceObject == NULL);

    //TODO:    设置线程等待时间
    Interval.QuadPart = 500 * ((-10) * 1000);        // 500毫秒
    //Interval = RtlConvertLongToLargeInteger(500* ( (-10)*1000 ) );
    while (gC2pKeyCount)
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);

    DbgPrint("[Message]: The Device of this driver unloading OK!\n");
    return;
}

VOID c2pDetach(IN PDEVICE_OBJECT DeviceObject)
{
    PC2P_DEV_EXT devExt = NULL;
    //TODO: 获取设备对象的扩展

    devExt = DeviceObject->DeviceExtension;
    if (!devExt->FilterDeviceObject)
        return;

    //TODO: 解除设备扩展里面绑定的设备对象的绑定
    IoDetachDevice(devExt->TargetDeviceObject);
    devExt->FilterDeviceObject = NULL;
    devExt->TargetDeviceObject = NULL;
    devExt->LowerDeviceObject = NULL;
    IoDeleteDevice(DeviceObject);
    return;
}

// 主要驱动分发函数(所有不处理的IRP)
NTSTATUS cwkDispatch(PDEVICE_OBJECT dev, PIRP Irp)
{
    // TODO: 请求不处理, 将请求直接下发
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(((PC2P_DEV_EXT)(dev->DeviceExtension))->LowerDeviceObject, Irp);    // 将IRP发送给真实设备的驱动
}


// 电源操作例程
NTSTATUS c2pDispatchPower(PDEVICE_OBJECT dev, PIRP Irp)
{

    //UNREFERENCED_PARAMETER(dev);
    //PDRIVER_OBJECT KbdDriverObject = NULL;
    //UNICODE_STRING DirverName;
    //NTSTATUS nStatus = STATUS_UNSUCCESSFUL;

    //RtlInitUnicodeString(&DirverName, KBD_DRIVER_NAME);
    //nStatus = ObReferenceObjectByName(&DirverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType,
    //    KernelMode, NULL, &KbdDriverObject);
    //if (!NT_SUCCESS(nStatus))
    //{
    //    DbgPrint("[Message]: Could't get the MyTest Device Object, ErrorCode:%x\n", nStatus);
    //    return nStatus;
    //}
    //DbgPrint("[Message]: Try error code start\n");
    //KbdDriverObject->MajorFunction[IRP_MJ_POWER](KbdDriverObject->DeviceObject, Irp);
    //DbgPrint("[Message]: Try error code End\n");

    //ObDereferenceObject(KbdDriverObject);
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(((PC2P_DEV_EXT)(dev->DeviceExtension))->LowerDeviceObject, Irp);    // 替代函数IoCallDriver
}


NTSTATUS c2pDispatchPNP(PDEVICE_OBJECT dev, PIRP Irp)
{
    PIO_STACK_LOCATION Irpsp = NULL;
    PC2P_DEV_EXT devExt = NULL;
    NTSTATUS nStatus = STATUS_UNSUCCESSFUL;

    // TODO; 请求直接下发, 然后判断是否是硬件拔出,如果是硬件拔出解除设备绑定,删除生成的过滤设备,
    Irpsp = IoGetCurrentIrpStackLocation(Irp);
    devExt = dev->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    nStatus = IoCallDriver(devExt->LowerDeviceObject, Irp);
    if (Irpsp->MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        DbgPrint("[Messag]: The device IRP_MN_REMOVE_DEVICE !\n");
        c2pDetach(dev);
    }
    return nStatus;
}


NTSTATUS c2pDispatchRead(PDEVICE_OBJECT dev, PIRP Irp)
{
    //PIO_STACK_LOCATION Irpsp = NULL;
    //TODO: 判断当前IRP是否在IRP栈最底端,如果是 这是错误的请求,结束IRP传递, 返回IRP
    if (Irp->CurrentLocation == 1)
    {
        DbgPrint("[Message]: ->Error Dispatch encountered bogus current location\n");
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    //TODO: 不在IRP栈最底端,IRP计数器加1
    gC2pKeyCount++;
    //TODO: 获取被过滤设备指针, 拷贝当前IRP栈空间,设置完成例程发送给下层驱动
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, c2pReadCompletion, dev, TRUE, TRUE, TRUE);
    return IoCallDriver(((PC2P_DEV_EXT)(dev->DeviceExtension))->LowerDeviceObject, Irp);
}

// IRP完成回调函数
NTSTATUS c2pReadCompletion(PDEVICE_OBJECT    dev, PIRP Irp, PVOID Context)
{
    UNREFERENCED_PARAMETER(dev);
    UNREFERENCED_PARAMETER(Context);
    //TODO:    IRP计数器减1,
    gC2pKeyCount--;

    //TODO: 判断当前IRP请求是否执行成功,如果执行成功读取设备流过数据,否则数据没有意义
    if (NT_SUCCESS(Irp->IoStatus.Status))
        c2pDataAnalysis(Irp);

    // 判断IRP的调度标识是否时是挂起状态, 如果否,手动设置IRP调度挂起状态
    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    return Irp->IoStatus.Status;
}


// 获取的数据解析键扫描码
void c2pDataAnalysis(PIRP Irp)
{
    KEYBOARD_INPUT_DATA* pKeyData = NULL;                /* 设备流过数据*/
    LONGLONG bufLen = 0;            /* 数据长度*/
    ULONGLONG KeyBoardNum = 0;            // KEYBOARD_INPUT_DATA结构个数
    pKeyData = Irp->AssociatedIrp.SystemBuffer;
    bufLen = Irp->IoStatus.Information;
    KeyBoardNum = bufLen / sizeof(KEYBOARD_INPUT_DATA);
    for (int i = 0; i < KeyBoardNum; i++)
    {
        /*
        switch (pKeyData->Flags)
        {
        case 0:
            DbgPrint("[Message]: 键盘按下,ScanCode[%x]", pKeyData->MakeCode);
            break;
        case 1:
            DbgPrint("[Message]: 键盘弹起,ScanCode[%x]", pKeyData->MakeCode);
            break;
        default:
            DbgPrint("[Message]: 键盘操作,ScanCode[%x]", pKeyData->MakeCode);
            break;
        }

        //测试,这里将扫描码++操作, 证实键盘按键是可以拦截修改的
        pKeyData->MakeCode += 1;
        */

        // 键盘按下
        if (!pKeyData->Flags)
            c2pKeyboardDown((UCHAR)pKeyData->MakeCode);

        // Shfit键弹起的时候, 还原Shfit标志
        if(pKeyData->Flags && (pKeyData->MakeCode== 0x36|| pKeyData->MakeCode == 0x2a))
            Kb_Status ^= K_SHFIT;

        pKeyData++;
    }

    return;
}

void __stdcall c2pKeyboardDown(UCHAR uch)
{
    //如果uch小于十进制128,则结果等于,if表达式成立
    //if ((uch & 0x80) == 0)
    //{
    //    if (uch < 0x47 || (uch >= 0x47 && uch < 0x54 && Kb_Status & K_NUM))
    //        ch = 0;
    //}
    UCHAR ch = 0;
    switch (uch)
    {
    case 0x3a:        // 按下大小写键 CapsLock
        Kb_Status ^= K_CAPS;
        return;

    case 0x36:        // 右Sshfit键
    case 0x2a:        // 左Shfit键
        Kb_Status |= K_SHFIT;
        return;

    case 0x45:        // NumLock键
        Kb_Status ^= K_NUM;
        return;

    default:
        break;
    }

    ch = KeyboarFindAssic(uch);
    if(ch)
        DbgPrint("[Message]: 键盘按下-->ScanCode[%x], Data:[%c]\n", uch, ch);
}

UCHAR __stdcall KeyboarFindAssic(UCHAR uch)
{
    char ch = 0;
    // 根据uch的值为索引去寻找自定义数组中存储的对应的ASIIC码的值  键盘扫描码表和ASCII码表中所代表的字符在表中的索引不同

    if ((uch >= 0x2 && uch <= 0xd))
        (Kb_Status & K_SHFIT) ? (ch = IsShfit[uch]) : (ch = UnShift[uch]);

    else if (uch >= 0x10 && uch <= 0x19)
    {
        if (Kb_Status & K_SHFIT)    // 判断是否 开了大写键的同时按下Shift键
            (Kb_Status & K_CAPS) ? (ch = UnShift[uch]) : (ch = IsShfit[uch]);
        else
            (Kb_Status & K_CAPS) ? (ch = IsShfit[uch]) : (ch = UnShift[uch]);
    }

    return ch;
}

NTSTATUS c2pSetHookIrp(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    PDRIVER_OBJECT KbdDriverObject = NULL;
    UNICODE_STRING DirverName;
    NTSTATUS nStatus = STATUS_UNSUCCESSFUL;

    RtlInitUnicodeString(&DirverName, KBD_DRIVER_NAME);
    nStatus = ObReferenceObjectByName(&DirverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType,
        KernelMode, NULL, &KbdDriverObject);
    if (!NT_SUCCESS(nStatus))
    {
        DbgPrint("[Message]: Could't get the MyTest Device Object, ErrorCode:%x\n", nStatus);
        return nStatus;
    }
    DbgPrint("[Message]: Function c2pHookDispatch Address:%p KbdDriverObject[IRP_MJ_READ]=%p\n", c2pHookDispatch,
                                                    KbdDriverObject->MajorFunction[IRP_MJ_READ]);
    for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        oldDriverDispatch[i] = KbdDriverObject->MajorFunction[i];
        //KbdDriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)&c2pHookDispatch;
        InterlockedExchangePointer((PVOID*)&KbdDriverObject->MajorFunction[i], (PVOID)&c2pHookDispatch);
    }


    DbgPrint("[Message]: oldDispatchFunc:%p  newHookDispatch=%p\n", oldDriverDispatch[IRP_MJ_READ],
            KbdDriverObject->MajorFunction[IRP_MJ_READ]);

    ObDereferenceObject(KbdDriverObject);
    IsHookKbdIRP = 1;
    return STATUS_SUCCESS;
}

NTSTATUS  c2pHookDispatch(PDEVICE_OBJECT dev, PIRP Irp)
{

    ULONG IrpType = 0;            // IRP消息宏
    PIO_STACK_LOCATION isIrp = IoGetCurrentIrpStackLocation(Irp);

    IrpType = isIrp->MajorFunction;
    DbgPrint("[Message]: Hook Kbdclass Get Data Start -------------\n");
    if (IrpType == IRP_MJ_READ && NT_SUCCESS(Irp->IoStatus.Status))
        c2pHookReadDisp(dev, Irp);

    oldDriverDispatch[IrpType](dev, Irp);

    if (IrpType == IRP_MJ_READ && NT_SUCCESS(Irp->IoStatus.Status))
        c2pHookReadDisp(dev, Irp);

    DbgPrint("[Message]: -------------Hook Kbdclass Get Data End\n");
    downKeyNum--;
    return Irp->IoStatus.Status;
}

NTSTATUS  c2pHookReadDisp(PDEVICE_OBJECT dev, PIRP Irp)        // Hook读消息例程
{
    UNREFERENCED_PARAMETER(dev);
    PKEYBOARD_INPUT_DATA pBuf = NULL;
    ULONGLONG buLen = 0;
    ULONGLONG boardNum = 0;

    pBuf = Irp->AssociatedIrp.SystemBuffer;
    buLen = Irp->IoStatus.Information;
    boardNum = buLen / sizeof(C2P_DEV_EXT);
    DbgPrint("[Message]: Hook Kbdclass , Buffer Size:%8x\n", buLen);
    for (int i = 0; i < boardNum; i++)
    {
        // 键盘按下
        if (!pBuf->Flags)
            c2pKeyboardDown((UCHAR)pBuf->MakeCode);

        // Shfit键弹起的时候, 还原Shfit标志
        if (pBuf->Flags && (pBuf->MakeCode == 0x36 || pBuf->MakeCode == 0x2a))
            Kb_Status ^= K_SHFIT;

        pBuf++;
    }

    return Irp->IoStatus.Status;
}

NTSTATUS c2pHookUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    PDRIVER_OBJECT KbdDriverObject = NULL;
    UNICODE_STRING DirverName;
    NTSTATUS nStatus = STATUS_UNSUCCESSFUL;

    RtlInitUnicodeString(&DirverName, KBD_DRIVER_NAME);
    nStatus = ObReferenceObjectByName(&DirverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType,
        KernelMode, NULL, &KbdDriverObject);
    if (!NT_SUCCESS(nStatus))
    {
        DbgPrint("[Message]:HookUnload Could't get the MyTest Device Object, ErrorCode:%x\n", nStatus);
        return nStatus;
    }

    for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
        KbdDriverObject->MajorFunction[i] = oldDriverDispatch[i];

    DbgPrint("[Message]: Unload Kbdclass Hook OK!\n");
    ObDereferenceObject(KbdDriverObject);
    return STATUS_SUCCESS;
}

// 获取中间层端口驱动并查找设备扩展里类驱动回调例程函数地址
NTSTATUS FindDriverObject(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    NTSTATUS nStatus = STATUS_SUCCESS;
    UNICODE_STRING DirverNameString = { 0 };
    PDRIVER_OBJECT KbdhidDriverObject = NULL;        // USB键盘驱动对象指针
    PDRIVER_OBJECT i8042DirverObject = NULL;        // PS2键盘驱动对象指针
    PDRIVER_OBJECT usingDriverObject = NULL;    // 保存获取成功的驱动对象指针
    PDEVICE_OBJECT usingDeviceObject = NULL;    // 保存驱动对象的设备指针
    PVOID DeviceExt = NULL;                        // 指向设备对象扩展

    // TODO: 获取USB键盘和PS/2键盘的驱动对象
        // 获取USB键盘端口驱动
    RtlInitUnicodeString(&DirverNameString, USBKBD_DRIVER_NAME);
    nStatus = ObReferenceObjectByName(&DirverNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType,
        KernelMode, NULL, &KbdhidDriverObject);
    if (NT_SUCCESS(nStatus))
    {
        ObDereferenceObject(KbdhidDriverObject);
        DbgPrint("[Message]: Get USB Driver Object OK!\n");
    }else
        DbgPrint("[Message]: Get USB Driver Object fail ErrorCode=%X!\n", nStatus);

        // 获取PS/2键盘端口驱动
    RtlInitUnicodeString(&DirverNameString, PS2KBD_DRIVER_NAME);
    nStatus = ObReferenceObjectByName(&DirverNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType,
        KernelMode, NULL, &i8042DirverObject);
    if (NT_SUCCESS(nStatus))
    {
        ObDereferenceObject(i8042DirverObject);
        DbgPrint("[Message]: Get PS/2 Driver Object OK!\n");
    }else
        DbgPrint("[Message]: Get PS/2 Driver Object fail ErrorCode=%X!\n", nStatus);

    // TODO: 只考虑两种类型的键盘驱动只有一个起作用的情况下,如果同时搜索到两种型号的键盘驱动或者未搜索到直接返回错误
    if (KbdhidDriverObject && i8042DirverObject)
    {
        DbgPrint("[Message]: Get PS/2 driver object and  USB driver object at the same time!\n");
        return STATUS_UNSUCCESSFUL;
    }
    if (KbdhidDriverObject == 0 && i8042DirverObject == 0)
    {
        DbgPrint("[Message]: Get PS/2 driver object and  USB driver object at the fail!\n");
        return STATUS_UNSUCCESSFUL;
    }
    // TODO: 如果搜索到两种端口驱动中的其中一个驱动,获取驱动设备,获取设备的扩展,驱动空间起始地址和大小
    usingDriverObject = KbdhidDriverObject ? KbdhidDriverObject : i8042DirverObject;
    usingDeviceObject = usingDriverObject->DeviceObject;
    DeviceExt = usingDriverObject->DriverExtension;
    if (!searchServiceCallback(usingDeviceObject, DeviceExt))
        return STATUS_UNSUCCESSFUL;

    return STATUS_SUCCESS;
}

// 搜索保存在设备扩展里的 类驱动设备队形和回调处理函数
BOOLEAN searchServiceCallback(PDEVICE_OBJECT DeviceObject, PVOID DeviceExt)
{
    NTSTATUS nStatus = STATUS_SUCCESS;
    UNICODE_STRING DriverName = { 0 };
    PDRIVER_OBJECT KbdClassObject = NULL;            // 键盘 类驱动指针
    PVOID KbdDriverStart = NULL;                    // 驱动起始地址
    ULONG KbdDriverSize = 0;                        // 驱动大小
    PDEVICE_OBJECT TempDeviceObject = NULL;            // 临时设备对象指针
    PCHAR usingDeviceExt;                            // 设备扩展遍历指针
    PVOID AddreServiceCallback = NULL;
    KBD_CALLBACK m_KbdCallBack = { 0 };

    // TODO: 获取类驱动对象
    RtlInitUnicodeString(&DriverName, KBD_DRIVER_NAME);
    nStatus = ObReferenceObjectByName(&DriverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode,
        NULL, &KbdClassObject);
    if (!NT_SUCCESS(nStatus))
    {
        DbgPrint("[Message]: Get Kbdclass driver object at the fail!\n");
        return FALSE;
    }
    KbdDriverStart = KbdClassObject->DriverStart;
    KbdDriverSize = KbdClassObject->DriverSize;
    DbgPrint("[Message]: Kbdclass driver Address %p - %p!\n", KbdDriverStart, (PCHAR)KbdDriverStart+KbdDriverSize);
    // TODO: 遍历扩展里数据 是否有类驱动对象里的设备或者在类驱动里的地址,如果有保存
    TempDeviceObject = DeviceObject;
    while (TempDeviceObject)
    {
        usingDeviceExt = DeviceExt;
        for (int i = 0; i < 4096; i++, usingDeviceExt += sizeof(PCHAR))
        {
            PVOID tmp;
            if (!MmIsAddressValid(usingDeviceExt))
                break;

            if (m_KbdCallBack.DeviceObject && m_KbdCallBack.ServiceCallback)
            {
                gKbdCallBack.DeviceObject = m_KbdCallBack.DeviceObject;
                gKbdCallBack.ServiceCallback = m_KbdCallBack.ServiceCallback;
            }

            tmp = *(PVOID*)usingDeviceExt;

            if (tmp == TempDeviceObject)
            {
                m_KbdCallBack.DeviceObject = *(PVOID*)tmp;
                DbgPrint("[Message]: Get the kbdclass driver at the Device OK!  DeviceObject=%p\n", m_KbdCallBack.DeviceObject);
                continue;
            }

            // 键盘类驱动处理函数位于驱动设备之后,并且由于使用了未公开数据结构,这种方式可能暂时是有效的或者在某些情况下是有效的
            if (tmp > KbdDriverStart && tmp < (PVOID)((PCHAR)KbdDriverStart + KbdDriverSize)&& MmIsAddressValid(tmp))
            {
                if (!m_KbdCallBack.DeviceObject)
                    continue;
                m_KbdCallBack.ServiceCallback = (KeyBoardClassServiceCallback)tmp;
                AddreServiceCallback = (PVOID)usingDeviceExt;
                DbgPrint("[Message]: Get the kbdclass driver at the Callback OK!  tmp=%p AddreServiceCallback=%p!\n", *(PVOID*)usingDeviceExt, (PVOID)usingDeviceExt);
            }
        }
        TempDeviceObject = TempDeviceObject->NextDevice;
    }

    if (AddreServiceCallback && m_KbdCallBack.DeviceObject)
    {
        DbgPrint("[Message]: AddreServiceCallback=%p m_KbdCallBack.DeviceObject=%p m_KbdCallBack.ServiceCallback= %p\n",
            AddreServiceCallback, m_KbdCallBack.DeviceObject, m_KbdCallBack.ServiceCallback);
        return TRUE;
    }
    return FALSE;
}
