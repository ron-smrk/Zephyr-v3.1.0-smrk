#include <stdlib.h>
#include <sys/byteorder.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <drivers/gpio.h>
#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>
#include "lib.h"
#include "pmbus.h"
#include "pmbus_cmds.h"
#include "irps5401.h"
#include "vrails.h"
#include "my2c.h"
#include <errno.h>
#include <string.h>
#include <math.h>

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

// Remove when used....
__attribute__ ((unused))
static int
pmbus_getmfr_id(int bus)
{
	char id[5];
	int v;

	pmbus_rdblock(bus, PMBUS_MFR_ID, 3, id);
	// printk("ID: 0x%x 0x%x 0x%x\n", id[0], id[1], id[2]);

	v = toint(id, 3);
	// printk("Got 0x%06x for mfr_id!\n", v);
	return v;

}

int twos_5bit(int v)
{
	int val;

	if (v == 0)
		return 0;
	val = (v ^ 0x1f) + 1;
	// printf("v= 0x%x, val = 0x%d\n", v, val);
	return -val;
}

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
		exp = twos_5bit(exp);
		v = v & 0x7ff;
		return (v * pow(2, exp));
	} else if (type == PM_MAX_TEMP) {
		double i = (double) v;
		i = i * 10;		// * 10^-r, where r=-1 so 10...
		i = i - 5887;	// - b, where b=5887
		i = i / 21.0;	// divided by m, where m is 21.
		return i;
	} else if (type == PM_MAX_CURRENT) {
		double i = (double) v;
		double temp = pmbus_get_temp(VDD_1R2_DDR);
		i = i * 10;		// * 10^-r, where r=-1 so 10...
		i = i - 4845;	// - b, where b=(4976-131)*D, D=vin/vout ~=1.0
		i = i / 158.61;	// divided by m, where m is 153+5.61*D
		i = i * (.013 * (temp-50.0));
		return i;
	} else {
		printk("Unsupported format\n");
		return -1;
	}
}

double
pmbus_get_vout(int rail)
{
	unsigned char id[8];
	unsigned short v;
	double volt;

	int bus = get_bus(rail);

	if (bus < 0)
		return -1.0;

	int type = vrail[rail].type;
	// only IRPS supports page.
	if (type & ISIRPS_CHIP) {
		irps_setpage(bus, (unsigned char)type&LOOP_MASK);
	}
	//printk("Read cmd: 0x%x, sz: 0x%x\n", p->command, p->size);
	pmbus_read(bus, PMBUS_READ_VOUT, 2, id);
	v = toshort(id);
	volt =  decode(v, PM_LINEAR8);
	if (type & DIVBY2)
		volt /= 2.0;
	return volt;
}

double
pmbus_get_iout(int rail)
{
	unsigned char id[8];
	unsigned short v;
	int fmt;

	
	int bus = get_bus(rail);
	//printk("bus=%d\n", bus);

	if (bus < 0)
		return -1.0;

	int type = vrail[rail].type;
	//printk("type=0x%08x\n", type);
	// only IRPS supports page.
	if (type & ISIRPS_CHIP) {
		irps_setpage(bus, (unsigned char)type&LOOP_MASK);
	}

	pmbus_read(bus, PMBUS_READ_IOUT, 2, id);
	v = toshort(id);
	//printk("Read I got 0x%x 0x%x (v=0x%x)\n", id[0], id[1], v);
	if (vrail[rail].type & ISMAX_CHIP)
		fmt = PM_MAX_CURRENT;
	else
		fmt = PM_LINEAR11;
	return decode(v, fmt);
}

double
pmbus_get_temp(int rail)
{
	unsigned char id[8];
	unsigned short v;
	int fmt;

	int bus = get_bus(rail);
	// printk("bus=%d\n", bus);

	if (bus < 0)
		return -1.0;

	int type = vrail[rail].type;
	// only IRPS supports page.
	if (type & ISIRPS_CHIP) {
		irps_setpage(bus, (unsigned char)type&LOOP_MASK);
	}

	pmbus_read(bus, PMBUS_READ_TEMPERATURE_1, 2, id);
	//printk("Read Temp got 0x%x 0x%x\n", id[0], id[1]);
	v = toshort(id);
	if (vrail[rail].type & ISMAX_CHIP)
		fmt = PM_MAX_TEMP;
	else
		fmt = PM_LINEAR11;
	return decode(v, fmt);
}
