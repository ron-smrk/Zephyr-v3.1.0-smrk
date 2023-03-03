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
#include "i2c.h"
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "shell.h"
static int pmbus_reg_cmd(int, char **);


// Remove when used....
__attribute__ ((unused))
static int
pmbus_getmfr_id(int bus)
{
	char id[5];
	int v;

	pmbus_rdblock(bus, PMBUS_MFR_ID, 3, id);
	// printf("ID: 0x%x 0x%x 0x%x\n", id[0], id[1], id[2]);

	v = toint(id, 3);
	// printf("Got 0x%06x for mfr_id!\n", v);
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
pmbus_help_cmd(int argc, char **argv)
{
	printf("\nPower Sequencing Options\n");
	printf("pm seq [-w|-n [time]] <on|off>\n");
	//printf("-w - Wait for Key Input between stages\n");
	printf("     -n SEC - Wait for N seconds between stages\n");
	printf("     on|off - tune power on/off\n");
	printf("status - status of power management\n");
	printf("pwr - Display power info\n");
	printf("volt - Display Voltages\n");
	printf("amps - Display Current\n");
	printf("temp - Display Temperature\n");
	printf("set - Set voltage <Rail> <Voltage>\n");
	printf("reg - set register <Rail> <REGNUM> <VALUE>\n");
	return 0;
}

static int
pmbus_status_cmd(int argc, char **argv)
{
	printf("\nStatus\n");
	return 0;
}
static int
pmbus_seq_cmd(int argc, char **argv)
{
	int i;
	int n = -1;
	int wflg = 0;
	char *tmp = NULL;
	int val = -1;

	printf("\nPower Sequence\n");
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'w':
				wflg++;
				break;
			case 'n':
				n = atoi(argv[i+1]);
				if (n <- 0) {
					printf(" N must be > 0\n");
					return -1;
				}
				i++;
				break;
			default:
				printf("Bad option: %c\n", argv[i][1]);
				return -1;
			}
		} else {
			//printf("i=%d, argv=%s\n", i, argv[i]);
			tmp = strdup(argv[i]);
			tmp = toLower(tmp);
			if (strcmp(tmp, "on") == 0) {
				val = POWER_ON;
			} else if (strcmp(tmp, "off") == 0) {
				val = POWER_OFF;
			} else {
				printf("Bad arg: %s\n", argv[i]);
				free(tmp);
				return -1;
			}
			free(tmp);
		}
	}
	//printf("w=%d, val=%d\n", wflg, val);
	//if (n > 0 )
	//	printf("Delay: %d\n", n);
	//	if (wflg)
	//		console_init();

	// For now ignore
	wflg = 0;
	set_vrails(val, n, wflg);

	return 0;
}
static int
pmbus_volt_cmd(int argc, char **argv)
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

	printf("\nVoltage Rails: (* - Use PG signal to determine voltage)\n");
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
		printf("%20s: %s", vrail[i].signame, v);
		if (rawmode)
			printf(" %s", r);
		printf("\n");
		i++;
	}
	return 0;
}

static int
pmbus_set_cmd(int argc, char **argv)
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
			printf("usage: pm set <RAIL> <VOLTAGE>\n");
			return 0;
	}

	strcpy(r, toLower(argv[1]));
	if (argc == 2) {
		rdonly = 1;
		v = NULL;
		// printf("rail = %s, dump only\n", r);
	} else {
		v = argv[2];
		volt = atof(v);
		if ((volt <= 0.0) || (volt > 5.00)) {
			printf("Bad Voltage: %s\n", argv[2]);
			return 0;
		}
		// printf("rail = %s, voltage = %s \n", r, v);
	};

	i = 0;
	while (i < NUM_RAILS) {
		strcpy(name, vrail[i].signame);
		toLower(name);
		//printf("name=%s, r=%s\n", name, r);
		if (strncmp(name, r, 3) == 0) {
			match = 1;
			break;
		}
		i++;
	}
	if (!match) {
		printf("Bad rail: %s\n", r);
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
		//printf("volt = %s\n", f2str(volt, 2));
		//printf("nominal = %s\n", f2str(vrail[i].nominal, 2));
		//printf("Diff: %s\n", f2str(volt - vrail[i].nominal, 2));
		//printf("fabs(Diff: %s)\n", f2str(fabs(volt - vrail[i].nominal), 2));
		if ( fabs(volt - vrail[i].nominal) > vdiff ) {
			printf("New voltage needs to be within 10%% of nominal\n");
			return -1;
		}

		printf("setting rail %s nominal(%s) to %s (max diff=%s) \n", vrail[i].signame, f2str(vrail[i].nominal, 2), v, f2str(vdiff, 2));
	} else {
		ac = 3;
	}

	char *av[10];
	av[0] = "reg";
	av[1] = r;
	av[2] = "21";
	av[3] = v;

	
	return pmbus_reg_cmd(ac, av);
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
pmbus_reg_cmd(int argc, char **argv)
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
	printf("argc = %d\n", argc);
	printf("argv[0] = %s\n", argv[0]);

	for (i=0; i < argc; i++) {
		printf("argc=%d ptr=0x%x, str=--%s--\n", i, argv[i], argv[i]);
	}
