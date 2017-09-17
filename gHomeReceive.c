/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: gHomeReceive.c,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.1.1.1  2011/05/28 09:00:03  johan
 * gHome logging server
 *
 * Revision 1.5  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.4  2011/02/15 15:03:36  johan
 * Remove command parameter in message read
 *
 * Revision 1.3  2011/02/10 11:09:41  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:14  johan
 * Add CVS stuff
 *
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <popt.h>
#include <libgen.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include "zbSocket.h"
#include "zbSend.h"
#include "zbPacket.h"
#include "zbDisplay.h"
#include "zbCmdHelp.h"
#include "gversion.h"

int trace = 0;
int verbose = 0;
int noreply = 0;

int Count = 99999;

int ShowVersion = 0;
int Timeout = 0;
int HeartBeatPrd = 0;
int HeartBeatLoss = 0;
int StartIndex = 0;
int SleepTime = 2000;
int JoinTime = 0;

unsigned short ServerPort = 0;     /* Gateway port */
char * ServerIP = NULL;             /* Gateway IP address (dotted quad) */

/*! \struct Command line parsing data structure */
struct poptOption optionsTable[] =
{
	{"gateway", 	'G', POPT_ARG_STRING, 	&ServerIP, 0,  "Zigbee gateway",  "<ip_address>"},
	{"hbmins",	'H', POPT_ARG_INT, 	&HeartBeatPrd, 0, "Set heartbeat (in minutes)", "<integer>"},
	{"count", 	'N', POPT_ARG_INT, 	&Count, 0,  "Number if times to poll (default is 0)", "<integer>"},
	{"port", 	'P', POPT_ARG_INT, 	&ServerPort, 0,  "Gateway port(default is 5000)", "<integer>"},
	{"sleep", 	'S', POPT_ARG_INT, 	&SleepTime, 0, "Sleep time in milliseconds (default is 2000)",  "<integer>"},
	{"timeout", 	'T', POPT_ARG_INT, 	&Timeout, 0,  "Receive timeout period (default is 0)", "<integer>"},
 	{"trace", 	't', POPT_ARG_NONE, 	&trace, 	0, "Trace/debug output", NULL},
	{"verbose", 	'v', POPT_ARG_NONE, 	&verbose, 	0, "Verbose output", NULL},
	{"version", 	'V', POPT_ARG_NONE, 	&ShowVersion,  0, "Display version information", NULL},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

/* Local functions */
void sighandle(int signum);
int runPoll(int skt, char * rxBuffer, int rxSize,
		int sleepTime, int rdTimeout);
int sendReadFile(int skt, char * input_file_name, 
		char * txBuffer, 
		char * rxBuffer, 
		int rxSize, 
		int sleepTime,
		int rdTimeout);

/* ==========================================================================
   M A I N
   ========================================================================== */
int main(int argc, 
	 char *argv[])
{
	int skt, rc;
	poptContext optcon;        /*!< Context for parsing command line options */
	char rxBuffer[512];

	signal(SIGPIPE, sighandle);

	optcon = poptGetContext(argv[0],argc,(const char **)argv,optionsTable,0);
	rc = poptGetNextOpt(optcon);
	if (rc < -1)
	{
		printf("%s: %s - %s\n", basename(argv[0]), poptBadOption(optcon,0),
		poptStrerror(rc));
		poptPrintUsage(optcon, stderr, 0);
		return -1;
	}
	if (ShowVersion) displayVersion(argv[0]);
	if (! ServerIP)
	{
		fprintf(stderr, "No Gateway address\n");
		poptPrintUsage(optcon, stderr, 0);
		poptFreeContext(optcon);
		return -1;
	}
	
	if (! ServerPort) ServerPort = ZIGBEE_GATEWAY_PORT;

	skt = zbSendInit(ServerIP, ServerPort, Timeout);
	if (skt <= 0)
	{
		printf("Initialize failed\n");
		return -1;
	}
	
	poptFreeContext(optcon);
	rc = 0;

	if (trace) printf("\tPoll %d\n", Count);
	runPoll(skt, rxBuffer, sizeof(rxBuffer), SleepTime, Timeout);
	
	zbSocketClose(skt);

	return 0;
}

/*! \brief Send command and poll while waiting for reply
	\param	unit		unit number
	\param	skt		socket
	\param	txBuffer	transmit buffer
	\param	rxBuffer	receive buffer
	\param	rxSize		size of receive buffer
	\param	rdTimeout	timeout period in seconds
 */
int runPoll(int skt, 
	    char * rxBuffer, 
	    int rxSize, 
	    int sleepTime,
	    int rdTimeout)
{
	int i;
	int resp;
	int usec = sleepTime * 1e3;

	i = Count;
	if (trace) printf("\tPoll %d\n", i);
	if(zbSocketSendPing(skt) < 0)
	{
		if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
		return -1;
	}
	while(i)
	{
		if (trace) printf("\tPing %d\n", i);
		/* Wait for response */
		memset(rxBuffer, 0, rxSize);
		resp = zbSocketReadPacket(skt, rxBuffer);
		if (resp == 0);
		else if (resp == -1)
		{
			if (trace) fprintf(stderr, "zigbeeConnection zigBeePING-->ERROR1\n");
			return -1;
		}
		else
		{
			readBuffer(skt, NULL, rxBuffer, resp, usec);
			dispBuffer(rxBuffer, strlen(rxBuffer), "\n");
		}
		if(zbSocketSendPing(skt) < 0)
		{
			if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
			return -1;
		}
		usleep(usec);
		i--;
	}
	return 0;
}

/*! \brief Read and display buffer
	\param	skt		open socket
	\param	unit		unit info
	\param	cBuffer		data buffer
	\param	cLen		buffer size
	\param	unitName	unit name
	\param	unitIp		IP address
	\param	unitNum		unit number
 */
int checkBuffer(int skt,
		void * unit,
		char * cBuffer, 
		int cLen, 
		int usec)
{
	dispBuffer(cBuffer, cLen, "\n");
	return 0;
}

/*! \brief Signal handler
	\param	signum	signal number
 */
void sighandle(int signum)
{
	printf("Shutting down after signal %d\n", signum);
	exit(1);
}
