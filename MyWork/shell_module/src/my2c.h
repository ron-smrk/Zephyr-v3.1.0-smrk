#define QSFP_ADDR	0xFF	// Later...
#define EEP_ADDR	0xff	// also...
#define IRPS_ADDR	0x40
#define MAX_ADDR	0x54
#define EEPROM_ADDR	0x54
#define LED_ADDR	0x43

#define QSFP_BUS	0
#define EEP_BUS		1
#define IRPS_BUS	2
#define MAX_BUS		3
#define EEPROM_BUS	4
#define LED_BUS		5
#define LAST_BUS	LED_BUS

extern int get_i2c_addr(int);
