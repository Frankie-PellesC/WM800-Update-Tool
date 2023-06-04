#define WIN32_LEAN_AND_MEAN
#define __STDC_WANT_LIB_EXT2__ 1

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include "Update_tool.h"
#include "serial.h"
#include "rom.h"
#include "controls.h"

enum eSendType SendType = SendStringNormal;

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		GetSelectedCbString
 \date		Created  on Sun May 21 15:08:00 2023
 \date		Modified on Sun May 21 15:08:00 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
size_t GetSelectedCbString(HWND hDlg, int ID_CB, char *szString, size_t size_string)
{
	*szString = '\0';

	HWND comboBox = GetDlgItem(hDlg, ID_CB); // Obtain the handle to the combobox control

	LRESULT selectedIndex = SendMessage(comboBox, CB_GETCURSEL, 0, 0); // Get the index of the selected item

	if (selectedIndex == CB_ERR)
		return 0;

	size_t req_len = SendMessage(comboBox, CB_GETLBTEXTLEN, (WPARAM)selectedIndex, 0);

	if (req_len < size_string)
	    SendMessage(comboBox, CB_GETLBTEXT, (WPARAM)selectedIndex, (LPARAM)szString); // Retrieve the text of the selected item

	return req_len+1;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		MoveCursorToEditBoxEnd
 \date		Created  on Mon May 22 16:07:46 2023
 \date		Modified on Mon May 22 16:07:46 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void MoveCursorToEditBoxEnd(HWND hDlg, int IdCtrl)
{
	HWND hEditControl = GetDlgItem(hDlg, IdCtrl);  // Handle to the edit control
	if (!hEditControl)
		return;

    int textLength = GetWindowTextLength(hEditControl);
    SendMessage(hEditControl, EM_SETSEL, (WPARAM)textLength, (LPARAM)textLength);
}

#define MAX_EDIT_BUF	32768
#define CLEAN_EDIT_BUF	(MAX_EDIT_BUF/4)
#define LIMIT_EDIT_BUF	(CLEAN_EDIT_BUF*3)

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		AppendText2Edit
 \date		Created  on Sun May 21 16:01:45 2023
 \date		Modified on Sun May 21 16:01:45 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void AppendText2Edit(HWND hDlg, int IdEdit, char *szText)
{
	HWND editHandle = GetDlgItem(hDlg, IdEdit); // Obtain the handle to the edit control

	// Check if buffer si getting full, and in case remove old data
	int len = GetWindowTextLength(editHandle);
	if ( len > LIMIT_EDIT_BUF)
	{
		SendMessage(editHandle, EM_SETSEL, (WPARAM)0, (LPARAM)CLEAN_EDIT_BUF);	// Set the selection to the end of the text
		SendMessage(editHandle, EM_REPLACESEL, TRUE, (LPARAM)"");				// Replace the selection with the new text
	}

	MoveCursorToEditBoxEnd(hDlg, IdEdit);

	/*
	 * Append text to the edit control
	 */
	SendMessage(editHandle, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);			// Set the selection to the end of the text
	SendMessage(editHandle, EM_REPLACESEL, TRUE, (LPARAM)szText);		// Replace the selection with the new text
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		AppendText2EditFmt
 \date		Created  on Wed May 24 16:52:57 2023
 \date		Modified on Wed May 24 16:52:57 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void VAFMT34 AppendText2EditFmt(HWND hDlg, int IdEdit, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char *szText = NULL;
	if (vasprintf(&szText, fmt, ap) && szText)
	{
		AppendText2Edit(hDlg, IdEdit, szText);
		free(szText);
	}
	va_end(ap);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		MessageBoxf
 \date		Created  on Wed May 31 22:30:07 2023
 \date		Modified on Wed May 31 22:30:07 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int VAFMT45 MessageBoxf(HWND hWnd, LPCTSTR lpCaption, UINT uType, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char *szText = NULL;
	int result = 0;

	if (vasprintf(&szText, fmt, ap) && szText)
	{
		result = MessageBox(hWnd, szText, lpCaption, uType);
		free(szText);
	}
	va_end(ap);
	return result;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		GetCheckBoxState
 \date		Created  on Sun May 21 16:15:58 2023
 \date		Modified on Sun May 21 16:15:58 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
BOOL GetCheckBoxState(HWND hDlg, int IDC_CHECKBOX)
{
	HWND checkboxHandle = GetDlgItem(hDlg, IDC_CHECKBOX); // Obtain the handle to the checkbox control

	UINT state = (UINT)SendMessage(checkboxHandle, BM_GETCHECK, 0, 0); // Get the state of the checkbox

	/*
	 * Check the state to determine if the checkbox is checked or unchecked
	 */
	return (state == BST_CHECKED) ? TRUE : FALSE;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		EnableControlsList
 \date		Created  on Sat May 27 17:30:19 2023
 \date		Modified on Sat May 27 17:30:19 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void EnableControlsList(HWND hDlg, const int *controls, const int nControls)
{
	for (int i=0; i<nControls; i++)
		EnableWindow(GetDlgItem(hDlg, controls[i]), TRUE);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		DisableControlsList
 \date		Created  on Sat May 27 17:30:19 2023
 \date		Modified on Sat May 27 17:30:19 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void DisableControlsList(HWND hDlg, const int *controls, const int nControls)
{
	for (int i=0; i<nControls; i++)
		EnableWindow(GetDlgItem(hDlg, controls[i]), FALSE);
}

extern HMENU hDlSpeedMenu;

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		GetDownloadSpeed
 \date		Created  on Mon May 29 00:52:19 2023
 \date		Modified on Mon May 29 00:52:19 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
uint32_t GetDownloadSpeed(void)
{
	MENUITEMINFO mii;
	int i;

	mii.cbSize = sizeof(mii);
	mii.fMask  = MIIM_STATE;

	for (i=0; i<GetMenuItemCount(hDlSpeedMenu); i++)
	{
		GetMenuItemInfo(hDlSpeedMenu, i, TRUE, &mii);
		if (mii.fState & MFS_CHECKED)
			break;
	}

	switch(i)
	{
		case 0:
			return  115200L;
		case 1:
			return  460800L;
		case 2:
			return  921600L;
		case 3:
			return 1000000L;
	}

	return 2000000L;
}

#define RESET_DELAY_MS	500

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		Reset_uC
 \date		Created  on Sun May 21 17:48:03 2023
 \date		Modified on Sun May 21 17:48:03 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void Reset_uC(HWND hDlg, HANDLE hSerCom)
{
	EnableWindow(GetDlgItem(hDlg, ID_BT_RESET), FALSE);
	EscapeCommFunction(hSerCom, SETRTS);
	Sleep(RESET_DELAY_MS);
	EscapeCommFunction(hSerCom, CLRRTS);
	EnableWindow(GetDlgItem(hDlg, ID_BT_RESET), TRUE);
	HMENU hMenu = GetSubMenu(GetMenu(hDlg), 1);
	EnableMenuItem(hMenu, ID_MENU_QRY_WIFI,    MF_GRAYED );
	EnableMenuItem(hMenu, ID_MENU_QRY_BT,      MF_GRAYED );
	EnableMenuItem(hMenu, ID_MENU_QRY_QFLASH,  MF_GRAYED );
	EnableMenuItem(hMenu, ID_MENU_QRY_ROM,     MF_GRAYED );
	EnableMenuItem(hMenu, ID_MENU_ROM_MONITOR, MF_ENABLED);
	StartRxThread(hDlg, hSerCom);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		HandleSlitbutton
 \date		Created  on Fri Jun  2 23:48:16 2023
 \date		Modified on Fri Jun  2 23:48:16 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
BOOL HandleSlitbutton(HWND hDlg, LPARAM lParam)
{
    switch (((LPNMHDR)lParam)->code)
    {
        case BCN_DROPDOWN:
        {
            NMBCDROPDOWN* pDropDown = (NMBCDROPDOWN*)lParam;
            if (pDropDown->hdr.hwndFrom == GetDlgItem(hDlg, ID_BT_SEND_CMD))
            {

                // Get screen coordinates of the button.
                POINT pt;
                pt.x = pDropDown->rcButton.left;
                pt.y = pDropDown->rcButton.bottom;
                ClientToScreen(pDropDown->hdr.hwndFrom, &pt);
        
                // Create a menu and add items.
                HMENU hSplitMenu = CreatePopupMenu();
                AppendMenu(hSplitMenu, MF_BYPOSITION, SendStringNormal, "Send string");
                AppendMenu(hSplitMenu, MF_BYPOSITION, SendStringCR,     "Send string + \\r");
                AppendMenu(hSplitMenu, MF_BYPOSITION, SendStringNL,     "Send string + \\n");
                AppendMenu(hSplitMenu, MF_BYPOSITION, SendStringCRNL,   "Send string + \\r\\n");
                AppendMenu(hSplitMenu, MF_BYPOSITION, SendStringHex,    "Send string Hex");
        
                // Display the menu.
                TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hDlg, NULL);
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		UpdateSendButton
 \date		Created  on Sat Jun  3 00:28:34 2023
 \date		Modified on Sat Jun  3 00:28:34 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void UpdateSendButton(HWND hDlg, enum eSendType type)
{
	switch(type)
	{
		case SendStringNormal:
			SendMessage(GetDlgItem(hDlg, ID_BT_SEND_CMD), WM_SETTEXT, 0, (LPARAM)"Send");
			SendType = SendStringNormal;
			break;
		case SendStringCR:
			SendMessage(GetDlgItem(hDlg, ID_BT_SEND_CMD), WM_SETTEXT, 0, (LPARAM)"Send+\\r");
			SendType = SendStringCR;
			break;
		case SendStringNL:
			SendMessage(GetDlgItem(hDlg, ID_BT_SEND_CMD), WM_SETTEXT, 0, (LPARAM)"Send+\\n");
			SendType = SendStringNL;
			break;
		case SendStringCRNL:
			SendMessage(GetDlgItem(hDlg, ID_BT_SEND_CMD), WM_SETTEXT, 0, (LPARAM)"Send+\\r\\n");
			SendType = SendStringCRNL;
			break;
		case SendStringHex:
			SendMessage(GetDlgItem(hDlg, ID_BT_SEND_CMD), WM_SETTEXT, 0, (LPARAM)"Send Hex");
			SendType = SendStringHex;
			break;
	}
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		RomMonitor
 \date		Created  on Sat Jun  3 14:35:03 2023
 \date		Modified on Sat Jun  3 14:35:03 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int RomMonitor(HWND hDlg, HANDLE hSerCom)
{
	int res = MessageBox(hDlg,  "This action will stop device execution\n"
								"and enters the ROM monitor.\n"
								"Stop the execution ?", "Execution Stop", MB_YESNO|MB_ICONQUESTION);
	if (res == IDNO)
		return RET_ERROR;


	/*
	 * Set device in update mode.
	 */
	int ret = SetUpdateMode(hSerCom);
	if (ret < 0)
	{
		MessageBox(hDlg, "Cannot enter ROM monitor.", "Error", MB_ICONERROR|MB_OK);
		return RET_ERROR;
	}

	/*
	 * Wait serial sync...
	 */
	EscapeCommunication(hSerCom, RESET_DELAY_MS);  /* used for delay */

	HMENU hMenu = GetSubMenu(GetMenu(hDlg), 1);
	EnableMenuItem(hMenu, ID_MENU_QRY_WIFI,    MF_ENABLED);
	EnableMenuItem(hMenu, ID_MENU_QRY_BT,      MF_ENABLED);
	EnableMenuItem(hMenu, ID_MENU_QRY_QFLASH,  MF_ENABLED);
	EnableMenuItem(hMenu, ID_MENU_QRY_ROM,     MF_ENABLED);
	EnableMenuItem(hMenu, ID_MENU_ROM_MONITOR, MF_GRAYED);

	return RET_SUCCESS;
}

#define BUF_SIZE	128

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		RomQuery
 \date		Created  on Sat Jun  3 13:07:19 2023
 \date		Modified on Sat Jun  3 13:07:19 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void RomQuery(HWND hDlg, HANDLE hSerCom, int type)
{
	uint8_t buf[BUF_SIZE];
	uint8_t cmd;

	PurgeComm(hSerCom, PURGE_RXCLEAR);
	StopRxThread(hDlg);
	SetBlockingRx(hSerCom, TRUE);
	AppendText2Edit(hDlg, ID_EDIT, "\r\n");

	switch(type)
	{
		case ID_MENU_QRY_WIFI:
			cmd = w800_cmd_get_wifi_mac;
			AppendText2Edit(hDlg, ID_EDIT, "WiFi MAC: ");
			break;

		case ID_MENU_QRY_BT:
			cmd = w800_cmd_get_bt_mac;
			AppendText2Edit(hDlg, ID_EDIT, "BT MAC: ");
			break;

		case ID_MENU_QRY_QFLASH:
			cmd = w800_cmd_get_qflash_id;
			AppendText2Edit(hDlg, ID_EDIT, "QFLASH ID: ");
			break;

		case ID_MENU_QRY_ROM:
			cmd = w800_cmd_get_rom_version;
			AppendText2Edit(hDlg, ID_EDIT, "ROM Version: ");
			break;

		default:
			goto end;
	}

	int len = ExecRomCmd(hSerCom, cmd, NULL, buf);		// Query ROM monitor
	if (len <= 0)
		goto end;

	AppendText2Edit(hDlg, ID_EDIT, (char *)(buf + strlen(cmd_table[cmd].header)));
	AppendText2Edit(hDlg, ID_EDIT, "\r\n");

end:
	SetBlockingRx(hSerCom, FALSE);
	StartRxThread(hDlg, hSerCom);
}
