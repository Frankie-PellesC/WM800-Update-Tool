#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "Update_tool.h"

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		AboutDlgProcedure
 \date		Created  on Sat Jun  3 11:57:02 2023
 \date		Modified on Sat Jun  3 11:57:02 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
static INT_PTR CALLBACK AboutDlgProcedure(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDR_ICO_MAIN)));
			SendMessage(GetDlgItem(hDlg, ID_LBL_VERSION), WM_SETTEXT, 0, (LPARAM)"Update Tool V." UPDATE_TOOL_VERSION " (c) frankie 2023");
			return TRUE;
		}

		case WM_SIZE:
			return TRUE;

		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK:
				EndDialog(hDlg, 0);
				return TRUE;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg, 0);
			return TRUE;
	}

	return FALSE;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		About
 \date		Created  on Sat May 27 21:08:53 2023
 \date		Modified on Sat Jun  3 11:56:33 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void About(HWND hDlg)
{
	DialogBox(ghInstance, MAKEINTRESOURCE(DLG_ABOUT), hDlg, AboutDlgProcedure);
}
