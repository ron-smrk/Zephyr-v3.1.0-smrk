#ifdef EMUL
#include <unistd.h>
#define KERNEL_VERSION_STRING "EMUL"
#endif

#ifndef EMUL
#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <version.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NHIST	50
#define PREVH	'A'		// previous entry
#define NEXTH	'B'		// next entry

#define MAXARGS	20

struct command_table {
	char *name;
	int (*func)(int, char **);
	char *info;
};

struct history {
	int hnum;
	char *cmd;
	struct history *next;
};

extern struct command_table cmd_tab[];
extern struct history hist[];

extern int pmbus_cmd(int, char **);
extern int i2c_cmd(int, char **);
extern int led_cmd(int, char **);
extern int hist_cmd(int, char **);
extern int kern_cmd(int, char **);
extern int reboot_cmd(int, char **);
extern int sleep_cmd(int, char **);
extern int echo_cmd(int, char **);
extern int loop_cmd(int, char **);
extern int vers_cmd(int, char **);
extern int io_cmd(int, char **);
extern int buildargs(char *, char **);

extern void add2hist(char *);
extern char *prevhist(void);
extern char *nexthist(void);
extern int hist_getlabel(char *);
void resethist(void);
extern int i2c_scan(int, char **);
extern int i2c_set_bus(int, char **);
extern int i2c_set_dev(int, char **);

extern int zgetline(char **, char *);
extern int runcmdlist(struct command_table *, char *, char *, int, char *);
