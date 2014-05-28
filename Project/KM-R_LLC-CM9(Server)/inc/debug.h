#if !defined(DEBUG_H)

#define DEBUG_H

#define DEBUG_ENABLE

/* Baud rate */
// #define BAUD_RATE		0x00003a95		/*	2400 bps */
// #define BAUD_RATE		0x00000271		/*	57600 bps */
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
