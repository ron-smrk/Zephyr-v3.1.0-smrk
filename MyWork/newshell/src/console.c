#include "shell.h"
#include "console.h"

#ifdef EMUL
#include <termios.h>

static struct termios old, new1;

void
initterm(int echo) {
    tcgetattr(0, &old); /* grab old terminal i/o settings */
    new1 = old; /* make new settings same as old settings */
    new1.c_lflag &= ~ICANON; /* disable buffered i/o */
    new1.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
    tcsetattr(0, TCSANOW, &new1); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetterm(void) {
    tcsetattr(0, TCSANOW, &old);
}
#endif

static int isinited = 0;
void init_uart()
{
#ifdef EMUL
	// set raw mode
	initterm(0);
#else
	console_init();
#endif /* EMUL */
	isinited = 1;
}

int getch()
{
	if (!isinited) {
		init_uart();
	}
	
#ifdef EMUL
	char ch = fgetc(stdin);
	if (ch == EOF)
		return '\n';
	return ch;
#else
	char c;
	c = console_getchar();

	if (c == '\r') {
		c = '\n';
	}
	return c;
#endif
}

void putch(int c)
{
	if (!isinited) {
		init_uart();
	}
	
#ifdef EMUL
	putc(c, stdout);
	fflush(stdout);
#else
	/* if (c == 0x0a) { */
	/* 	console_putchar('+'); */
	/* 	console_putchar(0x0d); */
	/* 	console_putchar('-'); */
	/* 	console_putchar(0x0a); */
	/* 	console_putchar('!'); */
	/* } else */
		console_putchar(c);
#endif
}

int
puts(const char *s)
{
	char *p = (char *)s;
	while(*p) {
		putch(*p++);
	}
	return 0;
}

int
zgetline(char **ptr)
{
	if (!isinited) {
		init_uart();
	}
	
	char * line = malloc(100), * linep = line;
    size_t lenmax = 100, len = lenmax;
    int c, c1, c2, c3;
	int rval = GLSTR;		// are we processing ansii sequence??

	//printf("Entering zgetline: ");
    if (line == NULL)
        return GLERR;

    for(;;) {
        c = getch();
        if(c == '\n') {
			putch('\n');
			putch('\r');
            break;
		}
		//printf("\n(%x)-\n", c);
#if 1
 		if (c == ESC) {
			c1 = getch();
			//printf("\n-c1=%c (%x)-\n", c1, c1);
			if (c1 == ESC1) {	// c1 == '['
				c2 = getch();
				//printf("\n-c2=%c (%x)-\n", c2, c2);
				if (c2 < 0x40) {	// c2 < 40, do 4 char escape.
					c3 = getch();
					//printf("\n-c3=%c (%x)-\n", c3, c3);
					if (c3 == END4) {
						// Good 4 byte escape code
						line[0] = c2;
						line[1] = '\0';
						rval = GLESCODE;
					} else {
						line[0] = '\0';
						rval = GLERR;
					}
				} else {	// >= 40, 3 code escape
					line[0] = c2;
					line[1] = '\0';
					rval = GLESCODE;
				}
			} else {
				line[0] = '\0';
				rval = GLERR;
			}
			*ptr = linep;
			return rval;
		}
#endif
		if ((c == BS) || (c == CTLH)) {
			//printf("ERASE\n");
			// Dont try to erase before line starts!
			if (line > linep) {
				*--line = '\0';
				len++;
				putch(0x8);
				putch(' ');
				putch(0x8);
			}
			continue;
		}
					
		//printf("c=0x%x (%c)\n", c, c);
		putch(c);
		  

        if(--len == 0) {
            len = lenmax;
            char * linen = realloc(linep, lenmax *= 2);

            if(linen == NULL) {
                free(linep);
                return GLERR;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
	*ptr = linep;
    return rval;
}

