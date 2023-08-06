#include "DiverCtrl.h"
#include <string>

extern void DbgMessagOut(WCHAR*);

void CALLBACK RegisterKey::DiverRegister(WCHAR* DiverPath, WCHAR* DeviceName)
{
	SC_HANDLE scMagerHand = NULL;
	SC_HANDLE scSevHand = NULL;
	WCHAR ErrorMes[50];

	do
	{
		scMagerHand = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
		if (!scMagerHand)
		{
			DbgMessagOut((PWCHAR)TEXT("ϵͳ�����������ʧ��,����ԱȨ������..."));
			return;
		}
		scSevHand = OpenServiceW(scMagerHand, DeviceName, SERVICE_ALL_ACCESS);
		if (scSevHand)
		{
			DbgMessagOut((PWCHAR)TEXT("ע��ʧ�����������Ѵ���..."));
			break;
		}
		scSevHand = CreateServiceW(scMagerHand, DeviceName, DeviceName, SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, DiverPath, NULL, NULL, NULL,
			NULL, NULL);
		if (!scSevHand)
		{
			memset(ErrorMes, 0, sizeof(WCHAR) * 50);
			wsprintfW(ErrorMes, TEXT("��������ע��ʧ�� ERROR=%x"), GetLastError());
			DbgMessagOut(ErrorMes);
			break;
		}else
			DbgMessagOut((PWCHAR)TEXT("��������ע��ɹ�..."));

	} while (FALSE);
	if(scSevHand)
		CloseServiceHandle(scSevHand);
	if(scMagerHand)
		CloseServiceHandle(scMagerHand);
	return;
}


void CALLBACK RegisterKey::DiverDelete(WCHAR* DevName)
{
	SC_HANDLE scMagerHand = NULL;
	SC_HANDLE scSevHand = NULL;
	WCHAR szError[50];

	do
	{
		scMagerHand = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
		if (!scMagerHand)
		{
			DbgMessagOut((PWCHAR)TEXT("ϵͳ�����������ʧ��,����ԱȨ������..."));
			return;
		}
		scSevHand = OpenServiceW(scMagerHand, DevName, SERVICE_ALL_ACCESS);
		if (!scSevHand)
		{
			DbgMessagOut((PWCHAR)TEXT("����ж��ʧ��,δ�ҵ���������..."));
			break;
		}
		if (!DeleteService(scSevHand))
		{
			memset(szError, 0, sizeof(WCHAR) * 50);
			wsprintfW(szError, TEXT("��������ɾ��ʧ��, ErrorCode=%x"), GetLastError());
			DbgMessagOut(szError);
		}
		else
			DbgMessagOut((PWCHAR)TEXT("��������ɾ���ɹ�..."));

	} while (FALSE);

	if(scSevHand)
		CloseServiceHandle(scSevHand);
	if(scMagerHand)
		CloseServiceHandle(scMagerHand);
	return;
}

void CALLBACK RegisterKey::DriverRun(WCHAR* DevName)
{
	SC_HANDLE scMagerHand = NULL;
	SC_HANDLE scSevHand = NULL;
	WCHAR szError[50];

	do
	{
		scMagerHand = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
		if (!scMagerHand)
		{
			DbgMessagOut((PWCHAR)TEXT("ϵͳ�����������ʧ��,����ԱȨ������..."));
			return;
		}
		scSevHand = OpenServiceW(scMagerHand, DevName, SERVICE_ALL_ACCESS);
		if (!scSevHand)
		{
			DbgMessagOut((PWCHAR)TEXT("������������,����δע��..."));
			break;
		}
		if (!StartService(scSevHand, 0, 0))
		{
			memset(szError, 0, sizeof(WCHAR) * 50);
			wsprintfW(szError, TEXT("������������ʧ��, ERROR=%x"), GetLastError());
			DbgMessagOut(szError);
			break;
		}
		else
			DbgMessagOut((PWCHAR)TEXT("�������������ɹ�..."));

	} while (FALSE);
	
	if(scSevHand)
		CloseServiceHandle(scSevHand);
	if(scMagerHand)
		CloseServiceHandle(scMagerHand);
	return;
}

