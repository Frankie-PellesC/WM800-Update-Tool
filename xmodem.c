#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include "Update_tool.h"
#include "controls.h"
#include "serial.h"
#include "crc-ccitt.h"
#include "xmodem.h"

//#undef DBG_PRT
//#define DBG_PRT(...)	AppendText2EditFmt(hDlg, ID_EDIT, __VA_ARGS__)

#ifndef DBG_PRT
#define DBG_PRT
#endif

#define XMODEM_MAX_RETRAY		 100

#define XMODEM_SOH				0x01
#define XMODEM_STX				0x02
#define XMODEM_EOT				0x04
#define XMODEM_ACK				0x06
#define XMODEM_NAK				0x15
#define XMODEM_CAN				0x18
#define XMODEM_CRC_CHR			 'C'
#define XMODEM_DATA_SIZE_SOH	 128		/* for Xmodem protocol    */
#define XMODEM_DATA_SIZE_STX	1024		/* for 1K xmodem protocol */

#if (WM_TOOL_USE_1K_XMODEM)
#define XMODEM_DATA_SIZE  XMODEM_DATA_SIZE_STX
#define XMODEM_FRAME_ID XMODEM_STX
#else
#define XMODEM_DATA_SIZE XMODEM_DATA_SIZE_SOH
#define XMODEM_FRAME_ID XMODEM_SOH
#endif

/*
 * Xmodem-CRC Frame layout:
 *
 *  +----------------+
 *  |     <SOH>      |
 *  +----------------+
 *  |    <blk #>     |
 *  +----------------+
 *  |  <255-blk #>   |
 *  +----------------+
 *  |    .......     |
 *  | <data  bytes>  |
 *  |  (128 or 1K)   |
 *  |    .......     |
 *  +----------------+
 *  |   <CRC hi>     |
 *  +----------------+
 *  |   <CRC lo>     |
 *  +----------------+
 */
#pragma pack(push)
#pragma pack(1)
typedef struct
{
	uint8_t  idFrame;
	uint8_t  idBlock;
	uint8_t  nidBlock;
	uint8_t  data[XMODEM_DATA_SIZE];
	uint16_t crc16;
} XmodemFrame_t;
#pragma pack(pop)

#define INT16_BE(x)	((uint16_t)((((uint16_t)(x))>>8)|(((uint16_t)(x))<<8)))

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		FileSize
 \date		Created  on Sat Jun  3 12:27:27 2023
 \date		Modified on Sat Jun  3 12:27:27 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
