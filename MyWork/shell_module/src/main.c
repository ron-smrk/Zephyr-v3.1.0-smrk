/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>
#include <version.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <ctype.h>
#include "lib.h"
#include "pmbus.h"
#include "vrails.h"

LOG_MODULE_REGISTER(app);

#if defined CONFIG_SHELL_GETOPT
/* Thread save usage */
static int cmd_demo_getopt_ts(const struct shell *sh, size_t argc,
			      char **argv)
{
	struct getopt_state *state;
	char *cvalue = NULL;
	int aflag = 0;
	int bflag = 0;
	int c;

	while ((c = getopt(argc, argv, "abhc:")) != -1) {
		state = getopt_state_get();
		switch (c) {
		case 'a':
			aflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'c':
			cvalue = state->optarg;
			break;
		case 'h':
			/* When getopt is active shell is not parsing
			 * command handler to print help message. It must
			 * be done explicitly.
			 */
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if (state->optopt == 'c') {
				shell_print(sh,
					"Option -%c requires an argument.",
					state->optopt);
			} else if (isprint(state->optopt)) {
				shell_print(sh,
					"Unknown option `-%c'.",
					state->optopt);
			} else {
				shell_print(sh,
					"Unknown option character `\\x%x'.",
					state->optopt);
			}
			return 1;
		default:
			break;
		}
	}

	shell_print(sh, "aflag = %d, bflag = %d", aflag, bflag);
	return 0;
}

static int cmd_demo_getopt(const struct shell *sh, size_t argc,
			      char **argv)
{
	char *cvalue = NULL;
	int aflag = 0;
	int bflag = 0;
	int c;

	while ((c = getopt(argc, argv, "abhc:")) != -1) {
		switch (c) {
		case 'a':
			aflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'c':
			cvalue = optarg;
			break;
		case 'h':
			/* When getopt is active shell is not parsing
			 * command handler to print help message. It must
			 * be done explicitly.
			 */
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if (optopt == 'c') {
				shell_print(sh,
					"Option -%c requires an argument.",
					optopt);
			} else if (isprint(optopt)) {
				shell_print(sh, "Unknown option `-%c'.",
					optopt);
			} else {
				shell_print(sh,
					"Unknown option character `\\x%x'.",
					optopt);
			}
			return 1;
		default:
			break;
		}
	}

	shell_print(sh, "aflag = %d, bflag = %d", aflag, bflag);
	return 0;
}

static int cmd_demo_params(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "argc = %zd", argc);
	for (size_t cnt = 0; cnt < argc; cnt++) {
		shell_print(shell, "  argv[%zd] = %s", cnt, argv[cnt]);
	}

	return 0;
}

static int cmd_demo_hexdump(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "argc = %zd", argc);
	for (size_t cnt = 0; cnt < argc; cnt++) {
		shell_print(shell, "argv[%zd]", cnt);
		shell_hexdump(shell, argv[cnt], strlen(argv[cnt]));
	}

	return 0;
}
#endif

static int cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Version %s", KERNEL_VERSION_STRING);

	return 0;
}

SHELL_CMD_ARG_REGISTER(version, NULL, "Show kernel version", cmd_version, 1, 0);

#if 0
static int cmd_pmtst(const struct shell *shell, size_t argc, char **argv)
{
	int pg;

	if (argc < 2)
		pg = 0;
	else
		pg = atoi(argv[1]);
	
	shell_print(shell, "Set Page to %d", pg);
/*	set_mux(2); */
	pmset_page(0x40, pg);
 	pmget_mfr_id(0x40);
#if 0
	pmset_page(0x40, 0);
	pmget_mfr_id(0x40);
	pmset_page(0x40, 1);
	pmget_mfr_id(0x40);
	pmset_page(0x40, 2);
	pmget_mfr_id(0x40);
	pmset_page(0x40, 3);
	pmget_mfr_id(0x40);
	k_sleep(K_MSEC(1000));
#endif
	return 0;
}
static int pm_setrail(const struct shell *sh, size_t argc, char **argv)
{
	char rail[10];
	if (strlen(argv[1] > 8
}

SHELL_SUBCMD_SET_CREATE)(sub_pm, (pm));
SHELL_SUBCMD_ADD((pm), setrail, NULL, "Set Rail (A-D or LDO", pm_setrail, 2, 0);

SHELL_CMD_ARG_REGISTER(pm, &sub_pm, "PMBus Commands", cmd_pmtst, 2, 0);

#endif 
void main(void)
{
	printk("\nWelcome to smrk100g");

	setup_dev(DEV_SET);
	//printk("VDD 3.3 on.\n");
	set_ps_bit(SET_ALL, 1);
	//vrail_on(VDD_3R3);
	set_vrails(POWER_ON, 0, 0);
	init_cpu();

#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	const struct device *dev;
	uint32_t dtr = 0;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (!device_is_ready(dev) || usb_enable(NULL)) {
		return;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
#endif
}

struct pin {
	char *name;		/* name */
	int cval;		/* current value */
	int	func;		/* SET_POR, etc */
};
struct pin pintab[] = {
	{"srst", 0, SET_SRST},
	{"por", 0, SET_POR},
	{"prog", 0, SET_PROG},
	{"all", 0, SET_ALL},
	{NULL, 0, 0}
};
		
int
set_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	int i;
	if (argc == 1) {
		printk("\n");
		for (i = 0; pintab[i].func != SET_ALL; i++)
			printk("%s: %d\n", pintab[i].name, pintab[i].cval);
		return 0;
	}
	
	if (argc != 3) {
		printk("usage set <LINE> <VALUE>\n");
		return -1;
	}
	char *line = toLower(argv[1]);
	int val = atoi(argv[2]);

	if ((val < 0) || (val > 1)) {
		printk("val must be 1 or 0\n");
		return -1;
	}

	int match = 0;
	for (i = 0; pintab[i].name; i++) {
		if (strcmp(line, pintab[i].name) == 0) {
			set_ps_bit(pintab[i].func, val);
			if (pintab[i].func == SET_ALL) {
				int i2;
				for (i2 = 0; pintab[i2].func != SET_ALL; i2++) {
					pintab[i2].cval = val;
				}
			} else {
				pintab[i].cval = val;
			}
			match++;
		}
	}
	if (match)
		return 0;

	printk("line must one of: ");
	for (i = 0; pintab[i].name; i++) {
		printk("%s ", pintab[i].name);
	}
	printk("\n");
	return -1;
}
SHELL_CMD_ARG_REGISTER(set, NULL, "set <LINE> <VAL>", set_cmd, 0, 0);

struct pin status[] = {
	{"done_b", 0, GET_DONE_B},
	{"init", 0, GET_INIT},
	{NULL, 0, 0}
};

int
get_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	int i;

	if (argc == 1) {
		printk("\n");
		for (i = 0; status[i].name; i++)
			printk("%s: %d\n", status[i].name, get_ps_stat(status[i].func));
		return 0;
	}
	if (argc != 2) {
		printk("usage get <LINE>\n");
		return -1;
	}

	char *line = toLower(argv[1]);
	int match = 0;
	for (i = 0; status[i].name; i++) {
		if (strcmp(line, status[i].name) == 0) {
			printk("\n%s: %d\n", status[i].name, get_ps_stat(status[i].func));
			match++;
		}
	}
	if (match)
		return 0;
	printk("line must be one of: " );
	for (i = 0; status[i].name; i++) {
		printk("%s ", status[i].name);
	}
	printk("\n");
	return -1;
}



SHELL_CMD_ARG_REGISTER(get, NULL, "get <LINE>", get_cmd, 0, 0);
	
