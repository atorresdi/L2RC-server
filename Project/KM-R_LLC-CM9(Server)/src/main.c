/* Includes ------------------------------------------------------------------*/
#include "timing.h"
#include "usb_vcp.h"
#include "protocol_rx.h"
#include "protocol_tx.h"
#include "rdd_server.h"
#include "dxl_ax.h"

#include "error.h"
#include "debug.h"

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"

/* debug vars */
#ifdef DEBUG_ENABLE
	uint8_t state = 255;
#endif
	
/* Extern variables ----------------------------------------------------------*/
extern __IO uint8_t Receive_Buffer[64];
extern __IO  uint32_t Receive_length;
extern __IO  uint32_t length;
uint8_t Send_Buffer[64];
uint32_t packet_sent = 1;
uint32_t packet_receive = 1;

/* Time module variables */
struct Tm_Control c_time;
struct Tm_Period periods[NUM_PERIOD];		/* NUM_PERIOD can be set in timing.h */
unsigned int timeouts[NUM_TIMEOUT];			/* NUM_TIMEOUT can be set in timing.h */

/* Virtual COM port module variables */
Vcp_Control c_vcp;
uint8_t vcp_buf_in[VCP_BUF_LEN];

/* Communication protocol secuence number */
uint8_t prot_sec_num = 0;

/* Communication protocol receiver variables */
Prx_Control c_prx;
uint8_t prx_cmd_buf[PRX_CMD_BUF_LEN];
Pro_Package prx_pkg_buf[PRX_PKG_BUF_LEN];
uint8_t prx_pkg_data[PRX_PKG_BUF_LEN][PRX_MAX_PKG_DATA_LEN];

/* Communication protocol transmiter variables */
Ptx_Control c_ptx;
Ptx_Request ptx_rqst_buf[PTX_RQST_BUF_LEN];

/* Robot Device Driver server variables */
Rds_Control c_rds;

/* Dynamixel AX series driver variables */
Dax_Control c_dax;
uint8_t dax_inst_pkg[DAX_INST_PKG_MAX_LEN];
uint8_t dax_stus_pkg[DAX_STUS_PKG_MAX_LEN];

int main(void)
{			
	/* Time module initialization */
	Tm_Define(&c_time, NUM_PERIOD, periods, NUM_TIMEOUT, timeouts);
	
	#ifdef DEBUG_ENABLE
		/* Debug module initialization */
		Db_Define();
	#endif

	/* USB VCP initialization */
	Set_System();
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();
	Receive_length = 0;
	
	/* Virtual COM port module initialization */
	Vcp_Define(&c_vcp, vcp_buf_in);
	
	/* Communication protocol receiver initialization */
	Prx_Define(&c_prx,
						 &prot_sec_num,
						 prx_cmd_buf,
						 PRX_CMD_BUF_LEN,
						 prx_pkg_buf,
						 PRX_PKG_BUF_LEN,
						 prx_pkg_data);
	
	/* Communication protocol transmiter initialization */
	Ptx_Define(&c_ptx, &prot_sec_num, ptx_rqst_buf, PTX_RQST_BUF_LEN);
	
	/* Robot Device Driver server initialization */
	Rds_Define(&c_rds);
	
	while (bDeviceState != CONFIGURED){	};
	
	#ifdef DEBUG_ENABLE
		Db_Print_Line(" ");
		Db_Print_Line("Modules initialization complete");
		Db_Print_Line("Virtual COM Port configured");
		Db_Print_Line(" ");
	#endif
	
	while(1)
	{
		if (bDeviceState == CONFIGURED)
		{			
			if (Handle_Timer())
				Tm_Process(&c_time);
			
			if (Receive_length > 0)
				Vcp_Process(&c_vcp);
			
			Prx_Process(&c_prx);
			
			Ptx_Process(&c_ptx);		

			if (c_rds.dev_ena_flags & F_RDS_DXL_AX_ENABLE)
				Dax_Process(&c_dax);
			
			if (c_rds.flags & F_RDS_CONFIGURED)
				Rds_Process(&c_rds);
			else 
				Rds_Configure(&c_rds);
			
			#ifdef DEBUG_ENABLE
				if (c_rds.state != state)
				{
					state = c_rds.state;
					Db_Print_Val('~', state);
				};
			#endif
		}
		else
		{
			#ifdef DEBUG_ENABLE
			Db_Print_Line("USB disconected");
			#endif
		};
	};
} 

void USART1_IRQHandler(void)
{		
	if ( (USART1->SR & USART_SR_RXNE) || (USART1->SR & USART_SR_TXE) )
	{
		if (c_dax.flags & F_DAX_PORT_DIR)
			Dax_Port_Write(&c_dax);
		else
			Dax_Port_Read(&c_dax);
	};
}

void TIM4_IRQHandler(void)
{
	if(TIM4->SR & TIM_SR_UIF) 
	{
		TIM4->SR &= ~TIM_SR_UIF; 
		TIM4->DIER &= ~(TIM_DIER_UIE);						/* TIM4 Update Interrupt disable  */
		USART1->CR1 &= ~(USART_CR1_RXNEIE);			  /* RXNE interrupt disable */
		c_dax.flags |= F_DAX_RX_TIMEOUT;
	};
}
