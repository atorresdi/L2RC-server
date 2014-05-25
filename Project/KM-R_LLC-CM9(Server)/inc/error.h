/* error.h Definitions for reporting device drivers errors */

#if !defined(ERROR_H)
#define ERROR_H

#include "stdint.h"

/* data types */
typedef struct Error Error;
struct Error{
	uint8_t err_flags;
	uint8_t dev_instance;	
};

#endif
