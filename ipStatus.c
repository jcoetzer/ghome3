#define MAXNET 1024

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ipStatus.h"

extern int verbose;
extern int trace;

static struct ipStatusRec * ipStatus[MAXNET] = { NULL };

void initIpStatusRec(struct ipStatusRec * ipSt)
{
	ipSt->ipaddr[0] = 0;
	ipSt->macaddr[0] = 0;
	ipSt->sts = 0;
}

void setIpStatusRec(int idx,
		    char * ipa,
		    char * maca, 
		    char * neigh)
{
	ipStatus[idx] = malloc(sizeof(struct ipStatusRec));
	
	strcpy(ipStatus[idx]->ipaddr, ipa);
	if (neigh)
	{
		strcpy(ipStatus[idx]->macaddr, maca);
	}
	else
	{
		ipStatus[idx]->macaddr[0] = 0;
		neigh = maca;
		maca = NULL;
	}
	/* Set status */
	if (0 == strcmp(neigh, "REACHABLE"))
		ipStatus[idx]->sts = REACHABLE;
	else if (0 == strcmp(neigh, "STALE"))
		ipStatus[idx]->sts = STALE;
	else if (0 == strcmp(neigh, "INCOMPLETE"))
		ipStatus[idx]->sts = INCOMPLETE;
	else
		ipStatus[idx]->sts = UNKNOWN;
	if (verbose) printf("IP %s, MAC %s: %s\n", ipa, maca ? maca : "unknown", neigh);
}

static struct ipStatusRec * getIpStatus(char * ipa)
{
	int i;
	
	for(i=0; i<MAXNET; i++)
	{
		if (ipStatus[i] == NULL)
			break;
		if (0 == strcmp(ipa, ipStatus[i]->ipaddr))
			return ipStatus[i];
	}
	return NULL;
}

void clearIpStatus()
{
	int i;
	
	for(i=0; i<MAXNET; i++)
	{
		if (ipStatus[i] != NULL)
			free(ipStatus[i]);
	}
}

int readIpStatus()
{
	int n = 0;
	FILE * gstat;
	char * stok;
	char * ipa;
	char * maca;
	char * neigh;
	char stat[256];
	
	system("/sbin/ip neigh | cut -d' ' -f1,5,6 > /tmp/gNetStatus");
	
	gstat = fopen("/tmp/gNetStatus", "r");
	
	fgets(stat, sizeof(stat), gstat);
	while (! feof(gstat))
	{
		if (verbose) printf("%s", stat);
		ipa = maca = neigh = NULL;
		stok = strtok(stat, " ");
		if (stok)
		{
			ipa = stok;
			stok = strtok(NULL, " ");
			if (stok)
			{
				maca = stok;
				stok = strtok(NULL, " ");
				if (stok)
				{
					neigh = stok;
				}
			}
			setIpStatusRec(n, ipa, maca, neigh);
			++n;
			if (n >= MAXNET)
				break;
		}
		fgets(stat, sizeof(stat), gstat);	
	}

	return 0;
}