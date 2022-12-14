#ifndef __PMBUS_CMDS_H
#define __PMBUS_CMDS_H

#include <zephyr/sys/util_macro.h>



extern int irps_setop(int, int, int);
extern double pmbus_get_vout(int);
extern double pmbus_get_vin(int);
extern double pmbus_get_iout(int);
extern double pmbus_get_temp(int);
extern double decode(unsigned short, int);
extern int pmbus_get_vout_raw(int);
extern int pmbus_get_iout_raw(int);
extern int pmbus_get_temp_raw(int);

#endif /* __PMBUS_CMDS_H */
