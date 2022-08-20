#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/shell/shell.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>

#include "lib.h"
#include "gpio.h"


struct gpio {
	int rnum;
	char *desc;
} ginfo[] = { {GPIO_MODER, "MODER"},
	  {GPIO_OTYPER, "OTYPE"},
	  {GPIO_SPEEDR, "SPEEDR"},
	  {GPIO_PUPDR, "PUPDR"},
	  {GPIO_IDR, "IDR"},
	  {GPIO_ODR, "ODR"},
	  {GPIO_BSRR, "BSRR"},
	  {GPIO_LOCKR, "LOCKR"},
	  {GPIO_AFRL, "AFRL"},
	  {GPIO_AFRH, "AFRH"}
};

int gpiobase[] = { 0x40020000, 0x40020400, 0x40020800 };
char *gpioname[] = {"GPIOA", "GPIOB", "GPIOC"};

void gpio_dump1(int port, int reg)
{
	unsigned int *p;
	
	p = (unsigned int *)gpiobase[port];
	
	printk("%-8s 0x%08x\n", ginfo[reg].desc, p[reg]);

}

/*
 * yes this needs a mask, clr then set...
 */
int gpio_set1(int port, int reg, int val)
{
	unsigned int *p;
	
	p = (unsigned int *)gpiobase[port];
	p[reg] |= val;
	return 0;
}

int gpio_dump_regs(int port)
{
	int i;
	
	if ((port < 0) || (port > XGPIO_LAST)) {
		printk("Bad port %d\n", port);
		return -1;
	}
	printk("%s:\n", gpioname[port]);
	for (i = 0; i < GPIO_LAST_REG; i++)
		gpio_dump1(port, i);
	return 0;
}


