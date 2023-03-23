#include "shell.h"
#include "console.h"
#include "lib.h"

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

#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.mnt_point = "/lfs1"
};
#endif

#define PROMPT "# "
#define MAXARGS	20
#define WHITESPACE " \t\n"

struct command_table cmd_tab[] = {
	{"history", hist_cmd, "History"},
	{"i2c", i2c_cmd, "I2C Commands"},
	{"led", led_cmd, "LED Commands"},
	{"pmbus", pmbus_cmd, "Power Management commands."},
	{"kernel", kern_cmd, "Kernel Commands."},
	{"sleep", sleep_cmd, "Delay (seconds)."},
	{"echo", echo_cmd, "echo args."},
	{"version", vers_cmd, "Show Software Version."},
	{0}
};

/*
 * s is input string, argc, argv filled in
 */
int
buildargs(char *p, char **argv)
{
	char *p1;
	int argc = 0;

	if (p == NULL) {
		return 0;
	}
	if (strtok(p, WHITESPACE) == NULL) {	/* argv[0] is required */
		/* should not happen */
		// printf("Returning no cmd!\n");
		return 0;
	}
	argv[argc] = strdup(p);

	while ((p1 = strtok(NULL, WHITESPACE)) != NULL) {
		if (++argc >= MAXARGS) {
			return -1;
		}
		argv[argc] = strdup(p1);
	}
	argv[++argc] = NULL;
	return argc;
}

/*
 * Run 1 command is l points to a command string, else loop until empty string
 * (unless allowexit is 0
 */
int
runcmdlist(struct command_table *ct, char *subname, char *prompt, int allowexit, char *l)
{
	char *line;
	int argc;
	char *argv[MAXARGS];
	int i;
	int rval;
	struct command_table	*c;

	while (1) {
		char *savedline;
		int match = 0;

		if (!l) {
			puts(prompt);
			fflush(stdout);
		}

		// if line passed in, use it, otherwise read it in
		if (l) {
			line = strdup(l);
			// printf("dup new str is %s\n", line);
			rval = GLSTR;

		} else {
			rval = zgetline(&line, prompt);
			// printf("got %s from zgetline\n", line);
		}

		if (rval == GLESCODE) {
			printf("ESCAPE CODE: %s!!!\n", line);
			free(line);
			continue;
		} else if (rval == GLERR) {
			free(line);
			printf("read error!\n");
			continue;
		} else if (rval == GLEOF) {	// if empty && allow exit return else ignore
			free(line);
			if (allowexit) {
				return 1;
			}
			continue;
		}
		if (strcmp(line, "help")==0) {
			printf("\rAvailable commands:\n");
#ifdef EMUL
			printf("\n");
			fflush(stdout);
#endif
			for (c = ct; c->func; c++) {
				printf("%15s: %s\n", c->name, c->info);
			}
		} else {
			// savedline is freed when rm'd from history..
			if (subname) {
				savedline = malloc(strlen(subname)
								   +strlen(line) + 10);
				sprintf(savedline, "%s %s", subname, line);
			} else {
				savedline = strdup(line);
			}
			savedline[strcspn(savedline, "\r\n")] = 0;	// remove trailing \n

			argc = buildargs(line, argv);
			free(line);
			if (argc < 0) {
				printf("Input error!\n");
			} else if (argc > 0) {
				for (c = ct; c->func; c++) {
					// printf("testing %s against %s\n", argv[0], c->name);
					if (strncmp(argv[0], c->name, 2) == 0) {
						if (rval != GLHIST)
							add2hist(savedline);
						else
							free(savedline);
						// not exactly sure why......
						printf("\r");
						(*c->func)(argc, argv);
						resethist();
						match = 1;
						if (l)
							return 1;
						break;
					}
				}
				if (!match) {
					// not exactly sure why......
					printf("\r");
					printf("Bad command %s\n", argv[0]);
				}
			}
			// argc == 0, just newline entered....

			// Now cleanup
			for ( i = 0; i < argc; i++)
				free(argv[i]);
			if (!match && l)
				return 1;
		}
	}
	
#if 0
	while (ct->func) {
		printf("Command %s\n", ct->name);
		ct++;
	}
#endif
	return 0;
}

void
main()
{
	
	printf("Welcome to smrk100g (%s)\n", KERNEL_VERSION_STRING);

	setup_dev(DEV_SET);

	start_cpu();	// initialize power rails for CPU

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

	runcmdlist(cmd_tab, NULL, PROMPT, 0, NULL);	// 0, 0 means never exit, no args
	/*
	 * Never returns
	 */
	printf("HELPME!!!!!!!!! This shouldn't happen!!!!!\n");
	reboot_cmd(0, NULL);
}
