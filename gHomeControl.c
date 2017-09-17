/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: gHomeControl.c,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.1.1.1  2011/05/28 09:00:03  johan
 * gHome logging server
 *
 * Revision 1.5  2011/02/18 06:43:46  johan
 * Streamline polling
 *
 * Revision 1.4  2011/02/15 15:03:36  johan
 * Remove command parameter in message read
 *
 * Revision 1.3  2011/02/10 11:09:41  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:13  johan
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
#include <time.h>

#include "zbSocket.h"
#include "zbSend.h"
#include "zbPacket.h"
#include "zbDisplay.h"
#include "zbCmdHelp.h"
#include "gHomeControl.h"

int trace = 0;
int verbose = 0;
int noreply = 0;

int Timeout = 0;
int SleepTime = 1000;
int OnTime = 300;
int ConnectionTimer = 150;

unsigned short ServerPort = 0;     /* Gateway port */
char * ServerIP = NULL;             /* Gateway IP address (dotted quad) */
char * zigBeePktStr = NULL;
char * zigBeeCmd = NULL;
char * zigBeeData = NULL;
char * SwitchAddress = NULL;
char * AlarmAddress = NULL;
char * IeeeAddress = NULL;
char * AlarmEndPoint = NULL;
char * SwitchEndPoint = NULL;

void zbPollRunPoll(char * gwip,
		   int gwport,
		   char * addrendp,
		   char * almaddr,
		   int SleepTime,
		   int ConnectionTimer,
		   int rdTimeout,
		   int onTime);
static int zbPollGateway(int iZigbeeSocket, 
			 int rdTimeout);
static int powerOff(int iZigbeeSocket,
		    char * addrendp);
static int zbPollSoundkAlarm(int iZigbeeSocket);

