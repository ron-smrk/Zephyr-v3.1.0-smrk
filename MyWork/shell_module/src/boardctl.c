#include <stdlib.h>
#include <sys/byteorder.h>
#include <drivers/gpio.h>


#include "lib.h"
#include "gpio.h"
#include "pmbus.h"

char *i2c_dev = 0;
static int gpios_inited = 0;

#define EN_3R3_NODE	DT_ALIAS(en_3r3)
static const struct gpio_dt_spec en_3r3 = GPIO_DT_SPEC_GET(EN_3R3_NODE, gpios);

#define EN_0R85_NODE DT_ALIAS(en_0r85)
static const struct gpio_dt_spec en_0r85 = GPIO_DT_SPEC_GET_OR(EN_0R85_NODE, gpios, {0});
#define PG_0R85_NODE DT_ALIAS(pg_0r85)
static const struct gpio_dt_spec pg_0r85 = GPIO_DT_SPEC_GET_OR(PG_0R85_NODE, gpios, {0});

#define EN_1R2_DDR_NODE DT_ALIAS(en_1r2_ddr)
static const struct gpio_dt_spec en_1r2_ddr = GPIO_DT_SPEC_GET_OR(EN_1R2_DDR_NODE, gpios, {0});
#define PG_1R2_DDR_NODE DT_ALIAS(pg_1r2_ddr)
static const struct gpio_dt_spec pg_1r2_ddr = GPIO_DT_SPEC_GET_OR(PG_1R2_DDR_NODE, gpios, {0});

#define EN_0R6_NODE DT_ALIAS(en_0r6)
static const struct gpio_dt_spec en_0r6 = GPIO_DT_SPEC_GET_OR(EN_0R6_NODE, gpios, {0});
#define PG_0R6_NODE DT_ALIAS(pg_0r6)
static const struct gpio_dt_spec pg_0r6 = GPIO_DT_SPEC_GET_OR(PG_0R6_NODE, gpios, {0});

#define EN_1R2_MGT_NODE DT_ALIAS(en_1r2_mgt)
static const struct gpio_dt_spec en_1r2_mgt = GPIO_DT_SPEC_GET_OR(EN_1R2_MGT_NODE, gpios, {0});
#define PG_1R2_MGT_NODE DT_ALIAS(pg_1r2_mgt)
static const struct gpio_dt_spec pg_1r2_mgt = GPIO_DT_SPEC_GET_OR(PG_1R2_MGT_NODE, gpios, {0});

#define PG_1R8_NODE DT_ALIAS(pg_1r8)
static const struct gpio_dt_spec pg_1r8 = GPIO_DT_SPEC_GET_OR(PG_1R8_NODE, gpios, {0});

#define PG_2R5_NODE DT_ALIAS(pg_2r5)
static const struct gpio_dt_spec pg_2r5 = GPIO_DT_SPEC_GET_OR(PG_2R5_NODE, gpios, {0});

#define PG_0R9_NODE DT_ALIAS(pg_0r9)
static const struct gpio_dt_spec pg_0r9 = GPIO_DT_SPEC_GET_OR(PG_0R9_NODE, gpios, {0});

#define PG_1R0_NODE DT_ALIAS(pg_1r0)
static const struct gpio_dt_spec pg_1r0 = GPIO_DT_SPEC_GET_OR(PG_1R0_NODE, gpios, {0});
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
		gpio_pin_configure_dt(&en_3r3, GPIO_OUTPUT);

		gpio_pin_configure_dt(&en_0r85, GPIO_OUTPUT);
		gpio_pin_configure_dt(&pg_0r85, GPIO_INPUT);

		gpio_pin_configure_dt(&en_1r2_ddr, GPIO_OUTPUT);
		gpio_pin_configure_dt(&pg_1r2_ddr, GPIO_INPUT);

		gpio_pin_configure_dt(&en_0r6, GPIO_OUTPUT);
		gpio_pin_configure_dt(&pg_0r6, GPIO_INPUT);
		
		gpio_pin_configure_dt(&en_1r2_mgt, GPIO_OUTPUT);	
		gpio_pin_configure_dt(&pg_1r2_mgt, GPIO_INPUT);

		gpio_pin_configure_dt(&pg_1r8, GPIO_INPUT);
		gpio_pin_configure_dt(&pg_2r5, GPIO_INPUT);
		gpio_pin_configure_dt(&pg_0r9, GPIO_INPUT);
		gpio_pin_configure_dt(&pg_1r0, GPIO_INPUT);

