/* protocol_tx.c Implementation of the Comunication Protocol Transmision module  */

#include "protocol_tx.h"
#include "protocol_rx.h"
#include "hw_config.h"
#include "debug.h"

/* Extern variables */
extern Prx_Control c_prx;

/* Initialization */
void Ptx_Define(Ptx_Control *ptp,
								uint8_t *sec_num,
								Ptx_Request *rqst_buf,
								uint8_t buf_len)
{
	ptp->flags = 0;
	ptp->state = PTX_WAIT_RQST;
	ptp->sec_num = sec_num;
	
	ptp->buf_len = buf_len;
	ptp->idx_in = ptp->idx_out = buf_len - 1;
	ptp->rqst_buf = rqst_buf;
	ptp->curr_rqst = 0;
}

/* State machine */
void Ptx_Process(Ptx_Control *ptp)
{
	switch (ptp->state)
	{
		case PTX_WAIT_RQST:
			
			if (ptp->curr_rqst)				/* if a request is pending */
			{
				if (ptp->curr_rqst->type == PTX_CTRL_CH_RQST)
				{
					CDC_Send_DATA(&(ptp->curr_rqst->data), 1);
					Ptx_Ckout_Curr_Rqst(ptp);
				}
				else if (ptp->curr_rqst->type == PTX_CMD_RQST)
				{
					uint8_t cmd[3];
					cmd[0] = HASH;
					cmd[1] = ptp->curr_rqst->data;
					cmd[2] = ~(ptp->curr_rqst->data);
					
					CDC_Send_DATA(cmd, 3);
					ptp->state = PTX_WAIT_ACK;
				}
			};
			
			break;

		case PTX_WAIT_ACK:
			
			if (Prx_Ctrl_Ch_Avail(&c_prx))
			{
				uint8_t ctrl_ch = Prx_Get_Ctrl_Ch(&c_prx);
				
				if ( ctrl_ch == ACK )
					Ptx_Ckout_Curr_Rqst(ptp);
				
				ptp->state = PTX_WAIT_RQST;
			};
		
			break;
	};
}

/* Add a control character transmision request */
uint8_t Ptx_Add_Ctrl_Ch_Rqst(Ptx_Control *ptp, uint8_t ctrl_ch)
{
	/* Check for free space in buffer */
	if (ptp->curr_rqst && (ptp->idx_in == ptp->idx_out))
		return 0;
	
	ptp->rqst_buf[ptp->idx_in].type = PTX_CTRL_CH_RQST;
	ptp->rqst_buf[ptp->idx_in].data = ctrl_ch;
	
	if ( !(ptp->curr_rqst) )
		ptp->curr_rqst = &(ptp->rqst_buf[ptp->idx_in]);
	
	if (ptp->idx_in)
		--(ptp->idx_in);
	else
		ptp->idx_in = ptp->buf_len - 1;
	
	return 1;
}

/* Add a command transmision request */
uint8_t Ptx_Add_Cmd_Rqst(Ptx_Control *ptp, uint8_t cmd)
{
	/* Check for free space in buffer */
	if (ptp->curr_rqst && (ptp->idx_in == ptp->idx_out))
		return 0;
	
	ptp->rqst_buf[ptp->idx_in].type = PTX_CMD_RQST;
	ptp->rqst_buf[ptp->idx_in].data = cmd;
	
	if ( !(ptp->curr_rqst) )
		ptp->curr_rqst = &(ptp->rqst_buf[ptp->idx_in]);
	
	if (ptp->idx_in)
		--(ptp->idx_in);
	else
		ptp->idx_in = ptp->buf_len - 1;
	
	return 1;
}

/* Check out the current request */
void Ptx_Ckout_Curr_Rqst(Ptx_Control *ptp)
{
	if (ptp->idx_out)
		--(ptp->idx_out);
	else
		ptp->idx_out = ptp->buf_len - 1;
	
	if (ptp->idx_in == ptp->idx_out)
		ptp->curr_rqst = 0;
	else
		ptp->curr_rqst = &(ptp->rqst_buf[ptp->idx_out]);
}
