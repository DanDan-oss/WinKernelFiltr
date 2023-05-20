#include <Windows.h>
#include "DiverCtrl.h"
#include "resource.h"
//#include <minwinbase.h>

#define DriverPath TEXT("C:\\TEST\\FirstDirver.sys")		// 驱动路径
#define SymbolicLinkName TEXT("\\\\.\\\\MyDev")		// 链接符号
#define ServiceName TEXT("FirstDriver")				// 驱动服务名

void DbgMessagOut(WCHAR*);						// 输出调制消息到消息日志框
BOOL RunDriver = 0;								// 驱动是否启动

BOOL CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);	// 对话框消息回调
void DlogInit(HWND);							// 对话框初始化
void MenuOperation(HWND, WPARAM, LPARAM);		// 子控件被操作分发函数
void DirverOpenFile(HWND);						// 打开文件对框框获取驱动路径
void RegBtnDown(HWND);							// 驱动注册按钮被点击
void DeBtnDown(HWND);							// 驱动删除按钮被点击
void RunBtnDown(HWND);							// 驱动运行按钮被点击
void StopBtnDown(HWND);							// 驱动停止按钮被点击
void OpenSymb(HWND);							// 驱动设备符号链接按钮被点击

HINSTANCE WinInstan = NULL;						// 进程句柄
HWND OutEditHand = NULL;						// 消息日志框的句柄
HICON g_hicon = NULL;							// 图标句柄
HANDLE hdDirverSymb = NULL;						// 连接驱动设备的文件句柄

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevIntance, PCHAR szCmdLine, int iCmdShow)
{
	WinInstan = hInstance;
	DialogBoxW(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, DialogProc);
}


BOOL CALLBACK DialogProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		DlogInit(hWnd);
		return TRUE;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		return TRUE;

	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;


	case WM_COMMAND:
		MenuOperation(hWnd, wParam, lParam);
		return TRUE;

	default:
		break;
	}

	return FALSE;
}

void DlogInit(HWND hWnd)
{
	HWND menuHand = 0;			// 子控件句柄
	menuHand = GetDlgItem(hWnd, IDC_EDIT1);
	OutEditHand = menuHand;
	menuHand = GetDlgItem(hWnd, IDC_EDIT_DIVERPATH);
	SetWindowTextW(menuHand, DriverPath);
	DbgMessagOut(TEXT("初始化驱动路径..."));
	menuHand = GetDlgItem(hWnd, IDC_EDIT_SYMBOLICLINK);
	SetWindowTextW(menuHand, SymbolicLinkName);
	DbgMessagOut(TEXT("初始化驱动设备符号链接..."));
	menuHand = GetDlgItem(hWnd, IDC_EDIT_SEVNAME);
	SetWindowTextW(menuHand, ServiceName);
	DbgMessagOut(TEXT("初始化驱动注册表名..."));
	SendMessage(hWnd, WM_SETICON, ICON_SMALL2, IDB_BITMAP2);

	return;
}

void MenuOperation(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_BTN_OPENDIRFILE:
		DirverOpenFile(hWnd);
		return;

	case IDC_BTN_REG:
		RegBtnDown(hWnd);
		return;

	case IDC_BUTTON_DELETEDRI:
		DeBtnDown(hWnd);
		return;

	case IDC_BTN_STARTRUN:
		RunBtnDown(hWnd);
		return;

	case IDC_BTN_STOPSEV:
		StopBtnDown(hWnd);
		return;
	case IDC_BTN_CLEAREDIT:
		SetWindowTextW(OutEditHand, TEXT(""));
		return;

	case IDC_BTN_OPENSYMB:
		OpenSymb(hWnd);
		return;

	case IDC_BTN_CLOSESYMB:
		if (!hdDirverSymb)
		{
			DbgMessagOut(TEXT("未打开驱动符号链接"));
			return;
		}
		DbgMessagOut(TEXT("驱动链接关闭"));
		CloseHandle(hdDirverSymb);
		hdDirverSymb = NULL;
		return;
	case IDC_BUTTON6:
		if(DeviceIoControl(hdDirverSymb, DevReadCtrl, NULL, 0, NULL, 0, NULL, NULL))
			DbgMessagOut(TEXT("控制码发生成功"));
		return;
	default://
		break;
	}
	
}

