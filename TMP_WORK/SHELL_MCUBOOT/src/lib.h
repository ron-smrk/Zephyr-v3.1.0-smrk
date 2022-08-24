#include <stdlib.h>

#ifndef __MYLIB_H
#define __MYLIB_H

#define DEV_REINIT	0
#define DEV_SET	1
#define DEF_BUS		"I2C_1"

extern char *i2c_dev;

extern char *strdup(const char *);
extern char *itoa(int, char *, int);
extern char *toLower(char *);
extern char *toUpper(char *);

extern void setup_dev(int);
extern int set_mux(int);
extern int i2c_write_bytes(int, int, char *, int);
extern int i2c_read_bytes(int, int, char *, int);

extern int enable_vdd_3r3();
extern int disable_vdd_3r3();

#endif /* __MYLIB_H */
