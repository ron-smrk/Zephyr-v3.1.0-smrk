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

	shell_print(shell, "Zephyr version %s", KERNEL_VERSION_STRING);

	return 0;
}

SHELL_CMD_ARG_REGISTER(version, NULL, "Show kernel version", cmd_version, 1, 0);

void main(void)
{
	printk("\nWelcome to smrk100g");

	setup_dev(DEV_SET);
	vdd_3r3_on(1);

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
