#if !defined(DEBUG_H)

#define DEBUG_H

#define DEBUG_ENABLE

/* ASCII */

#define 	DB_A		65
#define 	DB_B		66
#define 	DB_C		67
#define 	DB_D		68
#define 	DB_E		69
#define 	DB_F		70
#define 	DB_G		71
#define 	DB_H		72
#define 	DB_I		73
#define 	DB_J		74
#define 	DB_K		75
#define 	DB_L		76
#define 	DB_M		77
#define 	DB_N		78
#define 	DB_O		79
#define 	DB_P		80
#define 	DB_Q		81
#define 	DB_R		82
#define 	DB_S		83
#define 	DB_T		84
#define 	DB_U		85
#define 	DB_V		86
#define 	DB_W		87
#define 	DB_X		88
#define 	DB_Y		89
#define 	DB_Z		90
#define 	DB_a		97
#define 	DB_b		98
#define 	DB_c		99
#define 	DB_d		100
#define 	DB_e		101
#define 	DB_f		102
#define 	DB_g		103
#define 	DB_h		104
#define 	DB_i		105
#define 	DB_j		106
#define 	DB_k		107
#define 	DB_l		108
#define 	DB_m		109
#define 	DB_n		110
#define 	DB_o		111
#define 	DB_p		112
#define 	DB_q		113
#define 	DB_r		114
#define 	DB_s		115
#define 	DB_t		116
#define 	DB_u		117
#define 	DB_v		118
#define 	DB_w		119
#define 	DB_x		120
#define 	DB_y		121
#define 	DB_z		122


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
