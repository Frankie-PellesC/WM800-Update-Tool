#ifndef CONTROLS_H__
#define CONTROLS_H__
#pragma once
#include <stdint.h>

#define VAFMT(f,n,m)	__declspec(vaformat(f,n,m))
#define VAFMT34			__declspec(vaformat(printf,3,4))
#define VAFMT45			__declspec(vaformat(printf,4,5))

enum eSendType
{
	SendStringNormal = ID_MENU_INPUT_NORMAL,
	SendStringCR     = ID_MENU_INPUT_CR,
	SendStringNL     = ID_MENU_INPUT_NL,
	SendStringCRNL   = ID_MENU_INPUT_CRNL,
	SendStringHex    = ID_MENU_INPUT_HEX
};

size_t GetSelectedCbString(HWND hDlg, int ID_CB, char *szString, size_t size_string);
void MoveCursorToEditBoxEnd(HWND hDlg, int IdCtrl);
void AppendText2Edit(HWND hDlg, int IdEdit, char *szText);
void VAFMT34 AppendText2EditFmt(HWND hDlg, int IdEdit, char *fmt, ...);
int VAFMT45 MessageBoxf(HWND hWnd, LPCTSTR lpCaption, UINT uType, char *fmt, ...);
BOOL GetCheckBoxState(HWND hDlg, int IDC_CHECKBOX);
void EnableControlsList(HWND hDlg, const int *controls, const int nControls);
void DisableControlsList(HWND hDlg, const int *controls, const int nControls);
uint32_t GetDownloadSpeed(void);
void Reset_uC(HWND hDlg, HANDLE hSerCom);

BOOL HandleSlitbutton(HWND hDlg, LPARAM lParam);
void UpdateSendButton(HWND hDlg, enum eSendType type);
int RomMonitor(HWND hDlg, HANDLE hSerCom);
void RomQuery(HWND hDlg, HANDLE hSerCom, int type);

void About(HWND hDlg);

extern enum eSendType SendType;

#endif	// CONTROLS_H__
