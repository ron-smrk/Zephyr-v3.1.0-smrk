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

#if 1

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
 * pm volt
 * pm amps
 * pm temp
 */

static int
pmbus_help_cmd(const struct shell *sh, size_t argc, char **argv)
{
	printk("\nPower Sequencing Options\n");
	printk("pm seq [-w|-n [time]] <on|off>\n");
	//printk("-w - Wait for Key Input between stages\n");
	printk("-n SEC - Wait for N seconds between stages\n");
	printk("on|off - tune power on/off\n");
	printk("status - status of power management\n");
	printk("pow[er] - Display power info\n");
	printk("volt - Display Voltages\n");
	printk("amps - Display Current\n");
	printk("temp - Display Temperature\n");
	return 0;
}

static int
pmbus_status_cmd(const struct shell *sh, size_t argc, char **argv)
{
	printk("\nStatus\n");
	return 0;
}

int
set_vrails(int sense, int sleep, int wait)
{
	printk("\nsetting vrails %s\n", (sense==POWER_ON)?"on":"off");
	int i;
	if (sense == POWER_ON) {
		i = 0;
		while(i < NUM_RAILS) {
			vrail_on(i);
		    vrail_isgood(i);
			if (sleep > 0)
				k_sleep(K_MSEC(1000*sleep));
			else if (wait) {
				//printk("press key to continue: ");
				//c=console_getchar();
				//printk("BACK\n");
			}
			i++;
		}
	} else {
		// point to last one
		i = NUM_RAILS-1;
		while (i >= 0) {
			vrail_off(i);
			if (sleep > 0)
				k_sleep(K_MSEC(1000*sleep));			else if (wait) {
				//printk("press key to continue: ");
				//c=console_getchar();
				//printk("BACK\n");
			}
			i--;
		}
	}
	return 0;
}

static int
pmbus_seq_cmd(const struct shell *sh, size_t argc, char **argv)
{
	int i;
	int n = -1;
	int wflg = 0;
	char *tmp = NULL;
	int val = -1;

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
				val = POWER_ON;
			} else if (strcmp(tmp, "off") == 0) {
				val = POWER_OFF;
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
	set_vrails(val, n, wflg);

	return 0;
}
static int
pmbus_volt_cmd(const struct shell *sh, size_t argc, char **argv)
{
	int i;
	char *mod;
	int rawmode = 0;
	struct power_vals pwr;
	int rval;
	char v[20], r[20];
	
	if (argc > 1) {
		if (strcmp(argv[1], "-r") == 0) {
			rawmode = 1;
		}
	}

	printk("\nVoltage Rails: (* - Use PG signal to determine voltage)\n");
	i = 0;
	while (i < NUM_RAILS) {
		if (vrail[i].type & GPIO_RD)
			mod = "V*";
		else
			mod = "V ";
		rval = vrail_rdvolt(i, &pwr);
		if (rval < 0) {
			sprintf(v, "????????");
			sprintf(r, "---");
		} else {
			sprintf(v, "%.4f%s", pwr.fval, mod);
			if (vrail[i].type & PMBUS_RD)
				sprintf(r, "(0x%x)", pwr.sval);
			else
				sprintf(r, "(---)");
				
		}
		printk("%20s: %s", vrail[i].signame, v);
		if (rawmode)
			printk(" %s", r);
		printk("\n");
		i++;
	}
	return 0;
}

