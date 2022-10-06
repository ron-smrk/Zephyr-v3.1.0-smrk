#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <sys/byteorder.h>
#include "my2c.h"

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
	char gall = 0, yall = 0, allmask = 0;
	int val;
	
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
		allmask |= p->mask;
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
#if 0
	printk("name: %s, ypin=%d, gpin=%d, mask=0x%x\n", name, ypin, gpin, mask);
	printk("yall=0x%x, gall = 0x%x, allmask = 0x%02x\n", yall, gall, allmask);
#endif
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
		mask = allmask;

#if 0
	printk("gval = 0x%02x\n", gval);
	printk("mask = 0x%02x\n", mask);
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
	printk("read 0x%02x\n", buf[0]);
#endif

	tmp = buf[0];

	val = tmp & ~mask;
#if 0
	printk("val=0x%x\n", val);
	printk("tmp=0x%x, mask=0x%x, ~mask=0x%x\n", tmp, mask, ~mask);
#endif		
	   
	buf[0] = (tmp & ~mask) | gval;
	printk("writing 0x%02x\n", buf[0]);
	
	i2c_write_bytes(LED_ADDR, 5, buf, 1);

	return 0;
}

SHELL_CMD_ARG_REGISTER(led, NULL, "usage: led [d8|d9|all] [on|off|green|yellow]", cmd_led, 3, 0);