//gpio_dump_regs(XGPIOA);
		//gpio_dump_regs(XGPIOB);
		//gpio_dump_regs(XGPIOC);

	}
}


/*
 * VDD_3r3: EN: PA12, PG: PC11
 */
int
vdd_3r3_off(char *v)
{
	int rval;
	// Testing only....
	if (v) {
		printk("%s off.\n", v);
		return 0;
	}
	if (v)
		printk("%s on\n", v);
	rval = gpio_pin_set_dt(&en_3r3, 0);

	return rval;

}

int
vdd_3r3_on(char *v)
{
	int rval;
	// This part for testing only for now!!!
	if (v) {
		printk("%s on\n", v);
			return 0;
	}
	if (v)
		printk("%s on\n", v);
	rval = gpio_pin_set_dt(&en_3r3, 1);

	return rval;
}

int
vdd_3r3_isgood(char *v)
{
	if (v)
		printk("%s good\n", v);
	return POWER_GOOD;
}

double
vdd_3r3_rdvolt(char *v)
{
	if (v)
		printk("%s rdvolt\n", v);
	if (vdd_3r3_isgood(NULL))
		return 3.3;
	else
		return 0.0;
}
		
	
/*
 * VDD_0R85: EN: PA15, PG: PC11
 */
int
vdd_0r85_on(char *v)
{
	// I thinks these need to be pmbus writes...
	int rval;
	if (v)
		printk("%s on\n", v);
	rval = gpio_pin_set_dt(&en_0r85, 1);

	return rval;
}
int
vdd_0r85_off(char *v)
{
	// I thinks these need to be pmbus writes...
	int rval;
	if (v)
		printk("%s off\n", v);
	rval = gpio_pin_set_dt(&en_0r85, 0);
	return rval;
}

int
vdd_0r85_isgood(char *v)
{

	int rval;
	int delay = 50;
	if (v)
		printk("%s good\n", v);
	
	while (delay--) {
		k_sleep(K_MSEC(10));
		rval = gpio_pin_get_dt(&pg_0r85);
		if (rval == 1)
			return POWER_GOOD;
	}

	return POWER_BAD;
}


double
vdd_0r85_rdvolt(char *v)
{
	// pmbus read..
	if (v)
		printk("%s rdvolt\n", v);
	if (vdd_0r85_isgood(NULL))
		return 0.85;
	else
		return 0.0;
}
		
/*
 * VDD_1R2_DDR: PA9, PG: PB15, can also read MAX chip for voltage.
 */
int
vdd_1r2_ddr_on(char *v)
{
	int rval;
	
	if (v)
		printk("%s on\n", v);

	rval = gpio_pin_set_dt(&en_1r2_ddr, 1);
	return rval;
}
int
vdd_1r2_ddr_off(char *v)
{
	int rval;

	if (v)
		printk("%s off\n", v);
	rval = gpio_pin_set_dt(&en_1r2_ddr, 0);
	return rval;
}

int
vdd_1r2_ddr_isgood(char *v)
{
	int rval;
	int delay = 50;
	if (v)
		printk("%s good\n", v);
	
	while (delay--) {
		k_sleep(K_MSEC(10));
		rval = gpio_pin_get_dt(&pg_1r2_ddr);
		//gpio_dump1(XGPIOB, GPIO_IDR);
		if (rval == 1)
			return POWER_GOOD;
	}

	return POWER_BAD;
}

double
vdd_1r2_ddr_rdvolt(char *v)
{
	if (v)
		printk("%s rdvolt\n", v);
	if (vdd_1r2_ddr_isgood(NULL))
		return 1.2;
	else
		return 0.0;
}
		

