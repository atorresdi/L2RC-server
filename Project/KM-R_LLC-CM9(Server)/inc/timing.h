/* timing.h Definitions for the time module */

#if !defined(TIME_MODULE_H)

#define TIME_MODULE_H

/* Timeouts and periods definition */

#define TIME_BASE		0.003125		/* 3.125 ms */

/* Timeouts and periods quantity */

#define NUM_PERIOD		1
#define NUM_TIMEOUT		1

/* Periods */		/* Each time unit is equal to 1 ms	*/
#define INSTR_EXEC_PERIOD_NUM		0

/* Timeouts */
#define RDS_DAX_INIT_POS_TOUT_NUM		0
#define RDS_DAX_INIT_POS_TOUT_VAL		3000			/* 3 secs */

/* HW flags */

#define TIMER_OVERFLOW			0x0001U

/* SW flags */

#define TM_PERIOD_ACTIVE		0x01U
#define TM_PERIOD_END				0x02U

/* Tipos de datos */

struct Tm_Period{
	unsigned char flags;
	unsigned int counter,
							 period;	
};

struct Tm_Control{
	unsigned char num_period;
	struct Tm_Period *period;
	
	unsigned char num_timeout;
	unsigned int *timeout;
};

/* Rutinas */ 

void Tm_Define(struct Tm_Control *tcp,
							 unsigned char num_period,
							 struct Tm_Period *pp,
							 unsigned char num_timeout,
							 unsigned int *tp);

void Tm_Process(struct Tm_Control *tcp);

void Tm_Start_Period(struct Tm_Control *tcp,
											 unsigned char num_period,
											 unsigned int period);

char Tm_Period_Complete(const struct Tm_Control *tcp,
										 unsigned char num_period);

void Tm_Clean_Period(struct Tm_Control *tcp,
										 unsigned char num_period);

void Tm_End_Period(struct Tm_Control *tcp,
										unsigned char num_period);
												
void Tm_Start_Timeout(struct Tm_Control *tcp,
											 unsigned char num_timeout,
											 unsigned int timeout);
											 
char Tm_Timeout_Complete(const struct Tm_Control *tcp,
													unsigned char num_timeout);
										 
char Handle_Timer (void);
							
#endif
