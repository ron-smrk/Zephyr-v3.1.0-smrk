#ifndef __PMBUS_CMDS_H
#define __PMBUS_CMDS_H

#include <zephyr/sys/util_macro.h>



extern int pmbus_get_vout(int, struct power_vals *);
extern int pmbus_get_vin(int, struct power_vals *);
extern int pmbus_get_iout(int, struct power_vals *);
extern int pmbus_get_temp(int, struct power_vals *);
extern int pmbus_set_vout(int, double);
extern double decode(unsigned short, int);
extern int pmbus_regnull(int, struct pmbus_op *);

#endif /* __PMBUS_CMDS_H */
