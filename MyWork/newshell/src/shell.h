#ifdef EMUL
#include <unistd.h>
#endif

#ifndef EMUL
#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NHIST	20

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

extern int pm_cmd(int, char **);
extern int i2c_cmd(int, char **);
extern void add2hist(char *);

extern int hist_cmd(int, char **);
extern int zgetline(char **);
