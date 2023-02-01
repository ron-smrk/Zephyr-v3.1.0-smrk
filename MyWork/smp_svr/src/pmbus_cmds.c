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
#include <stdio.h>

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

int
encode(double v, int type)
{
	double tmp;
	int t;
	if (type == PM_LINEAR8) {
		tmp = v/(pow(2, -8));
		t = (int)tmp;
		return t;
	}
	return 0;
}

double
decode(unsigned short v, int type)
{
	if (type == PM_LINEAR8)
		return (v*pow(2, -8));
	else if (type == PM_LINEAR9)
		return (v*pow(2, -9));
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
		struct power_vals pwr;
		
		// Formula from MAX AN6140
		// printk("MAXcurrent\n");
		double vout;
		double vin;

		pmbus_get_vout(VDD_1R2_DDR, &pwr);
		vout = pwr.fval;
		
		pmbus_get_vin(VDD_1R2_DDR, &pwr);
		vin = pwr.fval;
		double d = vout/vin;

		// printk("vin = %f, vout = %f\n", vin, vout);

		double b = 4976.0 - (131.0 * d);
		double m = 153.0 + (5.61 * d);
		double a = 0.013;
		double i = (double) v;
		pmbus_get_temp(VDD_1R2_DDR, &pwr);
		double temp = pwr.fval;

		i = i * 10;		// * 10^-r, where r=-1 so 10...
		i = i - b;
		i = i / m;
		i = i + (a * (temp-50.0));
		return i;
	} else if (type == PM_MAX_VIN) {
		// Formula from MAX AN6140
		// printk("v=%d\n", v);
		double volt = (double) v;
		volt *= 100.0;	// 10 ^ (-(-2)) --> 10 ^2 --> 100
		volt /= 3609.0;
		//printk("return volt=%f\n", volt);
		return volt;
	} else {
		printk("Unsupported format\n");
		return -1;
	}
}

int pmbus_set_vout(int rail, double volts)
{
	int bus = get_bus(rail);
	int raw;

	if (bus < 0) {
		// printk("pmbus_get_vout: Ret err\n");
		return -1;
	}

	int type = vrail[rail].type;
	// only IRPS supports page.
	if (type & ISIRPS_CHIP) {
		irps_setpage(bus, (unsigned char)type&LOOP_MASK);
	}

	char v[20];
	sprintf(v, "%.4f", volts);

	printk("pmbus set volt to %s\n", v);
	//pmbus_write(bus, PMBUS_VOUT_COMMAND, 2, buf);
	raw = encode(volts, PM_LINEAR8);
	printk("raw = %d (0x%x)\n", raw, raw);
	return 0;
}

int
pmbus_get_vout(int rail, struct power_vals *pwr)
{
	unsigned char id[8];
	unsigned short v;
	double volt;

	memset(&pwr, sizeof(struct power_vals), 0);
	int bus = get_bus(rail);

	if (bus < 0) {
		// printk("pmbus_get_vout: Ret err\n");
		return -1;
	}

	int type = vrail[rail].type;
	// only IRPS supports page.
	if (type & ISIRPS_CHIP) {
		irps_setpage(bus, (unsigned char)type&LOOP_MASK);
	}
	pmbus_read(bus, PMBUS_READ_VOUT, 2, id);
	v = toshort(id);
	pwr->sval = v & 0xffff;
	if (type & ISMAX_CHIP) {
		double RFB1 = 1.87 * 1000;
		double RFB2 = 2.21 * 1000;
#if 0
		printk("----id=%x %x %x %x, v = 0x%x---\n",
			   id[0], id[1], id[2], id[3], v);
#endif
		volt = decode(v, PM_LINEAR9);
		volt = volt * (1 + (RFB1/RFB2));
	} else {
		volt = decode(v, PM_LINEAR8);
	}
	if (type & DIVBY2)
		volt /= 2.0;
	pwr->fval = volt;

	//printk("volt=%f\n", volt);
	//printk("Returning volt=%f\n", pwr->fval);
	return 0;
}

int
pmbus_get_vin(int rail, struct power_vals *pwr)
{
	unsigned char id[8];
	unsigned short v;

	memset(&pwr, sizeof(struct power_vals), 0);
	int bus = get_bus(rail);

	if (bus < 0)
		return -1;

	int type = vrail[rail].type;
	// only IRPS supports page.
	if (type & ISIRPS_CHIP) {
		irps_setpage(bus, (unsigned char)type&LOOP_MASK);
	}
	pmbus_read(bus, PMBUS_READ_VIN, 2, id);
	v = toshort(id);
	pwr->sval = v & 0xffff;
	if (type & ISMAX_CHIP) {
#if 0
		printk("pm_get_vin: id=%x %x %x %x, v = 0x%x---\n",
			   id[0], id[1], id[2], id[3], v);
#endif
		pwr->fval = decode(v, PM_MAX_VIN);
	} else {
		pwr->fval = decode(v, PM_LINEAR8);
	}
	return 0;
}

int
pmbus_get_iout(int rail, struct power_vals *pwr)
{
	unsigned char id[8];
	unsigned short v;
	int fmt;

	memset(&pwr, sizeof(struct power_vals), 0);
	int bus = get_bus(rail);
	// printk("pmbus_get_iout: rail=%d, bus=%d\n", rail, bus);

	if (bus < 0)
		return -1;

	int type = vrail[rail].type;
	// printk("type=0x%08x\n", type);
	// only IRPS supports page.
	if (type & ISIRPS_CHIP) {
		irps_setpage(bus, (unsigned char)type&LOOP_MASK);
	}

	pmbus_read(bus, PMBUS_READ_IOUT, 2, id);
	v = toshort(id);
	pwr->sval = v & 0xffff;
	// printk("Read I got 0x%x 0x%x (v=0x%x)\n", id[0], id[1], v);
	if (vrail[rail].type & ISMAX_CHIP)
		fmt = PM_MAX_CURRENT;
	else
		fmt = PM_LINEAR11;
	pwr->fval = decode(v, fmt);
	// printk("get_iout return %f\n", pwr->fval);
	return 0;
}

int
pmbus_get_temp(int rail, struct power_vals *pwr)
{
	unsigned char id[8];
	unsigned short v;
	int fmt;

	memset(&pwr, sizeof(struct power_vals), 0);
	int bus = get_bus(rail);
	// printk("pmbus_get_temp: rail=%d, bus=%d\n", rail, bus);

	if (bus < 0)
		return -1;

	int type = vrail[rail].type;
	// only IRPS supports page.
	if (type & ISIRPS_CHIP) {
		irps_setpage(bus, (unsigned char)type&LOOP_MASK);
	}

	pmbus_read(bus, PMBUS_READ_TEMPERATURE_1, 2, id);
	// printk("Read Temp got 0x%x 0x%x\n", id[0], id[1]);
	v = toshort(id);
	pwr->sval = v & 0xffff;
	if (vrail[rail].type & ISMAX_CHIP)
		fmt = PM_MAX_TEMP;
	else
		fmt = PM_LINEAR11;
	pwr->fval = decode(v, fmt);
	return 0;
}
