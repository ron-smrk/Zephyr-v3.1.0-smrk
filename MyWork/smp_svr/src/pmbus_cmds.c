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

static int pmbus_reg_cmd(const struct shell *, size_t, char **);


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
	printk("reg - set register <Rail> <REGNUM> <VALUE>\n");
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
	int match = 0;
	double volt;
	double vdiff;
	char *v;
	// char v[20], r[20], name[20], v2[20], v3[20];
	char r[20], name[20];
	int rdonly = 0;

	if ((argc < 2) || (argc > 3)) {
			printk("usage: pm set <RAIL> <VOLTAGE>\n");
			return 0;
	}

	strcpy(r, toLower(argv[1]));
	if (argc == 2) {
		rdonly = 1;
		v = NULL;
		printk("rail = %s, dump only\n", r);
	} else {
		v = argv[2];
		volt = atof(v);
		if ((volt <= 0.0) || (volt > 5.00)) {
			printk("Bad Voltage: %s\n", argv[2]);
			return 0;
		}
		printk("rail = %s, voltage = %s \n", r, v);
	};

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
	 * we have rail (i), nominal voltage, desired voltage (v)
	 */
	int ac;
	if (!rdonly) {

		ac = 4;
		// This is maximum difference allowed (10%)
		vdiff = .1 * vrail[i].nominal;
		//printk("volt = %s\n", f2str(volt, 2));
		//printk("nominal = %s\n", f2str(vrail[i].nominal, 2));
		//printk("Diff: %s\n", f2str(volt - vrail[i].nominal, 2));
		//printk("fabs(Diff: %s)\n", f2str(fabs(volt - vrail[i].nominal), 2));
		if ( fabs(volt - vrail[i].nominal) > vdiff ) {
			printk("New voltage needs to be within 10%% of nominal\n");
			return -1;
		}

		printk("setting rail %s nominal(%s) to %s (max diff=%s) \n", vrail[i].signame, f2str(vrail[i].nominal, 2), v, f2str(vdiff, 2));
	} else {
		ac = 3;
	}

	char *av[10];
	av[0] = "reg";
	av[1] = r;
	av[2] = "21";
	av[3] = v;

	
	return pmbus_reg_cmd(NULL, ac, av);
}

struct reg_cmds {
	int regnum;
	int size;
	int encoding;
} reg_commands [] = {
	{0x21, 2, PM_LINEAR8},	// VOUT_COMMAND
	{0x24, 2, PM_LINEAR8},	// VOUT_MAX
	{0x5e, 2, PM_LINEAR8},	// POWER_GOOD_ON
	{0x5f, 2, PM_LINEAR8},  // POWER_GOOD_OFF
	{0x61, 2, PM_LINEAR2},  // TON_RISE
	{-1, -1, -1}
};

static int
pmbus_reg_cmd(const struct shell *sh, size_t argc, char **argv)
{
	int i;
	int match = 0;
	char lrail[20], tmpname[20];
	int reg;
	int rawmode = 0;
	char *rail, *regnum, *value;
	struct pmbus_op pmb;
	char **av = argv;
	int railval;

#if 0
	printk("argc = %d\n", argc);
	printk("argv[0] = %s\n", argv[0]);

	for (i=0; i < argc; i++) {
		printk("argc=%d ptr=0x%x, str=--%s--\n", i, argv[i], argv[i]);
	}
#endif

	av++;	/* past 'reg' */
	if (strcmp(*av, "-r") == 0) {
		argc--;
		av++;
		rawmode = 1;
	}
	if ((argc < 3) || (argc > 4)) {
			printk("usage: pm reg [-r] <RAIL> <REGNUM> [<VALUE>]\n");
			printk("  <VALUE> omitted, display current value\n");
			return 0;
	}
	
	rail = *av++;
	regnum = *av++;
	if (argc == 4)
		value = *av;
	else
		value = NULL;
#if 0
	printk("argc = %d\n", argc);

	printk("rail =%s\n", rail);
	printk("regnum =%s\n", regnum);
	printk("value =%p\n", value);
	printk("value =%s\n", value);
#endif
	/* lower case version of rail...*/
	strcpy(lrail, toLower(rail));
/*
	printk("\nrail: %s\n", lrail);
*/

	// put a function call here, used at least one other place
	for (i = 0; i < NUM_RAILS; i++) {
		strcpy(tmpname, vrail[i].signame);
		toLower(tmpname);
		if (strncmp(tmpname, lrail, 3) == 0) {
			match = 1;
			railval = i;
			break;
		}
	}
	if (!match) {
		printk("Unknown rail %s\n", rail);
		return -1;
	}

	match = 0;
	reg = strtol(regnum, NULL, 16);
	// printk("Regnum=0x%x\n", reg);
	for (i = 0; reg_commands[i].regnum != -1; i++) {
		// printk("reg[%d] = 0x%x\n", i, reg_commands[i].regnum);
		if (reg_commands[i].regnum == reg) {
			match = 1;
			pmb.reg = reg;
			pmb.encoding = reg_commands[i].encoding;
			pmb.size = reg_commands[i].size;
			pmb.rawmode = rawmode;
			if (value) {
				pmb.mode = PMWRITE;
				strcpy(pmb.value, value);
			} else {
				pmb.mode = PMREAD;
			}
			break;
		}
	}
	if (!match) {
		printk("Unknown Register 0x%x\n", reg);
		return -1;
	}
	vrail[railval].reg(railval, &pmb);
	return 0;
}
	
int
pmbus_regnull(int rail, struct pmbus_op *p)
{
	if (p->value) {
		printk("pmbus_null setting rail: %d, reg: 0x%x, value: %s\n", rail, p->reg, p->value);
	} else {
		printk("pmbus_null getting rail: %d, reg: 0x%x\n", rail, p->reg);
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
SHELL_SUBCMD_ADD((pm), reg, NULL, "Set/Display PMBus Registers", pmbus_reg_cmd, 3, 2);

SHELL_CMD_REGISTER(pm, &sub_pmbus, "Power Functions", NULL);
