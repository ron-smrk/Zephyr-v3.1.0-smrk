#include "shell.h"

struct history hist_tab[NHIST] = {0};

static int hnum = 0;			// ever increasing history number
static int currhist = NHIST-1;	// for get prev/next histor

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
	currhist = NHIST-1;
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

char *
prevhist()
{
	struct history *h = &hist_tab[currhist];

	if (h->cmd) {
		// printf("%d: %s\n", h->hnum, h->cmd);
		if ( (h-1)->cmd) {
			currhist--;
		}
		return(h->cmd);
	}
	return NULL;
}
		
char *
nexthist()
{
	struct history *h = &hist_tab[currhist];

	if ( (h+1)->cmd) {
		currhist++;
		h++;
	}
	if (h->cmd) {
		// printf("%d: %s\n", h->hnum, h->cmd);
		return(h->cmd);
	}
	return NULL;
}
