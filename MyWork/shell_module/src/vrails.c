#include "vrails.h"
#include "my2c.h"
#include "irps5401.h"
#include "pmbus_cmds.h"

struct VoltRails vrail[] = {
	[VDD_0R85] = {NULL, NULL, "0R85 (A)", 0.85, GPIO_EN|GPIO_PG|GPIO_RD|ISIRPS_CHIP|LOOPA, 
		vrail_on, vrail_off, vrail_isgood, vrail_rdvolt},
	[VDD_1R8] = {NULL, NULL, "1R8 (D)", 1.8, PMBUS_EN|GPIO_PG|PMBUS_RD|LOOPD|ISIRPS_CHIP,
		vrail_on, vrail_off, vrail_isgood, vrail_rdvolt},
	// VDD_3R3 has no power good signal yet! 
	[VDD_3R3] = {NULL, NULL, "3R3", 3.3, GPIO_EN|GPIO_RD,
		vrail_on, vrail_off, vrail_isgood, vrail_rdvolt},
	[VDD_1R2_DDR] = {NULL, NULL, "1R2_DDR", 1.2, GPIO_EN|GPIO_PG|PMBUS_RD|ISMAX_CHIP,
		vrail_on, vrail_off, vrail_isgood, vrail_rdvolt},
	[VDD_0R6] = {NULL, NULL, "0R6", 0.6, GPIO_EN|GPIO_PG|GPIO_RD,
		vrail_on, vrail_off, vrail_isgood, vrail_rdvolt},
	[VDD_2R5] = {NULL, NULL, "2r5 (B)", 2.5, PMBUS_EN|GPIO_PG|PMBUS_RD|LOOPB|ISIRPS_CHIP,
		vrail_on, vrail_off, vrail_isgood, vrail_rdvolt},
	[VDD_1R2_MGT] = {NULL, NULL, "1R2_MGT", 1.2, GPIO_EN|GPIO_PG|PMBUS_RD|ISMAX_CHIP,
		vrail_on, vrail_off, vrail_isgood, vrail_rdvolt},
	[VDD_0R9] = {NULL, NULL, "0R9 (C)", 0.9, PMBUS_EN|GPIO_PG|PMBUS_RD|LOOPC|ISIRPS_CHIP,
		vrail_on, vrail_off, vrail_isgood, vrail_rdvolt},
	[VDD_1R0] = {NULL, NULL, "1R0 (LDO)", 1.0,
		PMBUS_EN|GPIO_PG|PMBUS_RD|LOOPLDO|ISIRPS_CHIP|DIVBY2,
		vrail_on, vrail_off, vrail_isgood, vrail_rdvolt}
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
	}
	return 0;
}

int
vrail_off(int rail)
{
	int bus = get_bus(rail);

	if (vrail[rail].type & PMBUS_EN) {
		irps_setop(bus, vrail[rail].type & LOOP_MASK, VOLT_OFF);
	} else if  (vrail[rail].type & GPIO_EN) {
		gpio_pin_set_dt(vrail[rail].enable_node, 0);
		vrail[rail].isgood(rail);
	} else {
		printk("unknown on/off!!\n");
	}
	return 0;
}

int
vrail_isgood(int rail)
{
	int rval;
	int delay = 50;

	// only for 3r3
	if (vrail[rail].isgood_node == NULL) {
		//printk("PG good unset! (rail=%d)\n", rail);
		return POWER_GOOD;
	}
	if ((vrail[rail].type & GPIO_PG) != GPIO_PG) {
		printk("PG not set rail=%d!!!!!!\n", rail);
		return -1;
	}
	while (delay--) {
		k_sleep(K_MSEC(10));
		rval = gpio_pin_get_dt(vrail[rail].isgood_node);
		if (rval == 1)
			return POWER_GOOD;
	}

	return POWER_BAD;
}

double
vrail_rdvolt(int rail)
{
	//printk("Rail = %d, type = 0x%08x\n", rail, vrail[rail].type);

	if (vrail[rail].type & PMBUS_RD) {
		return pmbus_get_vout(rail);
	}
	if (vrail[rail].isgood(rail))
		return vrail[rail].nominal;
	else
		return 0.0;
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

