/*! \file gHomeListDev
	\brief List devices on Zigbee network
*/

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: gHomeListDev.c,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
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

const int MaxRetry = 100;

int trace = 0;
int verbose = 0;
int panic = 0;
int scan = 0;
int Timeout = 0;
int UnitNum = -1;
char * ShortAddress = NULL;

int update = 0;

unsigned short ServerPort = 0;     /* Gateway port */
char * ServerIP = NULL;            /* Gateway IP address (dotted quad) */

/*! \struct 	optionsTable	Command line parsing data */
struct poptOption optionsTable[] =
{
	{"gateway", 'G', POPT_ARG_STRING, &ServerIP, 0,  "Zigbee gateway",  "<ip_address>"},
	{"port", 'P', POPT_ARG_INT, &ServerPort, 0,  "Gateway port(default is 5000)", "<integer>"},
	{"timeout", 'T', POPT_ARG_INT, &Timeout, 0,  "Receive timeout period (default is 0)", "<integer>"},
	{"trace", 't', POPT_ARG_NONE, &trace, 0, "Trace/debug output", NULL},
	{"panic", 'p', POPT_ARG_NONE, &panic, 0, "Panic alarm test", NULL},
	{"scan", 'p', POPT_ARG_NONE, &scan, 0, "Panic alarm test", NULL},
	{"verbose", 'v', POPT_ARG_NONE, &verbose, 0, "Verbose output", NULL},
	{"update", 'u', POPT_ARG_NONE, &update, 0, "Update database", NULL},
	{"unit", 'U', POPT_ARG_INT, &UnitNum, 0, "Unit number",  "<integer>"},
	{"nwk", 	'W', POPT_ARG_STRING, 	&ShortAddress, 0, "Network address",  "<nwk_addr>"},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

int ListDev(int skt);
int ScanDev(int skt, int nwk, int count, int sleepTime);
int unitLqiCheckIeee(int skt, struct unitLqiStruct * lqiRec, char * txBuffer, char * rxBuffer);
int unitLqiCheckLogicalType(int skt, struct unitLqiStruct * lqiRec, char * txBuffer, 
		   char * rxBuffer);

/* Local functions */
void sighandle(int signum);

/*! \brief M A I N
 */
int main(int argc, 
	 char *argv[])
{
	int skt, rc, nwk;
	poptContext optcon;        /* Context for parsing command line options */

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

	if (! ServerIP)
	{
		printf("No gateway specified\n");
		poptPrintUsage(optcon, stderr, 0);
		return -1;
	}
	if (! ServerPort) ServerPort = ZIGBEE_GATEWAY_PORT;
	
	srand(time(NULL));

	skt = zbSendInit(ServerIP, ServerPort, Timeout);
	if (skt <= 0)
	{
		printf("Initialize failed\n");
		return -1;
	}
	poptFreeContext(optcon);

	if (scan)
	{
		if (ShortAddress) nwk = strtol(ShortAddress, NULL, 16);
		else nwk = 0;
		rc = ScanDev(skt, nwk, 99999, 1000);
	}
	else
		rc = ListDev(skt);
	
	return rc;
}

int ScanDev(int skt, 
	    int nwk, 
	    int count, 
	    int sleepTime)
{
	int i;
	int resp;
	int usec = sleepTime * 1e3;
	char rxBuffer[512];
	char txBuffer[512];
	int rxSize=sizeof(rxBuffer);
	unitInfoRec unitInfo;

	i = count;
	if (trace) printf("\tScan %d\n", i);
	if(zbSocketSendPing(skt) < 0)
	{
		if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
		return -1;
	}

	unitDbInit(&unitInfo, 0, update);
	if (nwk)
	{
		unitInfo.lqi[0].srcAddr = nwk;
		unitInfo.gwaddr = nwk;
		if (verbose) unitLqiDisp(&unitInfo.lqi[0]);
		if (trace) printf("\tGet LQI for system device %04x\n", nwk);
		sendLqi(skt, nwk);
	}

	/* Start polling */
	while(i)
	{
		if (! unitInfo.gwaddr)
		{
			/* Get system info */
			if (trace) printf("\tGet info for system device\n");
			zbPollSendReq(skt, PktSysInfo, "system info");
			usleep(usec);
		}
		else if (unitInfo.lqi[0].srcAddr < 0)
		{
			nwk = unitInfo.gwaddr;
			unitInfo.lqi[0].srcAddr = nwk;
			if (trace) printf("\tGet first LQI for system device %04x\n", nwk);
			sendLqi(skt, nwk);
			usleep(usec);
		}
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
			readBuffer(skt, &unitInfo, rxBuffer, resp, usec);
			dispBuffer(rxBuffer, strlen(rxBuffer), "\n");
		}
		if (trace) printf("\tPing %d\n", i);
		if (zbSocketSendPing(skt) < 0)
		{
			if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
			return -1;
		}
		if (0==i%LQI_POLL)
		{
			  if (verbose) unitLqiDisp(&unitInfo.lqi[0]);
			  /* Send LQI request */
			  deviceCheckLqi(skt, &unitInfo.lqi[0], LQI_INTVL);
			  deviceCheckIeee(skt, &unitInfo.lqi[0], rxBuffer, txBuffer, LQI_INTVL);
			  deviceCheckLogicalType(skt, &unitInfo.lqi[0], rxBuffer, txBuffer, LQI_INTVL);
		}
		usleep(usec);
		i--;
	}
	return 0;
}

