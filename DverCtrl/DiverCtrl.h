#pragma once
#include <Windows.h>

#define DRIVER_DEVICE_REGISTRY -1			// ��������

class RegisterKey
{
public:
	static void CALLBACK DiverRegister(WCHAR*, WCHAR*);				// ע����������
	static void CALLBACK DiverDelete(WCHAR*);							// ж����������
	static void CALLBACK DriverRun(WCHAR*);							// ������������
	static void CALLBACK DriverStop(WCHAR*);							// ֹͣ��������
};

HANDLE CALLBACK OpenDriverSymb(WCHAR*);						// �������豸�ķ�������

#define DevReadCtrl CTL_CODE(FILE_DEVICE_UNKNOWN, 0x911, METHOD_BUFFERED, FILE_ANY_ACCESS)