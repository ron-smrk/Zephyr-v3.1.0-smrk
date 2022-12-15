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
	int type;		// gpio pin or pmbus command
	int	(*on)(int);		// Enable/Turn on
	int (*off)(int);		// Disable/Turn off
	int	(*isgood)(int);	// Is Voltage good
	int (*rdvolt)(int, struct power_vals *);	// readback Voltage
};

extern struct VoltRails vrail[];
extern int vrail_on(int);
extern int vrail_off(int);
extern int vrail_isgood(int);
extern int vrail_rdvolt(int, struct power_vals *);
extern int get_bus(int);

