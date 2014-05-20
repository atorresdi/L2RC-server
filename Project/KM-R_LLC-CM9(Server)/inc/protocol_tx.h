/* protocol_tx.h Definitions for the Comunication Protocol Transmision module  */

#if !defined(PROTOCOL_TX_H)
#define PROTOCOL_TX_H

#include "stdint.h"
#include "protocol_rx.h"

/* Requests buffer length */
#define PTX_RQST_BUF_LEN			4
// #define PTX_RQST_BUF_LEN		16

/* Request types */
#define PTX_CTRL_CH_RQST		0
#define PTX_CMD_RQST				1
#define PTX_PKG_RQST				2

/* State machine */
#define PTX_WAIT_RQST				0			/* Wait for a tx request */
#define PTX_WAIT_ACK				1			/* Wait for ACK	*/
#define PTX_WAIT_1ST_SYN		2			/* Wait for SYN after sending SOH and package secuence num */
#define PTX_WAIT_2ND_SYN		3			/* Wait for SYN after sending package length, options and PTSF */
#define PTX_WAIT_3RD_SYN		4			/* Wait for SYN after sending package length, options and PTSF */


/* Data types */
typedef struct Ptx_Request Ptx_Request;
struct Ptx_Request{
	uint8_t type;
	uint8_t data;
	uint16_t sent_data_num;
	Pro_Package pkg;
};

typedef struct Ptx_Control Ptx_Control;
struct Ptx_Control{
	uint8_t flags;
	uint8_t state;
	uint8_t *sec_num;
	uint8_t cksum;
	
	uint8_t buf_len;
	uint8_t idx_in;
	uint8_t idx_out;
	Ptx_Request *rqst_buf;
	Ptx_Request *curr_rqst;
};

/* Routines */
void Ptx_Define(Ptx_Control *ptp,
								uint8_t *sec_num,
								Ptx_Request *rqst_buf,
								uint8_t buf_len);

void Ptx_Process(Ptx_Control *ptp);

uint8_t Ptx_Add_Ctrl_Ch_Rqst(Ptx_Control *ptp, uint8_t ctrl_ch);

uint8_t Ptx_Add_Cmd_Rqst(Ptx_Control *ptp, uint8_t cmd);

uint8_t Ptx_Add_Pkg_Rqst(Ptx_Control *ptp,
												 uint16_t length,
												 uint8_t opts,
												 uint8_t ptsf,
												 uint8_t *data);

uint8_t Ptx_Set_Pkg_Opts(uint8_t pkg_type,
												uint8_t device,
												uint8_t data_size);

#endif
