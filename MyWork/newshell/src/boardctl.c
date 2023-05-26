#include <stdlib.h>
#include <sys/byteorder.h>
#include <drivers/gpio.h>
#include "shell.h"


#include "lib.h"
#include "gpio.h"
#include "pmbus.h"

#include "vrails.h"

char *i2c_dev = 0;
static int gpios_inited = 0;

#define EN_0R85_NODE DT_ALIAS(en_0r85)
static const struct gpio_dt_spec en_0r85 = GPIO_DT_SPEC_GET_OR(EN_0R85_NODE, gpios, {0});
#define PG_0R85_NODE DT_ALIAS(pg_0r85)
static const struct gpio_dt_spec pg_0r85 = GPIO_DT_SPEC_GET_OR(PG_0R85_NODE, gpios, {0});

#define PG_1R8_NODE DT_ALIAS(pg_1r8)
static const struct gpio_dt_spec pg_1r8 = GPIO_DT_SPEC_GET_OR(PG_1R8_NODE, gpios, {0});

#define EN_3R3_NODE	DT_ALIAS(en_3r3)
static const struct gpio_dt_spec en_3r3 = GPIO_DT_SPEC_GET(EN_3R3_NODE, gpios);
#define PG_3R3_NODE DT_ALIAS(pg_3r3)
static const struct gpio_dt_spec pg_3r3 = GPIO_DT_SPEC_GET_OR(PG_3R3_NODE, gpios, {0});


#define EN_1R2_DDR_NODE DT_ALIAS(en_1r2_ddr)
static const struct gpio_dt_spec en_1r2_ddr = GPIO_DT_SPEC_GET_OR(EN_1R2_DDR_NODE, gpios, {0});
#define PG_1R2_DDR_NODE DT_ALIAS(pg_1r2_ddr)
static const struct gpio_dt_spec pg_1r2_ddr = GPIO_DT_SPEC_GET_OR(PG_1R2_DDR_NODE, gpios, {0});

#define EN_0R6_NODE DT_ALIAS(en_0r6)
static const struct gpio_dt_spec en_0r6 = GPIO_DT_SPEC_GET_OR(EN_0R6_NODE, gpios, {0});
#define PG_0R6_NODE DT_ALIAS(pg_0r6)
static const struct gpio_dt_spec pg_0r6 = GPIO_DT_SPEC_GET_OR(PG_0R6_NODE, gpios, {0});


#define PG_2R5_NODE DT_ALIAS(pg_2r5)
static const struct gpio_dt_spec pg_2r5 = GPIO_DT_SPEC_GET_OR(PG_2R5_NODE, gpios, {0});

#define EN_1R2_MGT_NODE DT_ALIAS(en_1r2_mgt)
static const struct gpio_dt_spec en_1r2_mgt = GPIO_DT_SPEC_GET_OR(EN_1R2_MGT_NODE, gpios, {0});
#define PG_1R2_MGT_NODE DT_ALIAS(pg_1r2_mgt)
static const struct gpio_dt_spec pg_1r2_mgt = GPIO_DT_SPEC_GET_OR(PG_1R2_MGT_NODE, gpios, {0});

#define PG_0R9_NODE DT_ALIAS(pg_0r9)
static const struct gpio_dt_spec pg_0r9 = GPIO_DT_SPEC_GET_OR(PG_0R9_NODE, gpios, {0});

#define PG_1R0_NODE DT_ALIAS(pg_1r0)
static const struct gpio_dt_spec pg_1r0 = GPIO_DT_SPEC_GET_OR(PG_1R0_NODE, gpios, {0});

#define ZYNQ_PS_POR_NODE DT_ALIAS(zynq_ps_por)
static const struct gpio_dt_spec zynq_por = GPIO_DT_SPEC_GET(ZYNQ_PS_POR_NODE, gpios);

#define ZYNQ_PS_SRST_NODE DT_ALIAS(zynq_ps_srst)
static const struct gpio_dt_spec zynq_srst = GPIO_DT_SPEC_GET(ZYNQ_PS_SRST_NODE, gpios);

#define ZYNQ_PS_PROG_NODE DT_ALIAS(zynq_ps_prog)
static const struct gpio_dt_spec zynq_prog = GPIO_DT_SPEC_GET(ZYNQ_PS_PROG_NODE, gpios);


