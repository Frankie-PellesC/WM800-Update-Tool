#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#include "Update_tool.h"
#include "serial.h"
#include "controls.h"
#include "crc-ccitt.h"
#include "rom.h"

/*
 * ROM monitor commands table.
 */
const w800_cmd_table_t cmd_table[] =
{
	{ 0x31, NULL,    4 },						// Baude rate setting. Max allowed speed 2MBaude
	{ 0x32, NULL, sizeof(w800_erase_t) },		// QFlash erase
	{ 0x33, NULL,    6 },						// Set BT MAC 48bits
	{ 0x33, NULL,    8 },						// Set BT MAC 64bits
	{ 0x34, "MAC:",  0 },						// Get BT MAC (48-64 bits)
	{ 0x35, NULL,   84 },						// Set WiFi TX Gain parameters (best don't modify!)
	{ 0x36,  "G:",   0 },						// Get WiFi TX Gain parameters
	{ 0x37, NULL,    6 },						// Set WiFi MAC 48bits
	{ 0x37, NULL,    8 },						// Set WiFi MAC 64bits
	{ 0x38, "MAC:",  0 },						// Get WiFi MAC (48-64 bits)
	{ 0x3B, "ERR:",  0 },						// Get last error
	{ 0x3C, "FID:",  0 },						// Get QFLASH ID and content quantity. I.e. GD 32MB returns: FID:C8,19 - PUYA 8MB returns: FID:85,17
	{ 0x3E, "R:",    0 },						// Get ROM version
	{ 0x3F, NULL,    0 }						// System restart
};

