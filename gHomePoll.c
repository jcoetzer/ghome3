/*! \file gHomePoll.c
	\brief Poll devices on Zigbee network
*/

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: gHomePoll.c,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.1.1.1  2011/05/28 09:00:03  johan
 * gHome logging server
 *
 * Revision 1.7  2011/02/18 06:43:46  johan
 * Streamline polling
 *
 * Revision 1.6  2011/02/15 15:23:14  johan
 * Change startup order
 *
 * Revision 1.5  2011/02/15 15:03:36  johan
 * Remove command parameter in message read
 *
 * Revision 1.4  2011/02/10 11:09:41  johan
 * Take out CVS Id tag
 *
 * Revision 1.3  2011/02/09 19:31:49  johan
 * Stop core dump during polling
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

#include "zbSocket.h"
#include "zbSend.h"
#include "zbData.h"
#include "zbState.h"
#include "zbPacket.h"
#include "zbDisplay.h"
#include "gHomeListDev.h"
#include "deviceCheck.h"
#include "zbPacket.h"
#include "zbState.h"
#include "gversion.h"
#include "dbConnect.h"
#include "unitDb.h"

const int MaxRetry = 100;

int trace = 0;
int verbose = 0;
int Timeout = 0;
char * ShortAddress = NULL;
int DoorSwitch = 0;

int UnitNum = -1;
int Update = 0;
int EndPoint = 0;
int ShowVersion = 0;

unsigned short ServerPort = 0;     /* Gateway port */
char * ServerIP = NULL;            /* Gateway IP address (dotted quad) */
char * WebDir = NULL;

char DefaultWebDir[] = "/var/www/html/iHome/conf";

/*! \struct 	optionsTable	Command line parsing data */
struct poptOption optionsTable[] =
{
	{"gateway", 'G', POPT_ARG_STRING, &ServerIP, 0,  "Zigbee gateway",  "<ip_address>"},
	{"port", 'P', POPT_ARG_INT, &ServerPort, 0,  "Gateway port(default is 5000)", "<integer>"},
	{"timeout", 'T', POPT_ARG_INT, &Timeout, 0,  "Receive timeout period (default is 0)", "<integer>"},
	{"trace", 't', POPT_ARG_NONE, &trace, 0, "Trace/debug output", NULL},
	{"verbose", 'v', POPT_ARG_NONE, &verbose, 0, "Verbose output", NULL},
	{"endpoint", 'e', POPT_ARG_NONE, &EndPoint, 0, "EndPoint check", NULL},
	{"quick", 'q', POPT_ARG_NONE, &Update, 0, "Do quick poll and exit when complete", NULL},
	{"unit", 	'U', POPT_ARG_INT, 	&UnitNum, 0, "Unit number",  "<integer>"},
	{"version", 	'V', POPT_ARG_NONE, 	&ShowVersion, 0, "Display version information", NULL},
	{"nwk", 	'W', POPT_ARG_STRING, 	&ShortAddress, 0, "Network address",  "<nwk_addr>"},
	{"www", 	'D', POPT_ARG_STRING, 	&WebDir, 0, "Web directory",  "<path>"},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

int PollDev(unitInfoRec * unit, int sleepTime, int rdTimeout, int update, int endpoint, char * webdir);
int unitLqiCheckIeee(int skt, struct unitLqiStruct * lqiRec, char * txBuffer, char * rxBuffer);
int unitLqiCheckLogicalType(int skt, struct unitLqiStruct * lqiRec, char * txBuffer, char * rxBuffer);

/* Local functions */
void sighandle(int signum);

unitInfoRec unitInfo;

/*! \brief M A I N
 */
int main(int argc, 
	 char *argv[])
{
	int rc;
	poptContext optcon;        /* Context for parsing command line options */
	PGconn * DbConn;

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

	if (UnitNum < 0)
	{
		fprintf(stderr, "Unit number not specified\n");
		poptPrintUsage(optcon, stderr, 0);
		return -1;
	}
	
	if (!WebDir) WebDir = DefaultWebDir;
	
	srand(time(NULL));

	poptFreeContext(optcon);

	DbConn = PQconnectdb(connection_str());
	if (PQstatus(DbConn) == CONNECTION_BAD) 
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Connection to %s failed, %s", connection_str(),
			PQerrorMessage(DbConn));
		return -1;
	}

	unitDbInit(&unitInfo, 0, Update);

	unitInfo.unitNo = UnitNum;
	unitInfo.dbConnctn = DbConn;
	
	unitDbSetLocation(&unitInfo);

	/* Read unit info from tables adomis_box and hardware */
	rc = unitDbGetInfo(&unitInfo);
	if (rc)
	{
		fprintf(stderr, "Unit info not found in database\n");
		return -1;
	}
	
	if (ServerIP) strcpy(unitInfo.gwip, ServerIP);
	if (! ServerPort) ServerPort = ZIGBEE_GATEWAY_PORT;
	
	unitInfo.gwport = ServerPort;

	if (ShortAddress) unitInfo.gwaddr = strtol(ShortAddress, NULL, 16);

	/* Run poll */
	rc = PollDev(&unitInfo, 800, 0, Update, EndPoint, WebDir);
	
	/* Close database connection */
	PQfinish(DbConn);
	
	return rc;
}

