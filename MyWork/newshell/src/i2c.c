/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/shell/shell.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>

#include "i2c.h"
#include "lib.h"
#include "gpio.h"
#include "shell.h"
static int get_bytes_count_for_hex(int);

LOG_MODULE_REGISTER(app_test);

#define MAX_BYTES_FOR_REGISTER_INDEX	4
#define MAX_I2C_BYTES	16




static int i2c_addresses[] = {
	[QSFP_BUS] = QSFP_ADDR,
	[EEP_BUS] = EEP_ADDR,
	[IRPS_BUS] = IRPS_ADDR,
	[MAX_BUS] = MAX_ADDR,
	[EEPROM_BUS] = EEPROM_ADDR,
	[LED_BUS] = LED_ADDR
};

/* #define LED0_NODE	DT_ALIAS(led0) */
/* static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios); */

#define RST_NODE	DT_ALIAS(rstmux)
static const struct gpio_dt_spec resetmux = GPIO_DT_SPEC_GET(RST_NODE, gpios);

int
reset_mux()
{
	int rval;

	if (!device_is_ready(resetmux.port)) {
		printf("Not ready mux\n");
		return -ENODEV;
	}
	rval = gpio_pin_configure_dt(&resetmux, GPIO_OUTPUT_ACTIVE);
	if (rval < 0) {
		printf("MUX config error: %d\n", rval);
		return -EIO;
	}

	// set type to open-drain
	gpio_set1(XGPIOB, GPIO_OTYPER, 0x20);
	// High Speed
	gpio_set1(XGPIOB, GPIO_SPEEDR, 0xc00);
	gpio_pin_set_dt(&resetmux, 0);
	k_msleep(10);
	gpio_pin_set_dt(&resetmux, 1);

	return 0;
}

static int current_mux = -1;
int
set_mux(int busid)
{
	int rval;
	char buf[20];
	
	// only write if different.
	if (current_mux == busid)
		return 0;
	
	if ((rval=reset_mux()) < 0)
		return rval;

	if ( (busid<0) > (busid>7) ) {
		printf("Bad BusID: %d\n", busid);
		return -1;
	}
	// printf("SetMux to %d\n", busid);
	buf[0] = 1<<busid;
	current_mux = busid;
	//printf("reset ok, setting bus to %d.\n", busid);
	i2c_write_bytes(0x74, 0, buf, 1);

	return 0;
}

int
get_mux()
{
	return current_mux;
}

int get_i2c_addr(int chip)
{
	if (chip > LAST_BUS)
		return -1;
	return i2c_addresses[chip];
}

int
i2c_scan(int argc, char **argv)
{
	const struct device *d;
	uint8_t cnt = 0, first = 0x04, last = 0x77;

	d = device_get_binding(i2c_dev);

	if (!d) {
		printf("I2C: Device driver %s not found.", i2c_dev);
		return -ENODEV;
	}
	printf("I2C Bus: %d\n", get_mux());
	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	for (uint8_t i = 0; i <= last; i += 16) {
		printf("%02x: ", i);
		for (uint8_t j = 0; j < 16; j++) {
			if (i + j < first || i + j > last) {
				printf("   ");
				continue;
			}

			struct i2c_msg msgs[1];
			uint8_t dst;

			/* Send the address to read from */
			msgs[0].buf = &dst;
			msgs[0].len = 0U;
			msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
			if (i2c_transfer(d, &msgs[0], 1, i + j) == 0) {
				printf("%02x ", i + j);
				++cnt;
			} else {
				printf("-- ");
			}
		}
		printf("\n");
		}

	printf("%u devices found on %s\n", cnt, i2c_dev);
	return 0;
}

/* 
 * set I2C_BUS_NUMBER (0-5)
 */
int
i2c_set_mux(int argc, char **argv)
{
	int bus;

	//printk("Before setup\n");
	//gpio_dump_regs(XGPIOB);
	if (argc < 2) {
		printf("Current mux value: %d\n", get_mux());
		return 0;
	}

 	bus = atoi(argv[1]);

	if ( (bus < 0) || (bus > 5)) {
		printf("BUS: %d, error: 0 <= BUSNUM <= 5", bus);
		return -ENODEV;
	}
	//print("set executed, bus=%d", bus);
	set_mux(bus);
	return 0;
}
/* 
 * DEV I2C_BUS_NAME (I2C_1, etc)
 */
