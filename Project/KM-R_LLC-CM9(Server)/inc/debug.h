#if !defined(DEBUG_H)

#define DEBUG_H

#define DEBUG_ENABLE

/* ASCII definitions */
#define ASTERISK			0x2aU
#define EXCLAMATION		0x21U
#define MINUS					0x2dU
#define TILDE					0x7eU
#define DASH					0x5fU
#define AT_SIGN				0x40U
#define PLUS					0x2BU
#define SLASH					0x2FU
#define DOT						0x2EU

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
