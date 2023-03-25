#include "shell.h"
#ifndef EMUL
#include <zephyr/sys/reboot.h>
#include <sys_clock.h>
#endif

#include "lib.h"

unsigned int
get_kuptime()
{
#ifdef EMUL
	return (unsigned int)1000;
#else
	return k_uptime_get_32();
#endif
}

int uptime_cmd(int argc, char **argv)
{
	printf("Uptime: %u ms\n", get_kuptime());
	return 0;
}

int cycles_cmd(int argc, char **argv)
{
#ifdef EMUL
	printf("cycles: 1000 hw cycles\n");
#else
	printf("cycles: %u hw cycles\n", k_cycle_get_32());
#endif
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

static struct command_table kern_tab[] = {
	{"uptime", uptime_cmd, "System uptime"},
	{"reboot", reboot_cmd, "Reboot system"},
	{"cycles", cycles_cmd, "HW cycles"},
	{0}
};

int loopcnt;
int loopnum;
/*
 * echo args
 * special args
 *	%P%	- current pass in a loop
 *  %L% - loop count in a loop
 *  %U% - uptime in ms
*/
int
echo_cmd(int argc, char **argv)
{
	int i;
	char *s;
	for ( i = 1; i < argc; i++) {
		s = argv[i];
		if (strcmp(s, "%P%") == 0)
			printf("%d ", loopnum);
		else if (strcmp(s, "%L%") == 0)
			printf("%d ", loopcnt);
		else if (strcmp(s, "%U%") == 0)
			printf("%u ", get_kuptime());
		else
			printf("%s ", argv[i]);
	}
	printf("\n");
	return 0;
}
int
sleep_cmd(int argc, char **argv)
{
	int sval;
#if 0
	int i;
	printf("run sleep cmd...argc = %d\n", argc);
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
#endif
	if (argc != 2) {
		printf("sleep [NSEC].\n");fflush(stdout);
		return -1;
	}

	sval = atoi(argv[1]);
#ifdef EMUL
	printf("Sleeping for %d seconds\n", sval);
#else
	k_sleep(K_MSEC(sval*1000));
	printf("\n");
#endif
	return 0;
}

/*
 * loop [LABEL] [COUNT]
 */
extern struct command_table cmd_tab[];
int
loop_cmd(int argc, char **argv)
{
	int idx, i, i2;
	int ac;
	char *av[MAXARGS];
	char *tcmd;
	struct command_table *c;
	
	extern struct history hist_tab[];
	
	if (argc != 3) {
		printf("loop <label> <count>\n");
		return -1;
	}
	idx = hist_getlabel(argv[1]);
#if 0
	{
		int z;
		printf("idx = %d\n", idx);
		for (z=0; z < NHIST; z++) {
			if (hist_tab[z].cmd)
				printf("[%d] - %d: %s\n", z, hist_tab[z].hnum, hist_tab[z].cmd);
			else
				printf("[%d]: NULL\n", z);
		}
	}
#endif			
	//  Global loopcnt for echo
	loopcnt = atoi(argv[2]);

	// printf("Looping %d times\n", loopcnt);
	
	for (i2 = 0; i2 < loopcnt; i2++) {
		//printf("Pass: %d\n", i2);
		loopnum = i2;	// global pass no for echo
		for (i = idx+1; i < (NHIST-1); i++) {
			//printf("run: %s\n", hist_tab[i].cmd);
			tcmd = strdup(hist_tab[i].cmd);
			ac = buildargs(tcmd, av);
			//printf("a0 = %s\n", av[0]);
			if (strcmp(tcmd, "loop") == 0 ) {
				free(tcmd);
				break;
			}
			free(tcmd);
			//printf("a0.0 = %s\n", av[0]);
			for (c = &cmd_tab[0]; c->func; c++) {
				//printf("cmd: %s\n", c->name);
				if (strncmp(av[0], c->name, 2) == 0) {
					(*c->func)(ac, av);
					break;
				}
			}
		}
	}
	
	return 0;
}

int
kern_cmd(int argc, char **argv)
{
	int i;
	char *p = NULL;

#if 0
	printf("run kernel submenu...argc = %d\n", argc);
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
	runcmdlist(kern_tab, "kernel", "Kernel> ", 1, p);
	if (p)
		free(p);

	//printf("Back...\n");
	return 0;
}

int
vers_cmd(int argc, char **argv)
{
	printf("Version %s\n", KERNEL_VERSION_STRING);
	return 0;
}

#ifdef EMUL
void
setup_dev(int f)
{
	printf("Setupdev: %d\n", f);
}

void
start_cpu()
{
}

i2c_scan(int argc, char **argv)
{
	int i;
	printf("--------running i2c scan...argc = %d\n", argc);
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
}
	
i2c_set_mux(int argc, char **argv)
{
	int i;
	printf("run i2c set_mux...argc = %d\n", argc);
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
}
i2c_set_bus(int argc, char **argv)
{
	int i;
	printf("run i2c set_bus...argc = %d\n", argc);
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
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

#if 1
	printf("running i2c submenu...argc = %d\n", argc);
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
	runcmdlist(i2c_tab, "i2c", "I2C> ", 1, p);
	if (p)
		free(p);

	//printf("Back...\n");
	return 0;
}
	
int
led_cmd(int argc, char **argv)
{
	int i;
	printf("run led submenu...argc = %d\n", argc);
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
	return 0;
}
int
pmbus_cmd(int argc, char **argv)
{
	int i;
	printf("run pmbus submenu...argc = %d\n", argc);
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
	return 0;
}
#endif