int
i2c_set_bus(int argc, char **argv)
{

/*	
	printf("i2c_dev = %d\n", (int) i2c_dev);
	printd("arg[1] = %s\n", argv[1]);
*/
	if (argc == 1) {
		if (!i2c_dev) {
			printf("Current bus unset\n");
		} else {
			printf("Current bus: %s\n", i2c_dev);
		}
		return 0;
	}
	
	if (strcmp("-r", argv[1]) == 0) {
		setup_dev(DEV_REINIT);
		printf("I2C bus reset to %s\n", DEF_BUS);
		return 0;
	}
			

	if (i2c_dev != (char *)NULL) {
		free(i2c_dev);
	}
		
	i2c_dev = malloc(strlen(argv[1])+1);
	strcpy(i2c_dev, argv[1]);
	printf("I2c Bus changed to %s\n", i2c_dev);

	return 0;
}

int
i2c_read_bytes(int dev_addr, int reg_addr, char *obuf, int olen)
{
	uint8_t reg_addr_buf[MAX_BYTES_FOR_REGISTER_INDEX];
	int reg_addr_bytes;
	const struct device *dev;
	int ret;

	dev = device_get_binding(i2c_dev);
	if (!dev) {
		printk("I2C: Device driver %s not found.", i2c_dev);
		return -ENODEV;
	}
	reg_addr_bytes = get_bytes_count_for_hex(reg_addr);
	sys_put_be32(reg_addr, reg_addr_buf);


	ret = i2c_write_read(dev, dev_addr,
			     reg_addr_buf +
			       MAX_BYTES_FOR_REGISTER_INDEX - reg_addr_bytes,
			     reg_addr_bytes, obuf, olen);
	if (ret < 0) {
		printk("Failed to read from device: 0x%02x", dev_addr);
		return -EIO;
	}
#if 0
	printk("i2c_write_read: 0x%02x  0x%02x  0x%02x  0x%02x\n",
		   obuf[0], obuf[1], obuf[2], obuf[3]);
#endif
	return 0;
}

int
i2c_write_bytes(int dev_addr, int reg_addr, char *ibuf, int ilen)
{
	int ret;
	/* [ 16 + 4 - 1 ] */
	uint8_t buf[MAX_I2C_BYTES + MAX_BYTES_FOR_REGISTER_INDEX - 1];
	const struct device *dev;
	int reg_addr_bytes;

	dev = device_get_binding(i2c_dev);
	if (!dev) {
		printk("I2C: Device driver %s not found.", i2c_dev);
		return -ENODEV;
	}
	reg_addr_bytes = get_bytes_count_for_hex(reg_addr);
	//printk("Write to addr: 0x%x\n", dev_addr);
	//printk("Write to reg:  0x%x\n", reg_addr);

	sys_put_be32(reg_addr, buf);

	
	int i;
	for (i = 0; i < ilen; i++) {
		buf[MAX_BYTES_FOR_REGISTER_INDEX + i] = ibuf[i];
	}

#if 0
	for (i=0; i<8; i++) {
		printk("0x%02x ", buf[i]);
	}
	printk("\n");
#endif
	/* len is 2, 1 for register size, 1 for our data */
	ret = i2c_write(dev,
					buf + MAX_BYTES_FOR_REGISTER_INDEX - reg_addr_bytes,
					ilen+reg_addr_bytes, dev_addr);

	return 0;
}

static
int get_bytes_count_for_hex(int val)
{
	char arg[10];
	itoa(val, arg, 16);

	//printk("%d number is string %s\n", val, arg);
	int length = (strlen(arg) + 1) / 2;

	if (length > 1 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
		length -= 1;
	}

	return MIN(MAX_BYTES_FOR_REGISTER_INDEX, length);
		
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

#if 0
	printf("run i2c submenu...argc = %d\n", argc);
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
