#include <stdlib.h>
#include <sys/byteorder.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <drivers/gpio.h>
#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>

#include "lib.h"
#include "pmbus.h"
#include "irps5401.h"
#include <errno.h>
#include <string.h>
#include <math.h>

int
irps_setpage(int addr, unsigned char page)
{
	unsigned char read_page;

	//printk("setting to page %d\n", page);
	pmbus_write(addr, PMBUS_PAGE, 1, &page);

	pmbus_read(addr, PMBUS_PAGE, 1, &read_page);

	if (page != read_page) {
		printk("Failed to set page to %d (read back %d instead)\n",
			   page, read_page);
		return -EIO;
	}
	return read_page;
}


/*
 * Mode setting, ON or OFF
 */
int
irps_setop(int addr, int rail, int mode)
{
	unsigned char byte;
	if (mode == VOLT_ON) {
		byte = 0x80;
	} else if (mode == VOLT_OFF) {
		byte = 0;
	} else {
		printk("Bad mode\n");
		return -EINVAL;
	}
	if (irps_setpage(addr, rail) < 0) {
		printk("Can't set page %d\n", rail);
		return -EIO;
	}
	return pmbus_write(addr, PMBUS_OPERATION, 1, &byte);
}

static int
irps_getmfr_id(int addr)
{
	char id[5];
	int v;

	pmbus_rdblock(addr, PMBUS_MFR_ID, 3, id);
	// printk("ID: 0x%x 0x%x 0x%x\n", id[0], id[1], id[2]);

	v = toint(id, 3);
	// printk("Got 0x%06x for mfr_id!\n", v);
	return v;

}


struct pm_list {
	unsigned char command;
	int size;
	char *name;
	char *units;
	int type;
} pm_info [] = {
	{PMBUS_READ_VOUT, 2, "Output Voltage", "V", PM_LINEAR8},
	{PMBUS_READ_IOUT, 2, "Output Current", "Amps", PM_IOUT},
	{PMBUS_READ_TEMPERATURE_1, 2, "Temperature", "Degrees C", PM_LINEAR11},
	{}
};

struct mapping {
	char *name;
	int val;
};

double
decode(unsigned short v, int type)
{
	if (type == PM_LINEAR8)
		return (v*pow(2, -8));
	else if (type == PM_IOUT) {
		double tmp = (double) (v*pow(2, -8));
		tmp =  mytrunc(tmp, 125);
		return tmp/1000;
	} else if (type == PM_LINEAR11) {
		int exp = (v >> 11) & 0x1f;
		v = v & 0x7ff;
		return (v * pow(2, -(exp)));
	} else {
		printk("Unsupported format\n");
		return -1;
	}
}

double
get_vout(int rail)
{
	unsigned char id[8];
	unsigned short v;

	irps_setpage(0x40, (unsigned char)rail);
	//printk("Read cmd: 0x%x, sz: 0x%x\n", p->command, p->size);
	pmbus_read(0x40, PMBUS_READ_VOUT, 2, id);
	v = toshort(id);
	return decode(v, PM_LINEAR8);
}

/*
 * Assume page is set already
 */
static int
dumpval(int addr, struct pm_list *p)
{
	unsigned char id[8];
	unsigned short v;
	//printk("Read cmd: 0x%x, sz: 0x%x\n", p->command, p->size);
	pmbus_read(addr, p->command, p->size, id);
	//printk("read 0x%02x 0x%02x\n", id[0], id[1]);
		  
	v = toshort(id);
	
	//printk("Got 0x%x for value for %s\n", v, p->name);
	switch (p->type) {
	case PM_LINEAR8:
	case PM_LINEAR11:
	case PM_IOUT:
		printk("  %s: %.4f %s\n", p->name, decode(v, p->type), p->units);
		break;
	default:
		if (p->size == 1)
			printk("  %s = 0x%02x\n", p->name, v & 0xff);
		else
			printk("  %s = 0x%04x\n", p->name, v);
		break;
	}

	return 0;
}