/*! \brief Send command to send LQI request
	\param 	iZigbeeSocket	socket already opened
	\param 	shortAddress	4-digit hexadecimal address
	\return true when successful, else false
 */
int sendLqi(int iZigbeeSocket, 
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
	retval = zbSocketWrite(iZigbeeSocket, zbStr, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "Send LQI FAIL LEN:%i\n", iLength);
		return -1;
	}
	return 0;
}

int ListDev(int skt)
{
	int i, j, count, rc, retry=0;
	int nwk;
	char txBuffer[512];
	char rxBuffer[512];
	DeviceInfo gateway, coordinator;
	DeviceInfo * alarm = NULL;
	DeviceInfo * devices[MAX_DEVICES];
	int usec = 2e6;
	
	gateway.shrtAddr[0] = 0;
	gateway.ieeeAddr[0] = 0;
	gateway.loctn = 0;
	
	coordinator.shrtAddr[0] = 0;
	coordinator.ieeeAddr[0] = 0;
	coordinator.loctn = 0;
	
	for (i=0; i<MAX_DEVICES; i++) devices[i] = NULL;

	rc = GatewayInfo(skt, txBuffer, rxBuffer, &gateway, Timeout);

	rc = CoordinatorInfo(skt, txBuffer, rxBuffer, &coordinator, Timeout);

	/* List devices associated with coordinator
	count = DeviceList(1, skt, "0000", txBuffer, rxBuffer, (DeviceInfo**) &devices, MAX_DEVICES, Timeout); */
	/* Reopen connection
	if (count < 0)
	{
		skt = zbSendInit(ServerIP, ServerPort, Timeout);
		if (skt <= 0)
		{
			printf("Re-initialize failed\n");
			return -1;
		}
	}
	*/
	/* List devices associated with gateway */ 
	count = DeviceList(1, skt, gateway.shrtAddr, txBuffer, rxBuffer, (DeviceInfo**) &devices, MAX_DEVICES, Timeout);

	for (i=0; i<MAX_DEVICES; i++)
	{
		if (! devices[i]) continue;
		
		nwk = strtol(devices[i]->shrtAddr, NULL, 16);

		/* Get IEEE Address */
		rc = deviceGetIeee(skt, nwk, txBuffer, rxBuffer, Timeout);
		if (rc < 0)
		{
			
			if (retry++ > MaxRetry) break;
			if (trace) printf("\tRetry %d get IEEE Address\n", retry);
			zbSocketClose(skt);
			skt = zbSendInit(ServerIP, ServerPort, Timeout);
			if (skt <= 0)
			{
				printf("Retry Get IEEE Address failed\n");
				return -1;
			}
			rc = deviceGetIeee(skt, -1, txBuffer, rxBuffer, Timeout);
		}
		else
		{
			rc = zbSendGetReply(skt, rxBuffer, Timeout);
			if (rc > 0)
			{
				readBuffer(skt, NULL, rxBuffer, rc, usec);
				strncpy(devices[i]->ieeeAddr, &rxBuffer[28], 16);
				devices[i]->ieeeAddr[16] =0;
			}
		}

		/* Get EndPoints */
		rc = GetEndPoints(skt, i, txBuffer, rxBuffer, devices[i], Timeout);
		if (rc < 0) 
		{
			if (retry++ > MaxRetry) break;
			if (trace) printf("\tRetry %d Get EndPoints\n", retry);
			zbSocketClose(skt);
			skt = zbSendInit(ServerIP, ServerPort, Timeout);
			if (skt <= 0)
			{
				printf("Retry Get EndPoints failed\n");
				return -1;
			}
			rc = GetEndPoints(skt, i, txBuffer, rxBuffer, devices[i], Timeout);
		}

		/* Read Attributes */
		rc = ReadAttributes(skt, i, txBuffer, rxBuffer, devices[i], Timeout);
		if (rc < 0) 
		{
			if (retry++ > MaxRetry) break;
			if (trace) printf("\tRetry %d Get EndPoints Read Attributes\n", retry);
			zbSocketClose(skt);
			skt = zbSendInit(ServerIP, ServerPort, Timeout);
			if (skt <= 0)
			{
				printf("Retry Read Attributes failed\n");
				return -1;
			}
			rc = ReadAttributes(skt, i, txBuffer, rxBuffer, devices[i], Timeout);
		}
	}
	if (rc < 0)
	{
		printf("Listing of %d devices failed\n", count);
		return -1;
	}
	
	printf("\nCoordinator \tshort address is %s, IEEE address is %s\n", coordinator.shrtAddr, coordinator.ieeeAddr);
	printf("Gateway \tshort address is %s, IEEE address is %s\n", gateway.shrtAddr, gateway.ieeeAddr);
	for (i=0; i<count; i++)
	{
		printf("Device %d \tshort address is %s, IEEE address is %s\n\t", i+1, devices[i]->shrtAddr, devices[i]->ieeeAddr);
		for (j=0; j<MAX_ENDPOINTS; j++)
			if (devices[i]->endPoint[j][0])
				printf("Endpoint %d: %s ", j+1, devices[i]->endPoint[j]);
		printf("\n\t");
		for (j=0; j<MAX_ENDPOINTS; j++)
			if (devices[i]->attributes[j][0])
				printf("Attributes %d: %s ", j+1, devices[i]->attributes[j]);
		/* TODO not sure of this */
		if (! strcmp(devices[i]->attributes[0], "010000001800")) alarm = devices[i];
		printf("\n");
	}
	if (alarm)
	{
		printf("Alarm \t\tshort address is %s, endpoint is %s, IEEE address is %s\n", alarm->shrtAddr, alarm->endPoint[0], alarm->ieeeAddr);
	
		printf("Alarm panic command for %s is %s\n", ServerIP, txBuffer);
	}
	else
		printf("Alarm not found\n");

	if (update)
	{
		printf("update hardware set (alrmaddr, alrmieee) = (x'%s'::int,'%s') where unit=%d", alarm->shrtAddr, alarm->ieeeAddr, UnitNum);
	}

	zbSocketClose(skt);

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
	int rc=0, rval;
	unitInfoRec * unitRec;

	unitRec = (unitInfoRec *) unit;
	if (scan && 0==strncmp(cBuffer, "020A8B", 6))
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
			rval = LQI;
		}
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
	else
		dispBuffer(cBuffer, cLen, "\n");
	return 0;
}

