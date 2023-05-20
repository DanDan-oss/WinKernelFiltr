#include <Windows.h>
#include "DiverCtrl.h"
#include "resource.h"
//#include <minwinbase.h>

#define DriverPath TEXT("C:\\TEST\\FirstDirver.sys")		// ����·��
#define SymbolicLinkName TEXT("\\\\.\\\\MyDev")		// ���ӷ���
#define ServiceName TEXT("FirstDriver")				// ����������

void DbgMessagOut(WCHAR*);						// ���������Ϣ����Ϣ��־��
BOOL RunDriver = 0;								// �����Ƿ�����

BOOL CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);	// �Ի�����Ϣ�ص�
void DlogInit(HWND);							// �Ի����ʼ��
void MenuOperation(HWND, WPARAM, LPARAM);		// �ӿؼ��������ַ�����
void DirverOpenFile(HWND);						// ���ļ��Կ���ȡ����·��
void RegBtnDown(HWND);							// ����ע�ᰴť�����
void DeBtnDown(HWND);							// ����ɾ����ť�����
void RunBtnDown(HWND);							// �������а�ť�����
void StopBtnDown(HWND);							// ����ֹͣ��ť�����
void OpenSymb(HWND);							// �����豸�������Ӱ�ť�����

HINSTANCE WinInstan = NULL;						// ���̾��
HWND OutEditHand = NULL;						// ��Ϣ��־��ľ��
HICON g_hicon = NULL;							// ͼ����
HANDLE hdDirverSymb = NULL;						// ���������豸���ļ����

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
	HWND menuHand = 0;			// �ӿؼ����
	menuHand = GetDlgItem(hWnd, IDC_EDIT1);
	OutEditHand = menuHand;
	menuHand = GetDlgItem(hWnd, IDC_EDIT_DIVERPATH);
	SetWindowTextW(menuHand, DriverPath);
	DbgMessagOut(TEXT("��ʼ������·��..."));
	menuHand = GetDlgItem(hWnd, IDC_EDIT_SYMBOLICLINK);
	SetWindowTextW(menuHand, SymbolicLinkName);
	DbgMessagOut(TEXT("��ʼ�������豸��������..."));
	menuHand = GetDlgItem(hWnd, IDC_EDIT_SEVNAME);
	SetWindowTextW(menuHand, ServiceName);
	DbgMessagOut(TEXT("��ʼ������ע�����..."));
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
			DbgMessagOut(TEXT("δ��������������"));
			return;
		}
		DbgMessagOut(TEXT("�������ӹر�"));
		CloseHandle(hdDirverSymb);
		hdDirverSymb = NULL;
		return;
	case IDC_BUTTON6:
		if(DeviceIoControl(hdDirverSymb, DevReadCtrl, NULL, 0, NULL, 0, NULL, NULL))
			DbgMessagOut(TEXT("�����뷢���ɹ�"));
		return;
	default://
		break;
	}
	
}

void DirverOpenFile(HWND hWnd)
{
	PWCHAR pFileName = NULL;				// ָ��洢·���Ļ�����
	OPENFILENAMEW OpenFile = { 0 };		// �ļ��Ի���
	HWND  menuHand = NULL;
	WCHAR str[MAX_PATH];

	pFileName = (PWCHAR)malloc(sizeof(WCHAR)*MAX_PATH);
	if (!pFileName)
		return;
	memset(pFileName, 0, sizeof(WCHAR) * MAX_PATH);

	ZeroMemory(&OpenFile, sizeof(OPENFILENAME));
	OpenFile.hwndOwner = NULL;
	OpenFile.lStructSize = sizeof(OpenFile); // �ṹ��С
	OpenFile.lpstrFile = pFileName; // ·��
	OpenFile.nMaxFile = MAX_PATH; // ·����С
	OpenFile.lpstrFilter = TEXT(".SYS\0*.sys\0ALL(*.*)\0*.*\0\0"); // �ļ�����
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
		wsprintfW(str, TEXT("����·�����¶�λ,[%s]..."), pFileName);
		DbgMessagOut(str);
	} while (FALSE);

	free(pFileName);
	return;
}

void RegBtnDown(HWND hWnd)
{
	HWND MenuHand = NULL;
	WCHAR szFileName[MAX_PATH] = { 0 };		// �������·��
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
	WCHAR szDevName[50] = { 0 };		// ����������
	if (RunDriver)
	{
		DbgMessagOut(TEXT("������������,ɾ��ʧ��"));
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
	WCHAR szDevName[50] = { 0 };		// ����������
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
	WCHAR szDevName[50] = { 0 };		// ����������
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
	HWND menuHand = NULL;			// ��������������Ĵ��ڲ˵����
	WCHAR FileDevName[50] = { 0 };
	if (!RunDriver)
	{
		DbgMessagOut(TEXT("�����豸��δ����,�޷��򿪷��������"));
		return;
	}
	if (hdDirverSymb)
	{
		DbgMessagOut(TEXT("�����豸�����Ѿ���,�����ظ���"));
		return;
	}
		
	menuHand = GetDlgItem(hWnd, IDC_EDIT_SYMBOLICLINK);
	if (!menuHand)
		return;
		
	GetWindowTextW(menuHand, FileDevName, 50);
	hdDirverSymb = OpenDriverSymb(FileDevName);
	if(hdDirverSymb)
		DbgMessagOut(TEXT("�������豸���ӳɹ�"));
	return;
}
void DbgMessagOut(WCHAR* pBuff)
{
	SYSTEMTIME Time = { 0 }; // ϵͳʱ��ṹ��
	WCHAR OutStr[255] = { 0 };
	WCHAR* pStr = NULL;
	LONG bufSize = 0;
	if (!OutEditHand)
	{
		MessageBoxW(NULL, TEXT("��־��Ϣ���쳣"), TEXT("ERROR"), MB_ICONERROR);
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