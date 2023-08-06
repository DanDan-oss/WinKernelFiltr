#ifndef _DRIVERCTRL_H
#define _DRIVERCTRL_H

#include <Windows.h>
#include <iostream>

#define DRIVER_DEVICE_REGISTRY -1			// ��������
#define DevReadCtrl CTL_CODE(FILE_DEVICE_UNKNOWN, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)		// ssdt HOOK
#define CWK_DVC_SEND_STR (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x911, METHOD_BUFFERED, FILE_WRITE_DATA)
#define CWK_DVC_RECV_STR (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x912, METHOD_BUFFERED, FILE_READ_DATA)


// ��������
class RegisterKey
{
public:
	static void CALLBACK DiverRegister(WCHAR* DiverPath, WCHAR* DeviceName);				// ע����������
	static void CALLBACK DiverDelete(WCHAR* DevName);							// ж����������
	static void CALLBACK DriverRun(WCHAR* DevName);							// ������������
	static void CALLBACK DriverStop(WCHAR* DevName);							// ֹͣ��������
};


// �����豸�����ӿ���
class DriverSymbolicLink
{
public:
	DriverSymbolicLink();
	~DriverSymbolicLink();

	HANDLE CALLBACK OpenSymbolicLink(WCHAR* SymbLink);						// �������豸�ķ�������
	VOID CloseSymbolicLink();

	HANDLE GetSymbolicLinkHandle();
	std::wstring GetSymbolicLinkName();

	BOOL SetSSDTHOOK();
	BOOL SendStringToDevice(CONST WCHAR* strBuffer, CONST DWORD dwBufferLen);

private:
	HANDLE m_DriverDevice = NULL;
	std::wstring m_SymbLinkName = { 0 };

private:

	BOOL DeviceControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
};

#endif !_DRIVERCTRL_H