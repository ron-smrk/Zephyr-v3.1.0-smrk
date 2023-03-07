#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/byteorder.h>
#include "i2c.h"

#include "shell.h"
#include "lib.h"

/*
 * Control individual LEDs (0-7)
 *
 * led LED <on|green|yellow|off>
 * LED: D8, D9, D10, D11
 * on is both LEDS on
 * D8 is closest to board, D11 furthest
 */
struct lmap {
	char *name;
	int  ypin; /* yellow */
	int  gpin; /* green */
	unsigned char mask; /* mask bits */
} ledmap[] = {
	{"d8", 3, 2, 0x0c},
	{"d9", 1, 0, 0x03},
	/* {"d10", 1, 0, 0xfc}, */
	/* {"d11", 6, 7, 0x3f}, */
	{0, 0, 0, 0}
};

/*
 * real work done here...
 */
static int
led_work(int argc, char **argv)
{
	char s[10];
	char buf[20];

	struct lmap *p;
	char *name = 0;
	int ypin = 0, gpin = 0;
	char gval = 0, tmp;
	unsigned char mask = 0;
	int all = 0;
	char gall = 0, yall = 0, allmask = 0;
	int val;

	if (argc < 2) {
		printf("usage: led <NAME> <STATE>\n");
		printf("NAME: [d8|d9|all]  (Case insensitive)\n");
		printf("STATE: [G|Y|on|off\n");
		return 0;
	}
	p = &ledmap[0];
	strcpy(s, toLower(argv[0]));
	for (; p->name != NULL; p++) {
		if (strcmp(p->name, s) == 0 ) {
			name = p->name;
			ypin = p->ypin;
			gpin = p->gpin;
			mask = p->mask;
		}
		gall |= (1 << p->gpin);
		yall |= (1 << p->ypin);
		allmask |= p->mask;
	}
	if (!name) {
		if (s[0] == 'a') {
			all = 1;
			name = "all";
		} else {
			printf("Not found: %s\n", s);
			return -ENOENT;
		}
	}
#if 0
	printf("name: %s, ypin=%d, gpin=%d, mask=0x%x\n", name, ypin, gpin, mask);
	printf("yall=0x%x, gall = 0x%x, allmask = 0x%02x\n", yall, gall, allmask);
#endif
	strcpy(s, toLower(argv[1]));
	switch (s[0]) {
	case 'g':
		if (all)
			gval = gall;
		else
			gval = 1 << gpin;
		break;
	case 'y':
		if (all)
			gval = yall;
		else
			gval = 1 << ypin;
		break;
	case 'o':
		if (s[1] == 'n') {	/* on */
			if (all)
				gval |= (gall|yall);
			else
				gval = (1 << gpin) | (1 << ypin);
			break;
		} else if (s[1] == 'f') {
			gval = 0;
			break;
		} else {
			printf("Bad option: %s\n", s);
			return -ENOENT;
		}
	default:
		printf("Bad option: %s\n", s);
		return -ENOENT;

	}

	if (all)
		mask = allmask;

#if 0
	printf("gval = 0x%02x\n", gval);
	printf("mask = 0x%02x\n", mask);
#endif
	
	/* LED Controller is on I2C Bus 5 */
	set_mux(LED_BUS);

	/* Set all pins to output */	
	buf[0] = 0xff;
	i2c_write_bytes(LED_ADDR, 3, buf, 1);
	/* High impedence off */
	buf[0] = 0;
	i2c_write_bytes(LED_ADDR, 7, buf, 1);
	/* disbale PU/PD */
	buf[0] = 0;
	i2c_write_bytes(LED_ADDR, 0xb, buf, 1);
	/* write data */

	i2c_read_bytes(LED_ADDR, 5, buf, 1);
#if 0
	printf("read 0x%02x\n", buf[0]);
#endif

	tmp = buf[0];
	val = tmp & ~mask;
	buf[0] = (tmp & ~mask) | gval;
#if 0
	printf("val=0x%x\n", val);
	printf("tmp=0x%x, mask=0x%x, ~mask=0x%x\n", tmp, mask, ~mask);
	printf("writing 0x%02x\n", buf[0]);
#endif
	
	i2c_write_bytes(LED_ADDR, 5, buf, 1);
	printf("\n");
	return 0;
}

static struct command_table led_tab[] = {
	{"d8", led_work, "LED D8 control"},
	{"d9", led_work, "LED D9 control"},
	{"all", led_work, "Control all LEDs"},
	{0}
};

int
led_cmd(int argc, char **argv)
{
	int i;
	char *p = NULL;

#if 0
	printf("run LED submenu...argc = %d\n", argc);
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
	runcmdlist(led_tab, "LED> ", 1, p);
	if (p)
		free(p);

	//printf("Back...\n");
	return 0;
}
