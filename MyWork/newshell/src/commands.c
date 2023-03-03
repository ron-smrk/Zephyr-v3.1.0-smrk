#include "shell.h"
#ifndef EMUL
#include <zephyr/sys/reboot.h>
#endif
struct history hist_tab[NHIST] = {0};

static int hnum = 0; // ever increasing history number

void
add2hist(char *l)
{
	int i;
	
	if (hist_tab[0].cmd) {
		// printf("Freeing (%s) %p\n", hist_tab[0].cmd, hist_tab[0].cmd);
		free(hist_tab[0].cmd);
	}
	for ( i = 0; i < (NHIST-1); i++) {
		hist_tab[i].cmd = hist_tab[i+1].cmd;
		hist_tab[i].hnum = hist_tab[i+1].hnum;
	}
	hist_tab[NHIST-1].cmd = l;
	hist_tab[NHIST-1].hnum = ++hnum;
}

int
hist_cmd(int arc, char **argv)
{
	int i;

	for (i=0; i < NHIST; i++) {
#ifdef DEBUG 
		if (hist_tab[i].cmd)
			printf("hist[%d]: %d - %s\n", i, hist_tab[i].hnum, hist_tab[i].cmd);
		else
			printf("hist[%d]: %d - %p\n", i, hist_tab[i].hnum, hist_tab[i].cmd);
#else
		if (hist_tab[i].cmd)
			printf("%d: %s\n", hist_tab[i].hnum, hist_tab[i].cmd);
#endif
	}
	return 0;
}

#if 0	//TMP
int
pm_cmd(int argc, char **argv)
{
	int i;
	
	printf("pm!!\n");
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
	return 0;
}


int i2c_scan(int argc, char **argv)
{
	int i;
	printf("scan\n");
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
	return 0;
}

int i2c_set_bus(int argc, char **argv)
{
	int i;
	printf("set bus\n");
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
	return 0;
}

int i2c_set_mux(int argc, char **argv)
{
	int i;
	printf("set mux\n");
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
	return 0;
}

static struct command_table i2c_tab[] = {
	{"scan", i2c_scan, "scan i2c bus"},
	{"mux", i2c_set_mux, "set i2c mux address"},
	{"bus", i2c_set_bus, "set i2c bus (I2C_1, etc)"},
	{0}
};

int
i2c_cmd(int argc, char **argv)
{
	int i;
	char *p = NULL;

#if 0
	printf("run i2c submenu...argc = %d\n", argc);
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
#endif
	if (argc > 1) {
		p = malloc(256);
		*p = '\0';
		for (i = 1; i < argc; i++) {
			strcat(p, argv[i]);
			strcat(p, " ");
		}
	} else {
	}
	runcmdlist(i2c_tab, "I2C> ", 1, p);
	if (p)
		free(p);

	//printf("Back...\n");
	return 0;
}
#endif

int
vers_cmd(int argc, char **argv)
{
	printf("Version %s\n", KERNEL_VERSION_STRING);
	return 0;
}

int
reboot_cmd(int argc, char **argv)
{
	printf("Rebooting.....\n\n\n");
#ifndef EMUL
	k_sleep(K_MSEC(100));
	sys_reboot(SYS_REBOOT_COLD);
#endif
	return 0;
}
