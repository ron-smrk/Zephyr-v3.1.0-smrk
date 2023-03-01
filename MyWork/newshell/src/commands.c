#include "shell.h"

struct history hist_tab[NHIST] = {0};

static int hnum = 0; // ever increasing history number

void
add2hist(char *l)
{
	int i;
	
	if (hist_tab[0].cmd) {
		// printf("Freeing (%s) %p\n", hist_tab[0].cmd, hist_tab[0].cmd);
		free(hist_tab[0].cmd);
	}
	for ( i = 0; i < (NHIST-1); i++) {
		hist_tab[i].cmd = hist_tab[i+1].cmd;
		hist_tab[i].hnum = hist_tab[i+1].hnum;
	}
	hist_tab[NHIST-1].cmd = l;
	hist_tab[NHIST-1].hnum = ++hnum;
}

int
hist_cmd(int arc, char **argv)
{
	int i;

	for (i=0; i < NHIST; i++) {
#ifdef DEBUG 
		if (hist_tab[i].cmd)
			printf("hist[%d]: %d - %s\n", i, hist_tab[i].hnum, hist_tab[i].cmd);
		else
			printf("hist[%d]: %d - %p\n", i, hist_tab[i].hnum, hist_tab[i].cmd);
#else
		if (hist_tab[i].cmd)
			printf("%d: %s\n", hist_tab[i].hnum, hist_tab[i].cmd);
#endif
	}
	return 0;
}

int
pm_cmd(int argc, char **argv)
{
	int i;
	
	printf("pm!!\n");
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
	return 0;
}


int
i2c_cmd(int argc, char **argv)
{
	int i;
	printf("i2c!!\n");
	for ( i = 0; i < argc; i++) {
		printf("arg[%d]: %s\n", i, argv[i]);
	}
	return 0;
}
