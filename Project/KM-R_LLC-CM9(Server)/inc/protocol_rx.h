/* protocol_rx.h Definitions for the Comunication Protocol Reception module  */

#if !defined(PROTOCOL_RX_H)
#define PROTOCOL_RX_H

#include "stdint.h"

/* Buffer lengths */
#define PRX_CMD_BUF_LEN		2U
#define PRX_PKG_BUF_LEN		2U

/* Max package data length */
#define PRX_MAX_PKG_DATA_LEN		256U

/* Max consecutive received bytes (flow control) */
#define PRX_MAX_CONSEC_RX_BYTES			16

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

/* Package types */
#define PRX_CONFIG_PKG				0
#define PRX_INSTRUCTION_PKG		1
#define PRX_ERROR_PKG					2

/* SW flags */
#define F_PRX_CMD_AVAIL				0x01U
#define F_PRX_CTRL_CH_AVAIL		0x02U
#define F_PRX_PKG_AVAIL				0x04U

/* State machine */
#define PRX_WAIT_CTRL_CHAR			0				/* Wait for #, SOH, ACK, NAK or SYN */
#define PRX_WAIT_CMD						1				/* Wait for a command character */
#define PRX_WAIT_CMD_CMP				2				/* Wait for the command character complement */
#define PRX_WAIT_SEC						3				/* Wait for the package secuence number */
#define PRX_WAIT_LEN						4				/* Wait for the package length */
#define PRX_WAIT_OPTS						5				/* Wait the package options */
#define PRX_WAIT_PTSF						6				/* Wait for the Package Type Specific Field (PTSF) */
#define PRX_WAIT_PKG_DATA				7				/* Wait for the package data */
#define PRX_WAIT_CKSUM					8				/* Wait for checksum */

/* Data types */
typedef struct Pro_Package Pro_Package;
struct Pro_Package {
	uint16_t length;
	uint8_t opts;
	uint8_t ptsf;
	uint8_t *data;
};

typedef struct Prx_Control Prx_Control;
struct Prx_Control {
	uint8_t flags;
	uint8_t state;
	uint8_t err_count;
	uint8_t *sec_num; 
	
	uint8_t *cmd_buf;
	uint8_t cmd_buf_len;
	uint8_t cmd_idx_in;
	uint8_t cmd_idx_out;
	
	uint8_t ctrl_ch;
	
	Pro_Package *pkg_buf;
	uint8_t pkg_buf_len;
	uint8_t pkg_idx_in;
	uint8_t pkg_idx_out;
	uint8_t cksum;
	uint16_t data_count;
};

/* Routines */
void Prx_Define(Prx_Control *prp,
								uint8_t *sec_num,
								uint8_t *cmd_buf,
								uint8_t cmd_buf_len,
								Pro_Package *pkg_buf,
								uint8_t pkg_buf_len,
								uint8_t pkg_data[PRX_PKG_BUF_LEN][PRX_MAX_PKG_DATA_LEN]);

void Prx_Process(Prx_Control *prp);

uint8_t Prx_Cmd_Avail(Prx_Control *prp);

uint8_t Prx_Get_Cmd(Prx_Control *prp);

uint8_t Prx_Ctrl_Ch_Avail(Prx_Control *prp);

uint8_t Prx_Get_Ctrl_Ch(Prx_Control *prp);

uint8_t Prx_Pkg_Avail(Prx_Control *prp);

Pro_Package *Prx_Get_Pkg(Prx_Control *prp);

uint8_t	Prx_Get_Pkg_Type(Pro_Package *pkg_p);

uint8_t	Prx_Get_Pkg_Type(Pro_Package *pkg_p);

uint8_t	Prx_Get_Pkg_Dev_Id(Pro_Package *pkg_p);

uint8_t	Prx_Get_Pkg_Data_Size(Pro_Package *pkg_p);

void Prx_Ckout_Curr_Pkg(Prx_Control *prp);

#endif
