#include <stdlib.h>
#include <sys/byteorder.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>

#include "lib.h"
#include "pmbus.h"
#include "irps5401.h"
#include <errno.h>
#include <string.h>
#include <math.h>


struct mapping {
	char *name;
	int val;
};

int map(char *s, struct mapping *p)
{
	s = toLower(s);
	for (; p->name != NULL; p++) {
		if (strcmp(s, p->name) == 0) {
			return p->val;
		}
	}

	return -1;
}

struct mapping pstates[] = {
	{"0", VOLT_OFF},
	{"off", VOLT_OFF},
	{"1", VOLT_ON},
	{"on", VOLT_ON},
	{NULL,0}
};

static int getpstate(char *s)
{
	return map(s, pstates);
}

struct mapping looptab[] = {
	{"0", 0},
	{"1", 1},
	{"2", 2},
	{"3", 3},
	{"4", 4},
	{"a", 0},
	{"b", 1},
	{"c", 2},
	{"d", 3},
	{"ldo", 4},
	{NULL,0}
};

static int
getloop(char *s)
{
	return map(s, looptab);
}

static int
irps_help(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("\nirps usage:\n");

	printk("irps <dev> [on|off] - Turn device on/off\n");
	printk("irps <dev> dump - dump data associated with rail/loop\n");
	printk("dev may be:\n");
	printk("all - apply to all devices\n");
	printk("[0-4] - devices 0-4, Loops A-D, Loop LDO\n");
	printk("A|B|C|D|LDO (case insensitive)\n");
	return 0;
}

static int
irps_dump(const struct shell *sh, size_t argc, char **argv)
{
	char *dev = toLower(argv[1]);
	int rail;
	double v;
	
	printk("\nDumping Device %s\n", dev);

	rail = getloop(dev);
	if (rail < 0) {
		printk("Bad rail %s\n", dev);
		return -1;
	}
	printk("rail = %d\n", rail);

	v = pmread_vout(0x40, rail);
	printk("V: %.4f\n", v);
	return 0;
}
static int
irps_main(const struct shell *sh, size_t argc, char **argv)
{
	int rail;
	int val;

	char *dev = toLower(argv[1]);
	char *state = toLower(argv[2]);

	rail = getloop(dev);
	if (rail < 0) {
		printk("Command error: Bad rail %s\n", dev);
		return -1;
	}

	val = getpstate(state);
	
	if (val < 0) {
		printk("Command error: Bad power state %s\n", state);
		return -1;
	}
	printk("\nturning rail %s (%d) %s\n", dev, rail, state);
	pmset_op(0x40, rail, val);

	return 0;
}

			  
SHELL_SUBCMD_SET_CREATE(sub_irps, (irps));
SHELL_SUBCMD_ADD((irps), help, NULL, "help for irps", irps_help, 1, 0);
SHELL_SUBCMD_ADD((irps), dump, NULL, "Dump Info", irps_dump, 2, 0);
//SHELL_CMD_REGISTER(irps, &sub_irps, "IRPS 5401 Commands", NULL);
SHELL_CMD_ARG_REGISTER(irps, &sub_irps, "IRPS 5401 Commands", irps_main, 1, 3);

/******************************************************************
 * IRPS commands
 * irps help
 * irps <dev> [on|offp]
 * irps dump <dev>
 *
 * dev: 
 */

#if 0
SHELL_SUBCMD_ADD((my2c), set, NULL, "help for set", i2c_set_bus, 2, 0);
SHELL_SUBCMD_ADD((my2c), dev, NULL, "help for dev", i2c_set_dev, 1, 1);

SHELL_CMD_REGISTER(my2c, &sub_my2c, "I2C Control Functions", NULL);
#endif