#endif

	av++;	/* past 'reg' */
	if (strcmp(*av, "-r") == 0) {
		argc--;
		av++;
		rawmode = 1;
	}
	if ((argc < 3) || (argc > 4)) {
			printf("usage: pm reg [-r] <RAIL> <REGNUM> [<VALUE>]\n");
			printf("  <VALUE> omitted, display current value\n");
			return 0;
	}
	
	rail = *av++;
	regnum = *av++;
	if (argc == 4)
		value = *av;
	else
		value = NULL;
#if 0
	printf("argc = %d\n", argc);

	printf("rail =%s\n", rail);
	printf("regnum =%s\n", regnum);
	printf("value =%p\n", value);
	printf("value =%s\n", value);
#endif
	/* lower case version of rail...*/
	strcpy(lrail, toLower(rail));
/*
	printf("\nrail: %s\n", lrail);
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
		printf("Unknown rail %s\n", rail);
		return -1;
	}

	match = 0;
	reg = strtol(regnum, NULL, 16);
	// printf("Regnum=0x%x\n", reg);
	for (i = 0; reg_commands[i].regnum != -1; i++) {
		// printf("reg[%d] = 0x%x\n", i, reg_commands[i].regnum);
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
		printf("Unknown Register 0x%x\n", reg);
		return -1;
	}
	vrail[railval].reg(railval, &pmb);
	return 0;
}
	
int
pmbus_regnull(int rail, struct pmbus_op *p)
{
	if (p->value) {
		printf("pmbus_null setting rail: %d, reg: 0x%x, value: %s\n", rail, p->reg, p->value);
	} else {
		printf("pmbus_null getting rail: %d, reg: 0x%x\n", rail, p->reg);
	}
	return 0;
}

static int
pmbus_pwr_cmd(int argc, char **argv)
{
	int i;
	char *mod;
	struct power_vals pwr;
	int rval;
	char v[20], a[20], w[20], *pg;
	double twatts = 0; // Total power (watts)
	double amps, volts, watts; 
	
	printf("Voltage Rails: (* - Use PG signal to determine voltage)\n");
	printf("                Rail  Volts       Amps      PG  Watts\n");
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
			   
			
		printf("%20s: %s    %s   %s   %s\n", vrail[i].signame, v, a, pg, w);
		i++;
	}
	sprintf(w, "%.4f Watts", twatts);
	printf("                                   Total Power %s\n", w);
	return 0;
}

static int trails[] = {VDD_2R5, VDD_0R9, VDD_1R2_DDR, -1};
static int
pmbus_temp_cmd(int argc, char **argv)
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
	
	printf("\nTemperature:\n");
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
		printf("%20s: %s", vrail[trails[i]].signame, temp);
		if (rawmode)
			printf(" %s", r);
		printf("\n");
		i++;
	}
	return 0;
}

static int irails[] = {VDD_0R85, VDD_1R8, VDD_1R2_DDR, VDD_2R5, VDD_0R9, VDD_1R0, -1};
static int
pmbus_amps_cmd(int argc, char **argv)
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
	
	printf("\nCurrent:\n");
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
		printf("%20s: %s", vrail[irails[i]].signame, amps);
		if (rawmode)
			printf(" %s", r);
		printf("\n");
		i++;
	}

	return 0;
}

static struct command_table pmbus_tab[] = {
	{"seq", pmbus_seq_cmd, "Power Sequencing"},
	{"status", pmbus_status_cmd, "Status"},
	{"pwr", pmbus_pwr_cmd, "Display Power Information"},
	{"volt", pmbus_volt_cmd, "Display Voltages"},
	{"amps", pmbus_amps_cmd, "Display Current"},
	{"temp", pmbus_temp_cmd, "Display Temperatures"},
	{"set", pmbus_set_cmd, "Set Voltages"},
	{"reg", pmbus_reg_cmd, "Set/Display PMBus Registers"},
	{0}
};

int
pmbus_cmd(int argc, char **argv)
{
	int i;
	char *p = NULL;

#if 0
	printf("run PMBUS submenu...argc = %d\n", argc);
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
	runcmdlist(pmbus_tab, "PMBus> ", 1, p);
	if (p)
		free(p);

	//printf("Back...\n");
	return 0;
}
