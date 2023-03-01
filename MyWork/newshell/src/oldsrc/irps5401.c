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
#include "pmbus_cmds.h"
#include "vrails.h"
#include "my2c.h"
#include <errno.h>
#include <string.h>
#include <math.h>

struct pm_list {
	unsigned char getcmd;
//	unsigned char setcmd;
	int size;
	char *name;
	char *units;
	int type;
} pm_info [] = {
	{PMBUS_READ_VOUT, 2, "Output Voltage", "Volts", PM_LINEAR8},
	{PMBUS_READ_IOUT, 2, "Output Current", "Amps", PM_LINEAR11},
	{PMBUS_READ_TEMPERATURE_1, 2, "Temperature", "Degrees C", PM_LINEAR11},
	{PMBUS_TON_RISE, 2, "Output Voltage Rise Time", "mSec", PM_LINEAR2},
	{PMBUS_POWER_GOOD_ON, 2, "Power Good On", "Volts", PM_LINEAR8},
	{PMBUS_POWER_GOOD_OFF, 2, "Power Good Off", "Volts", PM_LINEAR8},
	{}
};

struct mapping {
	char *name;
	int val;
};

int
irps_setpage(int bus, unsigned char page)
{
	unsigned char read_page;

	// printk("setting to page %d, bus=%d\n", page, bus);
	pmbus_write(bus, PMBUS_PAGE, 1, &page);

	pmbus_read(bus, PMBUS_PAGE, 1, &read_page);

	if (page != read_page) {
		printk("bus: %d, Failed to set page to %d (read back %d instead)\n",
			   bus, page, read_page);
		return -EIO;
	}
	return read_page;
}

int
irps_reg(int rail, struct pmbus_op *p)
{
	int bus = get_bus(rail);
	char	data[8];
	int		val;
	double	fval;
	unsigned short v;

	if (bus < 0) {
		// printk("pmbus_get_vout: Ret err\n");
		return -1;
	}

	int type = vrail[rail].type;
	if (type & ISIRPS_CHIP) {
		irps_setpage(bus, (unsigned char)type&LOOP_MASK);
	}

	if (p->mode == PMWRITE) {
		if (p->rawmode) {
			val = strtol(p->value, NULL, 16);
			// printk("data (raw) - 0x%x\n", val);
		} else {
			// printk("data str: --%s--\n", p->value);
			fval = atof(p->value);
			// printk("data (float) - %s\n", f2str(fval, 2));
			val = encode(fval, p->encoding);
			// printk("data (raw) - 0x%x\n", val);
		}
		data[1] = (val >> 8) & 0xff;
		data[0] = val & 0xff;
		//data[1] = 0xf0;
		// printk("irps setting rail: %d, reg: 0x%x, value: 0x%x 0x%x\n", rail, p->reg, data[0], data[1]);
		pmbus_write(bus, p->reg, p->size, data);
	} else {
		// printk("irps getting rail: %d, reg: 0x%x\n", rail, p->reg);
		pmbus_read(bus, p->reg, p->size, data);
		// printk("got data: 0x%x 0x%x\n", data[0], data[1]);
		v = toshort(data);
		fval = decode(v, p->encoding);
		printk("Reg: 0x%x, Value: %s\n", p->reg, f2str(fval, 4));
	}
	return 0;
}


/*
 * Mode setting, ON or OFF
 */
int
irps_setop(int bus, int rail, int mode)
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
	if (irps_setpage(bus, rail) < 0) {
		printk("Can't set page %d\n", rail);
		return -EIO;
	}
	return pmbus_write(bus, PMBUS_OPERATION, 1, &byte);
}


/*
 * Assume page is set already
 */
static int
dumpval(int bus, struct pm_list *p, int rail)
{
	unsigned char id[8];
	unsigned short v;
	//printk("Read cmd: 0x%x, sz: 0x%x\n", p->getcmd, p->size);
	pmbus_read(bus, p->getcmd, p->size, id);
	//printk("read 0x%02x 0x%02x\n", id[0], id[1]);
		  
	v = toshort(id);
	if ((p->getcmd == PMBUS_READ_VOUT) && (rail == VDD_1R0)) {
		v /= 2;
	}

	//printk("Got 0x%x for value for %s type=%d units=%s\n", v, p->name, p->type, p->units);
	switch (p->type) {
	case PM_LINEAR8:
	case PM_LINEAR11:
	case PM_LINEAR2:
	case PM_IOUT:
		printk("  %s: %s %s\n", p->name, f2str(decode(v, p->type), 4), p->units);
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

static int allrails[] = { VDD_0R85, VDD_2R5, VDD_0R9, VDD_1R8, VDD_1R0, -1 };


static int
dumpone(int rail)
{
	struct pm_list *p;
	int bus = IRPS_BUS;
	int type;

	type = vrail[rail].type;
	if (irps_setpage(bus, (unsigned char)type&LOOP_MASK) < 0) {
		printk("Can't set page %d\n", rail);
		return -EIO;
	}
	printk("Rail: %s\n", vrail[rail].signame);
	for (p = pm_info; p->name; p++) {
		if ((rail == VDD_1R0) && (p->getcmd == PMBUS_TON_RISE)) {
		printk("  ----Rise time not supported on LD0---\n");
		} else {
			dumpval(bus, p, rail);
		}
	}
	return 0;
}

static int
dumpall(int rail)
{
	int i;

	if (rail != NUM_RAILS) {
		return dumpone(rail);
	}

	for (i = 0; allrails[i] != -1; i++) {
		dumpone(allrails[i]);
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
	{"0", VDD_0R85},
	{"1", VDD_2R5},
	{"2", VDD_0R9},
	{"3", VDD_1R8},
	{"4", VDD_1R0},
	{"a", VDD_0R85},
	{"b", VDD_2R5},
	{"c", VDD_0R9},
	{"d", VDD_1R8},
	{"ldo", VDD_1R0},
	{"all", NUM_RAILS},
	{NULL,0}
};

static int
getrail(char *s)
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
	
	//printk("\nDumping Device %s\n", dev);

	rail = getrail(dev);
	//printk("rail = %d\n", rail);
	if (rail < 0) {
		printk("Bad rail %s\n", dev);
		return -1;
	}

	dumpall(rail);
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

	rail = getrail(dev);
	printk("irps_main, rail=%d\n", rail);
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
	irps_setop(IRPS_BUS, vrail[rail].type & LOOP_MASK, val);

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