/*! \brief Signal handler
	\param 	signum	Signal number
*/
void sighandle(int signum)
{
	printf("Shutting down after signal %d\n", signum);
	exit(1);
}

/*! \brief Obtain gatwaway info
	\param	skt		Socket ID
	\param	txBuffer	Transmit buffer
	\param	rxBuffer	Receive buffer
	\param	sysDev		Gateway info
	\param	rdTimeout	Timeout in seconds
*/
int GatewayInfo(int skt, 
		char * txBuffer, 
		char * rxBuffer, 
		DeviceInfo * sysDev, 
		int rdTimeout)
{
	int resp;
	int usec = 2e6;

	strcpy(txBuffer, PktSysInfo);
	if (trace) printf("> System Get Device Information: %s\n", txBuffer);
	zbSendString(skt, txBuffer);

	resp = zbSendGetReply(skt, rxBuffer, rdTimeout);
	readBuffer(skt, NULL, rxBuffer, resp, usec);
	
	if (strncmp(rxBuffer, "021014", 6))
	{
		fprintf(stderr, "Invalid reply for System Get Device Information: %s\n", rxBuffer);
		return -1;
	}
	if (strncmp(&rxBuffer[8], "00", 2))
	{
		fprintf(stderr, "Invalid status for System Get Device Information: %c%c\n", rxBuffer[8], rxBuffer[9]);
		return -1;
	}
	strncpy(sysDev->shrtAddr, &rxBuffer[26], 4);
	sysDev->shrtAddr[4] = 0;
	sysDev->loctn = 0;
	
	strncpy(sysDev->ieeeAddr, &rxBuffer[10], 16);
	sysDev->ieeeAddr[16] = 0;
	if (trace) printf("\tIEEE address for System Device: %s\n", sysDev->ieeeAddr);

	return 0;
}