static int
dumpall(int addr, int rail)
{
	struct pm_list *p;
	
	if (irps_setpage(addr, rail) < 0) {
		printk("Can't set page %d\n", rail);
		return -EIO;
	}
	
	for (p = pm_info; p->name; p++) {
		dumpval(0x40, p);
	}
	return 0;
}
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
	{"all", -1},
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
	
	printk("\nDumping Device %s\n", dev);

	rail = getloop(dev);
	if (rail < 0) {
		printk("Bad rail %s\n", dev);
		return -1;
	}
	// printk("rail = %d\n", rail);

	dumpall(0x40, rail);
#if 0
	double v;
	v = pmread_vout(0x40, rail);
	printk("V: %.4f\n", v);
#endif
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
	irps_setop(0x40, rail, val);

	return 0;
}

			  
/******************************************************************
 * IRPS commands
 * irps help
 * irps <dev> [on|offp]
 * irps dump <dev>
 *
 * dev: 0-4, a-d, ldo, all
 */
SHELL_SUBCMD_SET_CREATE(sub_irps, (irps));
SHELL_SUBCMD_ADD((irps), help, NULL, "help for irps", irps_help, 1, 0);
SHELL_SUBCMD_ADD((irps), dump, NULL, "Dump Info", irps_dump, 2, 0);
//SHELL_CMD_REGISTER(irps, &sub_irps, "IRPS 5401 Commands", NULL);
SHELL_CMD_ARG_REGISTER(irps, &sub_irps, "IRPS 5401 Commands", irps_main, 1, 3);

/*
 * VDD 1R8, enable: Loop D, PG: PC8, read VOUT
 */
int
vdd_1r8_on(char *v)
{
	if (v)
		printk("%s on\n", v);
	irps_setop(0x40, LOOPD, VOLT_ON);
	return 0;
}
int
vdd_1r8_off(char *v)
{
	if (v)
		printk("%s off\n", v);
	irps_setop(0x40, LOOPD, VOLT_OFF);
	return 0;
}

double
vdd_1r8_rdvolt(char *v)
{
	if (v)
		printk("%s rdvolt\n", v);
	return get_vout(LOOPD);
}
/* vdd_1r8_isgood() is in boardctl.c, uses GPIOS.... */

/*
 * VDD_2r5: enable Loop B, PG: PB13, read VOUT
 */
int
vdd_2r5_on(char *v)
{
	if (v)
		printk("%s on\n", v);
	irps_setop(0x40, LOOPB, VOLT_ON);
	return 0;
}
int
vdd_2r5_off(char *v)
{
	if (v)
		printk("%s off\n", v);
	irps_setop(0x40, LOOPB, VOLT_OFF);
	return 0;
}

/* vdd_2r5_isgood() is in boardctl.c, uses GPIOS.... */
double
vdd_2r5_rdvolt(char *v)
{
	if (v)
		printk("%s rdvolt\n", v);
	return get_vout(LOOPB);
}


/*
 * VDD_0r9: enable Loop C, PG: PC13, read VOUT
 */
int
vdd_0r9_on(char *v)
{
	if (v)
		printk("%s on\n", v);
	irps_setop(0x40, LOOPC, VOLT_ON);
	return 0;
}
int
vdd_0r9_off(char *v)
{
	if (v)
		printk("%s off\n", v);
	irps_setop(0x40, LOOPC, VOLT_OFF);
	return 0;
}

/* vdd_0r9_isgood() is in boardctl.c, uses GPIOS.... */

double
vdd_0r9_rdvolt(char *v)
{
	if (v)
		printk("%s rdvolt\n", v);

	return get_vout(LOOPC);
}


/*
 * VDD_1r0: enable Loop LDO, PG: PB12, read VOUT
 */
int
vdd_1r0_on(char *v)
{
	if (v)
		printk("%s on\n", v);
	irps_setop(0x40, LOOPLDO, VOLT_ON);
	return 0;
}
int
vdd_1r0_off(char *v)
{
	if (v)
		printk("%s off\n", v);
	irps_setop(0x40, LOOPLDO, VOLT_OFF);
	return 0;
}

/* vdd_1r0_isgood() is in boardctl.c, uses GPIOS.... */

double
vdd_1r0_rdvolt(char *v)
{
	if (v)
		printk("%s rdvolt\n", v);

	return get_vout(LOOPLDO);
}