/*! \struct Command line parsing data structure */
struct poptOption optionsTable[] =
{
	{"gateway", 	'G', POPT_ARG_STRING, 	&ServerIP, 0,  "Zigbee gateway",  "<ip_address>"},
	{"port", 	'P', POPT_ARG_INT, 	&ServerPort, 0,  "Gateway port(default is 5000)", "<integer>"},
	{"timeout", 	'T', POPT_ARG_INT, 	&Timeout, 0,  "Receive timeout period (default is 0)", "<integer>"},
	{"endpoint", 	'E', POPT_ARG_STRING, 	&SwitchEndPoint, 0, "Switch endpoint",  "<string>"},
 	{"trace", 	't', POPT_ARG_NONE, 	&trace, 0, "Trace/debug output", NULL},
	{"verbose", 	'v', POPT_ARG_NONE, 	&verbose, 0, "Verbose output", NULL},
	{"switch", 	'S', POPT_ARG_STRING, 	&SwitchAddress, 0, "Switch unit short address",  "<short_addr>"},
	{"alarm", 	'A', POPT_ARG_STRING, 	&AlarmAddress, 0, "Alarm unit short address",  "<short_addr>"},
	{"ieee", 	'I', POPT_ARG_STRING, 	&IeeeAddress, 0, "Set IEEE address",  "<ieee_addr>"},
	{"ontime", 	'X', POPT_ARG_INT, 	&OnTime, 0, "Set time to switch power on",  "<index>"},
	{"sleep", 	'S', POPT_ARG_INT, 	&SleepTime, 0, "Sleep time in milliseconds (default is 1000)",  "<integer>"},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

/* Local functions */
void sighandle(int signum);
int pollReply(void * unit, int skt, char * txBuffer, char * rxBuffer, int rxSize, int rdTimeout);

/* ==========================================================================
   M A I N
   ========================================================================== */
int main(int argc, 
	 char *argv[])
{
	int skt, rc;
	poptContext optcon;        /*!< Context for parsing command line options */
	int rdTimeout = 0;
	char addrendp[16];

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
	
	if (! ServerPort) ServerPort = ZIGBEE_GATEWAY_PORT;

	if (! ServerIP)
	{
		printf("No gateway specified\n");
		poptPrintUsage(optcon, stderr, 0);
		poptFreeContext(optcon);
		return -1;
	}
	
	if (! SwitchAddress)
	{
		printf("No short address specified\n");
		poptPrintUsage(optcon, stderr, 0);
		poptFreeContext(optcon);
		return -1;
	}
	if (strlen(SwitchAddress) != 4)
	{
		printf("Invalid short address specified\n");
		poptPrintUsage(optcon, stderr, 0);
		poptFreeContext(optcon);
		return -1;
	}
	
	if (SwitchEndPoint) snprintf(addrendp, sizeof(addrendp), "%s%s", SwitchAddress, SwitchEndPoint);
	else snprintf(addrendp, sizeof(addrendp), "%s%s", SwitchAddress, "0A");

	skt = zbSendInit(ServerIP, ServerPort, Timeout);
	if (skt <= 0)
	{
		printf("Initialize failed\n");
		return -1;
	}
	
	poptFreeContext(optcon);
	rc = 0;

	zbPollRunPoll(ServerIP, ServerPort, addrendp, AlarmAddress, SleepTime, ConnectionTimer, rdTimeout, OnTime);

	return 0;
}

/*! \brief Maintain the connection state machine
	\param unit		unit information from database
	\param SleepTime	milliseconds
	\param ConnectionTimer	count
	\param rdTimeout	timeout period in seconds

	Maintains a regular poll if there is no activity on the port
	Runs as a management thread

*/
void zbPollRunPoll(char * gwip,
		   int gwport,
		   char * addrendp,
		   char * almaddr,
		   int sleepTime,
		   int ConnectionTimer,
		   int rdTimeout,
		   int onTime)
{
	unsigned char cBuffer[2048]; /* FIXME - This should cover everything but not good */
	int iZigbeeSocket = -1;
	int rc = 0, resp;
	enum zbState gizigbeeConnectionState;
	int gizigbeeConnectionTimer;
	time_t this, prev, alarmOn;
	int timeOn, timeAlarm;
	int powerOn = 0;
	char zigBeePktStr[64];
	char txBuffer[64];
	useconds_t usec = SleepTime * 1e3;
	
	prev = time(NULL);
	if (trace) printf("\tConnect to gateway %s\n", gwip);

	/* Set initial state */
	gizigbeeConnectionState = zigbeeDOWN;

	/* Global timer, reset each time a packet is sent or recieved */
	gizigbeeConnectionTimer = 0;
	if (trace) printf("\tzigbee poll alarm\n");

	/* Endless task loop */
	while(1)
	{
		this = time(NULL);
		/* ZigBee conection state machine */
		switch(gizigbeeConnectionState)
		{
			case zigbeeDOWN:
				/* Prepare connect --> Go to connect */
				gizigbeeConnectionState = zigbeeCONNECT;
				if (trace) printf("\tzigbee Connection DOWN\n");
				break;
			case zigbeeCONNECT:
				/* Connect --> UP or ERROR */
				if (trace) printf("\tzigbee Connect to gateway %s\n", gwip);
				iZigbeeSocket = zbSocketOpen(gwip, gwport);
				if (iZigbeeSocket < 0)
				{
					gizigbeeConnectionState  = zigbeeERROR;
					if (trace) printf("\tzigbee Connection CONNECT-->ERROR\n");
					this = time(NULL);
					/* Only update once every 10 minutes */
					if (this - prev > 600)
					{
						prev = this;
					}
				}
				else
				{
					if (trace) printf("\tzigbee Connected to gateway %s\n", gwip);
					/* Make sure there are no buffered messages sitting there */
					zbSocketClear(iZigbeeSocket, rdTimeout);
					gizigbeeConnectionState = zigbeeUP;
					gizigbeeConnectionTimer = 0;
				}
				break;
			case zigbeeUP:
				if (trace) printf("\tzigbee Connection UP\n");
				usleep(1e6);
				gizigbeeConnectionState = zigbeeINITGW;
				break;
			case zigbeeINITGW :
				if (trace) printf("\tzigbee Connection INIT GATEWAY\n");
				zbPollGateway(iZigbeeSocket, rdTimeout);
				sleep(1e6);
				/* Wait for response */
				memset(cBuffer, 0, sizeof(cBuffer));
				resp = zbSocketReadPacket(iZigbeeSocket, cBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					gizigbeeConnectionState = zigbeeERROR;
					if (trace) fprintf(stderr, "\tzigbeeConnection init gateway-->ERROR\n");
				}
				else
				{
					if (0 == strncmp(cBuffer, "021014", 6))
					{
						DateTimeStamp();
						dispBuffer(cBuffer, resp, "<Sysdev>\n");
						gizigbeeConnectionState = zigBeePOLL;
					}
					else
					{
						DateTimeStamp();
						dispBuffer(cBuffer, resp, "\n");
					}
				}
				break;
			case zigBeePOLL:
				/* Wait for response */
				memset(cBuffer, 0, sizeof(cBuffer));
				resp = zbSocketReadPacket(iZigbeeSocket, cBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					gizigbeeConnectionState = zigbeeERROR;
					if (trace) fprintf(stderr, "zigbeeConnection zigBeePING-->ERROR1\n");
				}
				else
				{
					rc = readBuffer(iZigbeeSocket, NULL, cBuffer, resp, usec);
					switch (rc)
					{
						case PIR :
							prev = this;
							powerOn = 1;
							if (trace) printf("\tPower on at %d\n", (int) prev);
							snprintf(zigBeePktStr, sizeof(zigBeePktStr), "020C010602%s0006", addrendp);
							rc = zbSendCheck(zigBeePktStr, txBuffer);
							if (! rc)
							{
								if (verbose) dispBuffer(txBuffer, strlen(txBuffer), "\n");
								rc = zbSocketWrite(iZigbeeSocket, txBuffer, strlen(txBuffer));
								if (rc < 0)
								{
									printf("Packet send failed\n");
								}
							}
							break;
						case ARM :
							if (trace) printf("\treadBuffer returned ARM signal %d\n", rc);
							gizigbeeConnectionState = zigbeeARMED;
						default :
							if (trace) printf("\treadBuffer returned %d\n", rc);
					}
				}
				/* Issue PING Comand to keep connection alive */
				if (zbSocketSendPing(iZigbeeSocket) < 0)
				{
					gizigbeeConnectionState = zigbeeERROR;
					if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
				}
				usleep(usec);
				/* Start poll every 10 minutes, not sure if really needed */
				if (gizigbeeConnectionTimer > ConnectionTimer)
				{
					gizigbeeConnectionTimer = 0;
					gizigbeeConnectionState = zigbeeUP;
				}
				break;
			case zigbeeARMED:
				/* Wait for response */
				memset(cBuffer, 0, sizeof(cBuffer));
				resp = zbSocketReadPacket(iZigbeeSocket, cBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					gizigbeeConnectionState = zigbeeERROR;
					if (trace) fprintf(stderr, "zigbeeConnection zigBeePING-->ERROR1\n");
				}
				else
				{
					rc = readBuffer(iZigbeeSocket, NULL, cBuffer, resp, usec);
					if (trace) printf("\treadBuffer returned %d\n", rc);
					switch (rc)
					{
						case PIR :
						case ALARM :
							if (trace) printf("\tTurn alarm off\n");
							rc = zbSocketWrite(iZigbeeSocket, PktDisarm, strlen(PktDisarm));
							if (rc < 0)
							{
								printf("Packet send failed\n");
							}
							/* Wait for response */
							memset(cBuffer, 0, sizeof(cBuffer));
							resp = zbSocketReadPacket(iZigbeeSocket, cBuffer);
							if (resp == 0);
							else if (resp == -1)
							{
								gizigbeeConnectionState = zigbeeERROR;
								if (trace) fprintf(stderr, "zigbeeConnection zigBeePING-->ERROR1\n");
							}
							else
							{
								rc = readBuffer(iZigbeeSocket, NULL, cBuffer, resp, usec);
								if (trace) printf("\treadBuffer returned %d\n", rc);
								switch (rc)
								{
									case DISARM :
										if (trace) printf("\tAlarm off - wait for confirm\n");
										alarmOn = this;
										gizigbeeConnectionState = zigbeeALARM;
									default :
										if (trace) printf("\treadBuffer returned %d\n", rc);
								}
							}
							gizigbeeConnectionState = zigbeeALARM;
							break;
						case DISARM :
							if (trace) printf("\tTurned alarm off\n");
							alarmOn = this;
							gizigbeeConnectionState = zigBeePOLL;
						default :
							if (trace) printf("\treadBuffer returned %d\n", rc);
					}
				}
				/* Issue PING Comand to keep connection alive */
				if (zbSocketSendPing(iZigbeeSocket) < 0)
				{
					gizigbeeConnectionState = zigbeeERROR;
					if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
				}
				break;
			case zigbeeALARM:
				timeAlarm = (int) this - prev;
				if (trace) printf("\tzigbee Alarm wait for timeout %d\n", timeAlarm);
				if (timeAlarm > 60)
				{
					if (trace) printf("\tAlarm turned on after %d seconds\n", (int) timeAlarm);
					zbPollSoundkAlarm(iZigbeeSocket);
					gizigbeeConnectionState = zigBeePOLL;
				}
				/* Wait for response */
				memset(cBuffer, 0, sizeof(cBuffer));
				resp = zbSocketReadPacket(iZigbeeSocket, cBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					gizigbeeConnectionState = zigbeeERROR;
					if (trace) fprintf(stderr, "zigbeeConnection zigBeePING-->ERROR1\n");
				}
				else
				{
					rc = readBuffer(iZigbeeSocket, NULL, cBuffer, resp, usec);
					if (trace) printf("\treadBuffer returned %d\n", rc);
					switch (rc)
					{
						case DISARM :
							if (trace) printf("\tAlarm off - back to normal\n");
							alarmOn = this;
							gizigbeeConnectionState = zigBeePOLL;
						default :
							if (trace) printf("\treadBuffer returned %d\n", rc);
					}
				}
				/* Issue PING Comand to keep connection alive */
				if (zbSocketSendPing(iZigbeeSocket) < 0)
				{
					gizigbeeConnectionState = zigbeeERROR;
					if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
				}
				break;
			case zigbeeERROR:
				/* Clear out connection and try again --> DOWN */
				if (trace) fprintf(stderr, "\tzigbee Connection ERROR-->DOWN\n");
				gizigbeeConnectionState = zigbeeDOWN;
				zbSocketClose(iZigbeeSocket);
				break;
			default:
				if (trace) printf("\tzigbee Connection unknown state\n");
				gizigbeeConnectionState = zigbeeERROR;
				break;
		}
		if (powerOn)
		{
			timeOn = (int) this - prev;
			if (trace) printf("\tPower has been on for %d seconds\n", timeOn);
			if (timeOn > onTime)
			{
				if (trace) printf("\tPower turned off after %d seconds\n", (int) timeOn);
				if (almaddr) 
				{
					int alrmAddr = (int) strtol(almaddr, NULL, 16);
					zbPollSquawkAlarm(iZigbeeSocket, alrmAddr);
				}
				powerOff(iZigbeeSocket, addrendp);
				powerOn = 0;
			}
		}
		usleep(usec);
		gizigbeeConnectionTimer++;
	}
} /* zbPollRunPoll */

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
	zbEvent rval=0;

