/* timing.c Implementation of the time module */

#include "timing.h"
#include <stm32f10x.h>
#include "debug.h"
void Tm_Define(struct Tm_Control *tcp,
							 unsigned char num_period,
							 struct Tm_Period *pp,
							 unsigned char num_timeout,
							 unsigned int *tp)
		{
			unsigned char n;
			
			tcp->num_period = num_period;
			tcp->period = pp;
			
			for(n = num_period; n; --n, ++pp)
				pp->flags = 0;
			
			tcp->num_timeout = num_timeout;
			tcp->timeout = tp;
			
			for(n = num_timeout; n; --n, tp++)
				*tp = 0;			
			
			/* HW setup */
			
			RCC->APB1ENR |= 0x00000001;				/* Enable TIM2 clock	*/
			TIM2->PSC = 0x1c20;								/* Divide timer clock by 7200, tim_clk = 72MHz / 7.2 k = 10kHz	*/
			TIM2->ARR = 0x000a;								/* Counter stops when reach 10 (1 ms)	*/
			TIM2->CR1 |= 0x0080;							/* Enable ARR (ARR value is buffered and determines the end value of the count)	*/
			TIM2->CR1 |= 0x0001;							/* Enable Counter */
		};
		
void Tm_Process(struct Tm_Control *tcp)
		{
			unsigned char n;
			struct Tm_Period *pp;
			unsigned int *tp;
			
			for (n = tcp->num_period, pp = tcp->period; n ; --n, ++pp)
				{
					if (((pp->flags) & TM_PERIOD_ACTIVE))
						{
						--(pp->counter);
						
						if (!(pp->counter))
							{
								pp->flags |= TM_PERIOD_END;
								pp->counter = pp->period;
							};
						};
					};
						
			for (n = tcp->num_timeout, tp = tcp->timeout; n; --n, tp++)
				{
					if(*tp)
						--(*tp);
				}		
		};
		
void Tm_Start_Period(struct Tm_Control *tcp,
											 unsigned char num_period,
											 unsigned int period)
		{
			struct Tm_Period *pp = (tcp->period) + num_period;
			
			if (num_period >= (tcp->num_period))
				return;
			
			pp->flags |= TM_PERIOD_ACTIVE;
			pp->period = pp->counter = period;
		};
		
char Tm_Period_Complete(const struct Tm_Control *tcp,
										 unsigned char num_period)
		{
			return (tcp->period[num_period].flags & TM_PERIOD_END);
		};
		
void Tm_Clean_Period(struct Tm_Control *tcp,
										 unsigned char num_period)
		{
			tcp->period[num_period].flags &= ~(TM_PERIOD_END);
		};
		
void Tm_End_Period(struct Tm_Control *tcp,
												unsigned char num_period)
		{
			tcp->period[num_period].flags &= ~(TM_PERIOD_ACTIVE);
		};
		
void Tm_Start_Timeout(struct Tm_Control *tcp,
											 unsigned char num_timeout,
											 unsigned int timeout)
		{
			tcp->timeout[num_timeout] = timeout;
		};
		
char Tm_Timeout_Complete(const struct Tm_Control *tcp,
										 unsigned char num_timeout)
		{
			return(!(tcp->timeout[num_timeout]));
		};
		
char Handle_Timer (void)
		{
			if (TIM2->SR & TIMER_OVERFLOW)
				{
					TIM2->SR &= ~(TIMER_OVERFLOW);
					return 1;
				}
			else
				return 0;
		};
