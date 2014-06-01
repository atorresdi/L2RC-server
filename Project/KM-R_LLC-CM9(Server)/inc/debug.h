#if !defined(DEBUG_H)

#define DEBUG_H

// #define DEBUG_ENABLE
#ifdef DEBUG_ENABLE
// 	#define DAX_DEBUG_ENABLE
// 	#define RDS_DEBUG_ENABLE
#endif

/* Baud rate */
#define BAUD_RATE		0x00000135		/*	115200 bps */

/* HW flags */
#define TXE		0x80UL

/* Routines */

void Db_Define(void);

void Db_Print_Val(unsigned char ch, unsigned char id);

void Db_Print_Char(unsigned char ch);

void Db_Print_Str(char *str);

void Db_Print_Line(char *str);

#endif
