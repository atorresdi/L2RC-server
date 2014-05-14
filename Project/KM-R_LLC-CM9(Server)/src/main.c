/* Includes ------------------------------------------------------------------*/
#include "timing.h"
#include "usb_vcp.h"
#include "debug.h"

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"

#ifdef DEBUG_ENABLE
/* Test variables---------------------------------*/
unsigned char rx_count = 0;
unsigned int i;
unsigned int j;
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
struct Vcp_Control c_vcp;
unsigned char vcp_buf_in[VCP_BUF_IN_LEN];

int main(void)
{			
	/* Time module initialization */
	Tm_Define(&c_time, NUM_PERIOD, periods, NUM_TIMEOUT, timeouts);
	
	#ifdef DEBUG_ENABLE
	/* Debug module initialization */
	Db_Define();
	Tm_Start_Period(&c_time, TEST_PERIOD_NUM, TEST_PERIOD_VAL);
	#endif
	
	/* Virtual COM port module initialization */
	Vcp_Define(&c_vcp, vcp_buf_in);

	/* USB VCP initialization */
	Set_System();
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();
	Receive_length = 0;
	
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
			
			#ifdef DEBUG_ENABLE
			
			if (Tm_Period_Complete(&c_time, TEST_PERIOD_NUM))
			{
				Db_Print_Line(" ");
				Tm_Clean_Period(&c_time, TEST_PERIOD_NUM);
				CDC_Send_DATA(&vcp_test_var[33],64);
			}
			
			if (Vcp_Data_Avail(&c_vcp))
			{
				Db_Print_Val(Vcp_Take_Data(&c_vcp), ASTERISK);
				
				rx_count++;
				
				if (rx_count >= 16)
				{
					CDC_Send_DATA(&vcp_test_var[j*16],16);

					rx_count = 0;
					j++;
				};
			};
			
			if (j >= 16)
				j = 0;
			#endif
		}
		else
			Db_Print_Char(EXCLAMATION);
	};
} 
