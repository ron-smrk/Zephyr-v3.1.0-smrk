#include <stdlib.h>
#include <sys/byteorder.h>
#include <drivers/gpio.h>


#include "lib.h"
#include "gpio.h"

char *i2c_dev = 0;

#define VDD_EN_3R3_NODE	DT_ALIAS(en_vdd_3r3)
static const struct gpio_dt_spec enable_vdd_33 = GPIO_DT_SPEC_GET(VDD_EN_3R3_NODE, gpios);

static int
setup_vdd_3r3()
{
	int rval;
	if (!(rval=device_is_ready(enable_vdd_33.port))) {
		printk("Not ready Enable VDD 3R3 (%d)\n", rval);
		return -ENODEV;
	}
#if 1
/*	if ((rval=gpio_pin_configure_dt(&enable_vdd_33, GPIO_OUTPUT_ACTIVE)) < 0) {*/
	if ((rval=gpio_pin_configure_dt(&enable_vdd_33, GPIO_OUTPUT)) < 0) {
		printk("Enable VDD 3R3 config error: %d\n", rval);
		return -EIO;
	}
#endif
	//gpio_dump_regs(XGPIOA);
	return 0;
}

int
disable_vdd_3r3()
{
	int rval = setup_vdd_3r3();

	if (rval == 0)
		rval = gpio_pin_set_dt(&enable_vdd_33, 0);
	return rval;
}

int
enable_vdd_3r3()
{
	int rval = setup_vdd_3r3();

	if (rval == 0) {
		printk("\nEnable 3.3\n");
		//gpio_dump1(XGPIOA,GPIO_ODR); 
		rval = gpio_pin_set_dt(&enable_vdd_33, 1);
		//gpio_dump1(XGPIOA,GPIO_ODR); 
		// printk("rval=%d\n", rval);
	}
	return rval;
}

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
		
}

