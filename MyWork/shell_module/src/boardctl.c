#include <stdlib.h>
#include <sys/byteorder.h>
#include <drivers/gpio.h>


#include "lib.h"
#include "gpio.h"
#include "pmbus.h"

char *i2c_dev = 0;
static int gpios_inited = 0;

#define EN_0R85_NODE DT_ALIAS(en_0r85)
static const struct gpio_dt_spec en_0r85 = GPIO_DT_SPEC_GET_OR(EN_0R85_NODE, gpios, {0});
#define PG_0R85_NODE DT_ALIAS(pg_0r85)
static const struct gpio_dt_spec pg_0r85 = GPIO_DT_SPEC_GET_OR(PG_0R85_NODE, gpios, {0});


#define EN_1R2_DDR_NODE DT_ALIAS(en_1r2_ddr)
static const struct gpio_dt_spec en_1r2_ddr = GPIO_DT_SPEC_GET_OR(EN_1R2_DDR_NODE, gpios, {0});
#define PG_1R2_DDR_NODE DT_ALIAS(pg_1r2_ddr)
static const struct gpio_dt_spec pg_1r2_ddr = GPIO_DT_SPEC_GET_OR(PG_1R2_DDR_NODE, gpios, {0});
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

		gpio_pin_configure_dt(&en_1r2_ddr, GPIO_OUTPUT);
		gpio_pin_configure_dt(&pg_1r2_ddr, GPIO_INPUT);

		
		//gpio_dump_regs(XGPIOA);
		//gpio_dump_regs(XGPIOB);
		//gpio_dump_regs(XGPIOC);

	}
}


#define VDD_EN_3R3_NODE	DT_ALIAS(en_vdd_3r3)
static const struct gpio_dt_spec enable_vdd_33 = GPIO_DT_SPEC_GET(VDD_EN_3R3_NODE, gpios);

/*
 * VDD3r3: PA12
 */
static int
setup_vdd_3r3()
{
	int rval;
	if (!(rval=device_is_ready(enable_vdd_33.port))) {
		printk("Not ready Enable VDD 3R3 (%d)\n", rval);
		return -ENODEV;
	}
#if 1
	if ((rval=gpio_pin_configure_dt(&enable_vdd_33, GPIO_OUTPUT)) < 0) {
		printk("Enable VDD 3R3 config error: %d\n", rval);
		return -EIO;
	}
#endif
	//gpio_dump_regs(XGPIOA);
	return 0;
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

	rval = setup_vdd_3r3();
	if (rval == 0) {
		rval = gpio_pin_set_dt(&enable_vdd_33, 0);
		// see below..
		if (v)
			printk("%s off.\n", v);
	}
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

	rval = setup_vdd_3r3();
	if (rval == 0) {
		//printk("\nEnable 3.3\n");
		//gpio_dump1(XGPIOA,GPIO_ODR); 
		rval = gpio_pin_set_dt(&enable_vdd_33, 1);
		//gpio_dump1(XGPIOA,GPIO_ODR); 
		// printk("rval=%d\n", rval);
		// Yes I know this currently won't happen...
		if (v)
			printk("%s on\n", v);
	}
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
	if (v)
		printk("%s good\n", v);

	printk("en port 0x%x, pin %d\n", en_0r85.port, en_0r85.pin); 
	printk("pg_port 0x%x, pin %d\n", pg_0r85.port, pg_0r85.pin); 
	rval = gpio_pin_get_dt(&pg_0r85);
	gpio_dump1(XGPIOC, GPIO_IDR);
	gpio_dump_regs(XGPIOC);

	return rval;
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
	if (v)
		printk("%s on\n", v);
	return 0;
}
int
vdd_0r6_off(char *v)
{
	if (v)
		printk("%s off\n", v);
	return 0;
}

int
vdd_0r6_isgood(char *v)
{
	if (v)
		printk("%s good\n", v);
	return POWER_GOOD;
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
	if (v)
		printk("%s on\n", v);
	return 0;
}
int
vdd_1r2_mgt_off(char *v)
{
	if (v)
		printk("%s off\n", v);
	return 0;
}

int
vdd_1r2_mgt_isgood(char *v)
{
	if (v)
		printk("%s good\n", v);
	return POWER_GOOD;
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
		
