#pragma once
#include <Windows.h>

#define DRIVER_DEVICE_REGISTRY -1			// 驱动服务


void CALLBACK DiverRegister(WCHAR*, WCHAR*);				// 注册表注册驱动服务
void CALLBACK DiverDelete(WCHAR*);							// 卸载驱动服务
void CALLBACK DriverRun(WCHAR*);							// 启动驱动服务
void CALLBACK DriverStop(WCHAR*);							// 停止驱动服务
HANDLE CALLBACK OpenDriverSymb(WCHAR*);						// 打开驱动设备的符号链接

#define DevReadCtrl CTL_CODE(FILE_DEVICE_UNKNOWN, 0x911, METHOD_BUFFERED, FILE_ANY_ACCESS)