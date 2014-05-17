/* protocol_rx.h Definitions for the Comunication Protocol Reception module  */

#if !defined(PROTOCOL_RX_H)
#define PROTOCOL_RX_H

#include "stdint.h"

/* Buffer lengths */
// #define PRX_CMD_BUF_LEN		2U
#define PRX_CMD_BUF_LEN		32

/* Control characters */
#define SOH									0x01U
#define HASH								0X23U
#define ACK									0x06U
#define NAK									0x15U
#define SYN									0x16U

/* Commands characters */				
#define PRX_START			0x73U
#define PRX_PAUSE			0x70U
#define PRX_RESET			0x72U
#define PRX_TOKEN			0x74U

/* SW flags */
#define F_PRX_CMD_AVAIL				0x01U
#define F_PRX_CTRL_CH_AVAIL		0x02U

/* State machine */
#define PRX_WAIT_CTRL_CHAR			0				/* Wait for #, SOH, ACK, NAK or SYN */
#define PRX_WAIT_CMD						1				/* Wait for a command character */
#define PRX_WAIT_CMD_CMP				2				/* Wait for the command character complement */

/* Data types */
typedef struct Prx_Control Prx_Control;

struct Prx_Control {
	uint8_t flags;
	uint8_t state;
	uint8_t data_count;
	uint8_t err_count;
	uint8_t *sec_num; 
	
	uint8_t *cmd_buf;
	uint8_t cmd_buf_len;
	uint8_t cmd_idx_in;
	uint8_t cmd_idx_out;
	
	uint8_t ctrl_ch;
};

/* Routines */
void Prx_Define(Prx_Control *prp,
								uint8_t *sec_num,
								uint8_t *cmd_buf,
								uint8_t cmd_buf_len);

void Prx_Process(Prx_Control *prp);

uint8_t Prx_Cmd_Avail(Prx_Control *prp);

uint8_t Prx_Get_Cmd(Prx_Control *prp);

uint8_t Prx_Ctrl_Ch_Avail(Prx_Control *prp);

uint8_t Prx_Get_Ctrl_Ch(Prx_Control *prp);

#endif
