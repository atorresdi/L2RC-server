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

/* Check for free space in the package buffer */
static uint8_t Prx_Pkg_Buf_Free(Prx_Control *prp)
{
	if(Prx_Pkg_Avail(prp) && (prp->pkg_idx_in == prp->pkg_idx_out))
		return 0;
	
	return 1;
}

/* --------------------------------------------Static routines */

/* Initialization */
void Prx_Define(Prx_Control *prp,
								uint8_t *sec_num,
								uint8_t *cmd_buf,
								uint8_t cmd_buf_len,
								Pro_Package *pkg_buf,
								uint8_t pkg_buf_len,
								uint8_t pkg_data[PRX_PKG_BUF_LEN][PRX_MAX_PKG_DATA_LEN])
{
	uint8_t n;
	
	prp->flags = 0;
	prp->state = PRX_WAIT_CTRL_CHAR;
	prp->err_count = 0;
	prp->sec_num = sec_num;
	*(prp->sec_num) = 0;
	
	prp->cmd_buf = cmd_buf;
	prp->cmd_buf_len = cmd_buf_len;
	prp->cmd_idx_in = prp->cmd_idx_out = cmd_buf_len - 1;
	
	prp->pkg_buf = pkg_buf;
	prp->pkg_buf_len = pkg_buf_len;
	
	/* assign a data buffer to each package in pkg_buf */	
	for (n = 0; n < pkg_buf_len; n++)
		pkg_buf[n].data = &(pkg_data[n][0]);
	
	prp->pkg_idx_in = prp->pkg_idx_out = pkg_buf_len - 1;
	prp->cksum = 0;
	prp->data_count = 0;
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
				else if ( tmp_data == SOH)
					prp->state = PRX_WAIT_SEC;
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
						
						Ptx_Add_Ctrl_Ch_Rqst(&c_ptx, ACK);		/* Send ACK	*/
				}
				else
					Ptx_Add_Ctrl_Ch_Rqst(&c_ptx, NAK);			/* Send NAK	*/	
				
				prp->state = PRX_WAIT_CTRL_CHAR;
			};
			
			break;
			
		case PRX_WAIT_SEC:
			
			if (Vcp_Data_Avail(&c_vcp) && Prx_Pkg_Buf_Free(prp))
			{
				tmp_data = Vcp_Get_Data(&c_vcp);
				
				if ( !(tmp_data == (*(prp->sec_num))) )
				{
					Ptx_Add_Ctrl_Ch_Rqst(&c_ptx, NAK);			/* Send NAK */
					prp->state = PRX_WAIT_CTRL_CHAR;
					break;
				};
				
				Ptx_Add_Ctrl_Ch_Rqst(&c_ptx, SYN);			/* Send SYN */
				prp->state = PRX_WAIT_LEN;
			};
		
			break;
			
		case PRX_WAIT_LEN:
			
			if (Vcp_Data_Avail(&c_vcp))
			{
				tmp_data = Vcp_Get_Data(&c_vcp);
				
				if (!tmp_data)			/* a 256 length is represented by zero */
					prp->pkg_buf[prp->pkg_idx_in].length = 256;
				else
					prp->pkg_buf[prp->pkg_idx_in].length = tmp_data;				
				
				prp->cksum += tmp_data;
				prp->state = PRX_WAIT_OPTS;
			};
			
			break;
			
		case PRX_WAIT_OPTS:
			
			if (Vcp_Data_Avail(&c_vcp))
			{
				tmp_data = Vcp_Get_Data(&c_vcp);
				
				prp->pkg_buf[prp->pkg_idx_in].opts = tmp_data;
				prp->cksum += tmp_data;
				prp->state = PRX_WAIT_PTSF;
			};
			
			break;
			
		case PRX_WAIT_PTSF:
			
			if (Vcp_Data_Avail(&c_vcp))
			{
				tmp_data = Vcp_Get_Data(&c_vcp);
				
				prp->pkg_buf[prp->pkg_idx_in].ptsf = tmp_data;
				prp->cksum += tmp_data;
				Ptx_Add_Ctrl_Ch_Rqst(&c_ptx, SYN);			/* Send SYN */
				prp->state = PRX_WAIT_PKG_DATA;
			};
			
			break;
			
		case PRX_WAIT_PKG_DATA:
			
			if (Vcp_Data_Avail(&c_vcp))
			{
				tmp_data = Vcp_Get_Data(&c_vcp);
				
				prp->pkg_buf[prp->pkg_idx_in].data[prp->data_count] = tmp_data;
				(prp->data_count)++;
				prp->cksum += tmp_data;
				
				if (prp->data_count >= PRX_MAX_CONSEC_RX_BYTES)
				{
					if (!(prp->data_count % PRX_MAX_CONSEC_RX_BYTES))
						Ptx_Add_Ctrl_Ch_Rqst(&c_ptx, SYN);			/* Send SYN */
				};
				
				if ( (prp->data_count) >= prp->pkg_buf[prp->pkg_idx_in].length )
					prp->state = PRX_WAIT_CKSUM;
			};
			
			break;
			
		case PRX_WAIT_CKSUM:
			
			if (Vcp_Data_Avail(&c_vcp))
			{
				tmp_data = Vcp_Get_Data(&c_vcp);
				
				if (tmp_data == prp->cksum)
				{
					if (prp->pkg_idx_in)
						--(prp->pkg_idx_in);
					else
						prp->pkg_idx_in = prp->pkg_buf_len - 1;
						
					(*(prp->sec_num))++;
					prp->flags |= F_PRX_PKG_AVAIL;	
					Ptx_Add_Ctrl_Ch_Rqst(&c_ptx, ACK);		/* Send ACK */
				}
				else
					Ptx_Add_Ctrl_Ch_Rqst(&c_ptx, NAK);		/* Send NAK	*/
				
				prp->cksum = 0;
				prp->data_count = 0;
				
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

/* Check for an available control character */
uint8_t Prx_Ctrl_Ch_Avail(Prx_Control *prp)
{
	return (prp->flags & F_PRX_CTRL_CH_AVAIL);
}

/* Get the received control character */
uint8_t Prx_Get_Ctrl_Ch(Prx_Control *prp)
{
	prp->flags &= ~(F_PRX_CTRL_CH_AVAIL);
	
	return prp->ctrl_ch;
}

/* Check for an available package */
uint8_t Prx_Pkg_Avail(Prx_Control *prp)
{
	return (prp->flags & F_PRX_PKG_AVAIL);
};

/* Get a package from pkt_buf */
Pro_Package *Prx_Get_Pkg(Prx_Control *prp)
{
	return &(prp->pkg_buf[prp->pkg_idx_out]);
}

/* Get the package type */
uint8_t	Prx_Get_Pkg_Type(Pro_Package *pkg_p)
{
	return ((pkg_p->opts >> 6) & 0x03);
}

/* Get the package device identifier */ 
uint8_t	Prx_Get_Pkg_Dev_Id(Pro_Package *pkg_p)
{
	return ((pkg_p->opts >> 2) & 0x0F);
}

/* Get the package data size */
uint8_t	Prx_Get_Pkg_Data_Size(Pro_Package *pkg_p)
{
	return (pkg_p->opts & 0x03);
}

/* Check out an already used package */
void Prx_Ckout_Curr_Pkg(Prx_Control *prp)
{
	if (prp->pkg_idx_out)
		--(prp->pkg_idx_out);
	else
		prp->pkg_idx_out = prp->pkg_buf_len - 1;
	
	if(prp->pkg_idx_in == prp->pkg_idx_out)
		prp->flags &= ~(F_PRX_PKG_AVAIL);
}
