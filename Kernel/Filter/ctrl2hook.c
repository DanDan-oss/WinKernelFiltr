#include "ctrl2hook.h"


#define KBD_DRIVER_NAME    L"\\Driver\\Kbdclass"            // ����������
#define USBKBD_DRIVER_NAME L"\\Driver\\Kbdhid"            // USB���� �˿�����
#define PS2KBD_DRIVER_NAME L"\\Driver\\i8042prt"        // PS/2���� �˿�����

// ͨ��һ�����������һ�������ָ��(��Դ����,�ĵ�û�й���,����ֱ��ʹ����)
// ����ObReferenceObjectByName��ö������û�ʹ������������ü�������,ObDereferenceObject�ͷ�������������
NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectName, ULONG Attribuites, PACCESS_STATE AccessState, ACCESS_MASK DesiredAccess,
	POBJECT_TYPE ObjectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);

extern POBJECT_TYPE* IoDriverObjectType;	/* �ں�ȫ�ֱ���, ��������ֱ��ʹ��. ͨ�ü����������������*/

extern  int __stdcall  MakeCodeToAscii(UCHAR sch, const int Kb_Status);

enum KeyBoardCode
{
	KBC_CAPS_LOCK_DOWN = 0x3A,
	KBC_CAPS_LOCK_UP = 0xBA,
	KBC_NUM_LOCK_DOWN = 0x45,
	KBC_NUM_LOCK_UP = 0xC5,
	KBC_LCTRL_DOWN = 0x1D,
	KBC_RCTRL_DOWN = 0xE01D,
	KBC_LCTRL_UP = 0x9D,
	KBC_RCTRL_UP = 0xE09D,
	KBC_LSHFIT_DOWN = 0x2A,
	KBC_RSHFIT_DOWN = 0x36,
	KBC_LSHFIT_UP = 0xAA,
	KBC_RSHFIT_UP = 0xB6,
	KBC_P_DOWN = 0x19,
	KBC_W_DOWN = 0x11,
	KBC_A_DOWN = 0x1E,
	KBC_B_DOWN = 0x30,
	KBC_E_DOWN = 0x12
};

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
	// ����ObReferenceObjectByName��ʹ������������ü�������,�ͷ�������������
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
	// ����ObReferenceObjectByName��ʹ������������ü�������,�ͷ�������������
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
	Ԥ�Ȳ�֪��������װ����USB���̻���PS/2���̣�����һ��ʼ�ǳ��Դ�����������.�ںܶ������ֻ��-�����Դ�.
	�Ƚϼ��˵���������������Դ�(�û�ͬʱ��װ�����ּ���),�����������򲻿�(�û���װ�С�������û�����ļ���).
	���������ּ��˵����,���򵥵ط���ʧ��.
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

	// �򿪼���������
	RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME);
	nStatus = ObReferenceObjectByName(&uniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &KbdDriverObject);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("[dbg]: Couldn't get the  KbdDriver Object failed! DrverName=[%wZ]\n", uniNtNameString));
		return STATUS_UNSUCCESSFUL;
	}
	ObDereferenceObject(KbdDriverObject);
	// ��USB���̶˿���������
	RtlInitUnicodeString(&uniNtNameString, USBKBD_DRIVER_NAME);
	nStatus = ObReferenceObjectByName(&uniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &KbdHidDriverObject);
	if (NT_SUCCESS(nStatus))
	{
		ObDereferenceObject(KbdHidDriverObject);
		KdPrint(("[dbg]: Get the USB Driver Object OK! DrverName=[%wZ]\n", uniNtNameString));
	}
	else
		KdPrint(("[dbg]: Couldn't get the USB Driver Object failed!  DrverName=[%wZ]\n", uniNtNameString));

	// ��PS/2������������
	RtlInitUnicodeString(&uniNtNameString, PS2KBD_DRIVER_NAME);
	nStatus = ObReferenceObjectByName(&uniNtNameString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &Kbd8042DriverObject);
	if (NT_SUCCESS(nStatus))
	{
		ObDereferenceObject(Kbd8042DriverObject);
		KdPrint(("[dbg]: Get the PS/2 Driver Object OK! DrverName=[%wZ]\n", uniNtNameString));
	}
	else
		KdPrint(("[dbg]: Couldn't get the PS/2 Driver Object failed!, DrverName=[%wZ]\n", uniNtNameString));

	// TODO: ֻ�����������͵ļ�������ֻ��һ�������õ������,���ͬʱ�����������ͺŵļ�����������δ������ֱ�ӷ��ش���
	if (Kbd8042DriverObject && KbdHidDriverObject)
	{
		KdPrint(("[dbg]: More than two kbd, PS/2 Driver and USB Driver!\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// ������ּ����̶�û��,����ϵͳʹ������������ļ���,ֱ�ӷ���ʧ��
	if (!Kbd8042DriverObject && !KbdHidDriverObject)
	{
		KdPrint(("[dbg]: Couldn't get the kbd Driver, PS/2 USB Driver Object!\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// ��ȡ���������һ���豸������豸��չ
	UsingDriverObject = KbdHidDriverObject ? KbdHidDriverObject : Kbd8042DriverObject;

	KbdDriverStart = KbdDriverObject->DriverStart;
	KbdDriverSize = KbdDriverObject->DriverSize;
	nStatus = STATUS_UNSUCCESSFUL;

	// �������̶˿������µ������豸
	UsingDeviceObject = UsingDriverObject->DeviceObject;
	while (UsingDeviceObject)
	{
		UsingDeviceExt = UsingDeviceObject->DeviceExtension;
		// �����˿������豸��չ�µ�ÿһ��ָ��
		for (ULONG i = 0; i < 1024; ++i)
		{
			PVOID DeviceExt = (PCHAR)UsingDeviceExt + i;
			PVOID Address = NULL;
			if (!MmIsAddressValid(DeviceExt))
				continue;
			Address = *(PVOID*)DeviceExt;

			// �ж��Ƿ��Ѿ��ҵ�,�ҵ�ֱ������
			if (gKbdClassBack.classDeviceObject && gKbdClassBack.ServiceCallBack)
			{
				nStatus = STATUS_SUCCESS;
				break;
			}

			// ����KdbClass�������豸, ��һ���豸����ᱻ�����ڶ˿��������豸��չ��
			if (!gKbdClassBack.classDeviceObject)
			{
				pTargetDeviceObject = KbdDriverObject->DeviceObject;
				while (pTargetDeviceObject)
				{
					// �ж��Ƿ����豸�����ַ
					if (Address == pTargetDeviceObject)
					{
						gKbdClassBack.classDeviceObject = (PDEVICE_OBJECT)Address;
						KdPrint(("[dbg]: Get the classDeiceObject %p\n", Address));
						break;
					}
					pTargetDeviceObject = pTargetDeviceObject->NextDevice;
				}
			}

			// �ж��Ƿ��ǻص�����
			// ����������������λ�������豸֮��,��������ʹ����δ�������ݽṹ,���ַ�ʽ������ʱ����Ч�Ļ�����ĳЩ���������Ч��
			if (Address > KbdDriverStart && Address < (PVOID)((PCHAR)KbdDriverStart + KbdDriverSize) && MmIsAddressValid(Address) && !gKbdClassBack.ServiceCallBack)
			{
				gKbdClassBack.ServiceCallBack = (KeyBoardClassServiceCallBack)Address;
				gKbdClassBack.DevExtCallBackAddress = DeviceExt;
				KdPrint(("[dbg]: Get the ServiceClaaBack=%p, DevExtCallBackAddress=%p\n", *(PVOID*)DeviceExt, (PVOID)DeviceExt));
				continue;
			}
		}
		// �ж��Ƿ��Ѿ��ҵ�,�ҵ�ֱ������
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

	// HOOK KeyboardServiceCallBack����
	KdPrint(("[dbg]:  Hook KeyboardServiceCallBackFunc Recv Message \n"));
	KdPrint(("[dbg]:  DeviceObject=%p InputDataConsumed=%ld \n", DeviceObject, *InputDataConsumed));
	KdPrint(("[dbg]:  InputDataStart address=%p InputDataEnd address=%p \n", InputDataStart, InputDataEnd));

	for (ULONG i= 0; i < InputDataEnd- InputDataStart; ++i)
	{
		KdPrint(("[dbg]:  InputDataStart address=%p id=%d, makecode=%x \n", InputDataStart+i, (InputDataStart+i)->UnitId, (InputDataStart+i)->MakeCode));
		MakeCodeToAscii((UCHAR)(InputDataStart+i)->MakeCode, 0);
	}

	// �������ûص�����������һ�� 1 �İ�����Ϣ,�������۷���ʲô��Ϣ,������һ��1
	KEYBOARD_INPUT_DATA fakeKey = *InputDataStart;
	fakeKey.MakeCode = 2; // ���� 1 ��scancode �� 2
	PKEYBOARD_INPUT_DATA fakeInputDataStart = &fakeKey;
	PKEYBOARD_INPUT_DATA fakeInputDataEnd = fakeInputDataStart + 1;
	ULONG fakeInputDataConsumed = 0;

	KdPrint(("[dbg]:  ���밴��1 \n"));
	gKbdClassBack.ServiceCallBack(DeviceObject, fakeInputDataStart, fakeInputDataEnd, &fakeInputDataConsumed);
	gKbdClassBack.ServiceCallBack(DeviceObject, InputDataStart, InputDataEnd, InputDataConsumed);
}
