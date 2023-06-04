#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "Update_tool.h"
#include "controls.h"
#include "Init.h"

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		InitSpeedCb
 \date		Created  on Wed May 31 22:21:58 2023
 \date		Modified on Wed May 31 22:21:58 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void InitSpeedCb(HWND hDlg)
{
	static const char *speeds[] = {"4800", "9600", "19200", "38400", "57600", "74880", "115200",
										"230400", "460800", "576000", "921600", "1000000", "2000000"};

	HWND hComboBox = GetDlgItem(hDlg, ID_CB_SPEED);
	for (int i=0; i<NELEMS(speeds); i++)
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)speeds[i]);
	SendMessage(hComboBox, CB_SETCURSEL, (WPARAM)6, (LPARAM)0);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		InitComCb
 \date		Created  on Wed May 31 22:22:03 2023
 \date		Modified on Wed May 31 22:22:03 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void InitComCb(HWND hDlg)
{
    LONG ret;
    HKEY key;
	DWORD i = 0;
	DWORD knlen;
    char kname[MAX_PATH] = {0};
	DWORD kvlen;
    char kvalue[MAX_PATH] = {0};
    REGSAM kmask = KEY_READ;

	HWND hComboBox = GetDlgItem(hDlg, ID_CB_SERIAL);

	#if defined(KEY_WOW64_64KEY)
	    BOOL is_64;

	    if (IsWow64Process(GetCurrentProcess(), &is_64))
	    {
	        if (is_64)
	            kmask |= KEY_WOW64_64KEY;
	    }
	#endif

    ret = RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"),
												0, NULL, 0, kmask, NULL, &key, NULL);
    if (ret != ERROR_SUCCESS)
    {
        return;
    }

	do
	{
	    knlen = sizeof(kname);
	    kvlen = sizeof(kvalue);
		ret = RegEnumValue(key, i, kname, &knlen, 0, NULL, (LPBYTE)kvalue, &kvlen);
		if (ret == ERROR_SUCCESS)
		{
            if (strcmp(kvalue, "") != 0)
            {
				SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)kvalue);
		    }
		}

		i++;
	} while (ret != ERROR_NO_MORE_ITEMS);

    RegCloseKey(key);

	SendMessage(hComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    return;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		InituCCb
 \date		Created  on Wed May 31 22:22:11 2023
 \date		Modified on Wed May 31 22:22:11 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void InituCCb(HWND hDlg)
{
	HWND hComboBox = GetDlgItem(hDlg, ID_CB_uC);
	SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"W60X");
	SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"W80X");
	SendMessage(hComboBox, CB_SETCURSEL, (WPARAM)1, (LPARAM)0);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		InitDialog
 \date		Created  on Wed May 31 22:22:16 2023
 \date		Modified on Wed May 31 22:22:16 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void InitDialog(HWND hDlg)
{
	InitSpeedCb(hDlg);
	InitComCb(hDlg);
	InituCCb(hDlg);
	HMENU hMenu = GetSubMenu(GetMenu(hDlg), 1);
	EnableMenuItem(hMenu, ID_MENU_QRY_WIFI,    MF_GRAYED);
	EnableMenuItem(hMenu, ID_MENU_QRY_BT,      MF_GRAYED);
	EnableMenuItem(hMenu, ID_MENU_QRY_QFLASH,  MF_GRAYED);
	EnableMenuItem(hMenu, ID_MENU_QRY_ROM,     MF_GRAYED);
	EnableMenuItem(hMenu, ID_MENU_ROM_MONITOR, MF_GRAYED);
	SendMessage(GetDlgItem(hDlg, ID_PROGRESS), PBM_SETPOS, (WPARAM)1, 0);
	SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)"Update_tool "
								 UPDATE_TOOL_VERSION " (c)frankie 2023");
	UpdateSendButton(hDlg, SendStringNormal);
}
