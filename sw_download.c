#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "Update_tool.h"
#include "controls.h"
#include "serial.h"
#include "rom.h"
#include "xmodem.h"

#ifndef DBG_PRT
#define DBG_PRT
#endif

#define FN_TIMEOUT				500
#define DOWNLOAD_TIMEOUT_SEC	(60 * 1)

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		DownloadFirmware
 \date		Created  on Mon May 22 16:38:58 2023
 \date		Modified on Mon May 22 16:38:58 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int DownloadFirmware(HWND hDlg, HANDLE hSerCom, const char *image)
{
	int           ret     = RET_SUCCESS;
	int           cnt     = 0;
	int           note    = 1;
	int           timeout = 0;
	double        timeuse;
	time_t        start, end;
	unsigned char ch;

	if (!image || !*image)
	{
		MessageBox(hDlg, "Invalid image file.", "Error", MB_ICONERROR|MB_OK);
		return RET_ERROR;
	}

	/*
	 * Wait a while to be sure that COM port is initialized.
	 */
	Sleep(FN_TIMEOUT);

	/*
	 * Set device in update mode.
	 */
	ret = SetUpdateMode(hSerCom);
	if (ret < 0)
	{
		MessageBox(hDlg, "Set rts to reboot error.", "Error", MB_ICONERROR|MB_OK);
		return RET_ERROR;
	}

	/*
	 * Wait serial sync...
	 */
	EscapeCommunication(hSerCom, FN_TIMEOUT);  /* used for delay */

	StopRxThread(hDlg);

	start = time(NULL);

	do
	{
		ret = ReadSerial(hSerCom, &ch, 1);

		DBG_PRT("ret=%d, %x-%c\n", ret, ch, ch);

		if (ret > 0)
		{
			if (('C' == ch) || ('P' == ch))
				cnt++;
			else
				cnt = 0;
		}
		else
		{
			EscapeCommunication(hSerCom, 30);
		}

		end = time(NULL);
		timeuse = difftime(end, start);
		if (timeuse >= 1)
		{
			timeout++;
			if ((timeout >= (DOWNLOAD_TIMEOUT_SEC / 10)) && note)
			{
				ret = MessageBox(hDlg, "Please manually reset the device.",
					"User action required", MB_ICONEXCLAMATION|MB_RETRYCANCEL);
				if (ret == IDCANCEL)
				{
					ret = RET_ERROR;
					goto end;
				}
				note = 0;
			}
			else if (timeout > DOWNLOAD_TIMEOUT_SEC)
			{
				MessageBox(hDlg, "Serial sync timeout.", "Error", MB_ICONERROR|MB_OK);
				ret = RET_ERROR;
				goto end;
			}

			start = time(NULL);
		}
	} while (cnt < 3);

	AppendText2Edit(hDlg, ID_EDIT, "\r\n");

	ret  = QueryMAC(hDlg, hSerCom, w800_cmd_get_wifi_mac);
	ret |= QueryMAC(hDlg, hSerCom, w800_cmd_get_bt_mac);
	if (ret != RET_SUCCESS)
	{
		MessageBox(hDlg, "Download failed.", "Error", MB_ICONERROR|MB_OK);
		goto end;
	}

	if (GetCheckBoxState(hDlg, ID_CHK_ERASE))
	{
		ret = FlashErase(hDlg, hSerCom);
		EscapeCommunication(hSerCom, 500);
		if (ret)
		{
			MessageBox(hDlg, "Erase failed.", "Error", MB_ICONERROR|MB_OK);
			goto end;
		}
	}

	/*
	 * Download phase.
	 */
	SendMessage(GetDlgItem(hDlg, ID_LBL_PROGRESS_DESC), WM_SETTEXT, 0, (LPARAM)"Downloading image...");

	/*
	 * Set the selected download speed
	 */
	uint32_t cur_speed = GetDlgItemInt(hDlg, ID_CB_SPEED, NULL, FALSE);
	uint32_t dl_speed  = GetDownloadSpeed();
	if (cur_speed != dl_speed)
		SetDevSpeed(hSerCom, dl_speed);

	/*
	 * Xmodem transfer image.
	 */
	ret = XmodemDownload(hDlg, hSerCom, image);

	/*
	 * Restore serial speed
	 */
	if (cur_speed != dl_speed)
		SetDevSpeed(hSerCom, cur_speed);

	if (ret != RET_SUCCESS)
		SetUpdateMode(hSerCom);
end:
	SendMessage(GetDlgItem(hDlg, ID_LBL_PROGRESS_DESC), WM_SETTEXT, 0, (LPARAM)"");
	StartRxThread(hDlg, hSerCom);
	Reset_uC(hDlg, hSerCom);

	return ret;
}
