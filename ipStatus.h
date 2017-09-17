#ifndef IPSTATUS_H
#define IPSTATUS_H

enum ipNeigh
{
	UNKNOWN = 0,
	REACHABLE,
	STALE,
	INCOMPLETE
};

struct ipStatusRec
{
	char ipaddr[32];
	char macaddr[32];
	enum ipNeigh sts;
};

int readIpStatus();
static struct ipStatusRec * getIpStatus(char * ipa);
void clearIpStatus();

#endif