	/* PIR picked up movement */
	if (0 == strncmp(cBuffer, "021C0008", 8))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<PIR>\n");
		if (0 == strncmp(&cBuffer[14], "0500", 4))
		{
			if (trace) printf("\tPIR picked up movement\n");
			rval = PIR;
		}
	}
	/* Burglar alarm activated */
	else if (0 == strncmp(cBuffer, "021C0519", 8))
	{
		DateTimeStamp();
		if (0 == strncmp(&cBuffer[56], "01", 2))
		{
			dispBuffer(cBuffer, cLen, "<ALARM>\n");
			rval = ALARM;
		}
		else
		{
			dispBuffer(cBuffer, cLen, "<Heartbeat>\n");
			rval = 0;
		}
	}
	/* Arm or disarm command */
	else if (0 == strncmp(cBuffer, "021C0106", 8))
	{
		DateTimeStamp();
		if (0 == strncmp(&cBuffer[18], "50", 2))
		{
			dispBuffer(cBuffer, cLen, "<ARM>\n");
			rval = ARM;
		}
		else if (0 == strncmp(&cBuffer[18], "51", 2))
		{
			dispBuffer(cBuffer, cLen, "<DISARM>\n");
			rval = DISARM;
		}
		else
		{
			dispBuffer(cBuffer, cLen, "<StatusChange>\n");
			rval = 0;
		}
	}
	else
	{
		dispBuffer(cBuffer, cLen, "\n");
		rval = 0;
	}

	return rval;
}