#define QSFP_RESETL_NODE DT_ALIAS(qsfp_resetl)
// Remove when used
__attribute__ ((unused))
static const struct gpio_dt_spec qsfp_reset = GPIO_DT_SPEC_GET(QSFP_RESETL_NODE, gpios);

#define QSFP_LPMODE_NODE DT_ALIAS(qsfp_lpmode)
// Remove when used
__attribute__ ((unused))
static const struct gpio_dt_spec qsfp_lowpower_mode = GPIO_DT_SPEC_GET(QSFP_LPMODE_NODE, gpios);

#define ZYNQ_PS_INIT_NODE DT_ALIAS(zynq_ps_init)
static const struct gpio_dt_spec zynq_init = GPIO_DT_SPEC_GET(ZYNQ_PS_INIT_NODE, gpios);

#define ZYNQ_PS_DONE_B_NODE DT_ALIAS(zynq_ps_done_b)
static const struct gpio_dt_spec zynq_done_b = GPIO_DT_SPEC_GET(ZYNQ_PS_DONE_B_NODE, gpios);
/*
 * if func == DEV_REINIT, reinitialize to default
 */
void
setup_dev(int func)
{
	//printk("func = %d, i2c_dev = %d\n", func, (int) i2c_dev);
	if ((func == DEV_REINIT) && i2c_dev) {
		free(i2c_dev);
		i2c_dev = NULL;
	}
	if (!i2c_dev) {
		i2c_dev = strdup(DEF_BUS);
	}

	if (!gpios_inited) {
		gpios_inited = 1;
		//gpio_dump_regs(XGPIOA);

		gpio_pin_configure_dt(&en_0r85, GPIO_OUTPUT);
		gpio_pin_configure_dt(&pg_0r85, GPIO_INPUT);
		vrail[VDD_0R85].enable_node = &en_0r85;
		vrail[VDD_0R85].isgood_node = &pg_0r85;

		gpio_pin_configure_dt(&pg_1r8, GPIO_INPUT);
		vrail[VDD_1R8].isgood_node = &pg_1r8;

		gpio_pin_configure_dt(&en_3r3, GPIO_OUTPUT);
		gpio_pin_configure_dt(&pg_3r3, GPIO_INPUT);
		vrail[VDD_3R3].enable_node = &en_3r3;
		vrail[VDD_3R3].isgood_node = &pg_3r3;
		
		gpio_pin_configure_dt(&en_1r2_ddr, GPIO_OUTPUT);
		gpio_pin_configure_dt(&pg_1r2_ddr, GPIO_INPUT);
		vrail[VDD_1R2_DDR].enable_node = &en_1r2_ddr;
		vrail[VDD_1R2_DDR].isgood_node = &pg_1r2_ddr;

		gpio_pin_configure_dt(&en_0r6, GPIO_OUTPUT);
		gpio_pin_configure_dt(&pg_0r6, GPIO_INPUT);
		vrail[VDD_0R6].enable_node = &en_0r6;
		vrail[VDD_0R6].isgood_node = &pg_0r6;

		gpio_pin_configure_dt(&pg_2r5, GPIO_INPUT);
		vrail[VDD_2R5].isgood_node = &pg_2r5;
		
		gpio_pin_configure_dt(&en_1r2_mgt, GPIO_OUTPUT);	
		gpio_pin_configure_dt(&pg_1r2_mgt, GPIO_INPUT);
		vrail[VDD_1R2_MGT].enable_node = &en_1r2_mgt;
		vrail[VDD_1R2_MGT].isgood_node = &pg_1r2_mgt;

		
		gpio_pin_configure_dt(&pg_0r9, GPIO_INPUT);
		vrail[VDD_0R9].isgood_node = &pg_0r9;

		gpio_pin_configure_dt(&pg_1r0, GPIO_INPUT);
		vrail[VDD_1R0].isgood_node = &pg_1r0;

		gpio_pin_configure_dt(&zynq_srst, GPIO_OUTPUT);
		gpio_pin_configure_dt(&zynq_por, GPIO_OUTPUT);
		gpio_pin_configure_dt(&zynq_prog, GPIO_OUTPUT);
		gpio_pin_configure_dt(&qsfp_lowpower_mode, GPIO_OUTPUT);

		gpio_pin_configure_dt(&zynq_init, GPIO_INPUT);
		gpio_pin_configure_dt(&zynq_done_b, GPIO_INPUT);

//gpio_dump_regs(XGPIOA);
		//gpio_dump_regs(XGPIOB);
		//gpio_dump_regs(XGPIOC);

	}
}

