/* usb_vcp.c Implementation of the virtual com port module */

#include "usb_vcp.h"
#include "hw_config.h"
#include "debug.h"

/* Extern variables */
extern __IO uint8_t Receive_Buffer[64];
extern __IO  uint32_t Receive_length;
extern struct McpTx_Control c_ptx;

/* Initialize the vcp module */
void Vcp_Define (struct Vcp_Control *vcp,
								unsigned char *buffer)
{
	vcp->flags = 0;
	vcp->idx_in = vcp->idx_out = VCP_BUF_IN_LEN - 1;
	vcp->buffer = buffer;
}

/* Save incoming data into vcp->buffer */
void Vcp_Process (struct Vcp_Control *vcp)
{		
	while(Receive_length)
	{		
		if (!Vcp_Store(vcp))
			return;
		
		Receive_length = 0;
		CDC_Receive_DATA();
	};		
}

/* Store data in vcp->buffer, returns 0 if the buffer has no free space */
unsigned char Vcp_Store(struct Vcp_Control *vcp)
{	
	if(Vcp_Data_Avail(vcp) && (vcp->idx_in == vcp->idx_out))
		return 0;
	
	vcp->buffer[vcp->idx_in] = Receive_Buffer[0];
		
	if (vcp->idx_in)
		--(vcp->idx_in);
	else
		vcp->idx_in = VCP_BUF_IN_LEN - 1;
	
	vcp->flags |= VCP_DATA_AVAIL;
	
	return 1;
}

/* Check if there is available data in vcp->buffer */
unsigned char Vcp_Data_Avail(struct Vcp_Control *vcp)
{
	return ((vcp->flags) & VCP_DATA_AVAIL);
}

/* Take data from vcp->buffer */
unsigned char Vcp_Take_Data(struct Vcp_Control *vcp)
{
	unsigned char data = vcp->buffer[vcp->idx_out];
	
	if (vcp->idx_out)
		--(vcp->idx_out);
	else
		vcp->idx_out = VCP_BUF_IN_LEN - 1;
	
	if(vcp->idx_in == vcp->idx_out)
	{
		vcp->flags &= ~(VCP_DATA_AVAIL);
	}
	
	return data;
}

/* Clean the VCP buffer */
void Vcp_Flush_Buffer(struct Vcp_Control *vcp)
{
	vcp->idx_out = vcp->idx_in;
	vcp->flags &= ~(VCP_DATA_AVAIL);
}
