/* usb_vcp.h Definitions for the virtual com port module */

#if !defined(USB_VCP_H)
#define USB_VCP_H

#include "stdint.h"

/* Buffer lengths */
#define VCP_BUF_LEN				128U
#define VCP_BUF_OUT_LEN		32U

/* SW flags */
#define F_VCP_DATA_AVAIL			0x01U

/* Data types */
typedef struct Vcp_Control Vcp_Control;

struct Vcp_Control{
	uint8_t flags;
	uint8_t idx_in;
	uint8_t idx_out;
	uint8_t *buf;
};

/* Routines */
void Vcp_Define (Vcp_Control *vcp,
								uint8_t *buf);

void Vcp_Process (Vcp_Control *vcp);

uint8_t Vcp_Store(Vcp_Control *vcp);

uint8_t Vcp_Data_Avail(Vcp_Control *vcp);

uint8_t Vcp_Get_Data(Vcp_Control *vcp);

void Vcp_Flush_Buffer(Vcp_Control *vcp);

#endif
