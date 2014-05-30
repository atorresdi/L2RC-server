/* dxl_ax.c 	Implementation of the Dynamixel AX series driver module */

#include "dxl_ax.h"
#include "debug.h"
#include "stm32f10x.h"
#include <cstdlib>

/* static variables */
static Error dax_err;

/* static routines */
static void Dax_Set_Port_Dir(Dax_Control *dap, uint8_t dir)
{
	if (dir)
	{
		dap->flags |= F_DAX_PORT_DIR;		/* transmision */
		GPIOB->BSRR |= 0x00000020;      /* PB5 High: Transmission mode	*/
	}
	else
	{
		dap->flags &= ~F_DAX_PORT_DIR;		 /* reception */
		GPIOB->BRR |= 0x00000020;      		 /* PB5 Low: Reception mode	*/		
	};
}

static void Dax_Start_Port_Write(Dax_Control *dap)
{
	Dax_Set_Port_Dir(dap, 1);					/* transmision */
	dap->flags &= ~F_DAX_TX_COMPLETE;
	dap->flags &= ~F_DAX_STUS_PKG_AVAIL;
	
	dap->inst_pkg_idx = 1;
	USART1->SR &= ~USART_FLAG_TXE;
	USART1->DR = dap->inst_pkg[0];
	USART1->CR1 |= USART_CR1_TXEIE;									/* TXE interrupt enable */
}