/*
 * Sundar email - 1/10/23
 * deassert zynq_ps_prog (set PC1 to '0'')
 * deassert zynq_ps_srst (set PC3 to '0')
 * assert zynq_ps_por (set PC4 to '1')
 * After power is up.
 * deassert zynq_ps_por (set PC3 to '0')
 */
void
start_cpu()
{
	gpio_pin_set_dt(&zynq_prog, 0);
	gpio_pin_set_dt(&zynq_srst, 0);
	gpio_pin_set_dt(&zynq_por, 1);
	//printk("setting prog=0, stst=0, por=1\n");

	set_vrails(POWER_ON, 0, 0);

	int delay = 11;
	//printk("Delay %d mSec\n", delay);
	k_sleep(K_MSEC(delay));

	gpio_pin_set_dt(&zynq_por, 0);
	//printk("setting por=0\n");

}

void
set_ps_bit(int line, int val)
{
	switch(line) {
	case SET_POR:
		gpio_pin_set_dt(&zynq_por, val);
		break;
	case SET_SRST:
		gpio_pin_set_dt(&zynq_srst, val);
		break;
	case SET_PROG:
		gpio_pin_set_dt(&zynq_prog, val);
		break;
	case SET_ALL:
		gpio_pin_set_dt(&zynq_srst, val);
		gpio_pin_set_dt(&zynq_por, val);
		gpio_pin_set_dt(&zynq_prog, val);
		break;
	default:
		printf("bad line...\n");
	}
}

int get_ps_stat(int line)
{
	switch(line) {
	case GET_DONE_B:
		return gpio_pin_get_dt(&zynq_done_b);
	case GET_INIT:
		return gpio_pin_get_dt(&zynq_init);
	}
	printf("OOPS!!!\n");
	return 0xdeadbeef;
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
io_set(int argc, char **argv)
{
	int i;
	if (argc == 1) {
		printf("\n");
		for (i = 0; pintab[i].func != SET_ALL; i++)
			printf("%s: %d\n", pintab[i].name, pintab[i].cval);
		return 0;
	}

	if (argc != 3) {
		printf("usage set <LINE> <VALUE>\n");
		return -1;
	}
	char *line = toLower(argv[1]);
	int val = atoi(argv[2]);

	if ((val < 0) || (val > 1)) {
		printf("val must be 1 or 0\n");
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

	printf("line must one of: ");
	for (i = 0; pintab[i].name; i++) {
		printf("%s ", pintab[i].name);
	}
	printf("\n");
	return -1;
}

struct pin status[] = {
	{"done_b", 0, GET_DONE_B},
	{"init", 0, GET_INIT},
	{NULL, 0, 0}
};

int
io_get(int argc, char **argv)
{
	int i;

	if (argc == 1) {
		printf("\n");
		for (i = 0; status[i].name; i++)
			printf("%s: %d\n", status[i].name, get_ps_stat(status[i].func));
		return 0;
	}
	if (argc != 2) {
		printf("usage get <LINE>\n");
		return -1;
	}

	char *line = toLower(argv[1]);
	int match = 0;
	for (i = 0; status[i].name; i++) {
		if (strcmp(line, status[i].name) == 0) {
			printf("\n%s: %d\n", status[i].name, get_ps_stat(status[i].func));
			match++;
		}
	}
	if (match)
		return 0;
	printf("line must be one of: " );
	for (i = 0; status[i].name; i++) {
		printf("%s ", status[i].name);
	}
	printf("\n");
	return -1;
}


static struct command_table io_tab[] = {
	{"get", io_get, "read IO pin values"},
	{"set", io_set, "set IO pin values"},
	{0}
};

int
io_cmd(int argc, char **argv)
{
	int i;
	char *p = NULL;

#if 0
	printf("running IO submenu...argc = %d\n", argc);
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
	runcmdlist(io_tab, "io", "IO> ", 1, p);
	if (p)
		free(p);

	//printf("Back...\n");
	return 0;
}
