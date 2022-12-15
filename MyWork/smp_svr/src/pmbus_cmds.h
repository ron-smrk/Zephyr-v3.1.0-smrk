#ifndef __PMBUS_CMDS_H
#define __PMBUS_CMDS_H

#include <zephyr/sys/util_macro.h>



extern int irps_setop(int, int, int);
extern int pmbus_get_vout(int, struct power_vals *);
extern int pmbus_get_vin(int, struct power_vals *);
extern int pmbus_get_iout(int, struct power_vals *);
extern int pmbus_get_temp(int, struct power_vals *);
extern double decode(unsigned short, int);

#endif /* __PMBUS_CMDS_H */
