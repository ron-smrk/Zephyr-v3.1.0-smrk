#ifndef EMUL
#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/console/console.h>
//#define fflush(x)	;
#endif


extern int getch();
extern void putch(int);
extern int puts(const char *);

#define ESC		0x1b
#define ESC1	0x5b	// '['
// 3 char codes 'esc[X'
#define UPARROW	0x41	// 'A'
#define DNARROW 0x42    // 'B'
#define RARROW	0x43	// 'C'
#define LARROW	0x44	// 'D'
#define END		0x46	// 'F'
#define HOME	0x48	// 'H'

// 4 char codes 'esc[C~'
#define INS		0x32	// '2'
#define DEL		0x33	// '3'
#define PGUP	0x35	// '5'
#define PGDN	0x36	// '6'

#define	END4	0x7e	// '~'

#define BS		0x7f
#define CTLH	0x8

// zGLine return codes...
#define GLSTR		0x1
#define GLESCODE	0x2
#define GLEOF		0x3	// empty line...
#define GLERR		-1

