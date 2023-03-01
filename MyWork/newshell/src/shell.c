#include "shell.h"
#include "console.h"
#include "lib.h"

#define PROMPT "# "
#define MAXARGS	20
#define WHITESPACE " \t\n"

int
help_cmd(int argc, char **argv)
{
	int i;

	for (i = 0; cmd_tab[i].func; i++) {
		printf("%s: %s\n", cmd_tab[i].name, cmd_tab[i].info);
	}
	return 0;
}

struct command_table cmd_tab[] = {
	{"vers", vers_cmd, "Show Software Version."},
	{"reboot", reboot_cmd, "System reboot."},
	{"pm", pm_cmd, "Power Management commands."},
	{"i2c", i2c_cmd, "I2C Commands"},
	{"hist", hist_cmd, "History"},
	{"help", help_cmd, "Help"},
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

void
main()
{
	char *line;
	int argc;
	char *argv[MAXARGS];
	int i;
	int rval;

	printf("Welcome to smrk100g (%s)\n", KERNEL_VERSION_STRING);
	while (1) {
		char *savedline;

		puts(PROMPT);

		rval = zgetline(&line);
		if (rval == GLESCODE) {
			printf("ESCAPE CODE: %s!!!\n", line);
		} else if (rval == GLERR) {
			printf("read error!\n");
		} else {
			savedline = strdup(line); // this is freed when removed from history..
			savedline[strcspn(savedline, "\r\n")] = 0;	// remove trailing \n
			argc = buildargs(line, argv);
			free(line);
			if (argc < 0) {
				printf("Input error!\n");
			} else if (argc > 0) {
				int match = 0;
				for (i = 0; cmd_tab[i].func; i++) {

					if (strcmp(argv[0], cmd_tab[i].name) == 0) {
						add2hist(savedline);
						(*cmd_tab[i].func)(argc, argv);
						match = 1;
					}
				}
				if (!match) {
					printf("Bad command %s\n", argv[0]);
				}
			}
			// argc == 0, just newline entered....

			// Now cleanup
			for ( i = 0; i < argc; i++)
				free(argv[i]);
		}
	}
}
