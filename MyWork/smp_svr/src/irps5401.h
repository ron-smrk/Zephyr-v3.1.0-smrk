#ifndef __IRPS5401_H
#define __IRPS5401_H

#include <zephyr/sys/util_macro.h>


enum irps5401_registers {
	PMBUS_MFR_REG_ACCESS			= 0xD0, /* Block Write, R word */
	PMBUS_MFR_I2C_ADDRESS			= 0xD6, /* R/W byte */
	PMBUS_MFR_TPGDLY				= 0xD8, /* R/W word */
	PMBUS_MFR_FCCM					= 0xD9, /* R/W word */
	PMBUS_MFR_VOUT_PEAK				= 0xDB, /* R/W word */
	PMBUS_MFR_IOUT_PEAK				= 0xDC, /* R/W word */
	PMBUS_MFR_TEMPERATURE_PEAK		= 0xDD, /* R/W word */
	PMBUS_MFR_LDO_MARGIN			= 0xDE, /* R/W word */
};

#define LOOPA	0
#define LOOPB	1
#define LOOPC	2
#define LOOPD	3
#define LOOPLDO 4

extern int irps_setpage(int, unsigned char);
extern int irps_setop(int, int, int);
extern int irps_reg(int, struct pmbus_op *);

#endif /* __IRPS5401_H */
