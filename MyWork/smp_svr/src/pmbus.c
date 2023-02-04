#include <stdlib.h>
#include <sys/byteorder.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>
#include <zephyr/console/console.h>

#include "lib.h"
#include "pmbus.h"
#include "pmbus_cmds.h"
#include "my2c.h"
#include "vrails.h"
#include "irps5401.h"
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* is PEC supported */
// #define PEC 1	/* To turn on */
#undef PEC

#ifdef PEC
// Note that crc8 can be pre-calculated into an array of 256 bytes
static unsigned char
crc8(unsigned char x)
{
	int i;

	for (i=0; i<8; i++) {
		char toxor = (x & 0x80) ? 0x07 : 0;

		x = x << 1;
		x = x ^ toxor;
	}

	return x;
}

static unsigned char
smbus_pec(unsigned char crc, const unsigned char *p, int count)
{
	int i;

	for (i = 0; i < count; i++)
		crc = crc8(crc ^ p[i]);
	return crc;
}
#endif
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
	} else if (type == PM_LINEAR2) {
		v = v & 0x7ff;	// 11 bits
		return (v*pow(2, -2));
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

static int
pm_rd_common(int bus, int command, int length, unsigned char *data, int blk)
{
	int i = 0;

	unsigned char tmp[130];
	int i2c_addr = get_i2c_addr(bus);

	set_mux(bus);
	k_sleep(K_MSEC(1));
	set_mux(bus);

	if (i2c_read_bytes(i2c_addr, command, tmp, length + blk) < 0) {
		printk("PMBUS: Read error bus: %d, i2c_addr: 0x%x\n", bus, i2c_addr);
		return -EIO;
	}
			
	
#if 0
	{
		for (i = 0; i < 4; i++) {
			printk("0x%02x ", tmp[i]);
		}
		printk("\n");
	}
#endif

	if (blk) {
		printk("Blk=0x%02x\n", tmp[0]);
		for (i = 0; i < length; i++) {
			tmp[i] = tmp[i+blk];
		}
		tmp[i] = 0;
	}
	i = 0;
	while (length > 0) {
		//printk("i=%d, length=%d\n", i, length);
		data[i] = tmp[length-1];
		i++, length--;
	}
	return 0;
}
static int
pmbus_wr_common(int bus, int command, int length, unsigned char *data, int blk)
{
	unsigned char sendbuf[131];
	int i2c_addr = get_i2c_addr(bus);

	set_mux(bus);

	//printk("wr: length=0x%x, blk=%d\n", length, blk);
	if (blk) {
		sendbuf[0] = length;
		memcpy(sendbuf+1, data, length);
		length++;
	} else {
		memcpy(sendbuf, data, length);
	}
#if 0
	int i;
	for (i=0; i<8; i++) {
		printk("0x%02x ", sendbuf[i]);
	}
	printk("\n");
#endif
	return i2c_write_bytes(i2c_addr, command, sendbuf, length);
}


int pmbus_read(int pmdev, int command, int length, unsigned char *data)
{
	return pm_rd_common(pmdev, command, length, data, FALSE);
}

int
pmbus_rdblock(int pmdev, int command, int length, unsigned char *data)
{
	return pm_rd_common(pmdev, command, length, data, TRUE);
}

int
pmbus_wrblock(int pmdev, int command, int length, unsigned char *data)
{
	return pmbus_wr_common(pmdev, command, length, data, TRUE);
}

int
pmbus_write(int pmdev, int command, int length, unsigned char *data)
{
	return pmbus_wr_common(pmdev, command, length, data, FALSE);
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

int
pmbus_get_volt(int rail, struct power_vals *p)
{
	int rval = 0;
	//printk("Rail = %d, type = 0x%08x\n", rail, vrail[rail].type);

	if (vrail[rail].type & PMBUS_RD) {
		return pmbus_get_vout(rail, p);
	}
	if (vrail[rail].waitpg(rail))
		p->fval = vrail[rail].nominal;
	else {
		p->fval = 0.0;
	}
	return rval;
}

int
pmbus_set_vout(int rail, double volts)
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
