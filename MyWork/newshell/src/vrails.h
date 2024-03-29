#include <stdlib.h>
#include <ctype.h>
#include <sys/byteorder.h>
#include <drivers/gpio.h>

#include "lib.h"
#include "gpio.h"
#include "pmbus.h"


/*
 * gpio pins/loops
 * PMBUS_* use pmbus to enable power good, rd voltage
 * GPIO_* use gpio pins to enable power good, rd voltage (if power good, return nominal V)
 */
#define PMBUS_EN	(1<<31)
#define PMBUS_PG	(1<<30)
#define PMBUS_RD	(1<<29)
#define GPIO_EN		(1<<28)
#define GPIO_PG		(1<<27)
#define GPIO_RD		(1<<26)
#define ISIRPS_CHIP	(1<<25)
#define ISMAX_CHIP	(1<<24)
#define DIVBY2		(1<<23)

#define LOOP_MASK	0xf

enum { VDD_0R85=0, VDD_1R8, VDD_3R3, VDD_1R2_DDR, VDD_0R6, VDD_2R5,
	VDD_1R2_MGT, VDD_0R9, VDD_1R0,
	NUM_RAILS };

struct VoltRails {
	const struct gpio_dt_spec	*enable_node;
	const struct gpio_dt_spec	*isgood_node;
	char *signame;
	double nominal;
	int type;			// gpio pin or pmbus command
	int busnum;			// i2c busnum.
	int	(*setpwr)(int, int);		// Enable/Turn on (rail, On/Off)
	int	(*waitpg)(int);	// Wait for Voltage good
	int (*pg)(int);		// return POWER_GOOD or POWER_BAD
	int (*rdvolt)(int, struct power_vals *);	// readback Voltage
	int (*reg)(int, struct pmbus_op *);		// set/display registers
};

extern struct VoltRails vrail[];
extern int pmbus_vrail_pwr(int, int);
extern int gpio_vrail_pwr(int, int);
extern int vrail_wait_pg(int);
extern int vrail_pg(int);
extern int vrail_setvolt(int, double);
extern int get_bus(int);
extern int set_vrails(int, int, int);