void CALLBACK RegisterKey::DriverStop(WCHAR* DevName)
{
	SC_HANDLE scMagerHand = NULL;
	SC_HANDLE scSevHand = NULL;
	SERVICE_STATUS SerStance = { 0 };
	WCHAR szError[50];

	do
	{
		scMagerHand = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
		if (!scMagerHand)
		{
			DbgMessagOut((PWCHAR)TEXT("ϵͳ�����������ʧ��,����ԱȨ������..."));
			return;
		}
		scSevHand = OpenServiceW(scMagerHand, DevName, SERVICE_ALL_ACCESS);
		if (!scSevHand)
		{
			DbgMessagOut((PWCHAR)TEXT("����ֹͣʧ��, ����δ�ҵ�..."));
			break;
		}
		if (!ControlService(scSevHand, SERVICE_CONTROL_STOP, &SerStance))
		{
			memset(szError, 0, sizeof(WCHAR) * 50);
			wsprintfW(szError, TEXT("��������ֹͣʧ��, ERROR=%x"), GetLastError());
			DbgMessagOut(szError);
		}
		else
			DbgMessagOut((PWCHAR)TEXT("��������ֹͣ�ɹ�..."));

	} while (FALSE);
	if(scSevHand)
		CloseServiceHandle(scSevHand);
	if(scMagerHand)
		CloseServiceHandle(scMagerHand);
	return;
}

DriverSymbolicLink::DriverSymbolicLink()
{

}

DriverSymbolicLink::~DriverSymbolicLink()
{
	this->CloseSymbolicLink();
}

HANDLE DriverSymbolicLink::GetSymbolicLinkHandle()
{
	return this->m_DriverDevice;
}

std::wstring DriverSymbolicLink::GetSymbolicLinkName()
{
	return this->m_SymbLinkName;
}

HANDLE CALLBACK DriverSymbolicLink::OpenSymbolicLink(WCHAR* SymbLink)
{
	HANDLE hdFileDev = NULL;	// ���������ļ��豸���
	WCHAR Result[50] = { 0 };
	hdFileDev = CreateFileW(SymbLink, GENERIC_ALL, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, 0);
	//hdFileDev = CreateFileW(SymbLink, GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, 0);
	if (INVALID_HANDLE_VALUE == hdFileDev)
	{
		wsprintfW(Result, TEXT("�������豸�������Ӵ���, errorCode = %x"),  GetLastError());
		DbgMessagOut(Result);
		return 0;
	}
	if (this->m_DriverDevice)
		this->CloseSymbolicLink();

	this->m_DriverDevice = hdFileDev;
	this->m_SymbLinkName = SymbLink;
	return hdFileDev;
}

VOID DriverSymbolicLink::CloseSymbolicLink()
{
	WCHAR szError[50];
	if (this->m_DriverDevice)
	{
		wsprintfW(szError, TEXT("�رշ�������, HANDLE=%ld"), this->m_DriverDevice);
		DbgMessagOut(szError);
		CloseHandle(m_DriverDevice);
		m_DriverDevice = NULL;
	}
	return;
}

BOOL DriverSymbolicLink::SendStringToDevice(CONST WCHAR* strBuffer, CONST DWORD dwBufferLen)
{
	DWORD dwRetLen = 0;

	if (!this->m_DriverDevice)
		return FALSE;
	return this->DeviceControl(CWK_DVC_SEND_STR, (LPVOID)strBuffer, dwBufferLen, NULL, 0, &dwRetLen, 0);
	
}

BOOL DriverSymbolicLink::DeviceControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
	return DeviceIoControl(this->m_DriverDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
}

BOOL DriverSymbolicLink::SetSSDTHOOK()
{
	return this->DeviceControl(DevReadCtrl, NULL, 0, NULL, 0, NULL, NULL);
}

