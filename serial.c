#define WIN32_LEAN_AND_MEAN
#define __STDC_WANT_LIB_EXT2__ 1

#include <windows.h>
#include <stdio.h>
#include "Update_tool.h"
#include "controls.h"
#include "rom.h"
#include "serial.h"

static HANDLE hRxThread          = NULL;
static BOOL   bRunRxThread       = FALSE;
static DWORD dwUartReadBlockMode = 0;

const int aConnectCtls[]   = {ID_CB_SERIAL, ID_CB_SPEED};
const int iConnectCtlsSize = NELEMS(aConnectCtls);

const int aCmdCtls[]       = {ID_BT_UPDATE, ID_BT_RESET, ID_BT_SEND_CMD, ID_ED_CMD,
							 ID_CB_uC, ID_CHK_ERASE, ID_ED_IMG_FILE, ID_BT_SELECT_IMAGE};
const int iCmdCtlsSize     = NELEMS(aCmdCtls);

#define STABILIZE_DELAY_MS	50

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		SetUpdateMode
 \date		Created  on Mon May 29 01:08:43 2023
 \date		Modified on Mon May 29 01:08:43 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int SetUpdateMode(HANDLE hSerCom)
{
	int ret;
	ret = !EscapeCommFunction(hSerCom, CLRDTR);
	ret |= !EscapeCommFunction(hSerCom, SETRTS);
	Sleep(STABILIZE_DELAY_MS);
	ret = !EscapeCommFunction(hSerCom, SETDTR);
	ret |= !EscapeCommFunction(hSerCom, CLRRTS);
	Sleep(STABILIZE_DELAY_MS);
	ret |= !EscapeCommFunction(hSerCom, CLRDTR);
	return ret;
}