/* Initialization */
void Dax_Define(Dax_Control *dap,
								uint8_t dev_num,
								uint8_t *id,
								uint8_t *inst_pkg,
								uint8_t *stus_pkg)
{
	NVIC_InitTypeDef NVIC_InitStruct;
	
	dap->flags = 0;
	dap->dax_state = DAX_WAIT_RQST;
	dap->rx_state = DAX_WAIT_SOP;
	
	dap->dev_num = dev_num;
	dap->id = id;
	dap->curr_id = id;
	
	dap->inst_pkg = inst_pkg;
	dap->inst_pkg[0] = dap->inst_pkg[1] = DAX_SOP;
	dap->stus_pkg = stus_pkg;
	
	dap->data_rd = (uint8_t *)malloc(dev_num*2);
	
	/* HW set up --------------------- */
	
	/* GPIOB */
	RCC->APB2ENR |= 0x00000009;			/* GPIOB, AFIO Clk enable */
	AFIO->MAPR |= 0x00000004;				/* Remap USART 1 pins; TX/PB6, RX/PB7 */
	GPIOB->CRL = 0x4a344444;				/* PB6 as AF Output, PB7 as floating input, PB5 as Output */
	GPIOB->BSRR |= 0x00200000;      /* PB5 Low: Reception mode	*/
	
	/* USART1 */
	RCC->APB2ENR |= 0x00004000;			/* USART1 Clk enable */
	USART1->CR1 |= 0x00002000;			/* USART1 enable */
 	USART1->BRR = 0x00000047;				/* 1 Mbps	*/
	USART1->CR1 |= 0x0000000c;			/* RX, TX enable */
	
	/* USART1 Interrupt */
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	/* TIM4 */	
	RCC->APB1ENR |= 0x04UL;						/* Enable TIM3 clock	*/
	TIM4->PSC = 0x0e10;								/* Divide timer clock by 3600, tim_clk = 72MHz / 3.6 k = 20kHz	*/
	TIM4->ARR = 0x0f00;								/* Counter stops when reach 10 (0.5 ms)	*/
	TIM4->CR1 |= 0x0080;							/* Enable ARR (ARR value is buffered and determines the end value of the count)	*/
	TIM4->CR1 |= 0x0001;							/* Enable Counter */
	
	/* TIM4 interrupt */
	NVIC_InitStruct.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

/* State machine */
void Dax_Process(Dax_Control *dap)
{
	switch (dap->dax_state)
	{
		case DAX_WAIT_RQST:
			
			if (dap->flags & F_DAX_RQST_AVAIL)
			{
				dap->dev_count = 0;
				dap->curr_id = dap->id;
				dap->inst_pkg_idx = 0;
				
				dap->dax_state = DAX_SEND_INST_PKG;
			};

			break;
			
		case DAX_SEND_INST_PKG:
		
			dap->inst_pkg[DAX_ID_POS] = *dap->curr_id;
			*dap->pkg_cksum_p = dap->cksum_base + *dap->curr_id;					/* Checksum */
		
			if (dap->inst_pkg[DAX_INST_POS] == DAX_INST_WRITE)
			{
				uint8_t n = dap->data_size;
				uint8_t *inst_pkg = &dap->inst_pkg[DAX_ADDR_POS + 1];
				
				for ( ; n; n--, inst_pkg++, (dap->data_wr)++)
				{
					*inst_pkg = *(dap->data_wr);
					*dap->pkg_cksum_p += *(dap->data_wr);
				};
			};
			
			*dap->pkg_cksum_p = ~(*dap->pkg_cksum_p);
			Dax_Start_Port_Write(dap);
			
			if (dap->flags & F_DAX_WAIT_STUS_PKG)
				dap->dax_state = DAX_WAIT_STUS_PKG;
			else
				dap->dax_state = DAX_WAIT_TX_COMPLETE;
			
			#ifdef DAX_DEBUG_ENABLE
				{
				uint8_t n = dap->pkg_length;
				uint8_t *inst_pkg = dap->inst_pkg;
				
				Db_Print_Val('$', dap->pkg_length);
				
				for ( ; n; n--, inst_pkg++)
					Db_Print_Val('*', *inst_pkg);
				
				Db_Print_Val('k', dap->cksum_base);
				}
			#endif
			
			break;
			
		case DAX_WAIT_STUS_PKG:
			
			if (dap->flags & F_DAX_STUS_PKG_AVAIL)
			{				
				uint8_t *stus_pkg = &dap->stus_pkg[DAX_ID_POS];
				uint8_t n;
				uint8_t rxed_cksum = dap->stus_pkg[DAX_LENGTH_POS + dap->stus_pkg[DAX_LENGTH_POS]];
				
				#ifdef DAX_DEBUG_ENABLE
					for (n = dap->stus_pkg[DAX_LENGTH_POS] + 1; n; n--, stus_pkg++)
						Db_Print_Val('*', *stus_pkg);
					stus_pkg = &dap->stus_pkg[DAX_ID_POS];
				#endif
				
				dap->cksum = 0;
				
				for (n = dap->stus_pkg[DAX_LENGTH_POS] + 1; n; n--, stus_pkg++)				/* checksum calculation */
					dap->cksum += *stus_pkg;

				dap->cksum = ~(dap->cksum);
				
				if ( (((uint8_t)dap->cksum) == rxed_cksum) && (!(dap->stus_pkg[DAX_ERR_POS])) )			/* checksum and package error field verification */
				{
					dap->err_num = 0;
					
					if (dap->inst_pkg[DAX_INST_POS] == DAX_INST_READ)
					{
						stus_pkg = &dap->stus_pkg[DAX_1ST_PARAM_POS];
						n = dap->data_size;
						
						for ( ; n; n--, stus_pkg++, (dap->data_rd_p)++)
							*dap->data_rd_p = *stus_pkg;
					};
					
					(dap->dev_count)++;
					
					if (dap->dev_count >= dap->dev_num)					/* Request completed */
					{
						dap->flags |= F_DAX_RQST_COMPLETE;
						dap->flags &= ~F_DAX_RQST_AVAIL;
						
						if (dap->inst_pkg[DAX_INST_POS] == DAX_INST_READ)
							dap->flags |= F_DAX_RD_DATA_AVAIL;
						
						dap->dax_state = DAX_WAIT_RQST;
					}
					else
					{
						(dap->curr_id)++;
						dap->dax_state = DAX_SEND_INST_PKG;
					};
				}
				else					/* checksum error or package error filed != 0 */
				{
					dap->err_num++;
					
					if (dap->err_num >= DAX_MAX_ERR_NUM)
					{
						dax_err.dev_instance = *dap->curr_id;
						
						if (dap->stus_pkg[DAX_ERR_POS])
							dax_err.err_flags |= dap->stus_pkg[DAX_ERR_POS];
						else
							dax_err.err_flags |= F_DAX_CKSUM_ERR;
						
						dap->flags |= F_DAX_ERR;
						dap->flags &= ~F_DAX_RQST_AVAIL;
						dap->dax_state = DAX_WAIT_RQST;
						break;
					};
					
					Dax_Start_Port_Write(dap);
				};
			}
			else if (dap->flags & F_DAX_RX_TIMEOUT)
			{
				dap->err_num++;
					
				if (dap->err_num >= DAX_MAX_ERR_NUM)
				{
					dax_err.dev_instance = *dap->curr_id;
					dax_err.err_flags |= F_DAX_RX_TIMEOUT_ERR;
					
					dap->flags |= F_DAX_ERR;
					dap->flags &= ~F_DAX_RQST_AVAIL;
					dap->dax_state = DAX_WAIT_RQST;
					break;
				};
				
				Dax_Start_Port_Write(dap);
			};
		
			break;

		case DAX_WAIT_TX_COMPLETE:
			
			if (dap->flags & F_DAX_TX_COMPLETE)
			{
				(dap->dev_count)++;
				
				if (dap->dev_count >= dap->dev_num)					/* Request completed */
				{
					dap->flags |= F_DAX_RQST_COMPLETE;
					dap->flags &= ~F_DAX_RQST_AVAIL;
					
					dap->dax_state = DAX_WAIT_RQST;
				}
				else
				{
					(dap->curr_id)++;
					dap->dax_state = DAX_SEND_INST_PKG;
				};
			};
		
			break;
	};
}

/* Make a request for sending a PING instruction package to all Dynamixel AX devices */
void Dax_Ping_Rqst(Dax_Control *dap)
{
	dap->pkg_length = DAX_PING_PKG_LEN;
	
	dap->inst_pkg[DAX_LENGTH_POS] = DAX_PING_LENGTH;					/* package length = parameter num + 2 */
	dap->inst_pkg[DAX_INST_POS] = DAX_INST_PING;							/* package instruction value */
	
	dap->cksum_base	= DAX_PING_LENGTH + DAX_INST_PING;
	dap->pkg_cksum_p = &dap->inst_pkg[DAX_PING_PKG_LEN - 1];				/* points to the checksum position of the package */
	
	dap->flags &= ~F_DAX_RQST_COMPLETE;
	dap->flags |= F_DAX_RQST_AVAIL;
}

/* Make a request for sending a WRITE instruction package to all Dynamixel AX devices */
void Dax_Write_Rqst(Dax_Control *dap, uint8_t address, uint8_t data_size, uint8_t *data_wr)
{
	if (data_size > 1)
		return;
	
	dap->data_wr = data_wr;
	dap->data_size = 1 << data_size;
	dap->pkg_length = DAX_WR_PKG_LEN_BASE + dap->data_size;
	
	dap->inst_pkg[DAX_LENGTH_POS] = DAX_WRITE_LENGTH_BASE + dap->data_size;					/* package length = parameter num + 2 */
	dap->inst_pkg[DAX_INST_POS] = DAX_INST_WRITE;																		/* package instruction value */
	dap->inst_pkg[DAX_ADDR_POS] = address;																					/* address of the written parameter */
		
	dap->cksum_base	= dap->inst_pkg[DAX_LENGTH_POS] + DAX_INST_WRITE + address;
	dap->pkg_cksum_p = &dap->inst_pkg[dap->pkg_length - 1];								/* points to the checksum position of the package */
	
	dap->flags &= ~F_DAX_RQST_COMPLETE;
	dap->flags |= F_DAX_RQST_AVAIL;
}

/* Make a request for sending a READ instruction package to all Dynamixel AX devices */
void Dax_Read_Rqst(Dax_Control *dap, uint8_t address, uint8_t data_size)
{
	if (data_size > 1)
		return;
	
	dap->data_rd_p = dap->data_rd;
	dap->data_size = 1 << data_size;
	dap->pkg_length = DAX_RD_PKG_LEN;
	
	dap->inst_pkg[DAX_LENGTH_POS] = DAX_RD_LENGTH;					/* package length = parameter num + 2 */
	dap->inst_pkg[DAX_INST_POS] = DAX_INST_READ;							/* package instruction value */
	dap->inst_pkg[DAX_ADDR_POS] = address;
	dap->inst_pkg[DAX_DATA_SIZE_POS] = dap->data_size;
	
	dap->cksum_base	= DAX_RD_LENGTH + DAX_INST_READ + address + dap->data_size;
	dap->pkg_cksum_p = &dap->inst_pkg[DAX_RD_PKG_LEN - 1];				/* points to the checksum position of the package */
	
	dap->flags &= ~F_DAX_RQST_COMPLETE;
	dap->flags &= ~F_DAX_RD_DATA_AVAIL;
	dap->flags |= F_DAX_RQST_AVAIL;
}

/* Send an status package */
void Dax_Port_Write(volatile struct Dax_Control *dap)
{
	if (!(dap->flags & F_DAX_TX_COMPLETE))
	{
		USART1->DR = dap->inst_pkg[dap->inst_pkg_idx];
		(dap->inst_pkg_idx)++;
		
		if (dap->inst_pkg_idx >= dap->pkg_length)
			dap->flags |= F_DAX_TX_COMPLETE;
	}
	else
	{
		/* Delay --------------------- */				/* avoids corrupting the last sent byte by the change of direction */
		if (1)
		{
			int n;
			int k;
			
			for (n = 0; n <= 80; n++)
				k = n;
		};
		/* ----------------------Delay */
		
		GPIOB->BRR |= 0x00000020;      				/* PB5 low, reception mode	*/
		dap->flags &= ~F_DAX_PORT_DIR;				/* rx mode */
		USART1->CR1 &= ~(USART_CR1_TXEIE);    /* TXE interrupt disable */
		
		if (dap->flags & F_DAX_WAIT_STUS_PKG)
		{				
			TIM4->CNT = 0x0000;											/* setup reception timeout */
			TIM4->SR &= ~(TIM_SR_UIF);
			
			TIM4->DIER |= TIM_DIER_UIE;          		/* TIM4 Update Interrupt enable  */	
			USART1->SR &= ~USART_SR_RXNE;						/* Reset RXNE flag */
			USART1->CR1 |= USART_CR1_RXNEIE;			  /* RXNE interrupt enable */
		};
	};	
}

/* Receive a status package */
void Dax_Port_Read(volatile struct Dax_Control *dap)
{
	TIM4->CNT = 0x0000;							/* restart timer */
  TIM4->SR &= ~(TIM_SR_UIF);
	
	switch(dap->rx_state)
	{
		case DAX_WAIT_SOP:
			
			if(USART1->DR == DAX_SOP)
				dap->rx_state = DAX_WAIT_ID;
		
			break;
			
		case DAX_WAIT_ID:
			
			if(USART1->DR == DAX_SOP)
				break;
			
			dap->stus_pkg[DAX_ID_POS] = USART1->DR;
			dap->rx_state = DAX_WAIT_LENGTH;
			
			break;
			
		case DAX_WAIT_LENGTH:
			
			dap->stus_pkg[DAX_LENGTH_POS] = USART1->DR;
			dap->rxed_data_count = 0;
			dap->rx_state = DAX_WAIT_REST_OF_PKG;
		
			break;
		
		case DAX_WAIT_REST_OF_PKG:
			
			dap->stus_pkg[DAX_ERR_POS + dap->rxed_data_count] = USART1->DR;
			(dap->rxed_data_count)++;
			
			if (dap->rxed_data_count <  dap->stus_pkg[DAX_LENGTH_POS])
				break;
			
			TIM4->DIER &= ~TIM_DIER_UIE;						/* TIM4 Update Interrupt disable  */		
			USART1->CR1 &= ~USART_CR1_RXNEIE;				/* RXNE interrupt disable */
			
			dap->flags |= F_DAX_STUS_PKG_AVAIL;
			dap->rx_state = DAX_WAIT_SOP;
			
			break;
	};
}

/* Define if a status packet must be waited after sending an instruction package */
void Dax_Set_Stus_Rtn_Lvl(Dax_Control *dap, uint8_t return_level)
{
	if (return_level)
		dap->flags |= F_DAX_WAIT_STUS_PKG;
	else
		dap->flags &= ~F_DAX_WAIT_STUS_PKG;
}

/* Check if the request was completed */
uint8_t Dax_Rqst_Complete(Dax_Control *dap)
{
	return (dap->flags & F_DAX_RQST_COMPLETE);
}

/* Check if data is available after a read operation */
uint8_t Dax_Rd_Data_Avail(Dax_Control *dap)
{
	if (dap->flags & F_DAX_RD_DATA_AVAIL)
		return 1;
	else
		return 0;
}

/* Get the obtained data after a read operation */
uint8_t *Dax_Get_Rd_Data(Dax_Control *dap)
{
	dap->flags &= ~F_DAX_RD_DATA_AVAIL;
	return dap->data_rd;
}

uint8_t Dax_Err(Dax_Control *dap)
{
	return (dap->flags & F_DAX_ERR);
}

Error *Dax_Get_Err(Dax_Control *dap)
{
	dap->flags &= ~F_DAX_ERR;
	return (&dax_err);
}

