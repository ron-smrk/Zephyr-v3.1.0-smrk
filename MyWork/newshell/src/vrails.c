#include "vrails.h"
#include "i2c.h"
#include "irps5401.h"
#include "pmbus_cmds.h"

struct VoltRails vrail[] = {
	[VDD_0R85] = {NULL, NULL, "0R85 (A)", 0.85,
		GPIO_EN|GPIO_PG|GPIO_RD|ISIRPS_CHIP|LOOPA, vrail_on, vrail_off,
		vrail_wait_pg, vrail_pg, pmbus_get_volt, irps_reg},
	[VDD_1R8] = {NULL, NULL, "1R8 (D)", 1.8,
		PMBUS_EN|GPIO_PG|PMBUS_RD|LOOPD|ISIRPS_CHIP, vrail_on, vrail_off,
		vrail_wait_pg, vrail_pg, pmbus_get_volt, irps_reg},
	// VDD_3R3 has no power good signal yet! 
	[VDD_3R3] = {NULL, NULL, "3R3", 3.3, GPIO_EN|GPIO_RD, vrail_on, vrail_off,
		vrail_wait_pg, vrail_pg, pmbus_get_volt, pmbus_regnull},
	[VDD_1R2_DDR] = {NULL, NULL, "1R2_DDR/MGT", 1.2,
		GPIO_EN|GPIO_PG|PMBUS_RD|ISMAX_CHIP, vrail_on, vrail_off,
		vrail_wait_pg, vrail_pg, pmbus_get_volt, pmbus_regnull},
	[VDD_0R6] = {NULL, NULL, "0R6", 0.6, GPIO_EN|GPIO_PG|GPIO_RD, vrail_on,
		vrail_off, vrail_wait_pg, vrail_pg, pmbus_get_volt, pmbus_regnull},
	[VDD_2R5] = {NULL, NULL, "2R5 (B)", 2.5,
		PMBUS_EN|GPIO_PG|PMBUS_RD|LOOPB|ISIRPS_CHIP, vrail_on, vrail_off,
		vrail_wait_pg, vrail_pg, pmbus_get_volt, irps_reg},
	[VDD_0R9] = {NULL, NULL, "0R9 (C)", 0.9,
		PMBUS_EN|GPIO_PG|PMBUS_RD|LOOPC|ISIRPS_CHIP, vrail_on, vrail_off,
		vrail_wait_pg, vrail_pg, pmbus_get_volt, irps_reg},
	[VDD_1R0] = {NULL, NULL, "1R0 (LDO)", 1.0,
		PMBUS_EN|GPIO_PG|PMBUS_RD|LOOPLDO|ISIRPS_CHIP|DIVBY2, vrail_on,
		vrail_off, vrail_wait_pg, vrail_pg, pmbus_get_volt, irps_reg}
};

int
vrail_on(int rail)
{
	int bus = get_bus(rail);

	//printk("vrail_on rail=%d type=0x%08x, bus=%d\n", rail, vrail[rail].type, bus);
	if (vrail[rail].type & PMBUS_EN) {
		irps_setop(bus, vrail[rail].type & LOOP_MASK, VOLT_ON);
	} else if  (vrail[rail].type & GPIO_EN) {
		gpio_pin_set_dt(vrail[rail].enable_node, 1);
	} else {
		printk("unknown on/off!!\n");
		return POWER_BAD;
	}
	return vrail[rail].waitpg(rail);
}

int
vrail_off(int rail)
{
	int bus = get_bus(rail);

	if (vrail[rail].type & PMBUS_EN) {
		irps_setop(bus, vrail[rail].type & LOOP_MASK, VOLT_OFF);
	} else if  (vrail[rail].type & GPIO_EN) {
		gpio_pin_set_dt(vrail[rail].enable_node, 0);
	} else {
		printk("unknown on/off!!\n");
	}
	return 0;
}

/*
 * return POWER_GOOD (1) in PG, else POWER_BAD (0)
 */
int
vrail_pg(int rail)
{
	// only for 3r3
	if (vrail[rail].isgood_node == NULL) {
		//printk("PG good unset! (rail=%d)\n", rail);
		return POWER_GOOD;
	}
	if ((vrail[rail].type & GPIO_PG) != GPIO_PG) {
		printk("PG not set rail=%d!!!!!!\n", rail);
		return POWER_BAD;
	}
	if (gpio_pin_get_dt(vrail[rail].isgood_node) == 1)
		return POWER_GOOD;

	return POWER_BAD;
}

int
vrail_wait_pg(int rail)
{
	int delay = 50;

	while (delay--) {
		if (vrail_pg(rail) == POWER_GOOD)
			return POWER_GOOD;
		k_sleep(K_MSEC(10));
	}
	return POWER_BAD;
}

int
vrail_setvolt(int rail, double v)
{
	if (vrail[rail].type & PMBUS_RD) {
		return pmbus_set_vout(rail, v);
	}
	// Couldn't set
	return -1;
}

int get_bus(int rail)
{
	int typ = vrail[rail].type;

	if (typ & ISIRPS_CHIP)
		return IRPS_BUS;
	else if (typ & ISMAX_CHIP)
		return MAX_BUS;
	else {
		return -1;
	}
}


int
set_vrails(int sense, int sleep, int wait)
{
	printk("\nsetting vrails %s\n", (sense==POWER_ON)?"on":"off");
	int i;
	if (sense == POWER_ON) {
		i = 0;
		while(i < NUM_RAILS) {
			vrail_on(i);
		    vrail_wait_pg(i);	// wait for PG....
			if (sleep > 0)
				k_sleep(K_MSEC(1000*sleep));
			else if (wait) {
				//printk("press key to continue: ");
				//c=console_getchar();
				//printk("BACK\n");
			}
			i++;
		}
		// ON, set POR=0 after turning on..
		set_ps_bit(SET_POR, 0);
	} else {
		// point to last one
		i = NUM_RAILS-1;
		// OFF, set POR=1 before turning off..
		set_ps_bit(SET_POR, 1);
		while (i >= 0) {
			vrail_off(i);
			if (sleep > 0)
				k_sleep(K_MSEC(1000*sleep));			else if (wait) {
				//printk("press key to continue: ");
				//c=console_getchar();
				//printk("BACK\n");
			}
			i--;
		}
	}
	return 0;
}