static int
pmbus_pwr_cmd(const struct shell *sh, size_t argc, char **argv)
{
	int i;
	char *mod;
	struct power_vals pwr;
	int rval;
	char v[20], a[20], w[20], *pg;
	double twatts = 0; // Total power (watts)
	double amps, volts, watts; 
	
	printk("Voltage Rails: (* - Use PG signal to determine voltage)\n");
	printk("                Rail  Volts       Amps      PG  Watts\n");
	i = 0;
	while (i < NUM_RAILS) {
		if (vrail[i].type & GPIO_RD)
			mod = "V*";
		else
			mod = "V ";
		rval = vrail_rdvolt(i, &pwr);
		if (rval < 0) {
			sprintf(v, "????????");
			volts = -1.0;
		} else {
			sprintf(v, "%.4f%s", pwr.fval, mod);
			volts = pwr.fval;
		}
		rval = pmbus_get_iout(i, &pwr);
		if (rval < 0) {
			sprintf(a, "???????");
			amps = -1.0;
		} else {
			sprintf(a, "%.4fA", pwr.fval);
			amps = pwr.fval;
		}
		watts = amps * volts;
		if (watts >= 0.0) {
			twatts = twatts + watts;
			sprintf(w, "%.4fW", watts);
		} else {
			sprintf(w, "???????");
		}
		if ( (rval=vrail_isgood(i)) < 0)
			pg = "?";
		else if (rval == POWER_GOOD)
			pg = "T";
		else
			pg = "F";
			   
			
		printk("%20s: %s    %s   %s   %s\n", vrail[i].signame, v, a, pg, w);
		i++;
	}
	sprintf(w, "%.4f Watts", twatts);
	printk("                                   Total Power %s\n", w);
	return 0;
}

static int trails[] = {VDD_2R5, VDD_0R9, VDD_1R2_DDR, -1};
static int
pmbus_temp_cmd(const struct shell *sh, size_t argc, char **argv)
{
	int i;
	int rawmode = 0;
	struct power_vals pwr;
	int rval;
	char temp[20], r[20];
	
	if (argc > 1) {
		if (strcmp(argv[1], "-r") == 0) {
			rawmode = 1;
		}
	}
	
	printk("\nTemperature:\n");
	i = 0;
	while (trails[i] != -1) {
		rval =  pmbus_get_temp(trails[i], &pwr);
		if (rval < 0) {
			sprintf(temp, "????????");
			sprintf(r, "---");
		} else {
			sprintf(temp, "%.4fC", pwr.fval);
			sprintf(r, "(0x%x)", pwr.sval);
		}
		printk("%20s: %s", vrail[trails[i]].signame, temp);
		if (rawmode)
			printk(" %s", r);
		printk("\n");
		i++;
	}
	return 0;
}

static int irails[] = {VDD_0R85, VDD_1R8, VDD_1R2_DDR, VDD_2R5, VDD_0R9, VDD_1R0, -1};
static int
pmbus_amps_cmd(const struct shell *sh, size_t argc, char **argv)
{
	int i;
	int rawmode = 0;
	struct power_vals pwr;
	char amps[20], r[20];
	int rval;

	if (argc > 1) {
		if (strcmp(argv[1], "-r") == 0) {
			rawmode = 1;
		}
	}
	
	printk("\nCurrent:\n");
	i = 0;
	while (irails[i] != -1) {
		rval = pmbus_get_iout(irails[i], &pwr);
		if (rval < 0) {
			sprintf(amps, "????");
			sprintf(r, "(---)");
		} else {
			sprintf(amps, "%.4fA", pwr.fval);
			sprintf(r, "(0x%x)", pwr.sval);
		}
		printk("%20s: %s", vrail[irails[i]].signame, amps);
		if (rawmode)
			printk(" %s", r);
		printk("\n");
		i++;
	}

	return 0;
}

SHELL_SUBCMD_SET_CREATE(sub_pmbus, (pm));
SHELL_SUBCMD_ADD((pm), help, NULL, "Help for pmbus commands", pmbus_help_cmd, 1, 0);
SHELL_SUBCMD_ADD((pm), seq, NULL, "Power Sequencing", pmbus_seq_cmd, 2, 2);
SHELL_SUBCMD_ADD((pm), status, NULL, "Status", pmbus_status_cmd, 1, 1);
SHELL_SUBCMD_ADD((pm), pwr, NULL, "Display Power Information", pmbus_pwr_cmd, 1, 1);
SHELL_SUBCMD_ADD((pm), volt, NULL, "Display Voltages", pmbus_volt_cmd, 1, 1);
SHELL_SUBCMD_ADD((pm), amps, NULL, "Display Current", pmbus_amps_cmd, 1, 1);
SHELL_SUBCMD_ADD((pm), temp, NULL, "Display Temperatures", pmbus_temp_cmd, 1, 1);

SHELL_CMD_REGISTER(pm, &sub_pmbus, "Power Functions", NULL);