/*! \brief Obtain gatwaway info
	\param	skt		Socket ID
	\param	txBuffer	Transmit buffer
	\param	rxBuffer	Receive buffer
	\param	sysDev		System device (coordinator) info
	\param	rdTimeout	Timeout in seconds
 */
int CoordinatorInfo(int skt,
		    char * txBuffer, 
		    char * rxBuffer, 
		    DeviceInfo * coordDev, 
		    int rdTimeout)
{
	int resp;
	int usec = 2e6;

	strcpy(coordDev->shrtAddr, "0000");

	strcpy(txBuffer, "020A03060200000100000C");
	if (trace) printf("> Get coordinator IEEE Address: %s\n", txBuffer);
	zbSendString(skt, txBuffer);

	resp = zbSendGetReply(skt, rxBuffer, rdTimeout);
	readBuffer(skt, NULL, rxBuffer, resp, usec);
	
	if (strncmp(rxBuffer, "020A81", 6))
	{
		fprintf(stderr, "Invalid reply for IEEE response: %s\n", rxBuffer);
		return -1;
	}
	if (strncmp(&rxBuffer[26], "00", 2))
	{
		fprintf(stderr, "Invalid status for IEEE response: %c%c\n", rxBuffer[26], rxBuffer[27]);
		return -1;
	}
	
	strncpy(coordDev->ieeeAddr, &rxBuffer[28], 16);
	coordDev->ieeeAddr[16] = 0;
	if (trace) printf("\tIEEE address for coordinator: %s\n", coordDev->ieeeAddr);

	return 0;
}

/*! \brief Obtain gatwaway info
	\param	version		Firmware version number
	\param	skt		Socket ID
	\param	txBuffer	Transmit buffer
	\param	rxBuffer	Receive buffer
	\param	Dev		Device info
	\param	rdTimeout	Timeout in seconds
 */
