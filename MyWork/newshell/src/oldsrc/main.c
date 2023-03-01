/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/stats/stats.h>
#include <zephyr/shell/shell.h>
#include <version.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <ctype.h>
#include "lib.h"
#include "pmbus.h"
#include "vrails.h"

#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include "fs_mgmt/fs_mgmt.h"
#include <zephyr/fs/littlefs.h>
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
#include "stat_mgmt/stat_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_SHELL_MGMT
#include "shell_mgmt/shell_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
#include "fs_mgmt/fs_mgmt.h"
#endif

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_sample);

#include "common.h"

/* Define an example stats group; approximates seconds since boot. */
STATS_SECT_START(smp_svr_stats)
STATS_SECT_ENTRY(ticks)
STATS_SECT_END;

/* Assign a name to the `ticks` stat. */
STATS_NAME_START(smp_svr_stats)
STATS_NAME(smp_svr_stats, ticks)
STATS_NAME_END(smp_svr_stats);

/* Define an instance of the stats group. */
STATS_SECT_DECL(smp_svr_stats) smp_svr_stats;

#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.mnt_point = "/lfs1"
};
#endif

static int cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Version %s", KERNEL_VERSION_STRING);

	return 0;
}

SHELL_CMD_ARG_REGISTER(version, NULL, "Show kernel version", cmd_version, 1, 0);

void status_poll(void *id, void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	int my_id = POINTER_TO_INT(id);

	while (1) {
		//printk("Thread %d running\n", my_id);
		k_msleep(1000);
	}
}

#define STACK_SIZE (768 + CONFIG_TEST_EXTRA_STACK_SIZE)
static K_THREAD_STACK_DEFINE(pmstack, STACK_SIZE);
static struct k_thread pmpoll;	// Power Management
#define PRIORITY 0
static k_tid_t poll_tid;

void main(void)
{
	printk("\nWelcome to smrk100g (%s)", KERNEL_VERSION_STRING);

	setup_dev(DEV_SET);

	start_cpu(); // Bring up CPU

	poll_tid = k_thread_create(&pmpoll, &pmstack, STACK_SIZE, status_poll,
					NULL, NULL, NULL, 0, 0, K_MSEC(1000));
	/*              p1 p1 p3 PR op delay */

	k_thread_name_set(poll_tid, "Board Status");

	int rc = STATS_INIT_AND_REG(smp_svr_stats, STATS_SIZE_32,
				    "smp_svr_stats");

	if (rc < 0) {
		LOG_ERR("Error initializing stats system [%d]", rc);
	}

	/* Register the built-in mcumgr command handlers. */
#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
	rc = fs_mount(&littlefs_mnt);
	if (rc < 0) {
		LOG_ERR("Error mounting littlefs [%d]", rc);
	}

	fs_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
	os_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
	img_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
	stat_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_SHELL_MGMT
	shell_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_SMP_BT
	start_smp_bluetooth();
#endif
#ifdef CONFIG_MCUMGR_SMP_UDP
	start_smp_udp();
#endif

	if (IS_ENABLED(CONFIG_USB_DEVICE_STACK)) {
		rc = usb_enable(NULL);
		if (rc) {
			LOG_ERR("Failed to enable USB");
			return;
		}
	}
	/* using __TIME__ ensure that a new binary will be built on every
	 * compile which is convenient when testing firmware upgrade.
	 */
	LOG_INF("build time (DEADBEEF!): " __DATE__ " " __TIME__);

	/* The system work queue handles all incoming mcumgr requests.  Let the
	 * main thread idle while the mcumgr server runs.
	 */
	while (1) {
		k_sleep(K_MSEC(1000));
		STATS_INC(smp_svr_stats, ticks);
//		printk("BEEP!\n");
	}
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
	
