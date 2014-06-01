/* rdd_server.h  Definitions for the Robot Device Driver server module */

#if !defined(RDD_SERVER_H)
#define RDD_SERVER_H

#include "stdint.h"
#include "protocol_rx.h"
#include "error.h"

/* SW flags */
#define F_RDS_TOKEN						0x01
#define F_RDS_CONFIGURED			0x02

/* Device enable flags */
#define F_RDS_DXL_AX_ENABLE		0x01		

/* Parameter identifiers */
#define P_RDS_PERIOD					0
#define P_RDS_DEV_NUM					1
#define P_RDS_DEV_ID					2
#define P_RDS_INST_NUM				3					/* device instances num */
#define P_RDS_INST_ID					4					/* device instances identifiers */
#define P_RDS_PARAM_WR_NUM		5					
#define P_RDS_PARAM_WR_ADDR		6
#define P_RDS_PARAM_RD_NUM		7
#define P_RDS_PARAM_RD_ADDR		8
#define P_RDS_PARAM_RD_PER		9

/* Device types ids */
#define RDS_DXL_AX_ID					1

/* Configuration state machine */
#define RDS_WAIT_CONFIG_PKG									0
#define RDS_STORE_CONFIG_PARAM							1
#define RDS_INITIALIZE_DEVS									2
#define RDS_DAX_PING												3
#define RDS_DAX_REDUCE_MOVING_SPEED					4
#define RDS_DAX_SET_INIT_POS								5	
#define RDS_DAX_WAIT_MOVEMENT_END						6
#define RDS_DAX_SET_MAX_MOVING_SPEED				7
#define RDS_DAX_WAIT_RQST_COMPLETE					8

/* Instruction execution state machine */
#define RDS_WAIT_INSTR_PKG									0		
#define RDS_DAX_WAIT_PERIOD									1
#define RDS_WAIT_TOKEN											2

/* Data types */
typedef struct Rds_Device Rds_Device;
struct Rds_Device{
	uint8_t dev_id;						/* device type identifier */
	uint8_t inst_num;					/* device instances num */
	uint8_t *inst_id;					/* instances id array */
	uint8_t param_wr_num;			/* parameters to write num */
	uint8_t *param_wr_addr;		/* parameters to write address array */
	uint8_t param_rd_num;			/* parameters to read num */
	uint8_t *param_rd_addr;		/* parameters to read address array */
	uint8_t *param_rd_per;		/* parameters to read period */
};

typedef struct Rds_Control Rds_Control;
struct Rds_Control{
	uint8_t flags;	
	uint8_t dev_ena_flags;
	uint8_t state;
	uint8_t next_state;
	uint16_t period;					/* execution period (ms) */
	
	uint8_t dev_num;					/* number of different type devices */
	Rds_Device *dev;					/* devices array */
	uint8_t dev_idx;					/* points to the current device */
	
	Pro_Package *pkg_p;
	uint8_t pkg_dev_id;			/* current device identifier */
};

/* Routines */
void Rds_Define(Rds_Control *rdp);

void Rds_Configure(Rds_Control *rdp);

void Rds_Process(Rds_Control *rdp);

#endif
