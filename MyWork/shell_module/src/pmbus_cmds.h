#ifndef __PMBUS_CMDS_H
#define __PMBUS_CMDS_H

#include <zephyr/sys/util_macro.h>



extern int irps_setop(int, int, int);
extern double pmbus_get_vout(int);

#endif /* __PMBUS_CMDS_H */