int PollDev(unitInfoRec * unit,
	    int sleepTime,
	    int rdTimeout,
	    int update,
	    int endpoint,
	    char * webdir)
{
	int resp;
	int usec = sleepTime * 500;
	char rxBuffer[512];
	char txBuffer[512];
	int rxSize=sizeof(rxBuffer);
	int zbSocket = -1;
	enum zbState zbConnectionState;
	int connectionTimer;
	int loop, chk;
	int intvl, xmlf=0;
	
	if (update) intvl = LQI_QUICK_INTVL;
	else intvl = LQI_INTVL;

	/* Set initial state */
	xmlf = 0;
	connectionTimer = 0;
	zbConnectionState = zigbeeDOWN;

	/* Start polling */
	loop = 1;
	while (loop)
	{
		/* ZigBee conection state machine */
		switch(zbConnectionState)
		{
			case zigbeeDOWN:
				if (trace) printf("\t* zigbee Connection DOWN\n");
				/* Prepare connect --> Go to connect */
				zbConnectionState = zigbeeCONNECT;
				break;
			case zigbeeCONNECT:
				/* Connect --> UP or ERROR */
				if (trace) printf("\t* zigbee Connect to gateway %s\n", unit->gwip);
				zbSocket = zbSocketOpen(unit->gwip, unit->gwport);
				if (zbSocket < 0)
				{
					zbConnectionState  = zigbeeERROR;
					if (trace) printf("\tzigbee Connection CONNECT-->ERROR\n");
				}
				else
				{
					if (trace) printf("\t* zigbee Connected to gateway %s\n", unit->gwip);
					/* Make sure there are no buffered messages sitting there */
					zbSocketClear(zbSocket, rdTimeout);
					zbConnectionState = zigbeeINITGW;
					connectionTimer = 0;
				}
				break;
			case zigbeeINITGW :
				if (trace) printf("\t* zigbee Initialize gateway address\n");
				/* Get system info */
				if (trace) printf("\tGet info for system device\n");
				resp = zbPollSendReq(zbSocket, PktSysInfo, "system info");
				if (resp < 0)
				{
					if (trace) fprintf(stderr, "zigbee Connection send request ERROR\n");
					zbConnectionState = zigbeeERROR;
				}
				/* Send ping */
				usleep(usec);
				zbConnectionState = zigbeeUP;
				if (zbSocketSendPing(zbSocket) < 0)
				{
					if (trace) printf("\tzigbee Connection PING ERROR\n");
					zbConnectionState = zigbeeERROR;
				}
				break;
			case zigbeeUP :
				if (trace) printf("\t* zigbee Connection UP\n");
				memset(rxBuffer, 0, rxSize);
				resp = zbSocketReadPacket(zbSocket, rxBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					if (trace) fprintf(stderr, "zigbee Connection Read Packet ERROR\n");
					zbConnectionState = zigbeeERROR;
				}
				else
				{
					resp = readBuffer(zbSocket, unit, rxBuffer, resp, usec);
					dispBuffer(rxBuffer, strlen(rxBuffer), "\n");
					if (resp == SYSDEV) 
					{
						/* Send command for device discovery (no replay so do not wait for it) */
						deviceDiscovery(zbSocket, unit->gwaddr, txBuffer, rxBuffer);
						/* Go to next state */
						zbConnectionState = zigbeeSTART;
					}
				}
				/* Send ping */
				usleep(usec);
				if (zbSocketSendPing(zbSocket) < 0)
				{
					if (trace) printf("\tzigbee Connection PING ERROR\n");
					zbConnectionState = zigbeeERROR;
				}
				if (connectionTimer >= LQI_POLL)
				{
					printf("No reply from system device\n");
					/* Go back to previous state */
					zbConnectionState = zigbeeINITGW;
					connectionTimer = 0;
				}
				break;
			case zigbeeSTART :
				if (trace) printf("\t* zigbee Connection START\n");
				unit->lqi[0].srcAddr = unit->gwaddr;
				if (trace) printf("\tGet first LQI for system device %04x\n", unit->gwaddr);
				sendLqi(zbSocket, unit->gwaddr);
				connectionTimer = 0;
				/* Go to next state */
				if (update) zbConnectionState = zigBeeUPDATE;
				else zbConnectionState = zigBeePOLL;
				/* Send ping */
				usleep(usec);
				if (zbSocketSendPing(zbSocket) < 0)
				{
					if (trace) printf("\tzigbee Connection zigBeePING ERROR\n");
					zbConnectionState = zigbeeERROR;
				}
				break;
			case zigBeePOLL:
				if (trace) printf("%s\t* Poll %d/%d\n", DateTimeStampStr(-1), connectionTimer, LQI_POLL);
				/* Wait for response */
				memset(rxBuffer, 0, rxSize);
				resp = zbSocketReadPacket(zbSocket, rxBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					if (trace) fprintf(stderr, "zigbeeConnection zigBeePING-->ERROR1\n");
					zbConnectionState = zigbeeERROR;
				}
				else
				{
					readBuffer(zbSocket, unit, rxBuffer, resp, usec);
					dispBuffer(rxBuffer, strlen(rxBuffer), "\n");
				}
				if (connectionTimer >= LQI_POLL)
				{
					if (! unitLqiComplete(&unit->lqi[0], endpoint))
					{
						unitLqiDispXml(&unit->lqi[0], webdir, unit->unitNo);
						unitLqiDisp(&unit->lqi[0]);
						printf("%s Data is complete - reset and start over\n", DateTimeStampStr(-1));
						unitDbInit(&unitInfo, 0, Update);
						xmlf = 1;
						zbConnectionState = zigbeeINITGW;
					}
					else if ((!xmlf) && unitLqiCheckXml(webdir, unit->unitNo, intvl*2))
					{
						unitLqiDispXml(&unit->lqi[0], webdir, unit->unitNo);
						unitLqiDisp(&unit->lqi[0]);
					}
					else
					{
						chk = connectionTimer % 6;
						switch (chk)
						{
							case 0 :
								if (verbose) unitLqiDisp(&unit->lqi[0]);
								if (trace) printf("%s\tPoll check Lqi %d\n", DateTimeStampStr(-1), chk);
								deviceCheckLqi(zbSocket, &unit->lqi[0], intvl);
								break;
							case 1 :
								if (trace) printf("%s\tPoll check Ieee %d\n", DateTimeStampStr(-1), chk);
								deviceCheckIeee(zbSocket, &unit->lqi[0], rxBuffer, txBuffer, intvl);
								break;
							case 2 :
								if (trace) printf("%s\tPoll check LogicalType %d\n", DateTimeStampStr(-1), chk);
								deviceCheckLogicalType(zbSocket, &unit->lqi[0], rxBuffer, txBuffer, intvl);
								break;
							case 3 :
								if (trace) printf("%s\tPoll check EndPoints %d\n", DateTimeStampStr(-1), chk);
								deviceCheckEndPoints(zbSocket, &unit->lqi[0], rxBuffer, txBuffer, intvl);
								break;
							case 4 :
								if (trace) printf("%s\tPoll check Attributes %d\n", DateTimeStampStr(-1), chk);
								deviceCheckAttributes(zbSocket, unit->lqi, txBuffer, rxBuffer, intvl);
								break;
							default :
								connectionTimer = 0;
								break;
						}
					}
				}
				/* Send ping */
				usleep(usec);
				if (zbSocketSendPing(zbSocket) < 0)
				{
					if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
					zbConnectionState = zigbeeERROR;
				}
				break;
			case zigBeeUPDATE :
				if (trace) printf("\t* zigbee Connection check %d\n", connectionTimer);
				/* Wait for response */
				memset(rxBuffer, 0, rxSize);
				resp = zbSocketReadPacket(zbSocket, rxBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					if (trace) fprintf(stderr, "zigbeeConnection zigBeePING-->ERROR1\n");
					zbConnectionState = zigbeeERROR;
				}
				else
				{
					readBuffer(zbSocket, unit, rxBuffer, resp, usec);
					dispBuffer(rxBuffer, strlen(rxBuffer), "\n");
				}
				if (verbose) unitLqiDisp(&unit->lqi[0]);
				switch (connectionTimer)
				{
					case 1 :
						deviceCheckLqi(zbSocket, &unit->lqi[0], intvl);
						break;
					case 2 :
						deviceCheckIeee(zbSocket, &unit->lqi[0], rxBuffer, txBuffer, intvl);
						break;
					case 3 :
						deviceCheckLogicalType(zbSocket, &unit->lqi[0], rxBuffer, txBuffer, intvl);
						break;
					case 4 :
						deviceCheckEndPoints(zbSocket, &unit->lqi[0], rxBuffer, txBuffer, intvl);
						break;
					case 5 :
						deviceCheckAttributes(zbSocket, unit->lqi, txBuffer, rxBuffer, intvl);
						break;
					default :
						if (trace) unitLqiDisp(&unit->lqi[0]);
						connectionTimer = 0;
						/* Check for data completeness */
						if (! unitLqiComplete(&unit->lqi[0], endpoint))
						{
							unitLqiDisp(&unit->lqi[0]);
							loop = 0;
						}
						break;
				}
				/* Send ping */
				usleep(usec);
				if (zbSocketSendPing(zbSocket) < 0)
				{
					if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
					zbConnectionState = zigbeeERROR;
				}
				break;
			case zigbeeERROR:
				/* Clear out connection and try again --> DOWN */
				if (trace) fprintf(stderr, "\t* zigbee Connection ERROR-->DOWN in %s\n", unit->oname);
				zbSocketClose(zbSocket);
				zbConnectionState = zigbeeDOWN;
				break;
			default:
				if (trace) printf("\t* zigbee Connection unknown state\n");
				zbConnectionState = zigbeeERROR;
				break;
		}
		usleep(usec);
		connectionTimer++;
	}
	return 0;
}

