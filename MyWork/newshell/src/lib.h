#include <stdlib.h>

#ifndef __MYLIB_H
#define __MYLIB_H

#define DEV_REINIT	0
#define DEV_SET	1
#define DEF_BUS		"I2C_1"
#define MAX_ARGSZ_INT	8

extern char *i2c_dev;
extern int rail;	/* voltage rail on irps5401 (0-4) */



extern int ishex(char *);
extern char *strdup(const char *);
extern char *itoa(int, char *, int);
extern char *toLower(char *);
extern char *toUpper(char *);
extern unsigned int toint(unsigned char *, int);
extern unsigned short toshort(unsigned char *);
extern double mytrunc(double, int);

extern void setup_dev(int);
extern void setup_pos(void);
extern int set_mux(int);
extern int i2c_write_bytes(int, int, char *, int);
extern int i2c_read_bytes(int, int, char *, int);

#endif /* __MYLIB_H */

#define TRUE	(1==1)
#define FALSE	(1==0)

#define VOLT_ON		1
#define  VOLT_OFF	0

#define SET_ALL		-1
#define SET_POR		0
#define SET_SRST	1
#define SET_PROG	2

#define GET_DONE_B	0
#define GET_INIT	1

#define ENOENT		2
extern void set_ps_bit(int, int);
extern int get_ps_stat(int);
extern void start_cpu();
extern void set_psbits_hi();
extern char *f2str(double, int);

extern int i2c_write_bytes(int, int, char *, int);
extern int i2c_read_bytes(int, int, char *, int);
