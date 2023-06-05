#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <stdio.h>

#include "Update_tool.h"
#include "controls.h"
#include "Init.h"

HWND hToolTip= NULL;

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
 \brief		WordBreakProc
 \details	Custom word-breaking procedure
 \date		Created  on Sun Jun  4 23:29:10 2023
 \date		Modified on Sun Jun  4 23:29:10 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
static int CALLBACK WordBreakProc(LPTSTR lpch, int ichCurrent, int cch, int code)
{
    /*
	 *  Break the line each time margin is reached.
	 */
    switch (code)
    {
        case WB_ISDELIMITER:
                return TRUE;
            break;
        case WB_LEFT:
        case WB_RIGHT:
        case WB_MOVEWORDLEFT:
        case WB_MOVEWORDRIGHT:
            return FALSE;
    }
    return TRUE;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		ToolTipAdd
 \date		Created  on Mon Jun  5 22:14:28 2023
 \date		Modified on Mon Jun  5 22:14:28 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
BOOL ToolTipAdd(HWND hToolTip, HWND hDlg, int toolID, PTSTR pszText)
{
	if (!hToolTip | !toolID || !hDlg || !pszText)
	{
		return FALSE;
	}

	// Get the window of the tool.
	HWND hwndTool = GetDlgItem(hDlg, toolID);
	if (!hwndTool)
	{
		return FALSE;
	}

	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize   = sizeof(toolInfo);
	toolInfo.hwnd     = hDlg;
	toolInfo.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId      = (UINT_PTR)hwndTool;
	toolInfo.lpszText = pszText;
	SendMessage(hToolTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

	return TRUE;
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

	/*
	 * Adjust margins and auwrap text at margins.
	 */
    DWORD margins = MAKELONG(5, 5);
    SendMessage(GetDlgItem(hDlg, ID_EDIT), EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, margins);
	SendMessage(GetDlgItem(hDlg, ID_EDIT), EM_SETWORDBREAKPROC, 0, (LPARAM)WordBreakProc);

    // Create the tooltip.
    hToolTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
                              WS_POPUP |TTS_ALWAYSTIP | TTS_BALLOON,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              hDlg, NULL, 
                              ghInstance, NULL);
	ToolTipAdd(hToolTip, hDlg, ID_BT_SEND_CMD,     "Send string to the uC. Select for raw or Hex send, or to append CR or LF or Both.");
	ToolTipAdd(hToolTip, hDlg, ID_BT_CONNECT,      "Connect or disconnect serial line.");
	ToolTipAdd(hToolTip, hDlg, ID_BT_CLR_SCR,      "Clear terminal contents.");
	ToolTipAdd(hToolTip, hDlg, ID_BT_SAVE,         "Save terminal contents to the selected file.");
	ToolTipAdd(hToolTip, hDlg, ID_BT_SELECT_IMAGE, "Open dialog for image file selection.");
	ToolTipAdd(hToolTip, hDlg, ID_BT_RESET,        "Reset the microcontroller.");
	ToolTipAdd(hToolTip, hDlg, ID_BT_UPDATE,       "Download image file.");
	ToolTipAdd(hToolTip, hDlg, ID_CB_SERIAL,       "Select the serial COM port to connect.");
	ToolTipAdd(hToolTip, hDlg, ID_CB_SPEED,        "Select the serial connection speed.");
	ToolTipAdd(hToolTip, hDlg, ID_CB_uC,           "Select the microcontroller, W800 or W600 series.");
	ToolTipAdd(hToolTip, hDlg, ID_CHK_HEX,         "Set terminal to HEX mode.");
	ToolTipAdd(hToolTip, hDlg, ID_CHK_ERASE,       "Erase FLASH before to download firmware.");
	ToolTipAdd(hToolTip, hDlg, ID_ED_IMG_FILE,     "File to download.");
	ToolTipAdd(hToolTip, hDlg, ID_ED_CMD,          "string to send.");
}
