/*
 * GPIO Defs
 */
#define XGPIOA 0
#define XGPIOB 1
#define XGPIOC 2
#define XGPIOD 3
#define XGPIOE 4
#define XGPIO_LAST 2

#define GPIO_MODER	0
#define GPIO_OTYPER	1
#define GPIO_SPEEDR	2
#define GPIO_PUPDR  3
#define GPIO_IDR	4
#define GPIO_ODR	5
#define GPIO_BSRR	6
#define GPIO_LOCKR	7
#define GPIO_AFRL	8
#define GPIO_AFRH	9
#define GPIO_LAST_REG 10

extern void gpio_dump1(int, int);
extern int gpio_set1(int, int, int);
extern int gpio_dump_regs(int);
