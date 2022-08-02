#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <sys/byteorder.h>

#include "lib.h"
#define LED_ADDR 0x43

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
	{"d8", 5, 4, 0xcf},
	{"d9", 3, 2, 0xf3},
	{"d10", 1, 0, 0xfc},
	{"d11", 6, 7, 0x3f},
	{0, 0, 0, 0}
};
	
		
static int cmd_led(const struct shell *sh, size_t argc, char **argv)
{
	char s[10];
	char buf[20];

	struct lmap *p;
	char *name = 0;
	int ypin = 0, gpin = 0;
	char gval = 0, tmp;
	unsigned char mask = 0;
	int all = 0;
	char gall = 0, yall = 0, amask = 0;
	
	p = &ledmap[0];
	strcpy(s, toLower(argv[1]));
	for (; p->name != NULL; p++) {
		if (strcmp(p->name, s) == 0 ) {
			name = p->name;
			ypin = p->ypin;
			gpin = p->gpin;
			mask = p->mask;
		}
		gall |= (1 << p->gpin);
		yall |= (1 << p->ypin);
		amask |= p->mask;
	}
	if (!name) {
		if (s[0] == 'a') {
			all = 1;
			name = "all";
		} else {
			printk("Not found: %s\n", s);
			return -ENOENT;
		}
	}
	printk("name: %s, y=%d, g=%d\n", name, ypin, gpin);
	printk("yall=0x%x, gall = 0x%x, mask = 0x%02x\n", gall, yall, amask);
	strcpy(s, toLower(argv[2]));
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
			printk("Bad option: %s\n", s);
			return -ENOENT;
		}
	default:
		printk("Bad option: %s\n", s);
		return -ENOENT;

	}

	if (all)
		mask = amask;

	printk("gval = 0x%02x\n", gval);
	printk("mask = 0x%02x\n", mask);
	/* LED Controller is on I2C Bus 5 */
	set_mux(5);

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
	printk("read 0x%02x\n", buf[0]);

	tmp = buf[0];
	buf[0] = (tmp & !mask) | gval;
	printk("writing 0x%02x\n", buf[0]);
	
	i2c_write_bytes(LED_ADDR, 5, buf, 1);

	return 0;
}

SHELL_CMD_ARG_REGISTER(led, NULL, "Set LEDS", cmd_led, 3, 0);