int DeviceList(int version, 
	       int skt, 
	       char * sAddr,
	       char * txBuffer, 
	       char * rxBuffer, 
	       DeviceInfo ** Dev, 
	       int devCount,
	       int rdTimeout)
{
	int resp, dFound;
	char * lqiBuff; 
	char * lqiItem; 
	char countStr[8];
	char zigBeePktStr[256];
	int rc, i, j, count, lqilen;
	int usec = 2e6;

	/* Send Manage LQI request to coordinator */
	sprintf(zigBeePktStr, "020A0F0402%4s00", sAddr);
	rc = zbSendCheck(zigBeePktStr, txBuffer);
	if (rc)
	{
		printf("Packet check failed\n");
		return -1;
	}
	if (trace) printf("> Manage LQI to device %s: %s\n", sAddr, txBuffer);
	zbSendString(skt, txBuffer);

	resp = zbSendGetReply(skt, rxBuffer, rdTimeout);
	readBuffer(skt, NULL, rxBuffer, resp, usec);
	
	if (strncmp(rxBuffer, "020A8B", 6) || strncmp(&rxBuffer[12], "00", 2))
	{
		fprintf(stderr, "Invalid reply for Manage LQI response: %s\n", rxBuffer);
		return -1;
	}
	if (trace) printf("\tValid reply for Manage LQI response: %s\n", rxBuffer);
	
	lqiBuff = &rxBuffer[14];
	
	switch (version)
	{
		case 0 : lqilen = 12; break;
		case 1 : lqilen = 24; break;
		default : lqilen = 24;
	}

	if (trace) printf("\tEntries %c%c, ", lqiBuff[0], lqiBuff[1]);
	if (trace) printf("\tStartIndex %c%c, ", lqiBuff[2], lqiBuff[3]);
	strncpy(countStr, &lqiBuff[4], 2);
	countStr[2] = 0;
	count = strtol(countStr, NULL, 16);
	if (trace) printf("\tCount %d\n", count);
	
	for (i=0; i<count; i++) 
	{
		lqiItem = &lqiBuff[6+i*lqilen];
		dFound = 0;
		for (j=0; j<devCount; j++)
		{
			if (!Dev[j]) break;
			if (! strncmp(Dev[j]->shrtAddr, &lqiItem[4], 4))
			{
				if (trace) printf("\tDevice %s already in list\n", Dev[j]->shrtAddr);
				dFound = 1;
				break;
			}
		}
		if (dFound) continue;
		
		Dev[i] = malloc(sizeof(DeviceInfo));
		
		switch(version)
		{
			case 0 :
				sprintf(Dev[i]->shrtAddr, "%c%c%c%c", lqiItem[4], lqiItem[5], lqiItem[6], lqiItem[7]);
				break;
			case 1 :
				sprintf(Dev[i]->shrtAddr, "%c%c%c%c", lqiItem[16], lqiItem[17], lqiItem[18], lqiItem[19]);
				break;
			default :
				;
		}
		
		for (j=0; j<MAX_ENDPOINTS; j++)
		{
			Dev[i]->endPoint[j][0] = 0;
			Dev[i]->attributes[j][0] = 0;
		}
		Dev[i]->ieeeAddr[0] = 0;
	}

	return count;
}

/*! \brief Get active endpoints
	\param	skt		Socket ID
	\param	idx		Device index number
	\param	txBuffer	Transmit buffer
	\param	rxBuffer	Receive buffer
	\param	sysDev		Device info
	\param	rdTimeout	Timeout in seconds
 */
