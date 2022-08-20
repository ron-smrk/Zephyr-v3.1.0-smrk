#include <stdlib.h>
#include <sys/byteorder.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>
#include <zephyr/console/console.h>

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
int
pmbus_wr_common(int addr, int command, int length, unsigned char *data, int blk)
{
	unsigned char sendbuf[131];
	
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

/*
 * pm commands
 * pm help
 * pm seq [-w|-n [time]] <on|off>
 * pm status
 * pm rdvolt
 */

static int
pmbus_help(const struct shell *sh, size_t argc, char **argv)
{
	printk("\nPower Sequencing Options\n");
	printk("pm seq [-w|-n [time]] <on|off>\n");
	//printk("-w - Wait for Key Input between stages\n");
	printk("-n SEC - Wait for N seconds between stages\n");
	printk("on|off - tune power on/off\n");
	printk("status - status of power management\n");
	printk("rdvolt - read voltages\n");
	return 0;
}

static int
pmbus_status(const struct shell *sh, size_t argc, char **argv)
{
	printk("\nStatus\n");
	return 0;
}
struct power_rails {
	int(*on)(char *);
	int(*off)(char *);
	int(*isgood)(char *);
	double(*rdvolt)(char *);
	char *signal;
} rails[] = {
	{vdd_0r85_on, vdd_0r85_off, vdd_0r85_isgood, vdd_0r85_rdvolt,
	 "VDD_0R85"},
	{vdd_1r8_on, vdd_1r8_off, vdd_1r8_isgood, vdd_1r8_rdvolt,
	 "VDD_1R8FPGA"},
	{vdd_3r3_on, vdd_3r3_off, vdd_3r3_isgood, vdd_3r3_rdvolt,
	 "VDD_3R3"},
	{vdd_1r2_ddr_on, vdd_1r2_ddr_off, vdd_1r2_ddr_isgood, vdd_1r2_ddr_rdvolt,
	 "VDD_IR2_DDR"},
	{vdd_0r6_on, vdd_0r6_off, vdd_0r6_isgood, vdd_0r6_rdvolt,
	 "VDD_0R6_VTT"},
	{vdd_2r5_on, vdd_2r5_off, vdd_2r5_isgood, vdd_2r5_rdvolt,
	 "VDD_2R5"},
	{vdd_1r2_mgt_on, vdd_1r2_mgt_off, vdd_1r2_mgt_isgood, vdd_1r2_mgt_rdvolt,
	 "VDD_1R2_MGTVTT"},
	{vdd_0r9_on, vdd_0r9_off, vdd_0r9_isgood, vdd_0r9_rdvolt,
	 "VDD_0R9_MGTAVCC"},
	{vdd_1r0_on, vdd_1r0_off, vdd_1r0_isgood, vdd_1r0_rdvolt,
	 "VDD_1R0"},
};

	
static int
pmbus_seq(const struct shell *sh, size_t argc, char **argv)
{
	int i;
	int n = -1;
	int wflg = 0;
	char *tmp = NULL;
	int val = -1;
	int nrails;

	printk("\nPower Sequence\n");
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'w':
				wflg++;
				break;
			case 'n':
				n = atoi(argv[i+1]);
				if (n <- 0) {
					printk(" N must be > 0\n");
					return -1;
				}
				i++;
				break;
			default:
				printk("Bad option: %c\n", argv[i][1]);
				return -1;
			}
		} else {
			tmp = strdup(argv[i]);
			tmp = toLower(tmp);
			if (strcmp(tmp, "on") == 0) {
				val = 1;
			} else if (strcmp(tmp, "off") == 0) {
				val = 0;
			} else {
				printk("Bad arg: %s\n", argv[i]);
				free(tmp);
				return -1;
			}
			free(tmp);
		}
	}
	//printk("w=%d, val=%d\n", wflg, val);
	//if (n > 0 )
	//	printk("Delay: %d\n", n);
	//	if (wflg)
	//		console_init();

	// For now ignore
	wflg = 0;

	nrails = sizeof(rails)/sizeof(struct power_rails);
//	uint8_t c;
	if (val == 1) {
		i = 0;
		while(i < nrails) {
			rails[i].on(rails[i].signal);
		    rails[i].isgood(rails[i].signal);
			if (n>0)
				k_sleep(K_MSEC(1000*n));
			else if (wflg) {
				//printk("press key to continue: ");
				//c=console_getchar();
				//printk("BACK\n");
			}
			i++;
		}
	} else {
		// point to last one
		i = nrails-1;
		while (i >= 0) {
			rails[i].off(rails[i].signal);
			if (n>0)
				k_sleep(K_MSEC(1000*n));
			else if (wflg) {
			}
			i--;
		}
	}
	return 0;
}
static int
pmbus_rdvolt(const struct shell *sh, size_t argc, char **argv)
{
	int nrails, i;
	double volts;
	
	printk("\nVoltage Rails:\n");
	nrails = sizeof(rails)/sizeof(struct power_rails);
	i = 0;
	while(i < nrails) {
		volts = rails[i].rdvolt(NULL);
		printk("%20s: %.4fV\n", rails[i].signal, volts);
		i++;
	}
	return 0;
}

SHELL_SUBCMD_SET_CREATE(sub_pmbus, (pm));
SHELL_SUBCMD_ADD((pm), help, NULL, "Help for pmbus commands", pmbus_help, 1, 0);
SHELL_SUBCMD_ADD((pm), seq, NULL, "Power Sequencing", pmbus_seq, 2, 2);
SHELL_SUBCMD_ADD((pm), status, NULL, "Status", pmbus_status, 1, 1);
SHELL_SUBCMD_ADD((pm), rdvolt, NULL, "Read Voltages", pmbus_rdvolt, 1, 1);

SHELL_CMD_REGISTER(pm, &sub_pmbus, "Power Functions", NULL);
