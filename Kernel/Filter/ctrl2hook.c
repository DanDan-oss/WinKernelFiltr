#include "ctrl2hook.h"


#define KBD_DRIVER_NAME    L"\\Driver\\Kbdclass"            // 键盘类驱动
#define USBKBD_DRIVER_NAME L"\\Driver\\Kbdhid"            // USB键盘 端口驱动
#define PS2KBD_DRIVER_NAME L"\\Driver\\i8042prt"        // PS/2键盘 端口驱动

extern POBJECT_TYPE* IoDriverObjectType;	/* 内核全局变量, 声明可以直接使用. 通用键盘驱动对象的类型*/

// 通过一个名字来获得一个对象得指针(闭源函数,文档没有公开,声明直接使用了)
// 调用ObReferenceObjectByName获得对象引用会使驱动对象的引用计数增加,ObDereferenceObject释放驱动对象引用
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectName, ULONG Attribuites, PACCESS_STATE AccessState, ACCESS_MASK DesiredAccess,
	POBJECT_TYPE ObjectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);

KBD_CALLBACK gKbdClassBack = { 0 };

PDRIVER_DISPATCH OldDispatchFunction[IRP_MJ_MAXIMUM_FUNCTION] = { 0 };

NTSTATUS Ctrl2HookMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	// Ctrl2IrpHook(DriverObject);
	KeyBoardServiceCallBackHook();
	return STATUS_SUCCESS;
}

NTSTATUS Ctrl2HookUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	// Ctrl2IrpUnHook()
	KeyBoardServiceCallBackUnHook();
	return STATUS_SUCCESS;
}



NTSTATUS Ctrl2FilterHookDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);


	return STATUS_SUCCESS;
}


NTSTATUS Ctrl2IrpHook(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	PDRIVER_OBJECT KbdDriverObject = NULL;
	UNICODE_STRING uniNtNameString = { 0 };
	NTSTATUS Status = STATUS_UNSUCCESSFUL;


	RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME);
	Status = ObReferenceObjectByName(&uniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[dbg]: Couldn't get the  KbdDriver Object failed! DrverName=[%wZ]\n", uniNtNameString));
		return STATUS_UNSUCCESSFUL;
	}
	// 调用ObReferenceObjectByName会使驱动对象的引用计数增加,释放驱动对象引用
	ObDereferenceObject(KbdDriverObject);

	//OldDispatchFunction[IRP_MJ_READ] = KbdDriverObject->MajorFunction[IRP_MJ_READ];
	//InterlockedExchangePointer((PVOID*)&KbdDriverObject->MajorFunction[IRP_MJ_READ], (PVOID*)Ctrl2FilterHookDispatch);

	for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		OldDispatchFunction[i] = KbdDriverObject->MajorFunction[i];
		InterlockedExchangePointer((PVOID*)&KbdDriverObject->MajorFunction[i], (PVOID*)Ctrl2FilterHookDispatch);
	}
	return STATUS_SUCCESS;
}

NTSTATUS Ctrl2IrpUnHook(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	PDRIVER_OBJECT KbdDriverObject = NULL;
	UNICODE_STRING uniNtNameString = { 0 };
	NTSTATUS Status = STATUS_UNSUCCESSFUL;


	RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME);
	Status = ObReferenceObjectByName(&uniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[dbg]: Couldn't get the  KbdDriver Object failed! DrverName=[%wZ]\n", uniNtNameString));
		return STATUS_UNSUCCESSFUL;
	}
	// 调用ObReferenceObjectByName会使驱动对象的引用计数增加,释放驱动对象引用
	ObDereferenceObject(KbdDriverObject);

	//InterlockedExchangePointer((PVOID*)&KbdDriverObject->MajorFunction[IRP_MJ_READ], (PVOID*)OldDispatchFunction[IRP_MJ_READ]);
	for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
		if(OldDispatchFunction[i])
			InterlockedExchangePointer((PVOID*)&KbdDriverObject->MajorFunction[i], (PVOID*)OldDispatchFunction[i]);

	KdPrint(("[dbg]: Unload [%wZ] IRP Hook OK!\n", uniNtNameString));
	return STATUS_SUCCESS;
}

