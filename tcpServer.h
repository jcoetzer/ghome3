#ifndef TCPSERVER_H
#define TCPSERVER_H

#define MAX_CONNECTNS 1024

struct ConnectionRec
{
	int Unit;
	unsigned char Monitor;
	char * IpAddr;
};

#endif