/*! \brief Send command to read LQI 
	\param 	zbSocket	socket already opened
	\param 	shortAddress	4-digit hexadecimal address
	\return true when successful, else false
 */
int sendLqi(int zbSocket, 
	    int shortAddress)
{
	int iLength = 0;
	int retval;
	char zigBeePktStr[64];
	char zbStr[64];

	/* Create Zigbee command string using short address */
	if (shortAddress == 0)					/* Coordinator */
		strcpy(zigBeePktStr, "020A0F040200000003");
	else
		snprintf(zigBeePktStr, sizeof(zigBeePktStr), "020A0F0402%04X00", shortAddress);
	if (trace) printf("\tSend command to read LQI\n");

	retval = zbSendCheck(zigBeePktStr, zbStr);
	if (retval)
	{
		printf("Packet check failed\n");
		return -1;
	}
	if (trace) printf("\tGet LQI for %04x with command %s\n", shortAddress, zbStr);

	/* Length of data in Asci HEX Bytes ie. 2 Chars per byte */
	iLength = strlen(zbStr);
	if (! iLength) return -1;
	if (trace) dumpTXdata("\tZigBee TX", zbStr, iLength);
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zbStr, iLength, "<tx>\n");
	}

	if (trace) printf("\tSend command to read LQI: %s\n", zbStr);

	/* Send Zigbee command */
	retval = zbSocketWrite(zbSocket, zbStr, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "Send LQI FAIL LEN %i\n", iLength);
		return -1;
	}
	return 0;
}

