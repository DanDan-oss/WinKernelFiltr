#pragma once
#include <Windows.h>

#define DRIVER_DEVICE_REGISTRY -1			// ��������


void CALLBACK DiverRegister(WCHAR*, WCHAR*);				// ע���ע����������
void CALLBACK DiverDelete(WCHAR*);							// ж����������
void CALLBACK DriverRun(WCHAR*);							// ������������
void CALLBACK DriverStop(WCHAR*);							// ֹͣ��������
HANDLE CALLBACK OpenDriverSymb(WCHAR*);						// �������豸�ķ�������

#define DevReadCtrl CTL_CODE(FILE_DEVICE_UNKNOWN, 0x911, METHOD_BUFFERED, FILE_ANY_ACCESS)