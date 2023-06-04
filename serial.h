#ifndef SERIAL_H__
#define SERIAL_H__
#pragma once

#define DOWNLOAD_SPEED_MAX	2000000

int    ReadSerial(HANDLE hSerCom, void *data, unsigned int size);
int    WriteSerial(HANDLE hSerCom, const void *data, unsigned int size);
void   SetBlockingRx(HANDLE hSerCom, BOOL bBlock);
static DWORD WINAPI ThreadRxFn(LPVOID arg);
HANDLE StartRxThread(HWND hDlg, HANDLE hCom);
void   StopRxThread(HWND hDlg);
int    SetSerialSpeed(HANDLE hSerCom, int speed);
HANDLE ComOpen(const char *device, int speed);
HANDLE DoConnect(HWND hDlg);
void   Disconnect(HWND hDlg, HANDLE *hCon);
int    SetUpdateMode(HANDLE hSerCom);
int    EscapeCommunication(HANDLE hSerCom, int ms);
int    SetDevSpeed(HANDLE hSerCom, uint32_t speed);

extern const int aCmdCtls[] ;
extern const int iCmdCtlsSize;
extern const int aConnectCtls[];
extern const int iConnectCtlsSize;

#endif	// SERIAL_H__