int GetEndPoints(int skt, 
		 int idx, 
		 char * txBuffer, 
		 char * rxBuffer, 
		 DeviceInfo * Dev, 
		 int rdTimeout)
{
	int rc, resp;
	int i, n;
	char nStr[8];
	int usec = 2e6;

	if (trace) printf("\n\n");
	sprintf(rxBuffer, "020A070602%s%s00", Dev->shrtAddr, Dev->shrtAddr);
	rc = zbSendCheck(rxBuffer, txBuffer);
	if (rc)
	{
		fprintf(stderr, "Packet check failed\n");
		return -1;
	}

	if (trace) printf("> Get EndPoints %d: %s\n", ++idx, txBuffer);
	zbSendString(skt, txBuffer);

	resp = zbSendGetReply(skt, rxBuffer, rdTimeout);
	if (resp < 0) return -1;

	if (strncmp(rxBuffer, "020A85", 6))
	{
		printf("Invalid Get EndPoints response: %s\n", txBuffer);
		return -1;
	}

	readBuffer(skt, NULL, rxBuffer, resp, usec);
	
	strncpy(nStr, &rxBuffer[18], 2);
	nStr[2] = 0;
	n = strtol(nStr, NULL, 16);
	if (trace) printf("\tActiveEndpointCount %d, ActiveEndpointList ", n);
	for (i=0; i<n; i++) 
	{
		if (trace) printf("%s%c%c, ", i?" ":"", rxBuffer[20+i*2], rxBuffer[21+i*2]);
		strncpy(Dev->endPoint[i], &rxBuffer[20+i*2], 2);
		Dev->endPoint[i][2] = 0;
	}
	if (trace) printf("\n");
	
	Dev->ieeeAddr[16] =0;
	
	return 0;
}

/*! \brief Read Attributes
	\param	skt		Socket ID
	\param	idx		Device index number
	\param	txBuffer	Transmit buffer
	\param	rxBuffer	Receive buffer
	\param	sysDev		Device info
	\param	rdTimeout	Timeout in seconds
 */
int ReadAttributes(int skt, 
		   int idx, 
		   char * txBuffer, 
		   char * rxBuffer, 
		   DeviceInfo * Dev, 
		   int rdTimeout)
{
	int rc, resp;
	int i , n, num, dlen, sp, sts, dtype;
	char nan[64];
	int usec = 2e6;
	
	if (trace) printf("\n\n");
	if (Dev->endPoint[0][0] == 0)
	{
		printf("No endpoint for device %s (%s)\n", Dev->ieeeAddr, Dev->shrtAddr);
		return 1;
	}

	sprintf(rxBuffer, "020D000902%s%s0004010000", Dev->shrtAddr, Dev->endPoint[0]);
	rc = zbSendCheck(rxBuffer, txBuffer);
	if (rc)
	{
		fprintf(stderr, "Packet check failed\n");
		return -1;
	}

	if (trace) printf("> Read Attribute %d: %s\n", ++idx, txBuffer);
	zbSendString(skt, txBuffer);

	rxBuffer[0] = 0;
	resp = zbSendGetReply(skt, rxBuffer, rdTimeout);
	if (resp < 0) return -1;

	readBuffer(skt, NULL, rxBuffer, resp, usec);
	
	if (strncmp(rxBuffer, "020D01", 6))
	{
		fprintf(stderr, "Invalid Read Attribute response: %s\n", rxBuffer);
		return -1;
	}
	
	strcpy(Dev->attributes[0], &rxBuffer[18]);
	n = strlen(Dev->attributes[0]);
	Dev->attributes[0][n-3] = 0;

	strncpy(nan, &rxBuffer[18], 2);
	nan[2] = 0;
	num = (int) strtol(nan, NULL, 16);
	if (trace) printf("\tNumber %02d, ", num);
	sp = 20;
	for (n=0; n<num; n++)
	{
		if (trace) printf("\tAttributeId%d %c%c%c%c, ", n+1, rxBuffer[sp], rxBuffer[sp+1], rxBuffer[sp+2], rxBuffer[sp+3]);
		sts = Status(&rxBuffer[sp+4]);
		if (sts == 0)
		{
			dlen = dataType(&rxBuffer[sp+6], nan, &dtype);
			if (dlen == -8)
			{
				dispIeeeAddr(&rxBuffer[sp+8], 1);
				sp += 16;
			}
			else
			{
				if (trace) printf(" %d bytes ", dlen);
				dlen *= 2;
				for (i=0; i<dlen; i++)
					printf("%c", rxBuffer[sp+8+i]);
				sp += dlen + 8;
			}
			printf(", ");
		}
	}
	printf("\n");
	if (trace) printf("\n");

	return 0;
}

