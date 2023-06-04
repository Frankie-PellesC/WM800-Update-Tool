/*+@@file@@----------------------------------------------------------------*//*!
 \file		main.c
 \par Description 
            
 \par  Status: 
            
 \par Project: 
            
 \date		Created  on Thu May 25 22:27:36 2023
 \date		Modified on Thu May 25 22:27:36 2023
 \author	
\*//*-@@file@@----------------------------------------------------------------*/
#define WIN32_LEAN_AND_MEAN
#define __STDC_WANT_LIB_EXT2__ 1

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "Update_tool.h"
#include "controls.h"
#include "Init.h"
#include "serial.h"
#include "rom.h"
#include "sw_download.h"

static INT_PTR CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
static HANDLE hCom   = NULL;

HINSTANCE ghInstance    = NULL;
HMENU     hDlSpeedMenu  = NULL;
int       DownLoadSpeed = DOWNLOAD_SPEED_MAX;

#define SYS_DIALOG_CLASS	 MAKEINTRESOURCE(32770)

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		WinMain
 \date		Created  on Sun May 21 20:47:17 2023
 \date		Modified on Sun May 21 20:47:17 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, CHAR *pszCmdLine, int nCmdShow)
{
	INITCOMMONCONTROLSEX icc;
	WNDCLASSEX           wcx;

	ghInstance = hInstance;

	icc.dwSize = sizeof(icc);
	icc.dwICC  = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icc);

	/*
	 * Get system dialog information
	 */
	wcx.cbSize = sizeof(wcx);
	if (!GetClassInfoEx(NULL, SYS_DIALOG_CLASS, &wcx))
		return 0;

	/*
	 * Add our own stuff
	 */
	wcx.hInstance     = hInstance;
	wcx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_ICO_MAIN));
	wcx.lpszClassName = DIALOG_CLASS;
	if (!RegisterClassEx(&wcx))
		return 0;

	/*
	 * The user interface is a modal dialog box
	 */
	return (int)DialogBox(hInstance, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)MainDlgProc);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		SelectImage
 \date		Created  on Mon May 22 00:04:58 2023
 \date		Modified on Mon May 22 00:04:58 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void SelectImage(HWND hDlg)
{
	char szFile[MAX_PATH] = {0};
	OPENFILENAME of = {0};
	of.lStructSize = sizeof(of);
	of.hwndOwner   = hDlg;
	of.lpstrFilter = "WM image FLS\0*.fls\0WM image IMG\0*.img\0WM image ALL\0*.fls;*.img\0All files\0*.*\0";
	of.nFilterIndex = 1;
	of.lpstrFile    = szFile;
	of.nMaxFile     = sizeof(szFile);
	of.lpstrTitle   = "WinnerMicro Image selection";
	of.Flags        = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON;

	if (!GetOpenFileName(&of))
		return;

	SendMessage(GetDlgItem(hDlg, ID_ED_IMG_FILE), WM_SETTEXT, 0, (LPARAM)szFile);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		SaveScreen
 \date		Created  on Sun Jun  4 18:09:39 2023
 \date		Modified on Sun Jun  4 18:09:39 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void SaveScreen(HWND hDlg)
{
	char szFile[MAX_PATH] = {0};
	OPENFILENAME of = {0};
	of.lStructSize = sizeof(of);
	of.hwndOwner   = hDlg;
	of.lpstrFilter = "files\0*.*\0";
	of.nFilterIndex = 1;
	of.lpstrFile    = szFile;
	of.nMaxFile     = sizeof(szFile);
	of.lpstrTitle   = "Save screen to file";
	of.Flags        = OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON;

	if (!GetSaveFileName(&of))
		return;

	FILE *fp = fopen(szFile, "w+");
	if (!fp)
	{
		DWORD error = GetLastError();
		MessageBoxf(hDlg, "Error.", MB_OK|MB_ICONERROR,
						"Can't create file \"%s\".\nError %d (0x%02x).", szFile, error, error);
		return;
	}

	size_t size = SendMessage(GetDlgItem(hDlg, ID_EDIT), WM_GETTEXTLENGTH, 0, 0);
	if (!size)
	{
		MessageBox(hDlg, "No data to save.\nThe file will not be created.", "Info.", MB_OK|MB_ICONINFORMATION);
		return;
	}

	char *p = malloc(size);
	if (!p)
	{
		DWORD error = GetLastError();
		MessageBoxf(hDlg, "Error.", MB_OK|MB_ICONERROR,
						"Can't allocate memory \"%s\".\nError %d (0x%02x).", szFile, error, error);
		return;
	}

	SendMessage(GetDlgItem(hDlg, ID_EDIT), WM_GETTEXT, (WPARAM)size, (LPARAM)p);
	fwrite(p, size, 1, fp);
	fclose(fp);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		ThreadDownloadImageFn
 \date		Created  on Sat May 27 16:47:31 2023
 \date		Modified on Sat May 27 16:47:31 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
static DWORD WINAPI ThreadDownloadImageFn(LPVOID arg)
{
	char szImage[MAX_PATH];
	HWND hDlg = (HWND)arg;

	DisableControlsList(hDlg, aCmdCtls, iCmdCtlsSize);

	StopRxThread(hDlg);
	SendMessage(GetDlgItem(hDlg, ID_ED_IMG_FILE), WM_GETTEXT, (WPARAM)sizeof(szImage), (LPARAM)szImage);

	int ret = DownloadFirmware(hDlg, hCom, szImage);
	if (ret != RET_SUCCESS)
		StartRxThread(hDlg, hCom);

	EnableControlsList(hDlg, aCmdCtls, iCmdCtlsSize);

	return ret;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		DownloadImage
 \date		Created  on Mon May 22 18:32:56 2023
 \date		Modified on Mon May 22 18:32:56 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void DownloadImage(HWND hDlg)
{
	HANDLE hThread = CreateThread(NULL, 0, ThreadDownloadImageFn, (void *)hDlg, 0, NULL);
	if (!hThread)
	{
		DWORD error = GetLastError();
		MessageBoxf(hDlg, "Error", MB_ICONERROR|MB_OK,
				"Cant start download thread!\nError %d (0x%02x).", error, error);
		return;
	}
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		ToHex
 \date		Created  on Sat Jun  3 11:32:30 2023
 \date		Modified on Sat Jun  3 11:32:30 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
uint8_t ToHex(uint8_t c)
{
	c = (uint8_t)toupper(c) - '0';
	if (c > 9)
		c  -= 'A' - '9' - 1; 
	return c;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		GetHexBuffer
 \date		Created  on Sat Jun  3 00:57:06 2023
 \date		Modified on Sat Jun  3 00:57:06 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
static int GetHexBuffer(char *src, uint8_t *dst)
{
	int len = 0;
	while(isxdigit(src[0]) && isxdigit(src[1]))
	{
		dst[len++] = (ToHex(*src++) << 4) | ToHex(*src++);
		if (*src == ' ')
			src++;
	}
	*src = '\0';
	return len;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		SendString
 \date		Created  on Mon May 22 23:10:36 2023
 \date		Modified on Mon May 22 23:10:36 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void SendString(HWND hDlg)
{
	char szStr[MAX_PATH];
	SendMessage(GetDlgItem(hDlg, ID_ED_CMD), WM_GETTEXT, (WPARAM)sizeof(szStr), (LPARAM)szStr);
	int l = strlen(szStr);
	switch(SendType)
	{
		case SendStringNormal:
			break;
		case SendStringCR:
			szStr[l]   = '\r';
			szStr[l+1] = '\0';
			break;
		case SendStringNL:
			szStr[l]   = '\n';
			szStr[l+1] = '\0';
			break;
		case SendStringCRNL:
			szStr[l]   = '\r';
			szStr[l+1] = '\n';
			szStr[l+2] = '\0';
			break;
		case SendStringHex:
		{
			uint8_t data[MAX_PATH];
			int     len = GetHexBuffer(szStr, data);
			WriteSerial(hCom, data, (unsigned int)len);
			SendMessage(GetDlgItem(hDlg, ID_ED_CMD), WM_SETTEXT, 0, (LPARAM)szStr);
			return;
		}
	}

	WriteSerial(hCom, szStr, (unsigned int)strlen(szStr));
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		DlSpeedMenuReset
 \date		Created  on Sat May 27 20:04:21 2023
 \date		Modified on Sat May 27 20:04:21 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void DlSpeedMenuReset(void)
{
	int items = GetMenuItemCount(hDlSpeedMenu);
	for (int i=0; i<items; i++)
		CheckMenuItem(hDlSpeedMenu, i, MF_BYPOSITION|MF_UNCHECKED);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		InitMenu
 \date		Created  on Sat May 27 18:49:45 2023
 \date		Modified on Sat May 27 18:49:45 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void InitMenu(HWND hDlg)
{
	HMENU hMenu = LoadMenu(ghInstance, MAKEINTRESOURCE(ID_MENU));
	SetMenu(hDlg, hMenu);
	hMenu = GetMenu(hDlg);
	hDlSpeedMenu = GetSubMenu(GetSubMenu(hMenu, 0), 0);
	DlSpeedMenuReset();
	CheckMenuItem(hDlSpeedMenu, ID_DL_SPEED_2M, MF_BYCOMMAND|MF_CHECKED);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		ChangeDownloadSpeed
 \date		Created  on Sat May 27 20:18:03 2023
 \date		Modified on Sat May 27 20:18:03 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void ChangeDownloadSpeed(int speed)
{
	DlSpeedMenuReset();
	switch(speed)
	{
		case ID_DL_SPEED_115K:
			DownLoadSpeed = 115200;
			break;
		case ID_DL_SPEED_460K:
			DownLoadSpeed = 460800;
			break;
		case ID_DL_SPEED_921K:
			DownLoadSpeed = 921600;
			break;
		case ID_DL_SPEED_1M:
			DownLoadSpeed = 1000000;
			break;
		case ID_DL_SPEED_2M:
			DownLoadSpeed = 2000000;
			break;
	}
	CheckMenuItem(hDlSpeedMenu, speed, MF_BYCOMMAND|MF_CHECKED);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		MainDlgProc
 \date		Created  on Sun May 21 20:45:49 2023
 \date		Modified on Sun May 21 20:45:49 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
static INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			DisableControlsList(hDlg, aCmdCtls, iCmdCtlsSize);
			EnableControlsList(hDlg, aConnectCtls, iConnectCtlsSize);
			InitMenu(hDlg);
			InitDialog(hDlg);
			return TRUE;
		}

		case WM_SIZE:
			/*
			 * TODO: Add code to process resizing, when needed.
			 */
			return TRUE;

		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID(wParam, lParam))
			{
				case ID_MENU_ABOUT:
					About(hDlg);
					break;

				case ID_BT_CONNECT:
				{
					if (!hCom)
						hCom = DoConnect(hDlg);
					else
					{
						Disconnect(hDlg, &hCom);
						hCom = NULL;
					}
					break;
				}

				case ID_BT_CLR_SCR:
					SendMessage(GetDlgItem(hDlg, ID_EDIT), WM_SETTEXT, 0, (LPARAM)"");
					break;

				case ID_BT_RESET:
					Reset_uC(hDlg, hCom);
					break;

				case ID_BT_SELECT_IMAGE:
					SelectImage(hDlg);
					break;

				case ID_BT_SAVE:
					SaveScreen(hDlg);
					break;

				case ID_BT_UPDATE:
					DownloadImage(hDlg);
					break;

				case ID_DL_SPEED_115K:
				case ID_DL_SPEED_460K:
				case ID_DL_SPEED_921K:
				case ID_DL_SPEED_1M:
				case ID_DL_SPEED_2M:
					ChangeDownloadSpeed(GET_WM_COMMAND_ID(wParam, lParam));
					break;

				case IDOK:
				case ID_BT_SEND_CMD:
					SendString(hDlg);
					break;

				case SendStringNormal:
				case SendStringCR:
				case SendStringNL:
				case SendStringCRNL:
				case SendStringHex:
					UpdateSendButton(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
					break;

				case ID_MENU_ROM_MONITOR:
					RomMonitor(hDlg, hCom);
					break;

				case ID_MENU_QRY_WIFI:
				case ID_MENU_QRY_BT:
				case ID_MENU_QRY_QFLASH:
				case ID_MENU_QRY_ROM:
					RomQuery(hDlg, hCom, GET_WM_COMMAND_ID(wParam, lParam));
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg, 0);
			return TRUE;

		case WM_NOTIFY:
			return HandleSlitbutton(hDlg, lParam);
	}

	return FALSE;
}
