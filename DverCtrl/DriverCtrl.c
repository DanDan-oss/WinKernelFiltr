#include "DiverCtrl.h"

extern void DbgMessagOut(WCHAR*);

void CALLBACK DiverRegister(WCHAR* DiverPath, WCHAR* DeviceName)
{
	SC_HANDLE scMagerHand = NULL;
	SC_HANDLE scSevHand = NULL;
	WCHAR ErrorMes[50];
	scMagerHand = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
	if (!scMagerHand)
	{
		DbgMessagOut(TEXT("ϵͳ�����������ʧ��,����ԱȨ������..."));
		return;
	}
	do
	{
		scSevHand = OpenServiceW(scMagerHand, DeviceName, SERVICE_ALL_ACCESS);
		if (scSevHand)
		{
			DbgMessagOut(TEXT("ע��ʧ�����������Ѵ���..."));
			break;
		}
		scSevHand = CreateServiceW(scMagerHand, DeviceName, DeviceName, SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, DiverPath, NULL, NULL, NULL,
			NULL, NULL);
		if (!scSevHand)
		{
			memset(ErrorMes, 0, sizeof(WCHAR) * 50);
			wsprintfW(ErrorMes, TEXT("��������ע��ʧ�� ERROR=%d"), GetLastError());
			DbgMessagOut(ErrorMes);
			break;
		}else
			DbgMessagOut(TEXT("��������ע��ɹ�..."));

		CloseServiceHandle(scSevHand);

	} while (FALSE);
	CloseServiceHandle(scMagerHand);
	return;
}


void CALLBACK DiverDelete(WCHAR* DevName)
{
	SC_HANDLE scMagerHand = NULL;
	SC_HANDLE scSevHand = NULL;
	WCHAR szError[50];
	scMagerHand = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
	if (!scMagerHand)
	{
		DbgMessagOut(TEXT("ϵͳ�����������ʧ��,����ԱȨ������..."));
		return;
	}

	do
	{
		scSevHand = OpenServiceW(scMagerHand, DevName, SERVICE_ALL_ACCESS);
		if (!scSevHand)
		{
			DbgMessagOut(TEXT("����ж��ʧ��,δ�ҵ���������..."));
			break;
		}
		if (!DeleteService(scSevHand))
		{
			memset(szError, 0, sizeof(WCHAR) * 50);
			wsprintfW(szError, TEXT("��������ɾ��ʧ��, ErrorCode=%d"), GetLastError());
			DbgMessagOut(szError);
		}
		else
			DbgMessagOut(TEXT("��������ɾ���ɹ�..."));

		CloseServiceHandle(scSevHand);
	} while (FALSE);

	CloseServiceHandle(scMagerHand);
	return;
}

void CALLBACK DriverRun(WCHAR* DevName)
{
	SC_HANDLE scMagerHand = NULL;
	SC_HANDLE scSevHand = NULL;
	WCHAR szError[50];
	scMagerHand = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
	if (!scMagerHand)
	{
		DbgMessagOut(TEXT("ϵͳ�����������ʧ��,����ԱȨ������..."));
		return;
	}

	do
	{
		scSevHand = OpenServiceW(scMagerHand, DevName, SERVICE_ALL_ACCESS);
		if (!scSevHand)
		{
			DbgMessagOut(TEXT("������������,����δע��..."));
			break;
		}
		if (!StartService(scSevHand, 0, 0))
		{
			memset(szError, 0, sizeof(WCHAR) * 50);
			wsprintfW(szError, TEXT("������������ʧ��, ERROR=%d"), GetLastError());
			DbgMessagOut(szError);
			break;
		}
		else
			DbgMessagOut(TEXT("�������������ɹ�..."));

		CloseServiceHandle(scSevHand);
	} while (FALSE);
	
	CloseServiceHandle(scMagerHand);
	return;
}

void CALLBACK DriverStop(WCHAR* DevName)
{
	SC_HANDLE scMagerHand = NULL;
	SC_HANDLE scSevHand = NULL;
	SERVICE_STATUS SerStance = { 0 };
	WCHAR szError[50];
	scMagerHand = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
	if (!scMagerHand)
	{
		DbgMessagOut(TEXT("ϵͳ�����������ʧ��,����ԱȨ������..."));
		return;
	}

	do
	{
		scSevHand = OpenServiceW(scMagerHand, DevName, SERVICE_ALL_ACCESS);
		if (!scSevHand)
		{
			DbgMessagOut(TEXT("����ֹͣʧ��, ����δ�ҵ�..."));
			break;
		}
		if (!ControlService(scSevHand, SERVICE_CONTROL_STOP, &SerStance))
		{
			memset(szError, 0, sizeof(WCHAR) * 50);
			wsprintfW(szError, TEXT("��������ֹͣʧ��, ERROR=%d"), GetLastError());
			DbgMessagOut(szError);
		}
		else
			DbgMessagOut(TEXT("��������ֹͣ�ɹ�..."));

		CloseServiceHandle(scSevHand);

	} while (FALSE);

	CloseServiceHandle(scMagerHand);
	return;
}

HANDLE CALLBACK OpenDriverSymb(WCHAR* SymbLink)
{
	HANDLE hdFileDev = NULL;	// ���������ļ��豸���
	WCHAR Result[50] = { 0 };
	hdFileDev = CreateFileW(SymbLink, GENERIC_ALL, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM, NULL);
	if (!hdFileDev)
	{
		wsprintfW(Result, TEXT("�������豸���Ӵ���, errorCode = %d"), GetLastError());
		DbgMessagOut(Result);
		return 0;
	}
	return hdFileDev;
}