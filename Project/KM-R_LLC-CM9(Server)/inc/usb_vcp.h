/* usb_vcp.h Definitions for the virtual com port module */

#if !defined(USB_VCP_H)

#define USB_VCP_H

/* Application related definitions */

#define VCP_BUF_IN_LEN			128U

/* SW flags */

#define VCP_DATA_AVAIL			0x01U

/* Data types */

struct Vcp_Control{
	unsigned char data_count;
	unsigned char flags;
	unsigned char idx_in;
	unsigned char idx_out;
	unsigned char *buffer;
};

/* Routines */

void Vcp_Define (struct Vcp_Control *vcp,
								unsigned char *buffer);

void Vcp_Process (struct Vcp_Control *vcp);

unsigned char Vcp_Store(struct Vcp_Control *vcp);

unsigned char Vcp_Data_Avail(struct Vcp_Control *vcp);

unsigned char Vcp_Take_Data(struct Vcp_Control *vcp);

void Vcp_Flush_Buffer(struct Vcp_Control *vcp);

#endif