void DirverOpenFile(HWND hWnd)
{
	PWCHAR pFileName = NULL;				// 指向存储路径的缓冲区
	OPENFILENAMEW OpenFile = { 0 };		// 文件对话框
	HWND  menuHand = NULL;
	WCHAR str[MAX_PATH];

	pFileName = (PWCHAR)malloc(sizeof(WCHAR)*MAX_PATH);
	if (!pFileName)
		return;
	memset(pFileName, 0, sizeof(WCHAR) * MAX_PATH);

	ZeroMemory(&OpenFile, sizeof(OPENFILENAME));
	OpenFile.hwndOwner = NULL;
	OpenFile.lStructSize = sizeof(OpenFile); // 结构大小
	OpenFile.lpstrFile = pFileName; // 路径
	OpenFile.nMaxFile = MAX_PATH; // 路径大小
	OpenFile.lpstrFilter = TEXT(".SYS\0*.sys\0ALL(*.*)\0*.*\0\0"); // 文件类型
	OpenFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	do
	{
		GetOpenFileNameW(&OpenFile);
		if (*pFileName == 0)
			break;
		menuHand = GetDlgItem(hWnd, IDC_EDIT_DIVERPATH);
		if (!menuHand)
			break;
		SetWindowText(menuHand, pFileName);
		memset(str, 0, sizeof(WCHAR) * MAX_PATH);
		wsprintfW(str, TEXT("驱动路径重新定位,[%s]..."), pFileName);
		DbgMessagOut(str);
	} while (FALSE);

	free(pFileName);
	return;
}

void RegBtnDown(HWND hWnd)
{
	HWND MenuHand = NULL;
	WCHAR szFileName[MAX_PATH] = { 0 };		// 存放驱动路径
	WCHAR szDevName[50] = { 0 };
	MenuHand = GetDlgItem(hWnd, IDC_EDIT_DIVERPATH);
	if (!MenuHand)
		return;
	if (!GetWindowTextW(MenuHand, szFileName, MAX_PATH))
		return;
	MenuHand = GetDlgItem(hWnd, IDC_EDIT_SEVNAME);
	if (!GetWindowTextW(MenuHand, szDevName, 50))
		return;

	DiverRegister(szFileName, szDevName);
}

void DeBtnDown(HWND hWnd)
{
	HWND MenuHand = NULL;
	WCHAR szDevName[50] = { 0 };		// 驱动服务名
	if (RunDriver)
	{
		DbgMessagOut(TEXT("驱动尚在运行,删除失败"));
		return;
	}
	MenuHand = GetDlgItem(hWnd, IDC_EDIT_SEVNAME);
	if (!MenuHand)
		return;
	if (!GetWindowTextW(MenuHand, szDevName, 50))
		return;

	DiverDelete(szDevName);
	
}

void RunBtnDown(HWND  hWnd)
{
	HWND MenuHand = NULL;
	WCHAR szDevName[50] = { 0 };		// 驱动服务名
	MenuHand = GetDlgItem(hWnd, IDC_EDIT_SEVNAME);
	if (!MenuHand)
		return;
	if (!GetWindowTextW(MenuHand, szDevName, 50))
		return;
	DriverRun(szDevName);
	RunDriver = TRUE;
}

void StopBtnDown(HWND hWnd)
{
	HWND MenuHand = NULL;
	WCHAR szDevName[50] = { 0 };		// 驱动服务名
	MenuHand = GetDlgItem(hWnd, IDC_EDIT_SEVNAME);
	if (!MenuHand)
		return;
	if (!GetWindowTextW(MenuHand, szDevName, 50))
		return;
	DriverStop(szDevName);
	RunDriver = FALSE;
}

void OpenSymb(HWND hWnd)
{
	HWND menuHand = NULL;			// 保存符号链接名的窗口菜单句柄
	WCHAR FileDevName[50] = { 0 };
	if (!RunDriver)
	{
		DbgMessagOut(TEXT("驱动设备尚未运行,无法打开服务号链接"));
		return;
	}
	if (hdDirverSymb)
	{
		DbgMessagOut(TEXT("驱动设备链接已经打开,不用重复打开"));
		return;
	}
		
	menuHand = GetDlgItem(hWnd, IDC_EDIT_SYMBOLICLINK);
	if (!menuHand)
		return;
		
	GetWindowTextW(menuHand, FileDevName, 50);
	hdDirverSymb = OpenDriverSymb(FileDevName);
	if(hdDirverSymb)
		DbgMessagOut(TEXT("打开驱动设备链接成功"));
	return;
}
void DbgMessagOut(WCHAR* pBuff)
{
	SYSTEMTIME Time = { 0 }; // 系统时间结构体
	WCHAR OutStr[255] = { 0 };
	WCHAR* pStr = NULL;
	LONG bufSize = 0;
	if (!OutEditHand)
	{
		MessageBoxW(NULL, TEXT("日志信息框异常"), TEXT("ERROR"), MB_ICONERROR);
		return;
	}
	GetLocalTime(&Time);
	wsprintfW(OutStr, TEXT("[%d-%d-%d %d:%d:%d]  %s\r\n"), Time.wYear, Time.wMonth, Time.wDay, Time.wHour,
		Time.wMinute, Time.wSecond, pBuff);
	bufSize = GetWindowTextLengthW(OutEditHand) + lstrlenW(OutStr)+2;
	pStr = (WCHAR*)malloc(sizeof(WCHAR) * bufSize);
	if (!pStr)
		return;
	memset(pStr, 0, bufSize);
	GetWindowTextW(OutEditHand, pStr, bufSize + 2);
	lstrcatW(pStr, OutStr);
	SetWindowTextW(OutEditHand, pStr);
	free(pStr);
}