/*! \brief Read and display buffer 
	\param	skt		open socket
	\param	unit		Unit info
	\param	cBuffer		Buffer string
	\param	cLen		Buffer string length
	\param	unitName	Unit name/description
	\param	unitIp		System device (gateway) address
	\param	unitNum		Unit number
*/
int checkBuffer(int skt, 
	        void * unit, 
		char * cBuffer, 
		int cLen, 
		int usec)
{
	int rc=0, rval=0;
	unitInfoRec * unitRec;

	unitRec = (unitInfoRec *) unit;
	if (0==strncmp(cBuffer, "020A8B", 6))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<LQI>\n");
		rc = getStatus(&cBuffer[12]);
		if (!rc)
		{
			if (! unitRec->pan[0]) 
			{
				strncpy(unitRec->pan, &cBuffer[20], 16);
				unitRec->pan[16] = 0;
				if (trace) printf("\tPAN set to '%s'\n", unitRec->pan);
			}
			unitLqiUpdate(unitRec->dbConnctn, unitRec->unitNo, unitRec->pan, &unitRec->lqi[0], cBuffer, 1);
			if (verbose) unitLqiDisp(&unitRec->lqi[0]);
		}
		else
		{
			if (trace) printf("\tIgnore LQI response with status %02x\n", rc);
		}
		rval = LQI;
	}
	/* IEEE address received */
	else if (0 == strncmp(cBuffer, "020A81", 6))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<IEEE>\n");
		unitLqiUpdateIeee(unitRec->pan, &unitRec->lqi[0], cBuffer, 1);
		rval = IEEE;
	}
	/* Node descriptor received */
	else if (0 == strncmp(cBuffer, "020A8215", 8))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<NODE>\n");
		unitLqiUpdateNode(unitRec->pan, &unitRec->lqi[0], cBuffer, 1);
		rval = NODE;
	}
	/* System device info (Gateway IEEE address) received */
	else if (0 == strncmp(cBuffer, "021014", 6))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<Sysdev>\n");
		unitLqiGateway(unitRec, cBuffer);
		rval = SYSDEV;
	}
	else if (0 == strncmp(cBuffer, "020A85", 6))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<Endpoint>\n");
		unitLqiEndPoints(unitRec->lqi, cBuffer);
		rval = ENDPOINT;
	}
	else if (0 == strncmp(cBuffer, "020D01", 6))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<Attrib>\n");
		unitLqiAttributes(unitRec->lqi, cBuffer);
		rval = ATTRIB;
	}
	else
	{
		dispBuffer(cBuffer, cLen, "<Other>\n");
		rval = 0;
	}
	return rval;
}

/*! \brief Signal handler
	\param 	signum	Signal number
*/
void sighandle(int signum)
{
	switch (signum)
	{
		case 13 :
			printf("Restarting after signal %d\n", signum);
			PollDev(&unitInfo, 1000, 0, Update, EndPoint, WebDir);
			break;
		default :
			printf("Shutting down after signal %d\n", signum);
			exit(1);
	}
}
