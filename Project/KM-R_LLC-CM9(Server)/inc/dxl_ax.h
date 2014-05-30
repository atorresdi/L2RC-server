/* dxl_ax.h 	Definitions for the Dynamixel AX series driver module */

#if !defined(DXL_AX_H)
#define DXL_AX_H

#include "stdint.h"
#include "error.h"

/* Error definitions */
#define DAX_MAX_ERR_NUM						3
#define F_DAX_CKSUM_ERR						0x10
#define F_DAX_RX_TIMEOUT_ERR			0x80

/* Ping package */
#define DAX_PING_LENGTH						2
#define DAX_PING_PKG_LEN					6

/* Write package */
#define DAX_WRITE_LENGTH_BASE			3
#define DAX_WR_PKG_LEN_BASE				7

/* Read package */
#define DAX_RD_LENGTH							4
#define DAX_RD_PKG_LEN						8

/* Packages max length */
#define DAX_INST_PKG_MAX_LEN			9
#define DAX_STUS_PKG_MAX_LEN			9

/* SW flags */
#define F_DAX_TX_COMPLETE				0x01				/* Package transmission complete */
#define F_DAX_PORT_DIR					0x02				/* Dynamixel AX interface direction, high tx, low rx */
#define F_DAX_WAIT_STUS_PKG			0x04				/* Wait for status package */
#define F_DAX_STUS_PKG_AVAIL		0x08				/* Status package available */
#define F_DAX_ERR								0x10				/* Any type of error */
#define F_DAX_RQST_AVAIL				0x20				/* Request available */
#define F_DAX_RQST_COMPLETE			0x40				/* Request complete */
#define F_DAX_RX_TIMEOUT				0x80				/* Reception timeout */
#define F_DAX_RD_DATA_AVAIL			0x100				/* rd_data available */

/* Dynamixel AX series instruction set */
#define DAX_INST_PING						0x01
#define DAX_INST_READ						0x02
#define DAX_INST_WRITE					0x03

/* Dynamixel AX series parameters address */
#define DAX_GOAL_POSITION_ADDR		30
#define DAX_MOVING_SPEED_ADDR			32

/* Dinamixel AX series package */
#define DAX_SOP									0xFF		/* Start of package character */
#define	DAX_ID_POS							2				/* position num of the id field in the package */
#define	DAX_LENGTH_POS					3				/* position num of the length field in the package */
#define	DAX_INST_POS						4				/* position num of the instruction field in the instruction package */		
#define	DAX_ERR_POS							4				/* position num of the error field in the status package */
#define DAX_ADDR_POS						5				/* position num of the address to be written in a instruction package */
#define	DAX_1ST_PARAM_POS				5				/* position num of parameter 1 in the status package */
#define DAX_DATA_SIZE_POS				6				/* position num of the data size to be read */

/* Dax state machine */
#define DAX_WAIT_RQST						0
#define DAX_SEND_INST_PKG				1
#define DAX_WAIT_STUS_PKG				2
#define DAX_WAIT_TX_COMPLETE		3

/* Package reception state machine */
#define DAX_WAIT_SOP						0
#define DAX_WAIT_ID							1
#define DAX_WAIT_LENGTH					2
#define DAX_WAIT_REST_OF_PKG		3

/* Data types */
typedef struct Dax_Control Dax_Control;
struct Dax_Control
{
	uint16_t flags;
	uint8_t dax_state;
	uint8_t rx_state;
	uint16_t cksum;
	uint16_t cksum_base;
	uint8_t err_num;

	uint8_t dev_num;
	uint8_t dev_count;
	uint8_t *id;								/* array of ids */
	uint8_t *curr_id;						/* current id */
	
	uint8_t *inst_pkg;					/* instruction package */
	uint8_t *stus_pkg;					/* status package */
	uint8_t inst_pkg_idx;
	uint8_t rxed_data_count;		/* received data count */
	uint16_t pkg_length;
	uint8_t *pkg_cksum_p;				/* points to the package checksum field */
	
	uint8_t data_size;
	uint8_t *data_wr;
	uint8_t *data_rd;
	uint8_t *data_rd_p;
};

/* Routines */
void Dax_Define(Dax_Control *dap,
								uint8_t dev_num,
								uint8_t *id,
								uint8_t *inst_pkg,
								uint8_t *stus_pkg);

void Dax_Process(Dax_Control *dap);

void Dax_Ping_Rqst(Dax_Control *dap);

void Dax_Write_Rqst(Dax_Control *dap, uint8_t address, uint8_t data_size, uint8_t *data_wr);

void Dax_Read_Rqst(Dax_Control *dap, uint8_t address, uint8_t data_size);

void Dax_Port_Write(volatile struct Dax_Control *dap);

void Dax_Port_Read(volatile struct Dax_Control *dap);

void Dax_Set_Stus_Rtn_Lvl(Dax_Control *dap, uint8_t return_level);

uint8_t Dax_Rqst_Complete(Dax_Control *dap);

uint8_t Dax_Rd_Data_Avail(Dax_Control *dap);

uint8_t *Dax_Get_Rd_Data(Dax_Control *dap);

uint8_t Dax_Err(Dax_Control *dap);

Error *Dax_Get_Err(Dax_Control *dap);

#endif
