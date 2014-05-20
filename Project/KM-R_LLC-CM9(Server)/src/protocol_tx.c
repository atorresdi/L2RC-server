/* protocol_tx.c Implementation of the Comunication Protocol Transmision module  */

#include "protocol_tx.h"
#include "hw_config.h"
#include "usb_vcp.h"
#include "debug.h"

/* Extern variables */
extern Prx_Control c_prx;

/* Static routines ------------------------------------------- */
/* Check out the current request */
static void Ptx_Ckout_Curr_Rqst(Ptx_Control *ptp)
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
/* --------------------------------------------Static routines */

/* Initialization */
void Ptx_Define(Ptx_Control *ptp,
								uint8_t *sec_num,
								Ptx_Request *rqst_buf,
								uint8_t buf_len)
{
	ptp->flags = 0;
	ptp->state = PTX_WAIT_RQST;
	ptp->sec_num = sec_num;
	*(sec_num) = 0;
	ptp->cksum = 0;
	
	ptp->buf_len = buf_len;
	ptp->idx_in = ptp->idx_out = buf_len - 1;
	ptp->rqst_buf = rqst_buf;
	ptp->curr_rqst = 0;						/* will point to ptp->rqst_buf[ptp->idx_out] */
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
				else if (ptp->curr_rqst->type == PTX_PKG_RQST)
				{
					uint8_t header[2];
					header[0] = SOH;
					header[1] = *(ptp->sec_num);
					
					CDC_Send_DATA(header, 2);
					ptp->state = PTX_WAIT_1ST_SYN;
				};
			};
			
			break;

		case PTX_WAIT_ACK:
			
			if (Prx_Ctrl_Ch_Avail(&c_prx))
			{
				uint8_t ctrl_ch = Prx_Get_Ctrl_Ch(&c_prx);			
				
				if ( ctrl_ch == ACK )
				{
					if (ptp->curr_rqst->type == PTX_PKG_RQST)
						(*(ptp->sec_num))++;
					
					Ptx_Ckout_Curr_Rqst(ptp);			
				};					
				
				ptp->state = PTX_WAIT_RQST;
			};
		
			break;
			
		case PTX_WAIT_1ST_SYN:
			
			if (Prx_Ctrl_Ch_Avail(&c_prx))
			{
				uint8_t header[3];
				uint8_t *header_p = header;
				uint8_t n = Prx_Get_Ctrl_Ch(&c_prx);			
				
				if ( n == NAK )
				{
					ptp->state = PTX_WAIT_RQST;			
					break;
				};
				
				if (ptp->curr_rqst->pkg.length == 256)			/* a 256 length is represented by a zero */
					header[0] = 0;
				else
					header[0] = ptp->curr_rqst->pkg.length;
				
				header[1] = ptp->curr_rqst->pkg.opts;
				header[2] = ptp->curr_rqst->pkg.ptsf;
				
				CDC_Send_DATA(header, 3);
				
				for (n = 3; n; n--, header_p++)
					ptp->cksum += *header_p;
				
				ptp->curr_rqst->sent_data_num = 0;
				ptp->state = PTX_WAIT_2ND_SYN;
			};
			
			break;
			
		case PTX_WAIT_2ND_SYN:
			
			if (Prx_Ctrl_Ch_Avail(&c_prx))
			{
				uint8_t n = Prx_Get_Ctrl_Ch(&c_prx);			
				uint8_t data_length;
				uint8_t *data_p = &(ptp->curr_rqst->pkg.data[ptp->curr_rqst->sent_data_num]);			/* points to the first byte to be sent */
				
				if ( n == NAK )
				{
					ptp->state = PTX_WAIT_RQST;
					break;
				};
				
				if ( (ptp->curr_rqst->pkg.length - ptp->curr_rqst->sent_data_num) >= VCP_BUF_OUT_LEN )		/* if ( (pkg.length - sent_data_num) >= VCP_BUF_OUT_LEN */
				{						
					data_length = VCP_BUF_OUT_LEN;
					CDC_Send_DATA(data_p, VCP_BUF_OUT_LEN);
					ptp->curr_rqst->sent_data_num += VCP_BUF_OUT_LEN;							
				}
				else
				{
					data_length = ptp->curr_rqst->pkg.length - ptp->curr_rqst->sent_data_num;
					CDC_Send_DATA(data_p, data_length);
					ptp->curr_rqst->sent_data_num += data_length;
				};
				
				for (n = data_length; n; n--, data_p++)
						ptp->cksum += *data_p;
				
				if (ptp->curr_rqst->sent_data_num < ptp->curr_rqst->pkg.length)
						break;
				
				ptp->state = PTX_WAIT_3RD_SYN;
			};
			
			break;			
			
			case PTX_WAIT_3RD_SYN:
				
				if (Prx_Ctrl_Ch_Avail(&c_prx))
				{
					uint8_t n = Prx_Get_Ctrl_Ch(&c_prx);	
					
					if ( n == NAK )
					{
						ptp->state = PTX_WAIT_RQST;
						break;
					};
			
					CDC_Send_DATA(&ptp->cksum, 1);
					ptp->cksum = 0;
					ptp->state = PTX_WAIT_ACK;
				};
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

/* Add a package transmision request */
uint8_t Ptx_Add_Pkg_Rqst(Ptx_Control *ptp,
												 uint16_t length,
												 uint8_t opts,
												 uint8_t ptsf,
												 uint8_t *data)
{
	/* Check for free space in buffer */
	if (ptp->curr_rqst && (ptp->idx_in == ptp->idx_out))
		return 0;
	
	ptp->rqst_buf[ptp->idx_in].type = PTX_PKG_RQST;
	ptp->rqst_buf[ptp->idx_in].pkg.length = length;
	ptp->rqst_buf[ptp->idx_in].pkg.opts = opts;
	ptp->rqst_buf[ptp->idx_in].pkg.ptsf = ptsf;
	ptp->rqst_buf[ptp->idx_in].pkg.data = data;
	
	if ( !(ptp->curr_rqst) )
		ptp->curr_rqst = &(ptp->rqst_buf[ptp->idx_in]);
	
	if (ptp->idx_in)
		--(ptp->idx_in);
	else
		ptp->idx_in = ptp->buf_len - 1;
	
	return 1;
}

/* Calculate the package options field */
uint8_t Ptx_Set_Pkg_Opts(uint8_t pkg_type,
												uint8_t device,
												uint8_t data_size)
{
	uint8_t opts = 0;
	
	opts |= ( (pkg_type  << 6 ) & 0xc0 );
	opts |= ( (device << 2) & 0x3c );
	opts |= (data_size & 0x03);
	
	return opts;
}
