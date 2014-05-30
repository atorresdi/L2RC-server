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

#ifdef DEBUG_ENABLE
/* Test variables---------------------------------*/
unsigned char rx_count = 0;
uint8_t count = 255;
uint16_t i;
unsigned int j = 0;
uint8_t state = 255;
uint8_t state1 = 255;
Pro_Package *pkg;
unsigned char vcp_test_var[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,\
												 36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,\
												 69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,\
												 102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,\
												 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,\
												 154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,\
												 180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,\
												 206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,\
												 232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};
/* ------------------------------------------------*/
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
	Tm_Start_Period(&c_time, TEST_PERIOD_NUM, TEST_PERIOD_VAL);
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
			
				if (Tm_Period_Complete(&c_time, TEST_PERIOD_NUM))
				{
					Db_Print_Line(" ");
					Tm_Clean_Period(&c_time, TEST_PERIOD_NUM);
				};
				
// 				if (Prx_Pkg_Avail(&c_prx))
// 				{
// 					Pro_Package *pkg_p;
// 					pkg_p = Prx_Get_Pkg(&c_prx);
// 					
// 					Db_Print_Val('+', pkg_p->length);
// 					Db_Print_Val('-', pkg_p->opts);
// 					Db_Print_Val('/', pkg_p->ptsf);
// 					
// 					for (i = 0; i < pkg_p->length; i++)
// 						Db_Print_Val('*', pkg_p->data[i]);
// 					
// 				};
				
				if (c_rds.state != state1)
				{
					state1 = c_rds.state;
					Db_Print_Val('~', state1);
				};
				
// 				if (c_dax.dax_state != state)
// 				{
// 					state = c_dax.dax_state;
// 					Db_Print_Val('/', state);
// 				};

// 				if (c_ptx.state != state)
// 				{
// 					state = c_ptx.state;
// 					Db_Print_Val(state, TILDE);
// 				};
			#endif
		}
		else
			Db_Print_Line("USB disconected");
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