/*! \brief Turn power off
 */
static int powerOff(int skt,
		    char * addrendp)
{
	int rc;
	char zigBeePktStr[64];
	char txBuffer[64];

	if (trace) printf("\tTurn power off\n");
	snprintf(zigBeePktStr, sizeof(zigBeePktStr), "020C000602%s0006", addrendp);
	rc = zbSendCheck(zigBeePktStr, txBuffer);
	if (rc)
	{
		printf("Packet check failed\n");
		return -1;
	}
	if (verbose) dispBuffer(txBuffer, strlen(txBuffer), "\n");
	rc = zbSocketWrite(skt, txBuffer, strlen(txBuffer));
	if (rc < 0)
	{
		printf("Packet send failed\n");
		return -1;
	}
	return 0;
}

/*! \brief Send Zigbee command to get system device (gateway) information
	\param	iZigbeeSocket	socket already opened
	\return 0 when successful, else -1
 */
static int zbPollGateway(int iZigbeeSocket, 
			 int rdTimeout)
{
	int resp, iLength = 0;
	int retval;
	char cBuffer[128];

	if (trace) printf("\tSend Zigbee System Get Device Information\n");
	strcpy(cBuffer, PktSysInfo);
	iLength = strlen(cBuffer);
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(cBuffer, iLength, "<tx>\n");
	}
	retval = zbSocketWrite(iZigbeeSocket, cBuffer, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zigbeeStartAlarm FAIL LEN:%i\n", iLength);
		return -1;
	}

	resp = zbSocketReadPacket(iZigbeeSocket, cBuffer);
	if (resp > 0)
	{
		if (0 == strncmp(cBuffer, "021014", 6))
		{
			DateTimeStamp();
			dispBuffer(cBuffer, resp, "<Sysdev>\n");
		}
	}
	else if (trace) printf("\tNo reply for Zigbee System Get Device Information\n");

	return 0;
}

/*! \brief Send Zigbee command to sound alarm
	\param	iZigbeeSocket	socket already opened
	\param	shortAddress	short Zigbee address
	\return 0 when successful, else -1
 */
static int zbPollSoundkAlarm(int iZigbeeSocket)
{
	int iLength = 0;
	int retval;

	if (trace) printf("\tSend alarm: %s\n", PktSoundAlarm);
	iLength = strlen(PktSoundAlarm);	
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(PktSoundAlarm, iLength, "<ALARM>\n");
	}
	retval = zbSocketWrite(iZigbeeSocket, PktSoundAlarm, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollSquawkAlarm FAIL LEN:%i\n", iLength);
		return -1;
	}

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
