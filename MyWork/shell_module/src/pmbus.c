#include <stdlib.h>
#include <sys/byteorder.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include "lib.h"
#include "pmbus.h"
#include <errno.h>
#include <string.h>

/* is PEC supported */
/* #define PEC 1	/* To turn on */
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
pmbus_read(int addr, int command, int length, unsigned char *data)
{
	int i;
	
	unsigned char cmdbuf[2] = { addr & 0xfe, command };
	unsigned char dummybuf[130];
	unsigned char crc;

	struct i2c_msg msg[2];
	const struct device *dev;


	dev = device_get_binding(i2c_dev);
	if (!dev) {
		printk("I2C: Device driver %s not found.", i2c_dev);
		return -ENODEV;
	}

	const unsigned char *sendbuf[2] = { cmdbuf, dummybuf };
	const int tosend[2] = { 2, length + 2 };
	int sent, recvd;

	if (length > 128)
		exit(1);

	memset(dummybuf, 0, sizeof(dummybuf));
	dummybuf[0] = addr | 1;
	msg[0].buf = cmdbuf;
	msg[0].len = 1;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = dummybuf;
	msg[1].len = length;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	for (i = 0; i < 2; i++) {
	    i2c_transfer(dev, &msg[i], 1, addr);
	}

	memcpy(data, dummybuf, length);
	printk("Data: 0x%02x 0x%02x 0x%02x 0x%02x\n",
		   data[0], data[1], data[2], data[3]);
}


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
	char id[3];

	pmbus_read(addr, 0x99, 3, id);
	printk("0x%x 0x%x 0x%x\n", id[0], id[1], id[2]);
	return 0;
}
