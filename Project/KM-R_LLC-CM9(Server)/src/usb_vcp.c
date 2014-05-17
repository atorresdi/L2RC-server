/* usb_vcp.c Implementation of the virtual com port module */

#include "usb_vcp.h"
#include "hw_config.h"
#include "debug.h"

/* Extern variables */
extern __IO uint8_t Receive_Buffer[64];
extern __IO  uint32_t Receive_length;
extern struct McpTx_Control c_ptx;

/* Initialize the vcp module */
void Vcp_Define (Vcp_Control *vcp,
								uint8_t *buf)
{
	vcp->flags = 0;
	vcp->idx_in = vcp->idx_out = VCP_BUF_LEN - 1;
	vcp->buf = buf;
}

/* Save incoming data into vcp->buf */
void Vcp_Process (Vcp_Control *vcp)
{		
	while(Receive_length)
	{		
		if (!Vcp_Store(vcp))
			return;
		
		Receive_length = 0;
		CDC_Receive_DATA();
	};		
}

/* Store data in vcp->buf, returns 0 if the buffer has no free space */
uint8_t Vcp_Store(Vcp_Control *vcp)
{	
	if(Vcp_Data_Avail(vcp) && (vcp->idx_in == vcp->idx_out))
		return 0;
	
	vcp->buf[vcp->idx_in] = Receive_Buffer[0];
		
	if (vcp->idx_in)
		--(vcp->idx_in);
	else
		vcp->idx_in = VCP_BUF_LEN - 1;
	
	vcp->flags |= F_VCP_DATA_AVAIL;
	
	return 1;
}

/* Check if there is available data in vcp->buf */
uint8_t Vcp_Data_Avail(Vcp_Control *vcp)
{
	return ((vcp->flags) & F_VCP_DATA_AVAIL);
}

/* Get one byte from vcp->buf */
uint8_t Vcp_Get_Data(Vcp_Control *vcp)
{
	uint8_t data = vcp->buf[vcp->idx_out];
	
	if (vcp->idx_out)
		--(vcp->idx_out);
	else
		vcp->idx_out = VCP_BUF_LEN - 1;
	
	if(vcp->idx_in == vcp->idx_out)
	{
		vcp->flags &= ~(F_VCP_DATA_AVAIL);
	}
	
	return data;
}

/* Clean the VCP buf */
void Vcp_Flush_Buffer(Vcp_Control *vcp)
{
	vcp->idx_out = vcp->idx_in;
	vcp->flags &= ~(F_VCP_DATA_AVAIL);
}
