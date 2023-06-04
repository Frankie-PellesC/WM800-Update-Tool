#ifndef ROM_H__
#define ROM_H__
#pragma once
#include <stdint.h>

enum
{
	w800_cmd_set_bauderate = 0,		// Baude rate setting. Max allowed speed 2MBaude
	w800_cmd_qflash_erase,			// QFlash erase
	w800_cmd_set_bt_mac48,			// Set BT MAC 48bits
	w800_cmd_set_bt_mac64,			// Set BT MAC 64bits
	w800_cmd_get_bt_mac,			// Get BT MAC (48-64 bits)
	w800_cmd_set_wifi_gain,			// Set WiFi TX Gain parameters (best don't modify!)
	w800_cmd_get_wifi_gain,			// Get WiFi TX Gain parameters
	w800_cmd_set_wifi_mac48,		// Set WiFi MAC 48bits
	w800_cmd_set_wifi_mac64,		// Set WiFi MAC 64bits
	w800_cmd_get_wifi_mac,			// Get WiFi MAC (48-64 bits)
	w800_cmd_get_last_error,		// Get last error
	w800_cmd_get_qflash_id,			// Get QFLASH ID and content quantity. I.e. GD 32MB returns: FID:C8,19 - PUYA 8MB returns: FID:85,17
	w800_cmd_get_rom_version,		// Get ROM version
	w800_cmd_system_restart			// System restart
};

#pragma pack(push)
#pragma pack(1)
typedef struct
{
	uint8_t  magic;
	uint16_t len;
	uint16_t crc;
	uint32_t cmd;
	uint8_t  data[];
} cmd_t;

typedef struct
{
	uint16_t index:15;		// Starting position
	uint16_t ModeFlag:1;	// If set == Block Erase, 0 == Sector erase.
	uint16_t count;			// Number of blocks to erase
} w800_erase_t;
#pragma pack(pop)

typedef struct
{
	uint32_t cmd;
	char     *header;
	uint32_t datalen;
} w800_cmd_table_t;

extern const w800_cmd_table_t cmd_table[];

int ExecRomCmd(HANDLE hSerCom, uint8_t rom_cmd, void *data, void *answer);
int QueryMAC(HWND hDlg, HANDLE hSerCom, uint8_t cmd);
int FlashErase(HWND hDlg, HANDLE hSerCom);

#endif	// ROM_H__
