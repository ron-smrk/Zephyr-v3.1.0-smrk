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
	printk("     -n SEC - Wait for N seconds between stages\n");
	printk("     on|off - tune power on/off\n");
	printk("status - status of power management\n");
	printk("pwr - Display power info\n");
	printk("volt - Display Voltages\n");
	printk("amps - Display Current\n");
	printk("temp - Display Temperature\n");
	printk("set - Set voltage <Rail> <Voltage>\n");
	printk("setreg - set register <Rail> <REGNUM> <VALUE>\n");
	return 0;
}

static int
pmbus_status_cmd(const struct shell *sh, size_t argc, char **argv)
{
	printk("\nStatus\n");
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
			//printk("i=%d, argv=%s\n", i, argv[i]);
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
		rval = pmbus_get_volt(i, &pwr);
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
pmbus_set_cmd(const struct shell *sh, size_t argc, char **argv)
{
	int i;
	int rval;
	int match = 0;
	double volt;
	double vdiff;
	char v[20], r[20], name[20], v2[20], v3[20];

	if (argc != 3) {
			printk("usage: pm set <RAIL> <VOLTAGE>\n");
			return 0;
	}

	volt = atof(argv[2]);
	strcpy(r, toLower(argv[1]));
	sprintf(v, "%.2f", volt);	// printk doesn't to floats..
	if ((volt <= 0.0) || (volt > 5.00)) {
		printk("Bad Voltage: %s\n", argv[2]);
		return 0;
	}

	printk("rail = %s, voltage = %s \n", r, v);

	i = 0;
	while (i < NUM_RAILS) {
		strcpy(name, vrail[i].signame);
		toLower(name);
		//printk("name=%s, r=%s\n", name, r);
		if (strncmp(name, r, 3) == 0) {
			match = 1;
			break;
		}
		i++;
	}
	if (!match) {
		printk("Bad rail: %s\n", r);
		return -1;
	}
	/*
	 * we have rail (i), nominal voltage (v2), desired voltage (v)
	 */
	vdiff = .1 * vrail[i].nominal;
	sprintf(v3, "%.2f", vdiff);
	sprintf(v2, "%.2f", vrail[i].nominal);

	printk("setting rail %s nominal(%s) to %s (diff=%s) \n", vrail[i].signame, v2, v, v3);
	return vrail_setvolt(i, volt);
}

struct reg_cmds {
	int cmd;
	int size;
	int encoding;
} reg_commands [] = {
	{0x5e, 2, PM_LINEAR8},
	{}
};

static int
pmbus_setreg_cmd(const struct shell *sh, size_t argc, char **argv)
{
	int i;
	int rval;
	int match = 0;
	double volt;
	double vdiff;
	char v[20], rail[20], name[20], v2[20], v3[20];
	int reg;

	printk("argc = %d\n", argc);
	if (argc != 4) {
			printk("usage: pm setreg <RAIL> <REGNUM> <VALUE>\n");
			return 0;
	}
	strcpy(rail, toLower(argv[1]));
	printk("\nrail: %s\n", rail);

	// put a function call here, used at least one other place
	for (i = 0; i < NUM_RAILS; i++) {
		strcpy(name, vrail[i].signame);
		toLower(name);
		if (strncmp(name, rail, 3) == 0) {
			match = 1;
			break;
		}
	}
	// reg = atox(argv[2]);
	printk("reg = %s, value=%s\n", argv[2], argv[3]);
	reg = 0x5e;
	if (match) {
		vrail[i].setreg(i, reg, argv[3]);
	} else {
		printk("bad rail %s\n", argv[1]);
	}
}
	
int
pmbus_setregnull(int rail, int reg, char *val)
{
	printk("pmbus setreg null, rail: %d, reg: 0x%x value: %s\n", rail, reg, val);
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
		rval = pmbus_get_volt(i, &pwr);
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
		if ( (rval=vrail_pg(i)) < 0)
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
SHELL_SUBCMD_ADD((pm), set, NULL, "Set Voltages", pmbus_set_cmd, 2, 1);
SHELL_SUBCMD_ADD((pm), setreg, NULL, "Set PMBus Registers", pmbus_setreg_cmd, 4, 0);

SHELL_CMD_REGISTER(pm, &sub_pmbus, "Power Functions", NULL);
