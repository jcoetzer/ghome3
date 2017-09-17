/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: gHomeSend.c,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.7  2011/02/24 21:01:22  johan
 * Populate hwinfo table
 *
 * Revision 1.6  2011/02/23 15:48:48  johan
 * Reset alarm log
 *
 * Revision 1.5  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.4  2011/02/15 15:03:36  johan
 * Remove command parameter in message read
 *
 * Revision 1.3  2011/02/10 11:09:42  johan
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

#ifdef WIN32
#include "popt.h"
#else
#include <popt.h>
#endif

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

int disarm = 0;
int arm = 0;
int day = 0;
int nite = 0;
int ping = 0;
int burglar = 0;
int fire = 0;
int panic = 0;
int armmode = 0;
int lqi = 0;
int ext = 0;
int squawk = 0;
int sysinfo = 0;
int reset = 0;
int resetlog = 0;
int getieee = 0;
int getaddr = 0;
int getendp = 0;
int txon = 0;
int txoff = 0;
int txtog = 0;
int heartbeat = 0;
int recent = 0;

int Count = 0;

int ShowVersion = 0;
int Timeout = 0;
int HeartBeatPrd = 0;
int HeartBeatLoss = 0;
int StartIndex = 0;
int SleepTime = 2000;
int JoinTime = 0;

unsigned short ServerPort = 0;     /* Gateway port */
char * ServerIP = NULL;             /* Gateway IP address (dotted quad) */
char * zigBeePktStr = NULL;
char * zigBeeCmd = NULL;
char * zigBeeData = NULL;
char * ShortAddress = NULL;
char * IeeeAddress = NULL;
char * EndPoint = NULL;
char * ZigbeeFileName = NULL;
char * ClusterID = NULL;
char * UserDescrptn = NULL;

