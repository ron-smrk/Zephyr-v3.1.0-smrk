/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <version.h>

/* boot banner items */
#if defined(CONFIG_MULTITHREADING) && defined(CONFIG_BOOT_DELAY) &&            \
	CONFIG_BOOT_DELAY > 0
#define BOOT_DELAY_BANNER " (delayed boot " STRINGIFY(CONFIG_BOOT_DELAY) "ms)"
#else
#define BOOT_DELAY_BANNER ""
#endif

#if defined(CONFIG_BOOT_DELAY) || CONFIG_BOOT_DELAY > 0
void boot_banner(void)
{
#if defined(CONFIG_BOOT_DELAY) && CONFIG_BOOT_DELAY > 0
	static const unsigned int boot_delay = CONFIG_BOOT_DELAY;
#else
	static const unsigned int boot_delay;
#endif

	if (boot_delay > 0 && IS_ENABLED(CONFIG_MULTITHREADING)) {
		printk("***** delaying boot " STRINGIFY(
			CONFIG_BOOT_DELAY) "ms (per build configuration) *****\n");
		k_busy_wait(CONFIG_BOOT_DELAY * USEC_PER_MSEC);
	}

/* #define VBANNER "Zephyr OS" */
#define VBANNER "OS"
#if defined(CONFIG_BOOT_BANNER)
#ifdef BUILD_VERSION
	printk("*** Booting %s build %s %s ***\n", VBANNER,
	       STRINGIFY(BUILD_VERSION), BOOT_DELAY_BANNER);
#else
	printk("*** Booting %s version %s %s ***\n", VBANNER,
	       KERNEL_VERSION_STRING, BOOT_DELAY_BANNER);
#endif
#endif
}
#else
void boot_banner(void)
{
	/* do nothing */
}
#endif