#define ESCAPE_DELAY	10	  /* 10-50ms */
#define ESCAPE_SYMBOL	0x1b

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		EscapeCommunication
 \date		Created  on Tue May 30 22:19:05 2023
 \date		Modified on Tue May 30 22:19:05 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int EscapeCommunication(HANDLE hSerCom, int ms)
{
	int err              = RET_SUCCESS;
	unsigned char escape = ESCAPE_SYMBOL;

	for (int i=0; i < (ms/ESCAPE_DELAY); i++)
	{
		err = WriteSerial(hSerCom, &escape, 1);
		Sleep(ESCAPE_DELAY);  /* 10-50ms */
	}

	return err;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		ReadSerial
 \date		Created  on Sun May 21 16:07:43 2023
 \date		Modified on Sun May 21 16:07:43 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int ReadSerial(HANDLE hSerCom, void *data, unsigned int size)
{
    BOOL ret;
    DWORD wait;
	unsigned long len = 0;
	COMSTAT state;
	DWORD error;
    OVERLAPPED overlap;

   	overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    do
    {
    	memset(&overlap, 0, sizeof(OVERLAPPED));

    	ClearCommError(hSerCom, &error, &state);

        ret = ReadFile(hSerCom, data, size, &len, &overlap);
        if (!ret)
        {
            if (ERROR_IO_PENDING == GetLastError())
    		{
    			wait = WaitForSingleObject(overlap.hEvent, dwUartReadBlockMode);
    			if (WAIT_OBJECT_0 == wait)
				{
					GetOverlappedResult(hSerCom, &overlap, &len, TRUE);
    			    ret = TRUE;
				}
    		}
			else
			{
				CloseHandle(overlap.hEvent);
				return RET_ERROR;
			}
        }
    } while (dwUartReadBlockMode && !len);

	CloseHandle(overlap.hEvent);

	return (int)len;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		WriteSerial
 \date		Created  on Wed May 31 21:28:34 2023
 \date		Modified on Wed May 31 21:28:34 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int WriteSerial(HANDLE hSerCom, const void *data, unsigned int size)
{
    BOOL          ret;
    DWORD         wait;
    OVERLAPPED    event;
    COMSTAT       state;
	DWORD         error;
    unsigned int  snd = 0;
	unsigned long len = 0;

    memset(&event, 0, sizeof(OVERLAPPED));

    event.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    do
    {
        ClearCommError(hSerCom, &error, &state);

        ret = WriteFile(hSerCom, (char *)data + snd, size - snd, &len, &event);
        if (!ret)
        {
            if (ERROR_IO_PENDING == GetLastError())
    		{
    			wait = WaitForSingleObject(event.hEvent, INFINITE);
    			if (WAIT_OBJECT_0 == wait)
			        ret = TRUE;
    		}
        }

        if (ret)
    	{
            if ((len > 0) && (snd != size))
                 snd += len;
             else
     	        return size;
     	}
    	else
    	{
            break;
    	}
    } while (TRUE);

	return RET_ERROR;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		SetBlockingRx
 \date		Created  on Wed May 31 21:20:11 2023
 \date		Modified on Wed May 31 21:20:11 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void SetBlockingRx(HANDLE hSerCom, BOOL bBlock)
{
	dwUartReadBlockMode = bBlock ? INFINITE : 0;
	return;
}

#define RX_BUF_SIZE	1024
typedef struct {HWND hDlg; HANDLE hCom;} RxThreadArg_t;

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		ThreadRxFn
 \date		Created  on Sun May 21 15:59:30 2023
 \date		Modified on Sun May 21 15:59:30 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
static DWORD WINAPI ThreadRxFn(LPVOID arg)
{
	HWND hDlg   = ((RxThreadArg_t *)arg)->hDlg;
	HANDLE hCom = ((RxThreadArg_t *)arg)->hCom;
	int ret, i, j=0;
	char buf[RX_BUF_SIZE];

    while (bRunRxThread)
    {
        ret = ReadSerial(hCom, buf, RX_BUF_SIZE);
		if (ret < 0)
		{
			/*
			 * This is a comm error!
			 * Quit thread and signal the error.
			 */
			DWORD error = GetLastError();
			MessageBoxf(hDlg, "Error.", MB_OK|MB_ICONERROR,
				"Error reading serial.\nError code %d (0x%02x).", error, error);
			bRunRxThread = FALSE;
			CloseHandle(hRxThread);
			hRxThread = NULL;
			ExitThread(RET_ERROR); 
		}
        if (ret > 0)
        {
            if (!GetCheckBoxState(hDlg, ID_CHK_HEX))
            {
                buf[ret] = '\0';
				AppendText2Edit(hDlg, ID_EDIT, buf);
            }
            else
            {
				char hex[4];
                for (i = 0; i < ret; i++, j++)
                {
                    sprintf(hex, "%02X ", buf[i]);
					AppendText2Edit(hDlg, ID_EDIT, hex);
                    if ((j + 1) % 16 == 0)
 						AppendText2Edit(hDlg, ID_EDIT, "\r\n");
                }
            }
        }
        else
        {
            Sleep(1);
        }
    }

	return RET_SUCCESS;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		StartRxThread
 \date		Created  on Sun May 21 15:54:10 2023
 \date		Modified on Sun May 21 15:54:10 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
HANDLE StartRxThread(HWND hDlg, HANDLE hCom)
{
	static RxThreadArg_t arg;

	if (bRunRxThread == TRUE)
		return hRxThread;

	arg.hDlg     = hDlg;
	arg.hCom     = hCom;
	bRunRxThread = TRUE;
    hRxThread    = CreateThread(0, 0, ThreadRxFn, (void *)&arg, 0, NULL);
	if (!hRxThread)
	{
		MessageBox(hDlg, "Internal error.\nCannot create RX thread.",
										"Error", MB_ICONERROR|MB_OK);
		ExitProcess(-1);
	}

    return hRxThread;
}

#define THREAD_STOP_TIMEOUT_MS	2000

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		StopRxThread
 \date		Created  on Sun May 21 20:50:51 2023
 \date		Modified on Sun May 21 20:50:51 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void StopRxThread(HWND hDlg)
{
	if (!hRxThread)
		return;
	bRunRxThread = FALSE;
	CancelSynchronousIo(hRxThread);
	DWORD rWait = WaitForSingleObject(hRxThread, THREAD_STOP_TIMEOUT_MS);
	if (rWait != WAIT_OBJECT_0)
	{
		if (!TerminateThread(hRxThread, 0))
		{
			MessageBox(hDlg, "Internal error.\nCannot end RX thread.",
										"Error", MB_ICONERROR|MB_OK);
			ExitProcess(-1);
		}
	}
	CloseHandle(hRxThread);
	hRxThread = NULL;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		SetSerialSpeed
 \date		Created  on Wed May 31 21:53:21 2023
 \date		Modified on Wed May 31 21:53:21 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int SetSerialSpeed(HANDLE hSerCom, int speed)
{
	DCB cfg;

	if (GetCommState(hSerCom, &cfg))
	{
		cfg.DCBlength         = sizeof(DCB);
		cfg.BaudRate          = speed;
		cfg.fBinary           = TRUE;
		cfg.fParity           = FALSE;
		cfg.fDtrControl       = DTR_CONTROL_DISABLE;
		cfg.fDsrSensitivity   = FALSE;
		cfg.fRtsControl       = RTS_CONTROL_DISABLE;
		cfg.ByteSize          = 8;
		cfg.StopBits          = ONESTOPBIT;
		cfg.fAbortOnError     = FALSE;
		cfg.fOutX             = FALSE;
		cfg.fInX              = FALSE;
		cfg.fErrorChar        = FALSE;
		cfg.fNull             = FALSE;
		cfg.fOutxCtsFlow      = FALSE;
		cfg.fOutxDsrFlow      = FALSE;
		cfg.Parity            = NOPARITY;
		cfg.fTXContinueOnXoff = FALSE;

		return SetCommState(hSerCom, &cfg) ? RET_SUCCESS : RET_ERROR;
	}
	else
	{
		return RET_ERROR;
	}
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		SetSerialTimeout
 \date		Created  on Wed May 31 21:57:12 2023
 \date		Modified on Wed May 31 21:57:12 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
static int SetSerialTimeout(HANDLE hSerCom)
{
    BOOL         ret;
    COMMTIMEOUTS timeout;

	timeout.ReadIntervalTimeout         = MAXDWORD;
	timeout.ReadTotalTimeoutConstant    = 0;
	timeout.ReadTotalTimeoutMultiplier  = 0;
	timeout.WriteTotalTimeoutConstant   = 0;
	timeout.WriteTotalTimeoutMultiplier = 0;

	ret = SetCommTimeouts(hSerCom, &timeout);
	if (ret)
	    ret = SetCommMask(hSerCom, EV_TXEMPTY);

    return ret ? RET_SUCCESS : RET_ERROR;
}

#define COMM_BUFFER_SIZE	4096

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		ComOpen
 \date		Created  on Wed May 31 21:43:15 2023
 \date		Modified on Wed May 31 21:43:15 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
HANDLE ComOpen(const char *device, int speed)
{
    char name[40];

	sprintf(name,"\\\\.\\%s", device);

	HANDLE hSerCom = CreateFile(name, GENERIC_WRITE | GENERIC_READ,
                                     0, NULL, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                     NULL);

	if (hSerCom == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

    if (!SetupComm(hSerCom, COMM_BUFFER_SIZE, COMM_BUFFER_SIZE))
		return NULL;

	if ( SetSerialSpeed(hSerCom, speed) | SetSerialTimeout(hSerCom))
        return NULL;

    return hSerCom;
}

#define COM_BUF_SIZE	16

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		DoConnect
 \date		Created  on Sun May 21 15:27:46 2023
 \date		Modified on Sun May 21 15:27:46 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
HANDLE DoConnect(HWND hDlg)
{
	char szCom[COM_BUF_SIZE];
	char *szMbMsg = NULL;

	DisableControlsList(hDlg, aConnectCtls, NELEMS(aConnectCtls));

	if (!GetSelectedCbString(hDlg, ID_CB_SERIAL, szCom, sizeof(szCom)) || !*szCom)
	{
		szMbMsg = "Invalid COM port!";
		goto error;
	}
	int speed = GetDlgItemInt(hDlg,ID_CB_SPEED, FALSE, FALSE);
	HANDLE hCom = ComOpen(szCom, speed);
	if (!hCom)
	{
		szMbMsg = "Can't open COM port!";
		goto error;
	}

	hRxThread = StartRxThread(hDlg, hCom);
    if (!hRxThread)
	{
		szMbMsg = "Internal error. Can't create Thread!";
		CloseHandle(hCom);
		hCom = NULL;
		goto error;
	}
	SetBlockingRx(hCom, FALSE);

	EnableControlsList(hDlg, aCmdCtls, NELEMS(aCmdCtls));
	SetWindowText(GetDlgItem(hDlg, ID_BT_CONNECT), "Disconnect");

	HMENU hMenu = GetSubMenu(GetMenu(hDlg), 1);
	EnableMenuItem(hMenu, ID_MENU_ROM_MONITOR, MF_ENABLED);

	return hCom;

error:
	MessageBox(hDlg, szMbMsg, "Error", MB_ICONERROR|MB_OK);
	EnableControlsList(hDlg, aConnectCtls, NELEMS(aConnectCtls));
	return NULL;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		Disconnect
 \date		Created  on Sun May 21 15:40:15 2023
 \date		Modified on Sun May 21 15:40:15 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
void Disconnect(HWND hDlg, HANDLE *hCom)
{
	if (hRxThread)
		StopRxThread(hDlg);
	CloseHandle(*hCom);
	*hCom = NULL;
	DisableControlsList(hDlg, aCmdCtls, iCmdCtlsSize);
	EnableControlsList(hDlg, aConnectCtls, iConnectCtlsSize);
	SetWindowText(GetDlgItem(hDlg, ID_BT_CONNECT), "Connect");
}

#define SPEED_CHANGE_ASSESS_MS	100

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		SetDevSpeed
 \date		Created  on Sun Jun  4 17:05:15 2023
 \date		Modified on Sun Jun  4 17:05:15 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int SetDevSpeed(HANDLE hSerCom, uint32_t speed)
{
	int ret = ExecRomCmd(hSerCom, w800_cmd_set_bauderate, (void *)&speed, NULL);
	if (ret == RET_SUCCESS)
	{
		Sleep(SPEED_CHANGE_ASSESS_MS);
		SetSerialSpeed(hSerCom, speed);
	}
	return ret;
}