#define MAX_RX_RETRY			200
#define FRAME_START_CHAR		('!')			// 0X21
#define FRAME_END_CHAR			('\n')
#define BUF_SIZE				1024
#define CRC_CCITT_FALSE_INI_VAL	0xffff

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		ExecRomCmd
 \date		Created  on Sun May 28 19:57:30 2023
 \date		Modified on Sun May 28 19:57:30 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int ExecRomCmd(HANDLE hSerCom, uint8_t rom_cmd, void *data, void *answer)
{
	char buf[BUF_SIZE];
	cmd_t *cmd = (cmd_t *)buf;
	int    ret = RET_ERROR;

	/*
	 * Prepare the command frame.
	 */
	cmd->magic = FRAME_START_CHAR;
	cmd->len   = (uint16_t)(cmd_table[rom_cmd].datalen + sizeof(cmd->cmd) + sizeof(cmd->crc));	// len includes also the CRC
	cmd->cmd   = cmd_table[rom_cmd].cmd;

	/*
	 * copy data if required
	 */
	if (cmd_table[rom_cmd].datalen)
		memcpy(cmd->data, data, cmd_table[rom_cmd].datalen);

	/*
	 * Compute the CRC-16/CCITT-FALSE for the frame starting from command.
	 */ 
	cmd->crc   = crc_ccitt_false(CRC_CCITT_FALSE_INI_VAL, (uint8_t *)&cmd->cmd,
													cmd->len - sizeof(cmd->crc));

	/*
	 * Clear serial buffer and set receiver in block mode, then send frame.
	 */
	PurgeComm(hSerCom, PURGE_RXCLEAR);
	SetBlockingRx(hSerCom, TRUE);
	int err = WriteSerial(hSerCom, cmd, sizeof(cmd_t) + cmd_table[rom_cmd].datalen);
	if (err <= 0)
	{
		ret = RET_ERROR;
		goto end;
	}

	/*
	 * If no header is defined there is no answer.
	 */
	if (!cmd_table[rom_cmd].header)
	{
		ret = RET_SUCCESS;
		goto end;
	}

	/*
	 * Prepare for answer extraction:
	 *  - Wait for the header characters.
	 *  - Store characters in correct sequence.
	 *  - Store data up to the closing char
	 */
	int cnt = 0;
	int idx = 0;
	do
	{
		if (cnt++ > MAX_RX_RETRY)
		{
			ret = RET_ERROR;
			goto end;
		}
		err = ReadSerial(hSerCom, &buf[idx], 1);
		if (err <= 0)
			break;
		if (toupper(buf[idx]) != cmd_table[rom_cmd].header[idx])
			continue;
		if (++idx == strlen(cmd_table[rom_cmd].header))
			break;
	} while (TRUE);

	if (err <= 0)
	{
		ret = RET_ERROR;
		goto end;
	}

	do
	{
		if (cnt++ > MAX_RX_RETRY)
		{
			ret = RET_ERROR;
			goto end;
		}
		err = ReadSerial(hSerCom, &buf[idx], 1);
		if (err <= 0)
			break;
		if (buf[idx++] != FRAME_END_CHAR)
			continue;
		buf[idx-1] = '\0';
		break;
	} while (TRUE);

	if (err <= 0)
	{
		ret = RET_ERROR;
		goto end;
	}

	/*
	 * Copy received frame data.
	 */
	strcpy(answer, buf);

	/*
	 * Return the received string length.
	 */
	ret = idx;
end:
	SetBlockingRx(hSerCom, FALSE);
	return ret;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		copy_mac
 \date		Created  on Mon May 29 00:12:47 2023
 \date		Modified on Mon May 29 00:12:47 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
static char *copy_mac(uint8_t *src, uint8_t *dst)
{
	int len = 0;

	for (int idx = 0; src[idx]; idx += 2)
	{
		if (idx > 0)
			dst[len++] = '-';
		dst[len++] = src[idx];
		dst[len++] = src[idx+1];
	}
	dst[len]   = '\0';

	return (char *)dst;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		QueryMAC
 \date		Created  on Mon May 22 16:51:13 2023
 \date		Modified on Mon May 22 16:51:13 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int QueryMAC(HWND hDlg, HANDLE hSerCom, uint8_t cmd)
{
	uint8_t buf[128];
	char    out[128];
	int ret = RET_ERROR;

	PurgeComm(hSerCom, PURGE_RXCLEAR);

	SetBlockingRx(hSerCom, TRUE);

	int len = ExecRomCmd(hSerCom, cmd, NULL, buf);		// Get WiFi MAC
	if (len <= 0)
		goto end;

	AppendText2Edit(hDlg, ID_EDIT, cmd==w800_cmd_get_wifi_mac ? "Wi-Fi MAC " : "BT MAC ");
	AppendText2Edit(hDlg, ID_EDIT, copy_mac(buf+4, (uint8_t *)out));
	AppendText2Edit(hDlg, ID_EDIT, "\r\n");
	ret = 0;

end:
	SetBlockingRx(hSerCom, FALSE);
	return ret;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		FlashErase
 \date		Created  on Mon May 22 17:25:58 2023
 \date		Modified on Mon May 22 17:25:58 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int FlashErase(HWND hDlg, HANDLE hSerCom)
{
	int pb_cnt = 1;
	int error  = 0;
	int cnt    = 0;
	int ret    = RET_ERROR;
	unsigned char ch;
	unsigned char rom_cmd[] = {0x21, 0x0a, 0x00, 0xc3, 0x35, 0x32, 0x00, 0x00, 0x00, 0x02, 0x00, 0xfe, 0x01};  /*2M-8k*/

	AppendText2Edit(hDlg, ID_EDIT, "Erasing flash...\r\n");
	SendMessage(GetDlgItem(hDlg, ID_LBL_PROGRESS_DESC), WM_SETTEXT, 0, (LPARAM)"Erasing flash...");
	PurgeComm(hSerCom, PURGE_RXCLEAR);

	ret = WriteSerial(hSerCom, rom_cmd, sizeof(rom_cmd));
	if (ret <= 0)
		return RET_ERROR;

	do
	{
		Sleep(50);
		ret = ReadSerial(hSerCom, &ch, 1);
		if (ret > 0)
		{
			if (('C' == ch) || ('P' == ch))
			{
				cnt++;
				if (ch != 'C')
					error = ch;
			}
			else
				cnt = 0;
		}
		else if (ret == 0)
		{
			SendMessage(GetDlgItem(hDlg, ID_PROGRESS), PBM_SETPOS, (WPARAM)pb_cnt++, 0);
			if (pb_cnt == 100)
				pb_cnt = 1;
		}
		else
		{
			MessageBox(hDlg, "Erase error.", "Error", MB_ICONERROR|MB_OK);
			SetBlockingRx(hSerCom, FALSE);
			return RET_ERROR;
		}
	} while (cnt < 3);

	SetBlockingRx(hSerCom, FALSE);
	AppendText2Edit(hDlg, ID_EDIT, "Erase finished.\r\n");
	SendMessage(GetDlgItem(hDlg, ID_LBL_PROGRESS_DESC), WM_SETTEXT, 0, (LPARAM)"");

	return RET_SUCCESS;
}
