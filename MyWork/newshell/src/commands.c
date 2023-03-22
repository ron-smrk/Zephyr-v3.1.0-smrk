#include "shell.h"
#ifndef EMUL
#include <zephyr/sys/reboot.h>
#endif

int uptime_cmd(int argc, char **argv)
{
#ifdef EMUL
	printf("Uptime: 1000 ms\n");
#else
	printf("Uptime: %u ms\n", k_uptime_get_32());
#endif
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

int
i2c_cmd(int argc, char **argv)
{
	int i;
	printf("run i2c submenu...argc = %d\n", argc);
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
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
