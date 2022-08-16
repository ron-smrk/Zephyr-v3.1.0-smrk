#include <stdlib.h>
#include <sys/byteorder.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>

#include "lib.h"
#include "pmbus.h"
#include <errno.h>
#include <string.h>
#include <math.h>

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

#if 1

int
pm_rd_common(int addr, int command, int length, unsigned char *data, int blk)
{
	int i = 0;

	unsigned char tmp[130];

	/* reads return first byte junk??? */
	i2c_read_bytes(addr, command, tmp, length + blk);
#if 1
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
int
pmbus_wr_common(int addr, int command, int length, unsigned char *data, int blk)
{
	unsigned char sendbuf[131];
	
	printk("wr: lenght=0x%x, blk=%d\n", length, blk);
	if (blk) {
		sendbuf[0] = length;
		memcpy(sendbuf+1, data, length);
		length++;
	} else {
		memcpy(sendbuf, data, length);
	}
#if 1
	int i;
	for (i=0; i<8; i++) {
		printk("0x%02x ", sendbuf[i]);
	}
	printk("\n");
#endif
	return i2c_write_bytes(addr, command, sendbuf, length);
}


int pmbus_read(int addr, int command, int length, unsigned char *data)
{
	return pm_rd_common(addr, command, length, data, FALSE);
}

int
pmbus_rdblock(int addr, int command, int length, unsigned char *data)
{
	return pm_rd_common(addr, command, length, data, TRUE);
}

int
pmbus_wrblock(int addr, int command, int length, unsigned char *data)
{
	return pmbus_wr_common(addr, command, length, data, TRUE);
}

int
pmbus_write(int addr, int command, int length, unsigned char *data)
{
	return pmbus_wr_common(addr, command, length, data, FALSE);
}

#else
int
pmbus_write(int addr, int command, int length, unsigned char *data)
{
	unsigned char sendbuf[131];
	int pec_len = 0;

	if (length > 128)
		return -EINVAL;
	printk("pmbus_write: 0x%x, 0x%x\n", addr, command);

	sendbuf[0] = command;
	memcpy(sendbuf + 1, data, length);
	printk("pmbus_write: 0x%x, 0x%x\n", addr, command);
#ifdef PEC
	sendbuf[length + 1] = smbus_pec(0, sendbuf, length+1);
	pec_len = 1;
	printk("pmbus_write: PEC=0x%x\n", sendbuf[length+1]);
#endif /* PEC */
	{
		printk("WR Buf: 0x%02x 0x%02x 0x%02x 0x%02x\n", sendbuf[0], sendbuf[1],
			   sendbuf[2], sendbuf[3]);
	}

	struct i2c_msg msg;
	const struct device *dev;

	dev = device_get_binding(i2c_dev);
	if (!dev) {
		printk("I2C: Device driver %s not found.", i2c_dev);
		return -ENODEV;
	}

	msg.buf = sendbuf;
	msg.len = length + 1 + pec_len;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;
	return i2c_transfer(dev, &msg, 1, addr);
}

int
pmbus_rdblock(int addr, int command, int length, unsigned char *data)
{
	int i;
	
	unsigned char cmdbuf[1] = {command};
	unsigned char rdbuf[130];
	unsigned char crc;

	struct i2c_msg msg[2];
	const struct device *dev;


	dev = device_get_binding(i2c_dev);
	if (!dev) {
		printk("I2C: Device driver %s not found.", i2c_dev);
		return -ENODEV;
	}

//	const unsigned char *sendbuf[2] = { cmdbuf, dummybuf };
//	const int tosend[2] = { 2, length + 2 };
//	int sent, recvd;

	if (length > 128)
		exit(1);

	msg[0].buf = cmdbuf;
	msg[0].len = 1;
	msg[0].flags = I2C_MSG_WRITE;

	memset(rdbuf, 0, sizeof(rdbuf));
	//rdbuf[0] = addr | 1;
	msg[1].buf = rdbuf;
	msg[1].len = length;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	for (i = 0; i < 2; i++) {
	    i2c_transfer(dev, &msg[i], 1, addr);
	}

	memcpy(data, rdbuf, length);
	printk("Data: 0x%02x 0x%02x 0x%02x 0x%02x\n",
		   data[0], data[1], data[2], data[3]);
}
#endif

int
pmset_page(int addr, unsigned char page)
{
	unsigned char read_page;

	printk("setting to page %d\n", page);
	pmbus_write(addr, 0x00, 1, &page);

	pmbus_read(addr, 0x00, 1, &read_page);

	if (page != read_page) {
		printk("Failed to set page to %d (read back %d instead)\n",
			   page, read_page);
		return -EIO;
	}
	return read_page;
}

int
pmget_mfr_id(int addr)
{
	char id[5];
	double volt;
	int v;

	pmbus_rdblock(addr, 0x99, 3, id);
	printk("ID: 0x%x 0x%x 0x%x\n", id[0], id[1], id[2]);

	v = toint(id, 3);
	printk("Got 0x%06x for mfr_id!\n", v);
	return ;



	pmbus_rdblock(addr, 0x9a, 4, id);
	printk("Model: 0x%x 0x%x 0x%x 0x%x\n", id[0], id[1], id[2], id[3]);


	pmbus_rdblock(addr, 0xad, 1, id);
	printk("DevID: 0x%x\n", id[0]);

	pmbus_read(addr, 0x20, 1, id);
	printk("VOUT Mode: 0x%x\n", id[0]);

	pmbus_read(addr, 0x8b, 2, id);
	printk("VOUT (0x8b): 0x%x 0x%x\n", id[0], id[1]);
	v = toshort(id);

	volt = (double)v;
	printk("vout = %.4f\n", volt);
	printk("vout (div) = %.4f\n", volt/4096.0);
	printk("vout (exp) = %.4f\n", volt * pow(2, -8));


	int sec = 2;
	printk("%d sec...\n", sec);
	k_sleep(K_MSEC(sec * 1000));


	unsigned char val = 0;	// Immediate off
	pmbus_write(addr, 0x01, 1, &val);

	pmbus_read(addr, 0x8b, 2, id);
	printk("VOUT (0x8b): 0x%x 0x%x\n", id[0], id[1]);
	v = toshort(id);

	volt = (double)v;
	printk("vout = %.4f\n", volt);
	printk("vout (div) = %.4f\n", volt/4096.0);
	printk("vout (exp) = %.4f\n", volt * pow(2, -8));

	printk("%d sec...\n", sec);
	k_sleep(K_MSEC(sec * 1000));
	val = 0x80;
	pmbus_write(addr, 0x01, 1, &val);

	pmbus_read(addr, 0x8b, 2, id);
	printk("VOUT (0x8b): 0x%x 0x%x\n", id[0], id[1]);
	v = toshort(id);

	volt = (double)v;
	printk("vout = %.4f\n", volt);
	printk("vout (div) = %.4f\n", volt/4096.0);
	printk("vout (exp) = %.4f\n", volt * pow(2, -8));

	sec = 2;
	printk("%d sec...\n", sec);
	k_sleep(K_MSEC(sec * 1000));

	pmbus_read(addr, 0x8b, 2, id);
	printk("VOUT (0x8b): 0x%x 0x%x\n", id[0], id[1]);
	v = toshort(id);

	volt = (double)v;
	printk("vout = %.4f\n", volt);
	printk("vout (div) = %.4f\n", volt/4096.0);
	printk("vout (exp) = %.4f\n", volt * pow(2, -8));
	return 0;
}


/*
 * Mode setting, ON or OFF
 */
int
pmset_op(int addr, int mode)
{
	int byte;
	if (mode == VOLT_ON) {
		byte = 0x80;
	} else if (mode == VOLT_OFF) {
		byte = 0;
	} else {
		printk("Bad mode\n");
		return -EINVAL;
	}
	return pmbus_write(addr, 0x01, 1, &byte);
}
unsigned short
toshort(unsigned char *x)
{
	return (unsigned short) ((x[0] << 8) | x[1]);
}

unsigned int
toint(unsigned char *x, int nbytes)
{
	unsigned int rval = 0;
	int i = 0;

	for (i = 0; i < nbytes; i++) {
		rval <<= 8;
		printk("B: rval=0x%x, x[%d]=0x%x\n", rval, i, x[i]);
		rval |= x[i];
		printk("A: rval=0x%x\n", rval);
	}			
	printk("return 0x%x (sz=%d)\n", rval, nbytes);
	return rval;
}


int
pm_list()
{
	struct pmcommand *p = &pbus_cmd[0];

	printk("OP   Command\n");
	while (p->op != -1) {
		printk("%02x - %s\n", p->op, p->name);
		p++;
	}

	
}

int
pm_help()
{
	printk("HELP!!\n");
}

struct pmcommand pbus_cmd[] = {
	{-2, "list", pm_list},
	{-3, "help", pm_help},
	{PMBUS_PAGE, "page", pmset_page},
	{PMBUS_OPERATION, "op", pmset_op},
	{PMBUS_MFR_ID, "mfr_id", pmget_mfr_id},
	{-1, NULL, NULL}
};

static int cmd_pmtst(const struct shell *shell, size_t argc, char **argv)
{

	int hval = -1;
	int v = ishex(argv[1]);
	
	if (v < 0) {
		printk("\nBad arg %s\n", argv[1]);
		return v;
	} else if (v == 0) {
		printk("\nCommand: %s\n", argv[1]);
	} else {
		hval = strtol(argv[1], NULL, 16);
		printk("\nhex: 0x%x\n", hval);
	}

	struct pmcommand *p = &pbus_cmd[0];

	int found = 0;
	while (p->op != -1) {
		if (strcmp(argv[1], p->name) == 0) {
			found++;
			p->function();
			break;
		}
		p++;
	}
	
	if (!found) {
		printk("Bad command\n");
		return -EINVAL;
	}
	return 0;
	
}

SHELL_CMD_ARG_REGISTER(pm, NULL, "PMBus Commands", cmd_pmtst, 2, 0);