/*! \struct Command line parsing data structure */
struct poptOption optionsTable[] =
{
	{"cmd", 	'C', POPT_ARG_STRING, 	&zigBeeCmd, 0, "Zigbee command",  "<string>"},
	{"data", 	'D', POPT_ARG_STRING, 	&zigBeeData, 0, "Zigbee data",  "<string>"},
	{"endpoint", 	'E', POPT_ARG_STRING, 	&EndPoint, 0, "Endpoint",  "<string>"},
	{"file", 	'F', POPT_ARG_STRING, 	&ZigbeeFileName, 0, "Zigbee packet file name",  "<string>"},
	{"gateway", 	'G', POPT_ARG_STRING, 	&ServerIP, 0,  "Zigbee gateway",  "<ip_address>"},
	{"hbmins",	'H', POPT_ARG_INT, 	&HeartBeatPrd, 0, "Set heartbeat (in minutes)", "<integer>"},
	{"ieee", 	'I', POPT_ARG_STRING, 	&IeeeAddress, 0, "IEEE address",  "<ieee_addr>"},
	{"join", 	'J', POPT_ARG_INT, 	&JoinTime, 0, "Permit join", "<seconds>"},
	{"startidx", 	'K', POPT_ARG_INT, 	&StartIndex, 0, "Set start index",  "<index>"},
	{"hbloss", 	'L', POPT_ARG_INT, 	&HeartBeatLoss, 0, "Max Loss Heartbeat",  "<integer>"},
	{"count", 	'N', POPT_ARG_INT, 	&Count, 0,  "Number if times to poll (default is 0)", "<integer>"},
	{"port", 	'P', POPT_ARG_INT, 	&ServerPort, 0,  "Gateway port(default is 5000)", "<integer>"},
	{"descrptn", 	'Q', POPT_ARG_STRING, 	&UserDescrptn, 0, "User description",  "<string>"},
	{"sleep", 	'S', POPT_ARG_INT, 	&SleepTime, 0, "Sleep time in milliseconds (default is 2000)",  "<integer>"},
	{"timeout", 	'T', POPT_ARG_INT, 	&Timeout, 0,  "Receive timeout period (default is 0)", "<integer>"},
	{"cluster", 	'U', POPT_ARG_STRING, 	&ClusterID, 0, "Cluster ID",  "<cluster_id>"},
	{"version", 	'V', POPT_ARG_NONE, 	&ShowVersion, 0, "Display version information", NULL},
	{"nwk", 	'W', POPT_ARG_STRING, 	&ShortAddress, 0, "Network address",  "<nwk_addr>"},
	{"pkt", 	'Z', POPT_ARG_STRING, 	&zigBeePktStr, 0,  "Zigbee packet",  "<string>"},
	{"arm", 	'a', POPT_ARG_NONE, 	&arm, 0, "Arm alarm", NULL},
	{"day", 	'b', POPT_ARG_NONE, 	&day, 0, "Arm day zone", NULL},
	{"nite", 	'c', POPT_ARG_NONE, 	&nite, 0, "Arm night zone", NULL},
	{"disarm", 	'd', POPT_ARG_NONE, 	&disarm, 0, "Disarm alarm", NULL},
	{"getendp", 	'e', POPT_ARG_NONE, 	&getendp, 0, "Get active endpoints",  NULL},
	{"off",	 	'f', POPT_ARG_NONE, 	&txoff, 0, "Send power off", NULL},
	{"squawk", 	'g', POPT_ARG_NONE, 	&squawk, 0, "Sound squawk", NULL},
	{"getheart", 	'h', POPT_ARG_NONE, 	&heartbeat, 0, "Read heartbeat", NULL},
	{"getieee", 	'i', POPT_ARG_NONE, 	&getieee, 0, "Get IEEE address",  NULL},
	{"resetlog",	'j', POPT_ARG_NONE, 	&resetlog, 0, "Reset alarm log", NULL},
	{"recent", 	'k', POPT_ARG_NONE, 	&recent, 0, "Get recent alarm info", NULL},
	{"lqi", 	'l', POPT_ARG_NONE, 	&lqi, 0, "Get Link Quality info", NULL},
	{"mode", 	'm', POPT_ARG_NONE, 	&armmode, 0, "Get Arm Mode", NULL},
	{"on",	 	'n', POPT_ARG_NONE, 	&txon, 0, "Send power on", NULL},
	{"toggle", 	'o', POPT_ARG_NONE, 	&txtog, 0, "Send power toggle", NULL},
	{"ping", 	'p', POPT_ARG_NONE, 	&ping, 0, "System ping", NULL},
	{"sys", 	'q', POPT_ARG_NONE, 	&sysinfo, 0, "Get System Device info", NULL},
	{"noreply", 	'r', POPT_ARG_NONE, 	&noreply, 0, "Do not wait for reply", NULL},
	{"getaddr", 	's', POPT_ARG_NONE, 	&getaddr, 0, "Get network (short) address",  NULL},
 	{"trace", 	't', POPT_ARG_NONE, 	&trace, 0, "Trace/debug output", NULL},
 	{"reset", 	'u', POPT_ARG_NONE, 	&reset, 0, "Reset to factory defaults", NULL},
	{"verbose", 	'v', POPT_ARG_NONE, 	&verbose, 0, "Verbose output", NULL},
	{"ext", 	'w', POPT_ARG_NONE, 	&ext, 0, "Get System Extended Address", NULL},
	{"alarm", 	'x', POPT_ARG_NONE, 	&burglar, 0, "Sound burglar alarm", NULL},
	{"fire", 	'y', POPT_ARG_NONE, 	&fire, 0, "Sound fire alarm", NULL},
	{"panic", 	'z', POPT_ARG_NONE, 	&panic, 0, "Sound panic alarm", NULL},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

/* Local functions */
void sighandle(int signum);
int sendPollReply(void * unit, int skt, char * txBuffer, char * rxBuffer, int rxSize,
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
	int i, skt, rc;
	poptContext optcon;        /*!< Context for parsing command line options */
	char pktBuffer[512];
	char txBuffer[512];
	char rxBuffer[512];
	char zbValStr[16];

	signal(SIGPIPE, sighandle);

	optcon = poptGetContext(argv[0],argc,(const char **)argv,optionsTable,0);
	rc = poptGetNextOpt(optcon);
	if (rc < -1)
	{
		printf("Could not read command line\n");
		printf("%s: %s - %s\n", basename(argv[0]), poptBadOption(optcon,0), poptStrerror(rc));
		poptPrintUsage(optcon, stderr, 0);
		return -1;
	}
	if (ShowVersion) displayVersion(argv[0]);
	
	if (! ServerPort) ServerPort = ZIGBEE_GATEWAY_PORT;

	if (zigBeeCmd && zigBeeData)
	{
		sprintf(pktBuffer, "02%4s%02X%s", zigBeeCmd, (int) strlen(zigBeeData)/2, zigBeeData);
		zigBeePktStr = pktBuffer;
	}
	else if (zigBeeCmd)
	{
		helpDisplay(zigBeeCmd);
		return 0;
	}
	else if (! ServerIP)
	{
		printf("No gateway specified\n");
		poptPrintUsage(optcon, stderr, 0);
		poptFreeContext(optcon);
		return -1;
	}
	else if (JoinTime)
	{
		if (trace) printf("\tSet join on for %d seconds\n", JoinTime);
		if (JoinTime > 0xFF)
		{
			printf("Invalid join time %d (must be %d or less)\n", JoinTime, 0xFF);
			poptFreeContext(optcon);
			return -1;
		}
		snprintf(rxBuffer, sizeof(rxBuffer), "020A160502FFFF%02X00", JoinTime);
		if (trace) printf("\tPermit join (%d seconds) %s\n", JoinTime, zigBeePktStr);
		zigBeePktStr = rxBuffer;
	}
	else if (arm)
	{
		zigBeePktStr = PktArm;
		if (trace) printf("\tSend arm %s\n", zigBeePktStr);
	}
	else if (day)
	{
		zigBeePktStr = PktArmDay;
		if (trace) printf("\tArm day zone %s\n", zigBeePktStr);
	}
	else if (nite)
	{
		zigBeePktStr = PktArmNite;
		if (trace) printf("\tArm night zone%s\n", zigBeePktStr);
	}
	else if (disarm)
	{
		zigBeePktStr = PktDisarm;
		if (trace) printf("\tSend disarm %s\n", zigBeePktStr);
	}
	else if (recent)
	{
		zigBeePktStr = PktGetRecentAlrm;
		if (trace) printf("\tGet recent alarm %s\n", zigBeePktStr);
	}
	else if (HeartBeatPrd)
	{
		if (trace) printf("\tSet heartbeat to %d seconds\n", HeartBeatPrd);
		snprintf(zbValStr, sizeof(zbValStr), "%04X", HeartBeatPrd);
		if (! HeartBeatLoss) HeartBeatLoss = 3;
		if (ShortAddress) 
			snprintf(rxBuffer, sizeof(rxBuffer), "020C080902%4s010501%c%c%c%c%02X",
				 ShortAddress, zbValStr[2], zbValStr[3], zbValStr[0], zbValStr[1], HeartBeatLoss);
		else 
			snprintf(rxBuffer, sizeof(rxBuffer), "020C0809020000010501%c%c%c%c%02X",
				 zbValStr[2], zbValStr[3], zbValStr[0], zbValStr[1], HeartBeatLoss);
		zigBeePktStr = rxBuffer;
	}
	else if (heartbeat)
	{
		zigBeePktStr = PktReadHeartbeat;
		if (trace) printf("\tSend read heartbeat %s\n", zigBeePktStr);
	}
	else if (reset)
	{
		if (trace) printf("\tSend reset to %s\n", ShortAddress);
		if (! ShortAddress)
		{
			fprintf(stderr, "No short address specified\n");
			return -1;
		}
		else if (strlen(ShortAddress) != 4)
		{
			fprintf(stderr, "Invalid short address %s\n", ShortAddress);
			return -1;
		}
		if (! ClusterID)
		{
			fprintf(stderr, "No Cluster ID specified\n");
			return -1;
		}
		else if (strlen(ClusterID) != 4)
		{
			fprintf(stderr, "Invalid Cluster ID %s\n", ClusterID);
			return -1;
		}
		if (EndPoint && strlen(EndPoint) != 2)
		{
			fprintf(stderr, "Invalid EndPoint %s\n", EndPoint);
			return -1;
		}
		if (! EndPoint) snprintf(rxBuffer, sizeof(rxBuffer), "020C000602%s%s%s", ShortAddress, "01", ClusterID);
		else snprintf(rxBuffer, sizeof(rxBuffer), "020C000602%s%2s%s", ShortAddress, EndPoint, ClusterID);
		zigBeePktStr = rxBuffer;
	}
	else if (resetlog)
	{
		if (trace) printf("\tSend reset alarm log to %s\n", ShortAddress);
		if (! ShortAddress)
		{
			fprintf(stderr, "No short address specified\n");
			return -1;
		}
		else if (strlen(ShortAddress) != 4)
		{
			fprintf(stderr, "Invalid short address %s\n", ShortAddress);
			return -1;
		}
		if (! ClusterID)
		{
			fprintf(stderr, "No Cluster ID specified\n");
			return -1;
		}
		else if (strlen(ClusterID) != 4)
		{
			fprintf(stderr, "Invalid Cluster ID %s\n", ClusterID);
			return -1;
		}
		if (EndPoint && strlen(EndPoint) != 2)
		{
			fprintf(stderr, "Invalid EndPoint %s\n", EndPoint);
			return -1;
		}
		if (! EndPoint) snprintf(rxBuffer, sizeof(rxBuffer), "020C030602%s%s%s", ShortAddress, "01", ClusterID);
		else snprintf(rxBuffer, sizeof(rxBuffer), "020C030602%s%2s%s", ShortAddress, EndPoint, ClusterID);
		zigBeePktStr = rxBuffer;
	}
	else if (squawk)
	{
		if (trace) printf("\tSend squawk to %s\n", ShortAddress);
		if (! ShortAddress)
		{
			fprintf(stderr, "No short address specified\n");
			return -1;
		}
		else if (strlen(ShortAddress) != 4)
		{
			fprintf(stderr, "Invalid short address %s\n", ShortAddress);
			return -1;
		}
		snprintf(rxBuffer, sizeof(rxBuffer), "020C010702%s0A050200", ShortAddress);
		zigBeePktStr = rxBuffer;
		if (trace) printf("\tSend Zigbee command to sound squawk %s\n", zigBeePktStr);
	}
	else if (UserDescrptn)
	{
		if (trace) printf("\tSet user description to %s\n", UserDescrptn);
		if (! ShortAddress)
		{
			fprintf(stderr, "No short address specified\n");
			return -1;
		}
		else if (strlen(UserDescrptn) > 16)
		{
			fprintf(stderr, "User description '%s' longer than 16 bytes\n", UserDescrptn);
			return -1;
		}
		else if (strlen(ShortAddress) != 4)
		{
			fprintf(stderr, "Invalid short address %s\n", ShortAddress);
			return -1;
		}
		rc = strlen(UserDescrptn);
		/* * 020A131102B198B1980B4C6976696E6720726F6F6D0D       (44) 
		 * req 0a13 ZDO, Len 17, User Description Set request: 
		 * AddrMode Short Address 02, DstAddr B198, NWKAddrOfInterest B198, DescLen 11, Descriptor 'Living room', SecuritySuite 72, FCS 0D 
		 */
		snprintf(rxBuffer, sizeof(rxBuffer), "020A13%02X%s%s", rc, ShortAddress, ShortAddress);
		for (i=0; i<rc; i++)
		{
			sprintf(zbValStr, "%02X", UserDescrptn[i]);
			strcat(rxBuffer, zbValStr);
		}
		strcat(rxBuffer, "72");
		zigBeePktStr = rxBuffer;
		if (trace) printf("\tSend Zigbee command to set user description %s\n", zigBeePktStr);
	}
	else if (txon)
	{
		if (trace) printf("\tSend on command to %s\n", ShortAddress);
		if (! ShortAddress)
		{
			fprintf(stderr, "No short address specified\n");
			return -1;
		}
		else if (strlen(ShortAddress) != 4)
		{
			fprintf(stderr, "Invalid short address %s\n", ShortAddress);
			return -1;
		}
		if (! EndPoint)
			snprintf(rxBuffer, sizeof(rxBuffer), "020C010602%s%s0006", ShortAddress, "0A");
		else if (strlen(EndPoint) != 2)
		{
			fprintf(stderr, "Invalid EndPoint %s\n", EndPoint);
			return -1;
		}
		else
			snprintf(rxBuffer, sizeof(rxBuffer), "020C010602%s%s0006", ShortAddress, EndPoint);
		zigBeePktStr = rxBuffer;
		if (trace) printf("\tSend on %s\n", zigBeePktStr);
	}
	else if (txoff)
	{
		if (trace) printf("\tSend off command to %s\n", ShortAddress);
		if (! ShortAddress)
		{
			fprintf(stderr, "No short address specified\n");
			return -1;
		}
		else if (strlen(ShortAddress) != 4)
		{
			fprintf(stderr, "Invalid short address %s\n", ShortAddress);
			return -1;
		}
		if (! EndPoint)
			snprintf(rxBuffer, sizeof(rxBuffer), "020C000602%s%s0006", ShortAddress, "0A");
		else if (strlen(EndPoint) != 2)
		{
			fprintf(stderr, "Invalid EndPoint %s\n", EndPoint);
			return -1;
		}
		else
			snprintf(rxBuffer, sizeof(rxBuffer), "020C000602%s%s0006", ShortAddress, EndPoint);
		zigBeePktStr = rxBuffer;
		if (trace) printf("\tSend off %s\n", zigBeePktStr);
	}
	else if (txtog)
	{
		if (trace) printf("\tSend toggle command to %s\n", ShortAddress);
		if (! ShortAddress)
		{
			fprintf(stderr, "No short address specified\n");
			return -1;
		}
		else if (strlen(ShortAddress) != 4)
		{
			fprintf(stderr, "Invalid short address %s\n", ShortAddress);
			return -1;
		}
		if (! EndPoint)
			snprintf(rxBuffer, sizeof(rxBuffer), "020C020602%s%s0006", ShortAddress, "0A");
		else if (strlen(EndPoint) != 2)
		{
			fprintf(stderr, "Invalid EndPoint %s\n", EndPoint);
			return -1;
		}
		else
			snprintf(rxBuffer, sizeof(rxBuffer), "020C020602%s%s0006", ShortAddress, EndPoint);
		zigBeePktStr = rxBuffer;
		if (trace) printf("\tSend toggle %s\n", zigBeePktStr);
	}
	else if (ping)
		zigBeePktStr = PktSysPing;
	else if (ext)
		zigBeePktStr = PktSysExt;
	else if (armmode)
		zigBeePktStr = PktGetArmMode;
	else if (sysinfo)
		zigBeePktStr = PktSysInfo;
	else if (burglar)
		zigBeePktStr = PktSoundAlarm;
	else if (fire)
		zigBeePktStr = PktSoundFire;
	else if (panic)
		zigBeePktStr = PktSoundPanic;

	skt = zbSendInit(ServerIP, ServerPort, Timeout);
	if (skt <= 0)
	{
		printf("Initialize failed\n");
		return -1;
	}
	
	poptFreeContext(optcon);
	rc = 0;
	if (lqi)
	{
		if (! ShortAddress)
		{
			fprintf(stderr, "No short address specified!\n");
			return -1;
		}
		if (strlen(ShortAddress) != 4)
		{
			fprintf(stderr, "Invalid short address %s\n", ShortAddress);
			return -1;
		}
		snprintf(rxBuffer, sizeof(rxBuffer), "020A0F0402%s%02X", ShortAddress, StartIndex);
		zigBeePktStr = rxBuffer;
		if (trace) printf("\tGet LQI for %s : %s\n", ShortAddress, zigBeePktStr);
	}
	else if (getieee)
	{
		if (! ShortAddress)
		{
			fprintf(stderr, "No short address specified!\n");
			return -1;
		}
		if (strlen(ShortAddress) != 4)
		{
			fprintf(stderr, "Invalid short address %s\n", ShortAddress);
			return -1;
		}
		snprintf(rxBuffer, sizeof(rxBuffer), "020A030602%s000000", ShortAddress);
		zigBeePktStr = rxBuffer;
		if (trace) printf("\tGet IEEE address for %s : %s\n", ShortAddress, zigBeePktStr);
	}
	else if (getendp)
	{
		if (! ShortAddress)
		{
			fprintf(stderr, "No short address specified!\n");
			return -1;
		}
		if (strlen(ShortAddress) != 4)
		{
			fprintf(stderr, "Invalid short address %s\n", ShortAddress);
			return -1;
		}
		snprintf(rxBuffer, sizeof(rxBuffer), "020A070602%s%s00", ShortAddress, ShortAddress);
		zigBeePktStr = rxBuffer;
		if (trace) printf("\tGet endpoint for %s : %s\n", ShortAddress, zigBeePktStr);
	}
	else if (getaddr)
	{
		if (! IeeeAddress)
		{
			fprintf(stderr, "No IEEE address specified!\n");
			return -1;
		}
		if (strlen(IeeeAddress) != 16)
		{
			fprintf(stderr, "Invalid IEEE address '%s'\n", IeeeAddress);
			return -1;
		}
		snprintf(rxBuffer, sizeof(rxBuffer), "020A020C03%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c000000", 
			 IeeeAddress[14], IeeeAddress[15], IeeeAddress[12], IeeeAddress[13], 
    			 IeeeAddress[10], IeeeAddress[11], IeeeAddress[8], IeeeAddress[9],
			 IeeeAddress[6], IeeeAddress[7], IeeeAddress[4], IeeeAddress[5], 
    			 IeeeAddress[2], IeeeAddress[3], IeeeAddress[0], IeeeAddress[1]);
		zigBeePktStr = rxBuffer;
		if (trace) printf("\tGet short address for %s : %s\n", IeeeAddress, zigBeePktStr);
	}
	else if (ZigbeeFileName)
	{
		rc = sendReadFile(skt, ZigbeeFileName, txBuffer, rxBuffer, sizeof(txBuffer), SleepTime, Timeout);
		return rc;
	}
	else if (! zigBeePktStr)
	{
		fprintf(stderr, "No packet string specified!\n");
		return -1;
	}
	if (trace) printf("\tSend %s\n", zigBeePktStr);

	if (Count)
	{
		if (trace) printf("\tPoll %d\n", Count);
		strcpy(txBuffer, zigBeePktStr);
		sendPollReply(NULL, skt, txBuffer, rxBuffer, sizeof(txBuffer), SleepTime, Timeout);
	}
	else
	{
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
		if (noreply);
		else 
		{
			zbSendGetReply(skt, rxBuffer, Timeout);
			dispBuffer(rxBuffer, strlen(rxBuffer), "\n");
		}
	}
	
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
int sendPollReply(void * unit, 
		  int skt, 
		  char * txBuffer, 
		  char * rxBuffer, 
		  int rxSize, 
		  int sleepTime,
		  int rdTimeout)
{
	int i, rc;
	int resp;
	int usec = sleepTime * 1e3;

	if (verbose) dispBuffer(txBuffer, strlen(txBuffer), "\n");
	rc = zbSocketWrite(skt, txBuffer, strlen(txBuffer));
	if (rc < 0)
	{
		printf("Packet send failed\n");
		return -1;
	}

	i = Count;
	if (trace) printf("\tPoll and wait for Reply %d\n", i);
	while(i)
	{
		if (trace) printf("\tPing %d\n", i);
		/* Wait for response */
		memset(rxBuffer, 0, rxSize);
		resp = zbSocketReadPacket(skt, rxBuffer);
		if (resp == 0);
		else if (resp <= -1)
		{
			if (trace) fprintf(stderr, "Zigbee connection read error\n");
			return -1;
		}
		else
		{
			readBuffer(skt, NULL, rxBuffer, resp, usec);
			dispBuffer(rxBuffer, strlen(rxBuffer), "\n");
		}
		if(zbSocketSendPing(skt) < 0)
		{
			if (trace) fprintf(stderr, "Zigbee connection ping error\n");
			return -1;
		}
		usleep(usec);
		i--;
	}
	return 0;
}

int sendReadFile(int skt, 
		 char * input_file_name, 
		 char * txBuffer, 
		 char * rxBuffer, 
		 int rxSize, 
		 int sleepTime,
		 int rdTimeout)
{
	FILE * input_file;
	char zig_buff[1024];
	int i, rc;
	int resp;
	int usec = sleepTime * 1e3;

	/* No file name means log file is not read - not an error
	*/
	if (NULL == input_file_name) return 0;
	if (input_file_name[0] == 0) return 0;

	if (verbose) printf("Reading log file '%s'\n", input_file_name);
	input_file = fopen(input_file_name, "r");
	if (NULL == input_file)
	{
	    fprintf(stderr, "Could not read Zigbee script file %s: %s\n",                 /* Error */
		  input_file_name, strerror(errno));
	    return -1;
	}

	/* Read Zigbee script file */
	fgets(zig_buff, sizeof(zig_buff), input_file);
	while (! feof(input_file))
	{
		rc = strlen(zig_buff);
		zig_buff[--rc] = 0;
		if (trace) printf("\t%s\n", zig_buff);
		/* Ignore lines that do not start with '02' */
		if (0 == strncmp(zig_buff, "02", 2))
		{
			rc = zbSendCheck(zig_buff, txBuffer);
			if (rc)
			{
				printf("Packet check failed\n");
				return -1;
			}
			/* Send command to Zigbee adapter */
			if (verbose) dispBuffer(txBuffer, strlen(txBuffer), "\n");
			rc = zbSocketWrite(skt, txBuffer, strlen(txBuffer));
			if (rc < 0)
			{
				printf("Packet send failed\n");
				return -1;
			}
			usleep(usec);
			/* Wait for response */
			memset(rxBuffer, 0, rxSize);
			resp = zbSocketReadPacket(skt, rxBuffer);
			if (resp == 0) printf("\n");
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
			/*? Send ping to keep connection alive 
			if( zbSocketSendPing(skt) )
			{
				if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
				return -1;
			}
			?*/
		}
		fgets(zig_buff, sizeof(zig_buff), input_file);
	}
	fclose(input_file);

	i = Count;
	if (trace) printf("\tPoll and wait for Reply %d\n", i);
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
