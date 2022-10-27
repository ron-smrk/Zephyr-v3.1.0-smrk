#ifndef __PMBUS_CMDS_H
#define __PMBUS_CMDS_H

#include <zephyr/sys/util_macro.h>



extern int irps_setop(int, int, int);
extern double pmbus_get_vout(int);
extern double pmbus_get_iout(int);
extern double pmbus_get_temp(int);
extern double decode(unsigned short, int);

#endif /* __PMBUS_CMDS_H */
