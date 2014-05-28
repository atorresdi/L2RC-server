#include "debug.h"
#include <cstdio>
#include "stm32f10x.h"

char atoip[4] = {0};

void Db_Define(void)
{
	/* HW setup */

	/* GPIO setup */
	RCC->APB2ENR |= 0x00000004;			/* GPIOA Clk enable */
	GPIOA->CRL |= 0x00000a00;				/* PA2 as AF Output */
	GPIOA->CRL &= ~(0x00000500);		/* PA2 as AF Output */
	
	/* USART setup */
	RCC->APB1ENR |= 0x00020000;			/* USART2 Clk enable */
	USART2->CR1 |= 0x00002000;			/* USART2 enable */
	USART2->BRR = BAUD_RATE;				/* 115200 bps	*/
	USART2->CR1 |= 0x00000008;			/* TX enable */
}

void Db_Print_Val( unsigned char id, unsigned char ch)
{
	unsigned char i;
	
// 	while (!(USART2->SR & TXE)){ };
// 	USART2->DR = 32;
	
	while (!(USART2->SR & TXE)){ };
	USART2->DR = id;
	
	sprintf(atoip,"%d",ch);
				
	for( i = 0; atoip[i]; i++)
	{
		while (!(USART2->SR & TXE)){ };
		USART2->DR = atoip[i];
	};
}

void Db_Print_Char(unsigned char ch)
{
	while (!(USART2->SR & TXE)){ };
	USART2->DR = ch;
}

void Db_Print_Str(char *str)
{	
	while (*str)
	{
		Db_Print_Char(*str);
		str++;
	};
}

void Db_Print_Line(char *str)
{
	char suffix[] = {0x0a, 0x0d};
	
	Db_Print_Str("> ");
	Db_Print_Str(str);
	Db_Print_Str(suffix);	
}
