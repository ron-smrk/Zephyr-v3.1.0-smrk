#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lib.h"


#include <errno.h>

char *strdup(const char *s)
{
	char *dest = malloc(strlen(s)+1);

	if (dest) {
		strcpy(dest, s);
	}

	return dest;
}

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char *ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

char* toLower(char* s) {
  for(char *p=s; *p; p++) *p=tolower(*p);
  return s;
}
char* toUpper(char* s) {
  for(char *p=s; *p; p++) *p=toupper(*p);
  return s;
}

/*
 * return char representation of a double, as printf doesn't support %f
 * STILL NEED THISS???? RON
 */
#define NSTR 10
#define SLEN 20
static char __tmpbuf[NSTR*SLEN];
static int fptr = 0;
char *
f2str(double v, int ndec)
{
	char *ptr = &__tmpbuf[SLEN*fptr];

	sprintf(ptr, "%.*f", ndec, v);
	fptr += 1;
	if (fptr >= NSTR)
		fptr = 0;
	return ptr;
}

int
ishex(char *s)
{
	int i, len;
	len = strlen(s);
	if (len > MAX_ARGSZ_INT)
		return -EINVAL;
	for ( i = 0; i < len; i++ )
		if (!isxdigit((int)s[i]))
			return 0;
	return 1;
}

double
mytrunc(double f, int roundto)
{

	int x = (int)f;

	x = x+roundto;
	x = (x/roundto)*roundto;
	return (double) x;
}
unsigned short
toshort(unsigned char *x)
{
	return (unsigned short) ((x[0] << 8) | x[1]);
}

unsigned int
toint(unsigned char *x, int nbytes)
{
	unsigned int rval = 0;
	int i = 0;

	for (i = 0; i < nbytes; i++) {
		rval <<= 8;
		//printf("B: rval=0x%x, x[%d]=0x%x\n", rval, i, x[i]);
		rval |= x[i];
		//printf("A: rval=0x%x\n", rval);
	}			
	//printf("return 0x%x (sz=%d)\n", rval, nbytes);
	return rval;
}

