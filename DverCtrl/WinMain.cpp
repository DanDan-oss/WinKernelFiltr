#include <Windows.h>
#include <string>
#include <atlstr.h>
#include "DiverCtrl.h"
#include "resource.h"
//#include <minwinbase.h>

#define DriverPath TEXT("C:\\TEST\\FirstDirver.sys")		// ����·��
#define SymbolicLinkName_DEMO	 TEXT("\\\\.\\slbkcdo_1187480520")
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

DriverSymbolicLink* pDemoDevice = new DriverSymbolicLink;					// ����dmeo���ɵ��豸����

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevIntance, PCHAR szCmdLine, int iCmdShow)
{
	WinInstan = hInstance;
	DialogBoxW(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, (DLGPROC)DialogProc);
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
	HWND hwLogHand = GetDlgItem(hWnd, IDC_EDIT1);			// �ӿؼ����
	HWND hwDriverPath = GetDlgItem(hWnd, IDC_EDIT_DIVERPATH);
	HWND hwServerName = GetDlgItem(hWnd, IDC_EDIT_SEVNAME);
	HWND hwSymbli = GetDlgItem(hWnd, IDC_EDIT_SYMBOLICLINK);
	CStringW cswDriverPath;

	::OutEditHand = hwLogHand;

	DbgMessagOut((PWCHAR)TEXT("��ʼ������·��..."));

	GetModuleFileName(NULL, cswDriverPath.GetBufferSetLength((MAX_PATH+1) * 2), (MAX_PATH+1) * 2);
	cswDriverPath.ReleaseBuffer();
	cswDriverPath = cswDriverPath.Left(cswDriverPath.ReverseFind('\\'));
	SetWindowTextW(hwDriverPath, cswDriverPath+"\\FirstDirver.sys");

	DbgMessagOut((PWCHAR)TEXT("��ʼ�������豸��������..."));
	SetWindowTextW(hwSymbli, SymbolicLinkName_DEMO);

	DbgMessagOut((PWCHAR)TEXT("��ʼ������ע�����..."));
	SetWindowTextW(hwServerName, ServiceName);

	SendMessage(hWnd, WM_SETICON, ICON_SMALL2, IDB_BITMAP2);

	return;
}

void MenuOperation(HWND hWnd, WPARAM wParam, LPARAM lParam)
{

	const wchar_t* str = L"Hello Kernel, my is 3333\0";
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
		SetWindowTextW(::OutEditHand, TEXT(""));
		return;

	case IDC_BTN_OPENSYMB:
		OpenSymb(hWnd);
		return;

	case IDC_BTN_CLOSESYMB:
		if (!pDemoDevice->GetSymbolicLinkHandle())
		{
			DbgMessagOut((PWCHAR)TEXT("δ��������������"));
			return;
		}
		pDemoDevice->CloseSymbolicLink();
		DbgMessagOut((PWCHAR)TEXT("�������ӹر�"));
		return;
	case IDC_BUTTON6:
		
		
		if(pDemoDevice->SendStringToDevice(str, lstrlenW(str) * 2 + 1))
			DbgMessagOut((PWCHAR)TEXT("�����뷢�ͳɹ�"));
		else
			DbgMessagOut((PWCHAR)TEXT("�����뷢��ʧ��"));
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

	RegisterKey::DiverRegister(szFileName, szDevName);
}

void DeBtnDown(HWND hWnd)
{
	HWND MenuHand = NULL;
	WCHAR szDevName[50] = { 0 };		// ����������
	if (RunDriver)
	{
		DbgMessagOut((PWCHAR)TEXT("������������,ɾ��ʧ��"));
		return;
	}
	MenuHand = GetDlgItem(hWnd, IDC_EDIT_SEVNAME);
	if (!MenuHand)
		return;
	if (!GetWindowTextW(MenuHand, szDevName, 50))
		return;

	RegisterKey::DiverDelete(szDevName);
	
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
	RegisterKey::DriverRun(szDevName);
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
	if (pDemoDevice->GetSymbolicLinkHandle())
		pDemoDevice->CloseSymbolicLink();
	
	RegisterKey::DriverStop(szDevName);
	RunDriver = FALSE;
}

void OpenSymb(HWND hWnd)
{
	HWND menuHand = NULL;			// ��������������Ĵ��ڲ˵����
	WCHAR FileDevName[50] = { 0 };
	HANDLE hdSymb = NULL;

	if (pDemoDevice->GetSymbolicLinkHandle())
	{
		DbgMessagOut((PWCHAR)TEXT("�����豸�����Ѿ���,�����ظ���"));
		return;
	}
		
	menuHand = GetDlgItem(hWnd, IDC_EDIT_SYMBOLICLINK);
	if (!menuHand)
		return;
		
	GetWindowTextW(menuHand, FileDevName, 50);

	hdSymb = pDemoDevice->OpenSymbolicLink(FileDevName);
	if(hdSymb)
		DbgMessagOut((PWCHAR)TEXT("�������豸���ӳɹ�"));
	else
		DbgMessagOut((PWCHAR)TEXT("�������豸����ʧ��"));
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