long int FileSize(const char* filename)
{
    FILE *fp = fopen(filename, "rb");
    if(!fp) return RET_ERROR;
    fseek(fp,0L,SEEK_END);
    long int size = ftell(fp);
    fclose(fp);
    return size;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		XmodemDownload
 \date		Created  on Mon May 22 17:43:08 2023
 \date		Modified on Mon May 22 17:43:08 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
int XmodemDownload(HWND hDlg, HANDLE hSerCom, const char *image)
{
    FILE         *fp_img;
	XmodemFrame_t frame;
	BOOL          bComplete   = FALSE;
	int           nRetray     = 0;
	int           PacketCnt   = 0;
	int           nReadCnt    = 0;
	int           nWriteCnt   = 0;
	size_t        SentCnt     = 0;
	int           iPercent    = 0;
	int           iPercentMem = 0;
	int           ret         = RET_ERROR;
	uint8_t       Answer      = XMODEM_ACK;
	uint16_t      crc16;

	/*
	 * Access image file.
	 */
	float ImgLen = FileSize(image);
	fp_img       = fopen(image, "rb");

	/*
	 * Sanity checks for image file.
	 */
    if (!fp_img || (ImgLen <= 0))
    {
		MessageBox(hDlg, "Cannot access image file.", "Error", MB_ICONERROR|MB_OK);
		return RET_ERROR;
    }

	/*
	 * Init UI.
	 */
	AppendText2Edit(hDlg, ID_EDIT, "Start download.\r\n");
	SendMessage(GetDlgItem(hDlg, ID_PROGRESS), PBM_SETPOS, 0, 0);
	SendMessage(GetDlgItem(hDlg, ID_LBL_PROGRESS), WM_SETTEXT, 0, (LPARAM)"0%");

	/*
	 * Reset UART buffer (discard any data).
	 */
    PurgeComm(hSerCom, PURGE_RXCLEAR);

	/*
	 * Set UART read in blocking mode.
	 * TODO: change to timeout mode.
	 */
    SetBlockingRx(hSerCom, TRUE);

	do
	{
	    DBG_PRT("switch Answer = %x\n", Answer);

        switch(Answer)
        {
        	case XMODEM_ACK:
        	{
				/*
				 * Increment frame ID and read data.
				 * Xmodem starts with the first frame numbered 1, not 0.
				 */
	            PacketCnt++;
	            nRetray  = 0;
	            nReadCnt = fread(frame.data, 1, XMODEM_DATA_SIZE, fp_img);
	            DBG_PRT("fread = %d\n", nReadCnt);

	            if (nReadCnt > 0)
	            {
					/*
					 * Data read from file.
					 * Increment the count of data sent to the device.
					 */
					SentCnt += nReadCnt;

					/*
					 * If we have read less data than the Xmodem buffur size means that we
					 * have reached the end of file.
					 * For sake of cleanless we set to zero the remaining bytes in the buffer.
					 */
					if (nReadCnt < XMODEM_DATA_SIZE)
	                {
	                    DBG_PRT("Filling the last frame with 0's!\n");
						memset(frame.data + nReadCnt, 0x00, XMODEM_DATA_SIZE - nReadCnt);
	                }

					/*
					 * Initialize frame header and checksum.
					 */
	                frame.idFrame  = XMODEM_FRAME_ID;
	                frame.idBlock  = (uint8_t)PacketCnt;
	                frame.nidBlock = (uint8_t)(255 - frame.idBlock);

	                crc16          = crc_ccitt_false(0, frame.data, XMODEM_DATA_SIZE);
					frame.crc16    = INT16_BE(crc16);

					/*
					 * Send data to device and checks that all data has been sent.
					 */
	                nWriteCnt = WriteSerial(hSerCom, &frame, sizeof(XmodemFrame_t));
	                if (nWriteCnt <= 0)
						AppendText2Edit(hDlg, ID_EDIT, "Write serial error.\r\n");

	                DBG_PRT("waiting for ack, %d, %d ...\n", PacketCnt, nWriteCnt);

	                while ((ReadSerial(hSerCom, &Answer, 1)) <= 0)
						Sleep(1);

	                if (Answer == XMODEM_ACK)
	                {
	                	DBG_PRT("Ok!\n");

						/*
						 * Update the progress in the UI.
						 */
						iPercent = (int)((float)SentCnt/ImgLen*100.0);
						if (iPercent > iPercentMem)
                        {
							iPercentMem = iPercent;
							SendMessage(GetDlgItem(hDlg, ID_PROGRESS), PBM_SETPOS, (WPARAM)iPercent, 0);
							char szPercentage[16];
							sprintf(szPercentage, "%3d%%", iPercent);
							SendMessage(GetDlgItem(hDlg, ID_LBL_PROGRESS), WM_SETTEXT, 0, (LPARAM)szPercentage);
						}
	                }
					else
					{
                    	DBG_PRT("error = %x!\n", Answer);
					}
	            }
	            else
	            {
					/*
					 * End of Transmission reached.
					 */
	                Answer = XMODEM_EOT;
	                bComplete = 1;

	                DBG_PRT("waiting for bComplete ack ...\n");

	                while (Answer != XMODEM_ACK)
	                {
	                    Answer = XMODEM_EOT;

	                    nWriteCnt = WriteSerial(hSerCom, &Answer, 1);
	                    if (nWriteCnt <= 0)
							AppendText2Edit(hDlg, ID_EDIT, "Write serial error.\r\n");

	                    while ((ReadSerial(hSerCom, &Answer, 1)) <= 0);
	                }

	                DBG_PRT("ok\n");

					/*
					 * UI ==> transfer completed succesful.
					 */
					AppendText2Edit(hDlg, ID_EDIT, "Download completed.\r\n");
					MessageBox(hDlg, "Download succesfully completed.", "Info", MB_ICONINFORMATION|MB_OK);
					SendMessage(GetDlgItem(hDlg, ID_PROGRESS), PBM_SETPOS, (WPARAM)1, 0);
					SendMessage(GetDlgItem(hDlg, ID_LBL_PROGRESS), WM_SETTEXT, 0, (LPARAM)"  0%");

	                ret = 0;
	            }
	            break;
            }
            case XMODEM_NAK:
            {
				/*
				 * Frame not acknowledged.
				 * Retry up to max repeats allowed.
				 */
                if ( nRetray++ > XMODEM_MAX_RETRAY)
                {
                    DBG_PRT("Retray max allowed, quit!\n");
					AppendText2Edit(hDlg, ID_EDIT, "Download firmware timeout.\r\n");

                    bComplete = 1;
                }
                else
                {
                    nWriteCnt = WriteSerial(hSerCom, &frame, sizeof(XmodemFrame_t));
                    if (nWriteCnt <= 0)
						AppendText2Edit(hDlg, ID_EDIT, "Write serial error.\r\n");

                    DBG_PRT("retry for ack, %d, %d ...\n", PacketCnt, nWriteCnt);

                    while ((ReadSerial(hSerCom, &Answer, 1)) <= 0);

                    if (Answer == XMODEM_ACK)
                    {
                    	DBG_PRT("ok\n");
                    }
    				else
    				{
                    	DBG_PRT("error!\n");
    				}
                }
                break;
            }
            default:
            {
                DBG_PRT("fatal error!\n");
                DBG_PRT("unknown xmodem protocal [%x].\n", Answer);
				AppendText2Edit(hDlg, ID_EDIT, "Download failed, please reset and try again.\r\n");

                bComplete = 1;
                break;
            }
        }
    } while (!bComplete);

    SetBlockingRx(hSerCom, FALSE);

    fclose(fp_img);

    return ret;
}