NTSTATUS KeyBoardServiceCallBackHook()
{
	/*
	预先不知道机器上装的是USB键盘还是PS/2键盘，所以一开始是尝试打开这两个驱动.在很多情况下只有-个可以打开.
	比较极端的情况是两个都可以打开(用户同时安装有两种键盘),或者两个都打不开(用户安装有—种我们没见过的键盘).
	对于这两种极端的情况,都简单地返回失败.
	*/

	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING uniNtNameString = { 0 };
	PDEVICE_OBJECT pTargetDeviceObject = NULL;
	PDRIVER_OBJECT KbdDriverObject = NULL;
	PDRIVER_OBJECT KbdHidDriverObject = NULL;
	PDRIVER_OBJECT Kbd8042DriverObject = NULL;
	PDRIVER_OBJECT UsingDriverObject = NULL;
	PDEVICE_OBJECT UsingDeviceObject = NULL;
	PVOID KbdDriverStart = NULL;
	ULONG KbdDriverSize = 0;
	PVOID UsingDeviceExt = NULL;

	// 打开键盘类驱动
	RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME);
	nStatus = ObReferenceObjectByName(&uniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("[dbg]: Couldn't get the  KbdDriver Object failed! DrverName=[%wZ]\n", uniNtNameString));
		return STATUS_UNSUCCESSFUL;
	}
	ObDereferenceObject(KbdDriverObject);
	// 打开USB键盘端口驱动对象
	RtlInitUnicodeString(&uniNtNameString, USBKBD_DRIVER_NAME);
	nStatus = ObReferenceObjectByName(&uniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &KbdHidDriverObject);
	if (NT_SUCCESS(nStatus))
	{
		ObDereferenceObject(KbdHidDriverObject);
		KdPrint(("[dbg]: Get the USB Driver Object OK! DrverName=[%wZ]\n", uniNtNameString));
	}
	else
		KdPrint(("[dbg]: Couldn't get the USB Driver Object failed!  DrverName=[%wZ]\n", uniNtNameString));

	// 打开PS/2键盘驱动对象
	RtlInitUnicodeString(&uniNtNameString, PS2KBD_DRIVER_NAME);
	nStatus = ObReferenceObjectByName(&uniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &Kbd8042DriverObject);
	if (NT_SUCCESS(nStatus))
	{
		ObDereferenceObject(Kbd8042DriverObject);
		KdPrint(("[dbg]: Get the PS/2 Driver Object OK! DrverName=[%wZ]\n", uniNtNameString));
	}
	else
		KdPrint(("[dbg]: Couldn't get the PS/2 Driver Object failed!, DrverName=[%wZ]\n", uniNtNameString));

	// TODO: 只考虑两种类型的键盘驱动只有一个起作用的情况下,如果同时搜索到两种型号的键盘驱动或者未搜索到直接返回错误
	if (Kbd8042DriverObject && KbdHidDriverObject)
	{
		KdPrint(("[dbg]: More than two kbd, PS/2 Driver and USB Driver!\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// 如果两种及那盘都没打开,可能系统使用了其他种类的键盘,直接返回失败
	if (!Kbd8042DriverObject && !KbdHidDriverObject)
	{
		KdPrint(("[dbg]: Couldn't get the kbd Driver, PS/2 USB Driver Object!\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// 获取驱动对象第一个设备对象的设备扩展
	UsingDriverObject = KbdHidDriverObject ? KbdHidDriverObject : Kbd8042DriverObject;

	KbdDriverStart = KbdDriverObject->DriverStart;
	KbdDriverSize = KbdDriverObject->DriverSize;
	nStatus = STATUS_UNSUCCESSFUL;

	UsingDeviceObject = UsingDriverObject->DeviceObject;
	while (UsingDeviceObject)
	{
		UsingDeviceExt = UsingDeviceObject->DeviceExtension;
		// 遍历端口驱动设备扩展下的每一个指针
		for (ULONG i = 0; i < 1024; ++i)
		{
			PVOID DeviceExt = (PCHAR)UsingDeviceExt + i;
			PVOID Address = NULL;
			if (!MmIsAddressValid(DeviceExt))
				continue;
			Address = *(PVOID*)DeviceExt;

			// 判断是否已经找到,找到直接跳出
			if (gKbdClassBack.classDeviceObject && gKbdClassBack.ServiceCallBack)
			{
				nStatus = STATUS_SUCCESS;
				break;
			}

			// 遍历KdbClass下所有设备, 有一个设备对象会被保存在端口驱动的设备扩展中
			if (!gKbdClassBack.classDeviceObject)
			{
				pTargetDeviceObject = KbdDriverObject->DeviceObject;
				while (pTargetDeviceObject)
				{
					// 判断是否是设备对象地址
					if (Address == pTargetDeviceObject)
					{
						gKbdClassBack.classDeviceObject = (PDEVICE_OBJECT)Address;
						KdPrint(("[dbg]: Get the classDeiceObject %p\n", Address));
						break;
					}
					pTargetDeviceObject = pTargetDeviceObject->NextDevice;
				}
			}

			// 判断是否是回调函数
			// 键盘类驱动处理函数位于驱动设备之后,并且由于使用了未公开数据结构,这种方式可能暂时是有效的或者在某些情况下是有效的
			if (Address > KbdDriverStart && Address < (PVOID)((PCHAR)KbdDriverStart + KbdDriverSize) && MmIsAddressValid(Address) && !gKbdClassBack.ServiceCallBack)
			{
				gKbdClassBack.ServiceCallBack = (KeyBoardClassServiceCallBack)Address;
				gKbdClassBack.DevExtCallBackAddress = DeviceExt;
				KdPrint(("[dbg]: Get the ServiceClaaBack=%p, DevExtCallBackAddress=%p\n", *(PVOID*)DeviceExt, (PVOID)DeviceExt));
				continue;
			}
		}
		// 判断是否已经找到,找到直接跳出
		if (gKbdClassBack.classDeviceObject && gKbdClassBack.ServiceCallBack)
		{
			nStatus = STATUS_SUCCESS;
			break;
		}
		UsingDeviceObject = UsingDeviceObject->NextDevice;
	}

	

	if (gKbdClassBack.DevExtCallBackAddress && gKbdClassBack.ServiceCallBack)
	{
		KdPrint(("[dbg]: Hook KeyboardClassServiceCallback\n"));
		*(PVOID*)gKbdClassBack.DevExtCallBackAddress = (PVOID)KeyboardServiceCallBackFunc;
	}
	else
		KdPrint(("[dbg]: Hook Fail, DevExtCallBackAddress=%p ServiceCallBack=%p\n", gKbdClassBack.DevExtCallBackAddress, gKbdClassBack.ServiceCallBack));
	return nStatus;
}

NTSTATUS KeyBoardServiceCallBackUnHook()
{
	if (gKbdClassBack.ServiceCallBack && gKbdClassBack.DevExtCallBackAddress)
	{
		KdPrint(("[dbg]: UnLoad Hook KeyboardClassServiceCallback\n"));
		*(PVOID*)gKbdClassBack.DevExtCallBackAddress = (PVOID)gKbdClassBack.ServiceCallBack;
	}
	return STATUS_SUCCESS;
}


VOID __stdcall KeyboardServiceCallBackFunc(PDEVICE_OBJECT DeviceObject, PKEYBOARD_INPUT_DATA InputDataStart, PKEYBOARD_INPUT_DATA InputDataEnd, PULONG InputDataConsumed) 
{
	// HOOK 函数内部
	KdPrint(("[dbg]:  Hook KeyboardServiceCallBackFunc Recv Message \n"));
	gKbdClassBack.ServiceCallBack(DeviceObject, InputDataStart, InputDataEnd, InputDataConsumed);
}