/*
 * VDD_0R6: En: PA10, PG: PC10
 */
int
vdd_0r6_on(char *v)
{
	int rval;
	
	if (v)
		printk("%s on\n", v);

	rval = gpio_pin_set_dt(&en_0r6, 1);
	return rval;
}
int
vdd_0r6_off(char *v)
{
	int rval;
	
	if (v)
		printk("%s ofd\n", v);

	rval = gpio_pin_set_dt(&en_0r6, 0);
	return rval;
}

int
vdd_0r6_isgood(char *v)
{
	int rval;
	int delay = 50;
	if (v)
		printk("%s good\n", v);
	
	while (delay--) {
		k_sleep(K_MSEC(10));
		rval = gpio_pin_get_dt(&pg_0r6);
		//gpio_dump1(XGPIOB, GPIO_IDR);
		if (rval == 1)
			return POWER_GOOD;
	}

	return POWER_BAD;
}

double
vdd_0r6_rdvolt(char *v)
{
	if (v)
		printk("%s rdvolt\n", v);
	if (vdd_0r6_isgood(NULL))
		return 0.6;
	else
		return 0.0;
}
		

/*
 * VDD_1R2_MGT: PA11, PG: PB14, can also read MAX chip for voltage.
 * but, its a copy of 1r2_ddr
 */
int
vdd_1r2_mgt_on(char *v)
{
	int rval;
	
	if (v)
		printk("%s on\n", v);

	rval = gpio_pin_set_dt(&en_1r2_mgt, 1);
	return rval;
}
int
vdd_1r2_mgt_off(char *v)
{
	int rval;

	if (v)
		printk("%s off\n", v);
	rval = gpio_pin_set_dt(&en_1r2_mgt, 0);
	return rval;
}

int
vdd_1r2_mgt_isgood(char *v)
{
	int rval;
	int delay = 50;
	if (v)
		printk("%s good\n", v);
	
	while (delay--) {
		k_sleep(K_MSEC(10));
		rval = gpio_pin_get_dt(&pg_1r2_mgt);
		//gpio_dump1(XGPIOB, GPIO_IDR);
		if (rval == 1)
			return POWER_GOOD;
	}

	return POWER_BAD;
}

double
vdd_1r2_mgt_rdvolt(char *v)
{
	if (v)
		printk("%s rdvolt\n", v);
	if (vdd_1r2_mgt_isgood(NULL))
		return 1.2;
	else
		return 0.0;
}
		
int
vdd_1r8_isgood(char *v)
{
	int rval;
	int delay = 50;
	if (v)
		printk("%s good\n", v);
	
	while (delay--) {
		k_sleep(K_MSEC(10));
		rval = gpio_pin_get_dt(&pg_1r8);
		if (rval == 1)
			return POWER_GOOD;
	}

	return POWER_BAD;
}


int
vdd_2r5_isgood(char *v)
{
	int rval;
	int delay = 50;
	if (v)
		printk("%s good\n", v);
	
	while (delay--) {
		k_sleep(K_MSEC(10));
		rval = gpio_pin_get_dt(&pg_2r5);
		if (rval == 1)
			return POWER_GOOD;
	}

	return POWER_BAD;
}


int
vdd_0r9_isgood(char *v)
{
	int rval;
	int delay = 50;
	if (v)
		printk("%s good\n", v);

	while (delay--) {
		k_sleep(K_MSEC(10));
		rval = gpio_pin_get_dt(&pg_0r9);
		if (rval == 1)
			return POWER_GOOD;
	}

	return POWER_BAD;
}


int
vdd_1r0_isgood(char *v)
{
	int rval;
	int delay = 50;
	if (v)
		printk("%s good\n", v);

	while (delay--) {
		k_sleep(K_MSEC(10));
		rval = gpio_pin_get_dt(&pg_1r0);
		if (rval == 1)
			return POWER_GOOD;
	}

	return POWER_BAD;
}


