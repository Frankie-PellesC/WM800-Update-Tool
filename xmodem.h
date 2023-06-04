#ifndef XMODEM_H__
#define XMODEM_H__
#pragma once

long int FileSize(const char* filename);
int XmodemDownload(HWND hDlg, HANDLE hSerCom, const char *image);

#endif	// XMODEM_H__
