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
		DbgMessagOut(TEXT("系统服务管理器打开失败,管理员权限运行..."));
		return;
	}
	do
	{
		scSevHand = OpenServiceW(scMagerHand, DeviceName, SERVICE_ALL_ACCESS);
		if (scSevHand)
		{
			DbgMessagOut(TEXT("注册失败驱动服务已存在..."));
			break;
		}
		scSevHand = CreateServiceW(scMagerHand, DeviceName, DeviceName, SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, DiverPath, NULL, NULL, NULL,
			NULL, NULL);
		if (!scSevHand)
		{
			memset(ErrorMes, 0, sizeof(WCHAR) * 50);
			wsprintfW(ErrorMes, TEXT("驱动服务注册失败 ERROR=%d"), GetLastError());
			DbgMessagOut(ErrorMes);
			break;
		}else
			DbgMessagOut(TEXT("驱动服务注册成功..."));

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
		DbgMessagOut(TEXT("系统服务管理器打开失败,管理员权限运行..."));
		return;
	}

	do
	{
		scSevHand = OpenServiceW(scMagerHand, DevName, SERVICE_ALL_ACCESS);
		if (!scSevHand)
		{
			DbgMessagOut(TEXT("驱动卸载失败,未找到驱动服务..."));
			break;
		}
		if (!DeleteService(scSevHand))
		{
			memset(szError, 0, sizeof(WCHAR) * 50);
			wsprintfW(szError, TEXT("驱动服务删除失败, ErrorCode=%d"), GetLastError());
			DbgMessagOut(szError);
		}
		else
			DbgMessagOut(TEXT("驱动服务删除成功..."));

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
		DbgMessagOut(TEXT("系统服务管理器打开失败,管理员权限运行..."));
		return;
	}

	do
	{
		scSevHand = OpenServiceW(scMagerHand, DevName, SERVICE_ALL_ACCESS);
		if (!scSevHand)
		{
			DbgMessagOut(TEXT("驱动启动错误,服务未注册..."));
			break;
		}
		if (!StartService(scSevHand, 0, 0))
		{
			memset(szError, 0, sizeof(WCHAR) * 50);
			wsprintfW(szError, TEXT("驱动服务启动失败, ERROR=%d"), GetLastError());
			DbgMessagOut(szError);
			break;
		}
		else
			DbgMessagOut(TEXT("驱动服务启动成功..."));

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
		DbgMessagOut(TEXT("系统服务管理器打开失败,管理员权限运行..."));
		return;
	}

	do
	{
		scSevHand = OpenServiceW(scMagerHand, DevName, SERVICE_ALL_ACCESS);
		if (!scSevHand)
		{
			DbgMessagOut(TEXT("驱动停止失败, 服务未找到..."));
			break;
		}
		if (!ControlService(scSevHand, SERVICE_CONTROL_STOP, &SerStance))
		{
			memset(szError, 0, sizeof(WCHAR) * 50);
			wsprintfW(szError, TEXT("驱动服务停止失败, ERROR=%d"), GetLastError());
			DbgMessagOut(szError);
		}
		else
			DbgMessagOut(TEXT("驱动服务停止成功..."));

		CloseServiceHandle(scSevHand);

	} while (FALSE);

	CloseServiceHandle(scMagerHand);
	return;
}

HANDLE CALLBACK OpenDriverSymb(WCHAR* SymbLink)
{
	HANDLE hdFileDev = NULL;	// 打开驱动的文件设备句柄
	WCHAR Result[50] = { 0 };
	hdFileDev = CreateFileW(SymbLink, GENERIC_ALL, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM, NULL);
	if (!hdFileDev)
	{
		wsprintfW(Result, TEXT("打开驱动设备链接错误, errorCode = %d"), GetLastError());
		DbgMessagOut(Result);
		return 0;
	}
	return hdFileDev;
}