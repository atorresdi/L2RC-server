/* protocol_rx.c Implementation of the Comunication Protocol Reception module  */

#include "protocol_rx.h"
#include "protocol_tx.h"
#include "usb_vcp.h"
#include "debug.h"

/* Extern variables */
extern Vcp_Control c_vcp;
extern Ptx_Control c_ptx;

/* Static variables */
static uint8_t tmp_data;		// Temp data

/* Static routines ------------------------------------------- */

/* Check for free space in the command buffer */
static uint8_t Prx_Cmd_Buf_Free(Prx_Control *prp)
{
	if(Prx_Cmd_Avail(prp) && (prp->cmd_idx_in == prp->cmd_idx_out))
		return 0;
	
	return 1;
}

/* --------------------------------------------Static routines */

/* Initialization */
void Prx_Define(Prx_Control *prp,
								uint8_t *sec_num,
								uint8_t *cmd_buf,
								uint8_t cmd_buf_len)
{
	prp->flags = 0;
	prp->state = PRX_WAIT_CTRL_CHAR;
	prp->data_count = 0;
	prp->err_count = 0;
	prp->sec_num = 0;
	
	prp->cmd_buf = cmd_buf;
	prp->cmd_buf_len = cmd_buf_len;
	prp->cmd_idx_in = prp->cmd_idx_out = cmd_buf_len - 1;
}

/* State machine */
void Prx_Process(Prx_Control *prp)
{
	switch (prp->state)
	{
		case PRX_WAIT_CTRL_CHAR:
			
			if (Vcp_Data_Avail(&c_vcp))
			{
				tmp_data = Vcp_Get_Data(&c_vcp);
				
				if (tmp_data == HASH)
					prp->state = PRX_WAIT_CMD;
				else if ((tmp_data == ACK) || (tmp_data == NAK) || (tmp_data == SYN))
				{
					prp->ctrl_ch = tmp_data;
					prp->flags |= F_PRX_CTRL_CH_AVAIL;
				}
			};
			
			break;
			
		case PRX_WAIT_CMD:
			
			if (Vcp_Data_Avail(&c_vcp))
			{
				tmp_data = Vcp_Get_Data(&c_vcp);
				
				prp->state = PRX_WAIT_CMD_CMP;
			};
			
			break;
			
		case PRX_WAIT_CMD_CMP:
		
			if (Vcp_Data_Avail(&c_vcp) && Prx_Cmd_Buf_Free(prp))
			{
				uint8_t cmd = ~(Vcp_Get_Data(&c_vcp));

				if ( tmp_data == cmd )
				{
						prp->cmd_buf[prp->cmd_idx_in] = tmp_data;
		
						if (prp->cmd_idx_in)
							--(prp->cmd_idx_in);
						else
							prp->cmd_idx_in = prp->cmd_buf_len - 1;
						
						prp->flags |= F_PRX_CMD_AVAIL;
						
						Ptx_Add_Ctrl_Ch_Rqst(&c_ptx, ACK);
				}
				else
				{ 
					Ptx_Add_Ctrl_Ch_Rqst(&c_ptx, NAK);
				};
				
				prp->state = PRX_WAIT_CTRL_CHAR;
			};
			
			break;
	};
}

/* Check for an available command */
uint8_t Prx_Cmd_Avail(Prx_Control *prp)
{
	return (prp->flags & F_PRX_CMD_AVAIL);
}

/* Get command from prp->cmd_buf */
uint8_t Prx_Get_Cmd(Prx_Control *prp)
{
	tmp_data = prp->cmd_buf[prp->cmd_idx_out];
	
	if (prp->cmd_idx_out)
		--(prp->cmd_idx_out);
	else
		prp->cmd_idx_out = prp->cmd_buf_len - 1;
	
	if(prp->cmd_idx_in == prp->cmd_idx_out)
	{
		prp->flags &= ~(F_PRX_CMD_AVAIL);
	};
	
	return tmp_data;
}

uint8_t Prx_Ctrl_Ch_Avail(Prx_Control *prp)
{
	return (prp->flags & F_PRX_CTRL_CH_AVAIL);
}

uint8_t Prx_Get_Ctrl_Ch(Prx_Control *prp)
{
	prp->flags &= ~(F_PRX_CTRL_CH_AVAIL);
	
	return prp->ctrl_ch;
}
