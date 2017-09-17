/*! \file zbPoll.c 
	\brief Poll Zigbee data packets
	\author Intelipower

Remote command of ZigBee basics
*/

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:45 $
 * $Revision: 1.13 $
 * $State: Exp $
 *
 * $Log: zbPoll.c,v $
 * Revision 1.13  2011/11/19 16:31:45  johan
 * Ping panel
 *
 * Revision 1.12  2011/06/15 22:59:04  johan
 * Send message when alarm goes off
 *
 * Revision 1.11  2011/06/13 21:57:26  johan
 * Update
 *
 * Revision 1.10  2011/06/13 21:51:40  johan
 * Fix bug
 *
 *
 *
 *
 */

#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#include "zbSocket.h"
#include "zbSend.h"
#include "zbPacket.h"
#include "zbData.h"
#include "zbDisplay.h"
#include "zbState.h"
#include "zbPoll.h"
#include "updateDb.h"
#include "unitDb.h"
#include "gHomeLogger.h"
#include "gHomeConf.h"

extern int trace;
extern int verbose;
extern int Heartbeat;

time_t AlarmActive = 0;

static int zbPollUpdatePir(int skt, unitInfoRec * unit, char * rxBuff, int usec);
static int zbPollUpdateHeartbeat(int skt, unitInfoRec * unit, char * rxBuff);
static int zbPollCheckPirs(unitInfoRec * unit, int zigbeeSocket, char * cBuffer, int forceUpd, int uSleepTime);
static int zbPollGetIeee(int zigbeeSocket, unitInfoRec * unit, int shortAddress, char * rxBuff, int usec);
static int zbPollGetUser(int zigbeeSocket, unitInfoRec * unit, int shortAddress, char * cBuffer, int usec);
static int zbPollGetShort(int zigbeeSocket,
			 unitInfoRec * unit,  char * ieeeAddress, char * cBuffer, int usec);
static int zbPollUpdateIeee(unitInfoRec * unit, char * rxBuff);
static int zbPollCheckRearm(unitInfoRec * unit, int zigbeeSocket);

static int zbPollUpdateUser(unitInfoRec * unit, char * rxBuff);
static int zbPollCheckUser(unitInfoRec * unit, int shortAd, char * userDesc);
static void zbPollPirListEvent(unitInfoRec * unit);

static int zbPollArm(int zigbeeSocket);
static int zbPollSendPkt(int zigbeeSocket,
			      unitInfoRec * unit, char * zbPkt, 
			 char * cBuffer, int usec);
static int zbPollCheckIeee(unitInfoRec * unit, int shortAd, char * ieeeAd);
static int zbPollAnnounceIeee(unitInfoRec * unit, char * rxBuff);
static int zbPollUpdatePirShort(unitInfoRec * unit, char * rxBuff, int shortAd);
static int zbPollSetHeartbeat(int zigbeeSocket, unitInfoRec * unit, char * cBuffer, int usec);

static void zbPollReadMsg(time_t thist, unitInfoRec * unit);
static int zbPollResetLogs(int skt, unitInfoRec * unit, char * rxBuff);
static int zbPollResetLog(int zigbeeSocket, unitInfoRec * unit, int dstAddr, int dstEndPoint, int clusterId, char * cBuffer);

/*! \brief Maintain the connection state machine
	\param unit		unit information from database
	\param SleepTime	seconds
	\param ConnectionMax	count
	\param rdTimeout	timeout period in seconds

	Maintains a regular poll if there is no activity on the port
	Runs as a management thread

*/
void zbPollRun(unitInfoRec * unit,
	       int rdTimeout,
	       char * zbCmd,
	       gHomeConfRec * cfg)
{
	unsigned char cBuffer[2048]; /* FIXME - This should cover everything but not good */
	char update[64];
	int zigbeeSocket = -1;
	int i, rc = 0, resp, retry=0;
	enum zbState zbConnectionState;
	int connectionTimer, checkUpdTimer;
	time_t thist, prev, hardwareUpd, adomisUpd;
	unitInfoRec unitStored;
	int hostPort = (int) unit->gwport;
	int usec = cfg->sleepTime * 1000;

	unitDbGetPirStatus(unit);
	if (trace)
	{
		for (i=0; i<24; i++)
			printf("\tPIR count %d = %d\n", i, unit->pirevnt[i]);
	}
	
	unitDbInit(&unitStored, 0, 0);
	unitDbCpy(&unitStored, unit);
	
	prev = 0;
	if (trace) printf("\tConnect to gateway %s\n", unit->gwip);

	/* Set initial state */
	zbConnectionState = zigbeeDOWN;
	updateDbStatus(ZBSTATUS_LOST_COMMS, unit);

	/* Global timer, reset each time a packet is sent or recieved */
	connectionTimer = checkUpdTimer = 0;
	if (trace) printf("\tzigbee poll alarm\n");

	/* Endless task loop */
	while(1)
	{
		thist = time(NULL);
		/* ZigBee conection state machine */
		switch(zbConnectionState)
		{
			case zigbeeDOWN:
				/* Prepare connection */
				if (trace) printf("\t* zigbee Connection DOWN\n");
				zbConnectionState = zigbeeCONNECT;
				break;
			case zigbeeCONNECT:
				/* Connect --> UP or ERROR */
				if (trace) printf("\t* zigbee Connect to gateway %s\n", unit->gwip);
				zigbeeSocket = zbSocketOpen(unit->gwip, hostPort);
				if (zigbeeSocket < 0)
				{
					zbConnectionState  = zigbeeERROR;
					if (trace) printf("\tzigbee Connection CONNECT-->ERROR\n");
					/* Only update once every 10 minutes */
					if (thist - prev > 600)
					{
						updateDbLogStatus(unit, ZBSTATUS_LOST_COMMS, 0);
						updateDbStatus(ZBSTATUS_LOST_COMMS, unit);
						prev = thist;
					}
				}
				else
				{
					if (trace) printf("\t* zigbee Connected to gateway %s\n", unit->gwip);
					updateDbLogStatus(unit, 0, 0);
					/* Make sure there are no buffered messages sitting there */
					zbSocketClear(zigbeeSocket, rdTimeout);
					zbConnectionState = zigbeeINITGW;
					retry = connectionTimer = 0;
				}
				usleep(usec);
				break;
			case zigbeeINITGW :
				if (trace) printf("\t* zigbee Connection INIT GATEWAY\n");
				updateDbStatus(ZBSTATUS_INIT, unit);
				/* Get system info */
				rc = zbPollSendReq(zigbeeSocket, PktSysInfo, "system info");
				if (rc < 0)
					zbConnectionState = zigbeeERROR;
				else
				{
					/* Wait for response */
					usleep(usec);
					memset(cBuffer, 0, sizeof(cBuffer));
					resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
					if (resp < 0) 
						zbConnectionState = zigbeeERROR;
					else
					{
						rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
						if (rc == SYSDEV) 
						{
							retry = 0;
							zbConnectionState = zigbeeSYSPING;
						}
						else if (retry > cfg->retry)
						{
							fprintf(stderr, "%s WARNING: no reply received for system info command (%d attempts)\n", 
								DateTimeStampStr(-1), cfg->retry);
							retry = 0;
							zbConnectionState = zigbeeSYSPING;
						}
						++retry;
					}
				}
				break;
			case zigbeeSYSPING :
				if (trace) printf("\t* zigbee Connection system ping\n");
				/* Send system ping */
				rc = zbPollSendReq(zigbeeSocket, PktSysPing, "system ping");
				if (rc < 0)
					zbConnectionState = zigbeeERROR;
				else
				{
					/* Wait for response */
					usleep(usec);
					memset(cBuffer, 0, sizeof(cBuffer));
					resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
					if (resp < 0) 
						zbConnectionState = zigbeeERROR;
					else
					{
						rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
						if (rc == SYSPING) 
						{
							zbConnectionState = zigbeeGETALARM;
							retry = 0;
						}
						else if (retry > cfg->retry)
						{
							fprintf(stderr, "%s WARNING: no reply received for system ping (%d attempts)\n", 
								DateTimeStampStr(-1), cfg->retry);
							retry = 0;
							zbConnectionState = zigbeeGETALARM;
						}
						++retry;
					}
				}
				break;
			case zigbeeGETALARM :
				if (trace) printf("\t* zigbee Connection INIT ALARM\n");
				/* Get alarm info */
				rc = zbPollSendReq(zigbeeSocket, PktGetAlarm, "get alarm");
				if (rc < 0)
					zbConnectionState = zigbeeERROR;
				else
				{
					/* Wait for response */
					usleep(usec);
					memset(cBuffer, 0, sizeof(cBuffer));
					resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
					if (resp < 0) 
						zbConnectionState = zigbeeERROR;
					else
					{
						rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
						if (rc == GET_ALARM) 
						{
							zbConnectionState = zigbeeHEARTBEAT;
							retry = 0;
						}
						else if (retry > cfg->retry)
						{
							fprintf(stderr, "%s WARNING: no reply received for get alarm command (%d attempts)\n", 
								DateTimeStampStr(-1), cfg->retry);
							zbConnectionState = zigbeeHEARTBEAT;
							retry = 0;
						}
						++retry;
					}
				}
				break;
			case zigbeeHEARTBEAT :
				if (Heartbeat)
				{
					if (trace) printf("\tzigbee Connection set HEARTBEAT\n");
					rc = zbPollSetHeartbeat(zigbeeSocket, unit, cBuffer, usec);
					if (rc < 0)
						zbConnectionState = zigbeeERROR;
					else
					{
						/* Wait for response */
						usleep(usec);
						memset(cBuffer, 0, sizeof(cBuffer));
						resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
						if (resp < 0) 
							zbConnectionState = zigbeeERROR;
						else
						{
							rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
							if (rc == HEARTBEAT) zbConnectionState = zigbeeARMMODE;
							else if (retry > cfg->retry)
							{
								fprintf(stderr, "%s WARNING: no reply received for heartbeat command (%d attempts)\n", 
									DateTimeStampStr(-1), cfg->retry);
								zbConnectionState = zigbeeARMMODE;
								retry = 0;
							}
							++retry;
						}
					}
				}
				else
				{
					if (verbose) printf("%s\t* Skip PIR heartbeat activation\n", DateTimeStampStr(-1));
					zbConnectionState = zigbeeARMMODE;
				}
				break;
			case zigbeeARMMODE :
				if (trace) printf("\tzigbee Connection START\n");
				checkUpdTimer = 0;
				/* Get arm mode */
				zbPollSendReq(zigbeeSocket, PktGetArmMode, "arm mode");
				if (rc < 0)
					zbConnectionState = zigbeeERROR;
				else
				{
					/* Wait for response */
					usleep(usec);
					memset(cBuffer, 0, sizeof(cBuffer));
					resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
					if (resp < 0) 
						zbConnectionState = zigbeeERROR;
					else
					{
						rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
						switch (rc)
						{
							case ARM :
							case ARM_DAY :
							case ARM_NITE :
							case DISARM :
								if (trace) printf("\tGet Alarm Response OK\n");
								DateTimeStamp();
								dispBuffer(cBuffer, resp, "\n");
								zbConnectionState = zigbeeDISCOVER;
								break;
							default :
								DateTimeStamp();
								dispBuffer(cBuffer, resp, "\n");
						}
						if (retry > cfg->retry)
						{
							fprintf(stderr, "%s WARNING: no reply received for arm mode command (%d attempts)\n", 
								DateTimeStampStr(-1), retry);
							zbConnectionState = zigbeeDISCOVER;
							retry = 0;
						}
						++retry;
					}
				}
				break;
			case zigbeeDISCOVER :
				if (Heartbeat && cfg->discover)
				{
					if (trace) printf("\tzigbee Connection manage discovery\n");
					rc = zbPollManageDiscovery(zigbeeSocket, unit, unit->gwaddr, cBuffer, usec);
					if (rc < 0)
						zbConnectionState = zigbeeERROR;
					else
					{
						resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
						if (resp < 0) 
							zbConnectionState = zigbeeERROR;
						else
						{
							rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
							if (rc < 0)
								zbConnectionState = zigbeeERROR;
							else
							{
								retry = 0;
								zbConnectionState = zigbeeTEMP;
							}
						}
					}
				}
				else
				{
					if (verbose) printf("%s\t* Skip Connection manage discovery\n", DateTimeStampStr(-1));
					zbConnectionState = zigbeeTEMP;
				}
				break;
			case zigbeeTEMP:
				if (trace) printf("\t* zigbee Connection TEMPERATURE reading\n");
				checkUpdTimer = 0;
				/* Send commands to read room temperature(s) from PIR(s) */
				if (cfg->readTemp) 
				{
					rc = zbPollTemperature(zigbeeSocket, unit, cBuffer, usec);
					if (rc < 0)
						zbConnectionState = zigbeeERROR;
					else
						zbConnectionState = zigbeePIR;
				}
				else
					zbConnectionState = zigbeePIR;
				break;
			case zigbeePIR:
				if (trace) printf("\t* zigbee Connection PIR check\n");
				checkUpdTimer = 0;
				/* Check PIR list for missing IEEE addresses e.a. and send messages where needed */
				rc = zbPollCheckPirs(unit, zigbeeSocket, cBuffer, cfg->unEnroll, usec);
				if (rc < 0)
					zbConnectionState = zigbeeERROR;
				else
				{
					retry = 0;
					zbConnectionState = zigbeeCMD;
				}
				break;
			case zigbeeCMD :
				/* Send additional command when specified */
				if (zbCmd) 
				{
					if (trace) printf("\t* zigbee send command %s\n", zbCmd);
					rc = zbPollSendPkt(zigbeeSocket, unit, zbCmd, cBuffer, usec);
					if (rc < 0)
						zbConnectionState = zigbeeERROR;
					else
					{
						usleep(usec);
						resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
						if (resp < 0) 
							zbConnectionState = zigbeeERROR;
						else
						{
							rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
							zbCmd = NULL;		/* only do additional command once */
							zbConnectionState = zigbeeUP;
						}
					}
				}
				else
					zbConnectionState = zigbeeUP;
				break;
			case zigbeeUP :
				if (trace) printf("\t* zigbee Connection UP\n");
				checkUpdTimer = 0;
				if (trace) zbPollPirListEvent(unit);
				/* Check database current status */
				update[0] = 0;
				updateDbStatusInfo(unit, update);
				if (trace) printf("\tCurrent status is %d (updated on %s)\n", unit->ustatus, update);
				/* Check if alarm should be turned on again */
				zbPollCheckRearm(unit, zigbeeSocket);
				/* Check if PIR or other info has changed */
				if (unitDbCmp(unit, &unitStored))
				{
					if (trace) printf("\tUpdate PIR records in database\n");
					updateDbSetPirHardware(unit);
				}
				else
				{
					if (trace) printf("\tPIR records in database are up to date\n");
				} 
				/* Check last time a message was read */
				zbPollReadMsg(thist, unit);
				/* Update logger heartbeat in database */
				updateDbLoggerCheck(unit);
				/* Update all PIR status */
				unitDbSetPirStatus(unit);
				/* Send PING command to keep connection alive */
				if (verbose) printf("%s\t* Start polling\n", DateTimeStampStr(-1));
				zbConnectionState = zigBeePOLL;
				//? if (zbSocketSendPing(zigbeeSocket) < 0) zbConnectionState = zigbeeERROR;
				break;
			case zigBeePOLL :
				if (trace)
				{
					DateTimeStamp();
					printf("\t* Poll %d/%d\n", connectionTimer, cfg->connectionTimer);
				}
				/* Wait for response */
				memset(cBuffer, 0, sizeof(cBuffer));
				resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
				if (resp < 0) 
				{
					if (trace) printf("\tSocket read error\n");
					zbConnectionState = zigbeeERROR;
				}
				else
				{
					rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
					switch (rc)
					{
						case ALARM :
							if (trace) printf("\tActivated alarm for %s\n", unit->oname);
							break;
						case ALARMcontinue :
							if (trace) printf("\tActive alarm for %s still on\n", unit->oname);
							break;
						case ARM_DAY :
							if (trace) printf("\tActivated alarm for %s, getting mode\n", unit->oname);
							break;
						case ARM_STAY :
							if (trace) printf("\tSquawk alarm for %s\n", unit->oname);
							zbPollSquawkAlarm(zigbeeSocket, unit->alrmaddr);
							break;
						case DISARM :
							if (trace) printf("\tAlarm for %s is off\n", unit->oname);
							break;
						default :
							break;
					}
					if (unit->alrmreset == 0 || checkUpdTimer % 2000 == 0)
					{
						/* Reset alarm log every 8000 seconds */
						if (trace) printf("\tReset alarm log\n");
						zbConnectionState = zigbeeRESETLOG;
					}
					else if (Heartbeat && (unit->hbeatreq == 0 || checkUpdTimer % 1000 == 0))
					{
						/* Check heartbeat every 4000 seconds */
						if (trace) printf("\tSet HEARTBEAT time\n");
						zbConnectionState = zigbeeHEARTBEAT;
					}
					else if (connectionTimer >= cfg->connectionTimer)
					{
						/* Start poll every 10 minutes, not sure if really needed */
						connectionTimer = 0;
						zbConnectionState = zigbeePIR;
					}
					else
					{
						/* Send PING command to keep connection alive */
						if(zbSocketSendPing(zigbeeSocket) < 0) zbConnectionState = zigbeeERROR;
					}
					usleep(usec);
				}
				break;
			case zigbeeRESETLOG :
				if (trace) printf("\tzigbee reset ALARM logs\n");
				rc = zbPollResetLogs(zigbeeSocket, unit, cBuffer);
				if (rc < 0)
					zbConnectionState = zigbeeERROR;
				else
				{
					checkUpdTimer = 0;
					zbConnectionState = zigBeePOLL;
				}
				break;
			case zigbeeERROR:
				/* Clear out connection and try again --> DOWN */
				if (trace) fprintf(stderr, "\t* zigbee Connection ERROR-->DOWN in %s\n", unit->oname);
				zbSocketClose(zigbeeSocket);
				updateDbStatus(ZBSTATUS_LOST_COMMS, unit);
				zbConnectionState = zigbeeDOWN;
				retry = 0;
				break;
			default:
				if (trace) printf("\t* zigbee Connection unknown state %d\n", zbConnectionState);
				zbConnectionState = zigbeeERROR;
				break;
		}
		++checkUpdTimer;
		/* Check update timer every 40 seconds */
		if (checkUpdTimer % 10 == 0)
		{
			/* Check if database has been updated from iHome web page */
			hardwareUpd = unitDbGetHardwareModif(unit);
			adomisUpd = unitDbGetAdomisModif(unit);
			if (hardwareUpd > unit->hwedit)
			{
				/* Update location info from database */
				printf("\n\n%s ***** Reload PIR data after external update *****\n", DateTimeStampStr(-1));
				unitDbReloadPir(unit);
				unit->hwedit = hardwareUpd;
				zbConnectionState = zigbeePIR;
			}
			else if (adomisUpd > unit->cfedit)
			{
				/* Update location info from database */
				printf("\n\n%s ***** Reload home data after external update *****\n", DateTimeStampStr(-1));
				unitDbUpdateInfo(unit);
				printf("%s Log level is %d\n", DateTimeStampStr(-1), unit->loglevel);
				switch(unit->loglevel)
				{
					case 2 : verbose = 1; trace = 0; break;
					case 3 : verbose = 1; trace = 1; break;
				}
				unit->cfedit = adomisUpd;
			}
			else
			{
				if (trace) printf("\tNo external change to database since %s\n", DateTimeStampStr(hardwareUpd));
			}
		}
		connectionTimer++;
	}
	close(zigbeeSocket);
} /* zbPollRunPoll */

#if 0
/*! \brief Maintain the connection state machine with entry delay

	\param unit		unit information from database
	\param SleepTime	seconds
	\param ConnectionMax	count
	\param rdTimeout	timeout period in seconds

	Maintains a regular poll if there is no activity on the port
	When alarm sounds, an entry delay allows time to deactivate alarm
	Runs as a management thread

	TODO : move delay functionality into zbPollRunPoll
*/
void zbPollRunPollD(int alarmDelay,
		    unitInfoRec * unit,
		    int SleepTime,
		    int ConnectionMax,
		    int rdTimeout)
{
	unsigned char cBuffer[2048]; /* FIXME - This should cover everything but not good */
	char update[64];
	int zigbeeSocket = -1;
	unsigned int cmd;
	int rc = 0, resp;
	enum zbState zbConnectionState;
	int connectionTimer;
	time_t thistime, prev, alarmOn = 0;
	int hostPort = (int) unit->gwport;
	int timeAlarm = 0, disarmCount = 0;

	prev = 0;
	if (trace) printf("\tConnect to gateway %s (entry delay %d)\n", unit->gwip, alarmDelay);

	/* Set initial state */
	zbConnectionState = zigbeeDOWN;

	/* Global timer, reset each time a packet is sent or recieved */
	connectionTimer = 0;
	if (trace) printf("\tzigbee poll alarm\n");

	/* Endless task loop */
	while(1)
	{
		thistime = time(NULL);
		if (trace) printf("\t%4d\t\r", connectionTimer);
		/* ZigBee conection state machine */
		switch(zbConnectionState)
		{
			case zigbeeDOWN:
				/* Prepare connect --> Go to connect */
				zbConnectionState = zigbeeCONNECT;
				if (trace) printf("\tzigbee Connection DOWN\n");
				break;
			case zigbeeCONNECT:
				/* Connect --> UP or ERROR */
				if (trace) printf("\tzigbee Connect to gateway %s\n", unit->gwip);
				zigbeeSocket = zbSocketOpen(unit->gwip, hostPort);
				if (zigbeeSocket < 0)
				{
					zbConnectionState  = zigbeeERROR;
					if (trace) printf("\tzigbee Connection CONNECT-->ERROR\n");
					thistime = time(NULL);
					/* Only update once every 10 minutes */
					if (thistime - prev > 600)
					{
						updateDbLogStatus(unit, ZBSTATUS_LOST_COMMS, 0);
						updateDbStatus(ZBSTATUS_LOST_COMMS, unit);
						prev = thistime;
					}
				}
				else
				{
					if (trace) printf("\tzigbee Connected to gateway %s\n", unit->gwip);
					updateDbLogStatus(unit, 0, 0);
					/* Make sure there are no buffered messages sitting there */
					zbSocketClear(zigbeeSocket, rdTimeout);
					zbConnectionState = zigbeeINITGW;
					connectionTimer = 0;
				}
				break;
			case zigbeeINITGW :
				if (trace) printf("\tzigbee Connection INIT GATEWAY\n");
				if (unit->gwieee[0])
				{
					zbConnectionState = zigbeeSTART;
				}
				else
				{
					resp = zbPollGateway(zigbeeSocket, unit);
					if (resp < 0) 
						zbConnectionState = zigbeeERROR;
					else
					{
						sleep(1);
						/* Wait for response */
						memset(cBuffer, 0, sizeof(cBuffer));
						resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
						readBuffer(zigbeeSocket, unit, cBuffer, resp);
					}
				}
				break;
			case zigbeeINITCORD :
				if (trace) printf("\tzigbee Connection INIT COORDINATOR\n");
				if (unit->sysping)
				{
					zbConnectionState = zigbeeSTART;
				}
				else
				{
					resp = zbPollGetIeee(zigbeeSocket, unit, 0, cBuffer);
					if (resp < 0) zbConnectionState = zigbeeERROR;
				}
				break;
			case zigbeeSTART :
				if (trace) printf("\tzigbee Connection START\n");
				/* Get arm mode */
				zbPollSendReq(zigbeeSocket, PktGetArmMode, "get arm mode");
				sleep(1);
				/* Wait for response */
				memset(cBuffer, 0, sizeof(cBuffer));
				resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					if (trace) fprintf(stderr, "\t%s: zigbeeConnection start alarm-->ERROR\n", unit->appt);
					zbConnectionState = zigbeeERROR;
				}
				else
				{
					if (trace) dumpRXdata("\tPoll buffer", cBuffer, resp);
					if (0 == strncmp(cBuffer, "021C01060000010500011EZ", 23))
					{
						if (trace) printf("\tCommand - Get Alarm - Response OK\n");
						DateTimeStamp();
						dispBuffer(cBuffer, resp, "\n");
						readBuffer(zigbeeSocket, unit, cBuffer, resp);
						zbConnectionState = zigbeeUP;
					}
					else if (0 == strncmp(cBuffer, "021C0806", 8))
					{
						if (trace) printf("\tCommand - Get Alarm Mode - Response OK\n");
						DateTimeStamp();
						dispBuffer(cBuffer, resp, "\n");
						readBuffer(zigbeeSocket, unit, cBuffer, resp);
						zbConnectionState = zigbeeUP;
					}
					else
					{
						DateTimeStamp();
						dispBuffer(cBuffer, resp, "\n");
					}
				}
				break;
			case zigbeeUP:
				if (trace) printf("\tzigbee Connection UP\n");
				/* Get link quality information */
				zbPollSendReq(zigbeeSocket, PktGetLqi, "LQI");
				if (readTemp) zbPollTemperature(zigbeeSocket, unit);
				zbPollCheckPirs(unit, zigbeeSocket, cBuffer);
				if (trace) zbPollPirListEvent(unit);
				updateDbLoggerCheck(unit);
				/* Check database for current status */
				update[0] = 0;
				updateDbStatusInfo(unit, update);
				zbPollCheckRearm(unit, zigbeeSocket);
				if (trace) printf("\tCurrent status is %d (updated on %s)\n", unit->ustatus, update);
				zbConnectionState = zigBeePOLL;
				sleep(1);
				break;
			case zigBeePOLL:
				/* Wait for response */
				memset(cBuffer, 0, sizeof(cBuffer));
				resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					zbConnectionState = zigbeeERROR;
					if (trace) fprintf(stderr, "%s: zigbeeConnection zigBeePING-->ERROR1\n", unit->appt);
				}
				else
				{
					rc = readBuffer(zigbeeSocket, unit, cBuffer, resp);
					switch (rc)
					{
						case ALARM :
							if (trace) printf("\tSound alarm for %s\n", unit->appt);
							/* Sound the alarm */
							zbPollSendReq(zigbeeSocket, PktSoundAlarm, "alarm");
							break;
						case ARM :
							if (trace) printf("\tActivate alarm for %s\n", unit->appt);
							updateDbLogStatus(unit, ZBSTATUS_ARMED, 0);
							updateDbStatus(ZBSTATUS_ARMED, unit);
							unit->ustatus = ZBSTATUS_ARMED;
							zbConnectionState = zigbeeARMED;
							break;
						case DISARM :
							if (trace) printf("\tDe-activate alarm for %s\n", unit->appt);
							updateDbLogStatus(unit, ZBSTATUS_DISARMED, 0);
							updateDbStatus(ZBSTATUS_DISARMED, unit);
							unit->ustatus = ZBSTATUS_DISARMED;
							break;
						default :
							/* Ignore */
							;
					}
				}
				/* Issue PING Comand to keep connection alive */
				if(zbSocketSendPing(zigbeeSocket) < 0)
				{
					zbConnectionState = zigbeeERROR;
					if (trace) printf("\t%s: zigbee Connection zigBeePING-->ERROR\n", unit->appt);
				}
				sleep(2);
				/* Start poll every 10 minutes, not sure if really needed */
				if (connectionTimer > ConnectionMax)
				{
					connectionTimer = 0;
					zbConnectionState = zigbeeUP;
				}
				break;
			case zigbeeARMED:
				/* Wait for response */
				memset(cBuffer, 0, sizeof(cBuffer));
				resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					zbConnectionState = zigbeeERROR;
					if (trace) fprintf(stderr, "zigbeeConnection zigBeePING-->ERROR1\n");
				}
				else
				{
					rc = readBuffer(zigbeeSocket, unit, cBuffer, resp);
					if (trace) printf("\treadBuffer returned %d\n", rc);
					switch (rc)
					{
						case PIR :
						case ALARM :
							if (trace) printf("\tTurn alarm off\n");
							rc = zbSocketWrite(zigbeeSocket, PktDisarm, strlen(PktDisarm));
							if (rc < 0)
							{
								printf("Packet send failed\n");
							}
							disarmCount = 0;
							zbConnectionState = zigbeeALARM;
							alarmOn = thistime;
							break;
						case DISARM :
							updateDbLogStatus(unit, ZBSTATUS_DISARMED, 0);
							updateDbStatus(ZBSTATUS_DISARMED, unit);
							unit->ustatus = ZBSTATUS_DISARMED;
							if (trace) printf("\tTurned alarm off - back to normal\n");
							zbConnectionState = zigBeePOLL;
						default :
							if (trace) printf("\treadBuffer returned %d\n", rc);
					}
				}
				/* Issue PING Comand to keep connection alive */
				if( zbSocketSendPing(zigbeeSocket) < 0 )
				{
					zbConnectionState = zigbeeERROR;
					if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
				}
				break;
			case zigbeeALARM:
				timeAlarm = (int) thistime - alarmOn;
				if (trace) printf("\tAlarm wait for timeout %d\n", timeAlarm);
				if (timeAlarm > alarmDelay)
				{
					if (trace) printf("\tAlarm turned on after %d seconds\n", (int) timeAlarm);
					updateDbLogEvent(unit, "Alarm", 6, rc);
					updateDbLogStatus(unit, ZBSTATUS_ALARM, rc);
					updateDbStatus(ZBSTATUS_ALARM, unit);
					/* Send alarm */
					zbPollSendReq(zigbeeSocket, PktSoundAlarm, "alarm");
					/*? zbPollSoundAlarm(zigbeeSocket); ?*/
					zbConnectionState = zigBeePOLL;
				}
				/* Wait for response */
				memset(cBuffer, 0, sizeof(cBuffer));
				resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
				if (resp == 0);
				else if (resp == -1)
				{
					zbConnectionState = zigbeeERROR;
					if (trace) fprintf(stderr, "zigbeeConnection zigBeePING-->ERROR1\n");
				}
				else
				{
					rc = readBuffer(zigbeeSocket, unit, cBuffer, resp);
					if (trace) printf("\treadBuffer returned %d\n", rc);
					switch (rc)
					{
						case DISARM :
							++disarmCount;
							if (disarmCount > 1)
							{
								if (trace) printf("\tAlarm off - back to normal\n");
								alarmOn = thistime;
								updateDbLogStatus(unit, ZBSTATUS_DISARMED, 0);
								updateDbStatus(ZBSTATUS_DISARMED, unit);
								unit->ustatus = ZBSTATUS_DISARMED;
								zbConnectionState = zigBeePOLL;
							}
							else
							{
								if (trace) printf("\tAlarm off - wait for confirm\n");
							}
							break;
						case PIR :
							if (trace) printf("\treadBuffer returned PIR message (%d)\n", rc);
						default :
							if (trace) printf("\treadBuffer returned %d\n", rc);
					}
				}
				/* Issue PING Comand to keep connection alive */
				if( zbSocketSendPing(zigbeeSocket) < 0 )
				{
					zbConnectionState = zigbeeERROR;
					if (trace) printf("\tzigbee Connection zigBeePING-->ERROR\n");
				}
				break;
			case zigbeeERROR:
				/* Clear out connection and try again --> DOWN */
				if (trace) fprintf(stderr, "\tzigbee Connection ERROR-->DOWN in %s\n", unit->appt);
				zbConnectionState = zigbeeDOWN;
				zbSocketClose(zigbeeSocket);
				break;
			default:
				if (trace) printf("\tzigbee Connection unknown state\n");
				zbConnectionState = zigbeeERROR;
				break;
		}
		sleep(SleepTime);
		connectionTimer++;
	}
} /* zbPollRunPollD */
#endif

/*! \brief Read and display buffer 
	\param	skt		open socket
	\param unit		unit record
	\param cBuffer		input buffer
	\param cLen		buffer length
 */
int checkBuffer(int skt, 
		void * unit, 
		char * cBuffer, 
		int cLen,
		int usec)
{
	int rc = 0, rval = 0;
	char updateStr[64];
	time_t prev_alarm = AlarmActive;
	unitInfoRec * unitRec;
	char tempValStr[8];

	unitRec = (unitInfoRec *) unit;
	
	if (trace) printf("\tRX '%s'\n", cBuffer);

	/* Burglar alarm activated */
	if (0 == strncmp(cBuffer, "021C0519", 8))
	{
		DateTimeStamp();
		if (0 == strncmp(&cBuffer[56], "01", 2))
		{
			AlarmActive = time(NULL);
			if (AlarmActive-prev_alarm > 180)
			{
				dispBuffer(cBuffer, cLen, "<ALARM>\n");
				AlarmActive = time(NULL);
				rc = readAlarmDeviceNotify(cBuffer, unit);
				if (trace) printf("\tAlarm in location %d (%s)\n", rc, unitDbGetLocStr(rc));
				updateDbLogEvent(unit, "Alarm", 6, rc);
				updateDbLogStatus(unit, ZBSTATUS_ALARM, rc);
				updateDbStatus(ZBSTATUS_ALARM, unit);
				updateDbAlarmMsg(unit, "Alarm", 6, rc);
				if (trace) printf("\tActivated alarm for %s\n", unitRec->oname);
				rval = ALARM;
			}
			else
			{
				dispBuffer(cBuffer, cLen, "<ALARMcontinue>\n");
				rval = ALARMcontinue;
			}
			unitRec->ustatus = ZBSTATUS_ALARM;
		}
		else
		{
			dispBuffer(cBuffer, cLen, "<Heartbeat>\n");
			zbPollUpdateHeartbeat(skt, unitRec, cBuffer);
			rval = HBEAT;
		}
	}
	/* Fire alarm activated */
	else if (0 == strncmp(cBuffer, "021C0305", 8))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<FIRE>\n");
		updateDbLogEvent(unitRec, "Fire alarm", 6, 0);
		updateDbLogStatus(unitRec, ZBSTATUS_FIRE, 0);
		updateDbStatus(ZBSTATUS_FIRE, unitRec);
		updateDbAlarmMsg(unitRec, "Fire alarm", 6, 0);
		unitRec->ustatus = ZBSTATUS_FIRE;
		rval = FIRE;
	}
	/* Panic button pressed on remote */
	else if (0 == strncmp(cBuffer, "021C0205", 8))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<PANIC>\n");
		updateDbLogEvent(unitRec, "Panic alert", 6, 0);
		updateDbLogStatus(unitRec, ZBSTATUS_PANIC, 0);
		updateDbStatus(ZBSTATUS_PANIC, unitRec);
		updateDbAlarmMsg(unitRec, "Panic alert", 6, 0);
		unitRec->ustatus = ZBSTATUS_PANIC;
		rval = PANIC;
	}
	/* Arm or disarm command */
	else if (0 == strncmp(cBuffer, "021C0106", 8))
	{
		/* Check database for current status */
		updateDbStatusInfo(unitRec, updateStr);
		if (trace) printf("\tCurrent status is %d (updated on %s)\n", unitRec->ustatus, updateStr);
		DateTimeStamp();
		if (0 == strncmp(&cBuffer[18], "50", 2))
		{
			dispBuffer(cBuffer, cLen, "<ARM>\n");
			switch (unitRec->ustatus)
			{
				case 8 :
					if (trace) printf("\tAlarm in day mode (%x)\n", unitRec->ustatus);
					updateDbLogStatus(unitRec, ZBSTATUS_ARM_DAY, 0);
					updateDbStatus(ZBSTATUS_ARMED, unit);
					/* Get arm mode */
					zbPollSendReq(skt, PktGetArmMode, "get arm mode");
					rval = ARM_DAY;
					break;
				case 9 :
					if (trace) printf("\tAlarm in night mode (%x)\n", unitRec->ustatus);
					updateDbLogStatus(unitRec, ZBSTATUS_ARM_NITE, 0);
					updateDbStatus(ZBSTATUS_ARMED, unitRec);
					/* Get arm mode */
					zbPollSendReq(skt, PktGetArmMode, "get arm mode");
					rval = ARM_NITE;
					break;
				default :
					updateDbLogStatus(unitRec, ZBSTATUS_ARMED, 0);
					updateDbStatus(ZBSTATUS_ARMED, unitRec);
					rval = ARM;
			}
		}
		else if (0 == strncmp(&cBuffer[18], "51", 2))
		{
			AlarmActive = 0;
			dispBuffer(cBuffer, cLen, "<DISARM>\n");
			updateDbLogStatus(unitRec, ZBSTATUS_DISARMED, 0);
			updateDbStatus(ZBSTATUS_DISARMED, unitRec);
			unitRec->ustatus = ZBSTATUS_DISARMED;
			rval = DISARM;
		}
		else if (0 == strncmp(&cBuffer[18], "01", 2))
		{
			AlarmActive = 0;
			dispBuffer(cBuffer, cLen, "<GET_ALARM>\n");
			rval = GET_ALARM;
		}
		else
			dispBuffer(cBuffer, cLen, "<StatusChange>\n");
	}
	/* Get alarm response */
	else if (0 == strncmp(cBuffer, "021C010D", 8))
	{
		dispBuffer(cBuffer, cLen, "<GET_ALARM>\n");
		rval = GET_ALARM;
	}
	/* Temperature reading from PIR */
	else if ((0 == strncmp(cBuffer, "020D010C", 8)) || (0 == strncmp(cBuffer, "020D010F", 8)))
	{
		if (0 == strncmp(&cBuffer[14], "0402", 4))
		{
			char shortAddr[8];
			long tempVal;
			float temp;
			int pidx;

			DateTimeStamp();
			dispBuffer(cBuffer, cLen, "<TEMP>\n");

			strncpy(shortAddr, &cBuffer[8], 4);
			shortAddr[4] = 0;

			strncpy(tempValStr, &cBuffer[28], 4);
			tempValStr[4] = 0;
			tempVal = strtol(tempValStr, NULL, 16);
			temp = (float) tempVal/100;
			pidx = unitDbFindNwk(unitRec, shortAddr);
			if (trace) printf("\tTemperature at PIR %d (%s) is %.2f\n", pidx+1, shortAddr, temp);

			if (pidx>=0) updateDbTemperature(unitRec, pidx, temp);
			rval = TEMP;
		}
	}
	/* PIR picked up movement */
	else if (0 == strncmp(cBuffer, "021C0008", 8))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<PIR>\n");
		zbPollUpdatePir(skt, unit, cBuffer, usec);
		rval = PIR;
	}
	/* IEEE address received */
	else if (0 == strncmp(cBuffer, "020A81", 6))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<IEEE>\n");
		zbPollUpdateIeee(unitRec, cBuffer);
		rval = IEEE;
	}
	/* New device announced */
	else if (0 == strncmp(cBuffer, "020AF00B", 8))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<ANNCE>\n");
		zbPollAnnounceIeee(unitRec, cBuffer);
		rval = ANNCE;
	}
	/* Short address received */
	else if (0 == strncmp(cBuffer, "020A80", 6))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<Short>\n");
		/* Check status */
		rc = getStatus(&cBuffer[26]);
		/* Check address mode and short address */
		strncpy(updateStr, &cBuffer[44], 4);
		updateStr[4] = 0;
		rc = strtol(updateStr, NULL, 16);
		if (trace) printf("\tShort address is %04x\n", rc);
		if (0 == rc)
			updateDbCoordCheck(unitRec);
		else
			zbPollUpdatePirShort(unitRec, cBuffer, rc);
		rval = SHORT;
	}
	/* System device info (Gateway IEEE address) received */
	else if (0 == strncmp(cBuffer, "021014", 6))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<Sysdev>\n");
		unitLqiGateway(unitRec, cBuffer);
		updateDbGateway(unit);
		rval = SYSDEV;
	}
	/* LQI received */
	else if (0 == strncmp(cBuffer, "020A8B", 6))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<LQI>\n");
		rc = getStatus(&cBuffer[12]);
		if (! unitRec->pan[0]) 
		{
			strncpy(unitRec->pan, &cBuffer[20], 16);
			unitRec->pan[16] = 0;
			if (trace) printf("\tPAN set to '%s'\n", unitRec->pan);
		}
		else if (strncmp(unitRec->pan, &cBuffer[20], 16))
		{
			strncpy(unitRec->pan, &cBuffer[20], 16);
			unitRec->pan[16] = 0;
			if (trace) printf("\tPAN changed to '%s'\n", unitRec->pan);
		}
		updateDbCoordCheck(unitRec);
		rval = LQI;
	}
	/* Set heartbeat response received */
	else if (0 == strncmp(cBuffer, "021C0706", 8))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<SetHeartbeat>\n");
		rc = getStatus(&cBuffer[12]);
		updateDbCoordCheck(unitRec);
		rval = HEARTBEAT;
	}
	/* Arm mode received */
	else if (0 == strncmp(cBuffer, "021C0806", 8))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<MODE>\n");
		rc = getArmMode(&cBuffer[18]);
		switch (rc)
		{
			case 0 :
				/* Disarm */
				updateDbStatus(ZBSTATUS_DISARMED, unit);
				rval = DISARM;
				break;
			case 1 :
				/* Arm Day/Home Zones Only */
				updateDbStatus(ZBSTATUS_ARM_DAY, unit);
				rval = ARM_DAY;
				break;
			case 2 :
				/* Arm Night/Sleep Zones Only */
				updateDbStatus(ZBSTATUS_ARM_NITE, unit);
				rval = ARM_NITE;
				break;
			case 3 :
				/* Arm All Zones */
				updateDbStatus(ZBSTATUS_ARMED, unit);
				rval = ARM;
				break;
			default : 
				fprintf(stderr, "ERROR: Reserved arm mode %d", rc);
		}
	}
	/* System ping */
	else if (0 == strncmp(cBuffer, "02100702", 8))
	{
		unitRec->sysping = time(NULL);
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<SysPing>\n");
		updateDbCoordCheck(unitRec);
		rval = SYSPING;
	}
	/* User description */
	else if (0 == strncmp(cBuffer, "020A8F", 6))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<IEEE>\n");
		zbPollUpdateUser(unitRec, cBuffer);
		rval = USER;
	}
	else
	{
		if (verbose) 
		{
			DateTimeStamp();
			dispBuffer(cBuffer, cLen, "\n");
		}
	}
	if (trace) dispEvent("\tReceived ", rval, "\n");
	return rval;
}

static void zbPollReadMsg(time_t thist, 
			  unitInfoRec * unit)
{
	time_t mintvl;
	char scmd[129];
	
	mintvl = thist - unit->readmsg;
	if (verbose) printf("%s Last message read was %d seconds ago\n", DateTimeStampStr(-1), (int) mintvl);
	if (mintvl > 300)
	{
		if (verbose) printf("%s Ping panel %s\n", DateTimeStampStr(-1), unit->dinip);
		sprintf(scmd, "/bin/ping -c 2 %s", unit->dinip);
		system(scmd);
	}
}

/*! \brief Check PIR list and commands to get missing IEEE addresses
	\param	unit		Record with addresses
	\param	zigbeeSocket	Socket for comms
 */
static int zbPollCheckPirs(unitInfoRec * unit, 
			    int zigbeeSocket,
			    char * cBuffer,
			    int forceUnenroll, 
			    int usec)
{
	int i, rc=0, resp=0, dstEndPoint;
	
	if (unit->ustatus == ZBSTATUS_ARMED) 
	{
		if (trace) printf("\tAlarm is set - skip check of PIR list\n");
		return 0;
	}

	/* Go through list of PIRs */
	if (trace) printf("\tCheck PIR list\n");
	
	if (trace) dbUnitDisplayPirInfo(unit);
	for (i=0; i<MAX_PIR; i++)
	{
		/* Network and IEEE addresses not known yet */
		if (0 == unit->piraddr[i] && 0 != unit->pirieee[i][0])
		{
			if (trace) printf("\tPIR %d short address not known\n", i+1);
			rc = zbPollGetShort(zigbeeSocket, unit, unit->pirieee[i], cBuffer, usec);
			if (rc < 0) return -1;
			usleep(usec);
			resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
			if (resp < 0) 
				return -1;
			else
			{
				resp = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
				if (resp < 0) return -1;
			}
		}
		/* IEEE address not known yet */ 
		else if (unit->piraddr[i] && 0 == unit->pirieee[i][0]) 
		{
			/* Send command to get IEEE address for this PIR */
			if (trace) printf("\tPIR %d (%04x) IEEE address not known\n", i+1, unit->piraddr[i]);
			rc = zbPollGetIeee(zigbeeSocket, unit, unit->piraddr[i], cBuffer, usec);
			if (rc < 0) return -1;
			usleep(usec);
			resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
			if (resp < 0) 
				return -1;
			else
			{
				resp = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
				if (resp < 0) return -1;
			}
		}
		/* User description not known yet */ 
		else if (Heartbeat && unit->piraddr[i] && 0 == unit->piruser[i][0]) 
		{
			/* Send command to get user description for this PIR */
			if (trace) printf("\tPIR %d (%04x) user description not known\n", i+1, unit->piraddr[i]);
			rc = zbPollGetUser(zigbeeSocket, unit, unit->piraddr[i], cBuffer, usec);
			if (rc < 0) return -1;
			usleep(usec);
			resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
			if (resp < 0) 
				return -1;
			else
			{
				resp = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
				if (resp < 0) return -1;
			}
		}
		else if (unit->piraddr[i] && forceUnenroll)
		{
			/* Send command to unenroll zone for this PIR */
			if (trace) printf("\tPIR %d (%04x) zone unenroll '%s' update\n", i+1, unit->piraddr[i], unit->pirieee[i]);
			for (dstEndPoint=1; dstEndPoint<=3; dstEndPoint++)
			{
				rc = zbPollZoneUnEnroll(zigbeeSocket, unit, unit->piraddr[i], dstEndPoint, 0, cBuffer, usec);
				if (rc < 0) return -1;
				usleep(usec);
				resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
				if (resp < 0) 
					return -1;
				else
				{
					resp = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
					if (resp < 0) return -1;
				}
			}
		}
		/* Network and IEEE addresses known */
		else if (unit->piraddr[i] && unit->pirieee[i][0])
		{
			if (trace) printf("\tPIR %d : %04x (%s) installed in %s\n", i+1, unit->piraddr[i], unit->pirieee[i], unit->pirdesc[i]);
		}
		else
		{
			if (trace) printf("\tNo PIR %d installed\n", i+1);
		}
	}
	return 0;
}

/*! \brief Update PIR
	\param	rxBuff	data received from Zigbee
 */
static int zbPollUpdatePirShort(unitInfoRec * unit, 
				char * cBuffer, 
				int shortAd)
{
	int i, amod;
	char ieeeAd[32];

	if (trace)
	{
		printf("\tShort Address response: ");
		amod = AddrMode(&cBuffer[8]);
		if (amod == 2)
			SrcAddr(&cBuffer[22], 2);
		else
			dispIeeeAddr(&cBuffer[10], 0);
		Status(&cBuffer[26]);
		dispIeeeAddr(&cBuffer[28], 0);
		printf("\n");
	}
	
	strncpy(ieeeAd, &cBuffer[28], 16);
	ieeeAd[16] = 0;
	if (trace) printf("\tShort adress for IEEE address %s is %04x\n", ieeeAd, shortAd);

	for (i=0; i<MAX_PIR; i++)
	{
		if (strcmp(ieeeAd, unit->pirieee[i]) == 0)
		{
			unit->piraddr[i] = shortAd;
			if (trace) printf("\tUpdate PIR %d short address to %04x\n", i+1,  unit->piraddr[i]);
			break;
		}
	}
	return 0;
}

/*! \brief Update PIR
	\param	rxBuff	data received from Zigbee
 */
static int zbPollUpdatePir(int skt, 
			   unitInfoRec * unit,
			   char * rxBuff,
			   int usec)
{
	char shortAdStr[8];
	int shortAd, zone;
	int i, pidx=-1;
	char pirIeeeAd[32];
	int pirShortAd = 0;
	
	/* Extract zone from message */
	strncpy(shortAdStr, &rxBuff[14], 4);
	shortAdStr[4] = 0;
	zone = (int) strtol(shortAdStr, NULL, 16);
	if (0 != strcmp(shortAdStr, "0500"))
	{
		if (verbose) printf("%s Zone %s not for PIR\n", DateTimeStampStr(-1), shortAdStr);
		updateDbPirError(unit);
		return -1;
	}

	/* Extract network address from message */
	strncpy(shortAdStr, &rxBuff[8], 4);
	shortAdStr[4] = 0;
	shortAd = (int) strtol(shortAdStr, NULL, 16);
	
	if (0 == shortAd)
	{
		if (verbose) printf("%s Invalid network address 0000 for PIR\n", DateTimeStampStr(-1));
		updateDbPirError(unit);
		return -1;
	}
	
	/* Go through list of PIRs */
	pirIeeeAd[0] = 0;
	if (trace) printf("\tUpdate PIR with short address %04x\n", shortAd);
	for (i=0; i<MAX_PIR; i++)
	{
		if (0==unit->piraddr[i] && 0==unit->pirieee[i][0]) break;
		if (shortAd == unit->piraddr[i]) 
		{
			pirShortAd = shortAd;
			strcpy(pirIeeeAd, unit->pirieee[i]);
			unit->pirtime[i] = time(NULL);
			pidx = i;
			if (verbose) 
				printf("%s Event on PIR %d: Nwk %04x IEEE %s in %s\n", 
				       DateTimeStampStr(-1), i+1, pirShortAd, pirIeeeAd, unit->pirdesc[i]);
			break;
		}
	}

	/* Check whether network address is in list */
	if (pirShortAd == 0)
	{
		if (trace) printf("\tNew PIR %04x\n", shortAd);
		/* Go through list of PIRs again */
		for (i=0; i<MAX_PIR; i++)
		{
			/* Add address to first available position in list */
			if (unit->piraddr[i] == 0 && unit->pirieee[i][0] == 0)
			{
				if (verbose) printf("New PIR %d: nwk %04x\n", i, shortAd);
				unit->piraddr[i]  = shortAd;
				pidx = i;
				break;
			}
		}
		if (pidx<0) fprintf(stderr, "WARNING: Could not add additional PIR %04X to list\n", shortAd);
	}
	else if (0 == pirIeeeAd[0])
	{
		if (trace) printf("\tPIR %d has no IEEE adress\n", i+1);
		zbPollGetIeee(skt, unit, pirShortAd, rxBuff, usec);
	}
	else if (pirIeeeAd[0] == ' ')
	{
		if (trace) printf("\tPIR %d has empty IEEE adress '%s'\n", i+1, pirIeeeAd);
		zbPollGetIeee(skt, unit, pirShortAd, rxBuff, usec);
	}
	else
	{
		/* Go through list of PIRs again */
		for (i=0; i<MAX_PIR; i++)
		{
			if (i!=pidx && shortAd==unit->piraddr[i] && 0==unit->pirieee[i][0])
			{
				if (trace) printf("\tReset duplicate PIR %d\n", i+1);
				unit->piraddr[i] = 0;
			}
		}
	}
	if (pidx>=0) unitDbPirEvent(unit, pidx);
	return 0;
}

/*! \brief Update PIR
	\param	rxBuff	data received from Zigbee
 */
static int zbPollUpdateHeartbeat(int skt, 
				 unitInfoRec * unit,
				 char * rxBuff)
{
	char shortAdStr[8];
	int shortAd;
	int i, pidx=-1;
	char pirIeeeAd[32];
	int pirShortAd = 0;

	snprintf(shortAdStr, sizeof(shortAdStr), "%c%c%c%c", rxBuff[42], rxBuff[43], rxBuff[40], rxBuff[41] );
	shortAdStr[4] = 0;
	errno = 0;
	shortAd = (int) strtol(shortAdStr, NULL, 16);
	if (0==shortAd || errno)
	{
		printf("%s Ignore invalid network address %s for heartbeat\n", DateTimeStampStr(-1), shortAdStr);
		return -1;
	}
	
#if 0
	pirIeeeAd[0] = 0;
	j = 0;
	for (i=14; i>=0; i-=2)
	{
		pirIeeeAd[j++] = rxBuff[i];
		pirIeeeAd[j++] = rxBuff[i+1];
	}
	pirIeeeAd[j] = 0;
#endif
	if (trace) printf("\tHeartbeat on nwk %04x\n", shortAd);
	
	/* Go through list of PIRs */
	for (i=0; i<MAX_PIR; i++)
	{
		if (0==unit->piraddr[i] && 0==unit->pirieee[i][0]) break;
		if (shortAd == unit->piraddr[i]) 
		{
			pirShortAd = shortAd;
			strcpy(pirIeeeAd, unit->pirieee[i]);
			unit->pirbeat[i] = time(NULL);
			pidx = i;
			if (trace) printf("\tMatched short address %04x to PIR %d in %s\n", pirShortAd, pidx+1, unit->pirdesc[i]);
			break;
		}
	}

	if (pidx>=0)
	{
		if (verbose) 
		{
			printf("%s ", DateTimeStampStr(-1));
			dispIeeeAddr(&rxBuff[24], 1);
			printf(" Heartbeat on PIR %d with network address %04x\n", pidx+1, shortAd);
		}
		updateDbHeartbeat(unit, pidx);
	}
	else
	{
		if (verbose) 
		{
			printf("%s Heartbeat on network address %04x (IEEE ", DateTimeStampStr(-1), shortAd);
			dispIeeeAddr(&rxBuff[24], 1);
			printf(")\n");
		}
	}
	return 0;
}

/*! \brief Update PIR
	\param	rxBuff	data received from Zigbee
 */
static int zbPollResetLogs(int skt, 
			   unitInfoRec * unit,
			   char * rxBuff)
{
	int shortAd;
	int i;
	
	if (unit->ustatus == ZBSTATUS_ARMED) 
	{
		if (trace) printf("\tAlarm is set - skip reset of alarm logs\n");
		return 0;
	}
	if (trace) printf("\tReset alarm logs\n");
	
	/* Coordinator first */
	zbPollResetLog(skt, unit, 0, 1, 0x500, rxBuff);

	/* Go through list of PIRs */
	for (i=0; i<MAX_PIR; i++)
	{
		shortAd = unit->piraddr[i]; 
		if (0==shortAd) break;
		zbPollResetLog(skt, unit, shortAd, 1, 0x500, rxBuff);
	}
	
	unit->alrmreset = time(NULL);

	return 0;
}

/*! \brief Status update for IEEE
	\param	unit	Record with addresses
	\param	rxBuff	Data received from Zigbee
	\return 1 if record updated otherwise 0
 */
static int zbPollUpdateIeee(unitInfoRec * unit, 
			    char * rxBuff)
{
	int shortAd;
	char shortAdStr[8];
	char ieeeAd[64];
	int rc;

	/* Go through list of PIRs */
	if (trace) printf("\tCheck PIR IEEE\n");

	if (trace) dbUnitDisplayPirInfo(unit);

	strncpy(shortAdStr, &rxBuff[22], 4);
	shortAdStr[4] = 0;
	shortAd = (int) strtol(shortAdStr, NULL, 16);
	if (trace) printf("\tConvert '%s' to %04x\n", shortAdStr, shortAd);
	
	if (0 != strncmp(&rxBuff[26], "00", 2))
	{
		if (trace) printf("\tStatus for IEEE is not OK\n");
		return 0;
	}
	
	strncpy(ieeeAd, &rxBuff[28], 16);
	ieeeAd[16] = 0;
	rc = zbPollCheckIeee(unit, shortAd, ieeeAd);
	return rc;
}

/*! \brief Status update for User description
	\param	unit	Record with addresses
	\param	rxBuff	Data received from Zigbee
	\return 1 if record updated otherwise 0
 */
static int zbPollUpdateUser(unitInfoRec * unit, 
			    char * rxBuff)
{
	int shortAd;
	char shortAdStr[8];
	char userDesc[64];
	int i, n, rc, dc;

	/* Go through list of PIRs */
	if (trace) printf("\n");

	if (trace) dbUnitDisplayPirInfo(unit);

	strncpy(shortAdStr, &rxBuff[10], 4);
	shortAdStr[4] = 0;
	shortAd = (int) strtol(shortAdStr, NULL, 16);
	
	strncpy(userDesc, &rxBuff[18], 2);
	userDesc[2] = 0;
	n = strtol(userDesc, NULL, 16);
	for (i=0; i<n; i++)
	{
		sprintf(shortAdStr, "%c%c", rxBuff[20+i*2], rxBuff[21+i*2]);
		dc = strtol(shortAdStr, NULL, 16);
		if (isprint(dc)) userDesc[i] = dc;
		else userDesc[i] = '.';
	}
	userDesc[n] = 0;
	if (trace) printf("\tCheck PIR user description DescLen %d, Descriptor '%s'\n", n, userDesc);
	rc = zbPollCheckUser(unit, shortAd, userDesc);
	return rc;
}

/*! \brief Status update for IEEE
	\param	unit	Record with addresses
	\param	rxBuff	Data received from Zigbee
	\return 1 if record updated otherwise 0
 */
static int zbPollAnnounceIeee(unitInfoRec * unit, 
			      char * rxBuff)
{
	int shortAd;
	char shortAdStr[8];
	char ieeeAd[64];
	int rc;

	/* Go through list of PIRs */
	if (trace) printf("\tCheck IEEE announced\n");

	if (trace) dbUnitDisplayPirInfo(unit);

	strncpy(shortAdStr, &rxBuff[8], 4);
	shortAdStr[4] = 0;
	shortAd = (int) strtol(shortAdStr, NULL, 16);

	readIeeeAddr(&rxBuff[12], ieeeAd);

	rc = zbPollCheckIeee(unit, shortAd, ieeeAd);
	return rc;
}

/*! \brief Check for update on IEEE
	\param	unit		Record with addresses
	\param	shortAd		Short address
	\param	ieeeAd		IEEE address
	\return 1 if record updated otherwise 0
 */
static int zbPollCheckIeee(unitInfoRec * unit, 
			   int shortAd, 
			   char * ieeeAd)
{
	int i;

	if (trace) printf("\tIEEE for %04x is %s\n", shortAd, ieeeAd);
	
	if (shortAd == 0)
	{
		strcpy(unit->cordieee, ieeeAd);
		updateDbCoordinatorIeee(unit);
		if (trace) printf("\tIEEE for cordinator set to %s\n", ieeeAd);
	}
	else
	{
		/* Go through list of IEEE addresses for PIRs */
		for (i=0; i<MAX_PIR; i++)
		{
			if (0==strcmp(ieeeAd, unit->pirieee[i])) 
			{
				unit->piraddr[i] = shortAd;
				if (trace) printf("\tShort address %d for %s set to %04x\n", i, ieeeAd, shortAd);
				if (trace) dbUnitDisplayPirInfo(unit);
				unitDbUpdateAddr(unit, i);
				ieeeAd[0] = 0;
				break;
			}
			else if (unit->piraddr[i]==shortAd)
			{
				strcpy(unit->pirieee[i], ieeeAd);
				if (trace) printf("\tIEEE address %d for %04x set to %s\n", i, shortAd, ieeeAd);
				unitDbUpdateAddr(unit, i);
				if (trace) dbUnitDisplayPirInfo(unit);
				ieeeAd[0] = 0;
				break;
			}
		}
		if (ieeeAd[0] == 0)
		{
			if (trace) printf("\tCheck for duplicate PIRs\n");
			for (i=0; i<MAX_PIR; i++)
			{
				if (0==unit->pirieee[i][0] && unit->piraddr[i] == shortAd)
				{
					if (trace) printf("\tRemove duplicate PIR %d from list\n", i+1);
					unit->piraddr[i] = 0;
				}
			}
		}
		else
		{
			if (trace) printf("\tIEEE for %04x not updated to '%s'\n", shortAd, ieeeAd);
		}
	}
	return 0;
}

/*! \brief Check for update on IEEE
	\param	unit		Record with addresses
	\param	shortAd		Short address
	\param	ieeeAd		IEEE address
	\return 1 if record updated otherwise 0
 */
static int zbPollCheckUser(unitInfoRec * unit, 
			   int shortAd, 
			   char * userDesc)
{
	int i, rc;

	if (trace) printf("\tUser description for %04x is %s\n", shortAd, userDesc);
	
	if (shortAd == 0)
	{
		if (trace) printf("\tIgnore user description for coordinator (set to %s)\n", userDesc);
		if (unitDbUpdateHwInfo(unit, shortAd, NULL, userDesc))
			unitDbInsertHwInfo(unit, shortAd, NULL, userDesc);
	}
	else
	{
		/* Go through list of IEEE addresses for PIRs */
		for (i=0; i<MAX_PIR; i++)
		{
			if (unit->piraddr[i]==shortAd)
			{
				strcpy(unit->piruser[i], userDesc);
				if (trace) printf("\tUser description %d for %04x set to %s\n", i, shortAd, userDesc);
				/* unitDbUpdateAddr(unit, i); */
				if (trace) dbUnitDisplayPirInfo(unit);
				rc = unitDbInsertHwInfo(unit, shortAd, NULL, userDesc);
				if (rc) unitDbUpdateHwInfo(unit, shortAd, NULL, userDesc);
				userDesc[0] = 0;
				break;
			}
		}
		if (userDesc[0] == 0)
		{
			if (trace) printf("\tCheck for duplicate PIRs\n");
			for (i=0; i<MAX_PIR; i++)
			{
				if (0==unit->piruser[i][0] && unit->piraddr[i] == shortAd)
				{
					if (trace) printf("\tRemove duplicate PIR %d from list\n", i+1);
					unit->piraddr[i] = 0;
				}
			}
		}
		else
		{
			if (trace) printf("\tUser description for %04x not updated to '%s'\n", shortAd, userDesc);
		}
	}
	return 0;
}

/*! \brief Display list of PIRs
	\param	unit	Record with addresses
 */
static void zbPollPirListEvent(unitInfoRec * unit)
{
	int i, lastloc = 0;
	static time_t lastevnt = 0;
	char dtEvent[64];
	
	printf("\tList PIR events\n");
	
	for (i=0; i<MAX_PIR; i++)
	{
		if (trace) 
		{
			if (unit->pirtime[i]) getDateTimeStamp(&unit->pirtime[i], dtEvent, sizeof(dtEvent));
			else strcpy(dtEvent, "none");
			printf("\tPIR %d: %04x (%s)\t%s update %s\n", 
			       i+1, unit->piraddr[i], unit->pirieee[i]?unit->pirieee[i]:"Unknown", unit->pirdesc[i], dtEvent);
		}
		if (unit->pirtime[i] > lastevnt)
		{
			lastevnt = unit->pirtime[i];
			lastloc = unit->pirloc[i];
		}
	}
}

/*! \brief Check when alarm should be armed
 */
static int zbPollCheckRearm(unitInfoRec * unit, 
			    int zigbeeSocket)
{
	struct tm * dtm;
	time_t ts, cts;
	 
	cts = time(NULL);
	
	if (unit->momentarm) 
	{
		ts = unit->momentarm;
		dtm = (struct tm *)localtime(&ts);
		if (trace) 
			printf("\tAlarm marked for rearming at '%04d-%02d-%02d %02d:%02d:%02d'\n",
				dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
				dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
		if (cts >= ts)
		{
			if (trace) printf("\tRearm system\n");
			zbPollArm(zigbeeSocket);
			updateDbResetRearm(unit);
		}
	}
	return 0;
}

/*! \brief Send Zigbee command to set alarm
	\param zigbeeSocket	socket already opened
	\return 0 when successful, else -1
 */
static int zbPollArm(int zigbeeSocket)
{
	int iLength = 0;
	int retval;
	char cBuffer[128];

	strcpy(cBuffer, PktArm);
	iLength = strlen(cBuffer);
	if (trace) printf("\tSend Zigbee command to set alarm\n");	
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(cBuffer, iLength, "<arm>\n");
	}
	retval = zbSocketWrite(zigbeeSocket, cBuffer, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollArm FAIL LEN:%i\n", iLength);
		return -1;
	}

	return 0;
}

/*! \brief Send command to read room temperature from PIR
	\param	zigbeeSocket	socket already opened
	\param	shortAddress	short Zigbee address
	\return 0 when successful, else -1

Request format e.g.
	020D000902F8A70304020100005D                       (28 bytes)
	* CmdId req 0d00 Att, Len  9, Read Attribute: AddrMode Short Address 02, DstAddr F8A7, 
	DstEndpoint 03, ClusterId Temperature (0x0402), Number 01, AttributeID1 0000, FCS 5D 
	
Reply format e.g.
	020D010CF8A70304020100000029072E5BZ                (35 bytes)
	* CmdId req 0d01 Att, Len 12, Read Attribute response: SrcAddr F8A7, SrcEndpoint 03, 
	ClusterId Temperature (0x0402), Number 01, AttributeId1 0000, Status OK, 
	DataType Signed 16-bit integer  2. bytes  01838, FCS 5B Z
 */
int zbPollTemperature(int zigbeeSocket, 
		      unitInfoRec * unit,
		      char * cBuffer,
		      int usec)
{
	int i, rc, resp, iLength = 0;
	int retval;
	char zigBeePktStr[64];
	char zbStr[64];
	int shortAddress = 0;

	for (i=0; i<MAX_PIR; i++)
	{
		if (unit->piraddr[i]) shortAddress = unit->piraddr[i];
	}
	if (0 == shortAddress)
	{
		fprintf(stderr, "%s ERROR: No short address for temperature readings\n", DateTimeStampStr(-1));
		return -1;
	}
	
	for (i=0; i<MAX_PIR; i++)
	{
		if (unit->piraddr[i])
		{
			shortAddress = unit->piraddr[i];
			if (trace) printf("\tGet temperature from PIR %d (%04x)\n", i+1, shortAddress);

			/* Create Zigbee command string using short address (see above) */
			snprintf(zigBeePktStr, sizeof(zigBeePktStr), 
				"020D000B02%04X0304020200000004", 
				shortAddress);

			retval = zbSendCheck(zigBeePktStr, zbStr);
			if (retval)
			{
				if (trace) printf("\tTemperature command packet check fixed\n");
			}
			if (trace) printf("\tTemperature command is %s\n", zbStr);

			/* Lemgth of data in Asci HEX Bytes ie. 2 Chars per byte */
			iLength = strlen(zbStr);
			if (! iLength) return -1;
			if (trace) dumpTXdata("\tZigBee TX", zbStr, iLength);
			if (verbose)
			{
				DateTimeStamp();
				dispBuffer(zbStr, iLength, "<tx>\n");
			}

			/* Send Zigbee command */
			retval = zbSocketWrite(zigbeeSocket, zbStr, iLength);
			if (retval < 0)
			{
				if (trace) fprintf(stderr, "zbPollTemperature FAIL LEN:%i\n", iLength);
				return -1;
			}
			
			/* Wait for reply */
			usleep(usec);
			resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
			if (resp < 0) 
				return -1;
			else
			{
				rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
				if (rc < 0) return -1;
			}
		}
	}
	return 0;
}

/*! \brief Send command to Manage Network Discovery
	\param 	zigbeeSocket	socket already opened
	\param 	shortAddress	4-digit hexadecimal address
	\return true when successful, else false
	
	020A0E0902F9CB07FFF80010002D                       (28) * req 0a0e ZDO, Len  9, Manage Network Discovery: 
		AddrMode Short Address 02, DstAddr F9CB, ScanChannels ALL CHANNELS, ScanDuration 16, StartIndex 0, FCS 2D 

 */
int zbPollManageDiscovery(int zigbeeSocket, 
			  unitInfoRec * unit, 
			  int dstAddr,
			  char * cBuffer,
			  int usec)
{
	int iLength = 0;
	int retval, rc, resp;
	char zigBeePktStr[64];
	char zbStr[64];

	/* Create Zigbee command string using short address */
	snprintf(zigBeePktStr, sizeof(zigBeePktStr), "020A0E0902%04X07FFF8001000", dstAddr);
	retval = zbSendCheck(zigBeePktStr, zbStr);
	if (retval)
	{
		printf("Packet check failed\n");
		return -1;
	}
	if (trace) printf("\tSend command to manage network discovery for nwk %04x with command %s\n", dstAddr, zbStr);

	/* Length of data in Asci HEX Bytes ie. 2 Chars per byte */
	iLength = strlen(zbStr);
	if (! iLength) return -1;
	if (trace) dumpTXdata("\tZigBee TX", zbStr, iLength);
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zbStr, iLength, "<tx>\n");
	}

	/* Send Zigbee command */
	retval = zbSocketWrite(zigbeeSocket, zbStr, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollGetIeee FAIL LEN:%i\n", iLength);
		return -1;
	}
	
	/* Wait for reply */
	usleep(usec);
	resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
	if (resp < 0) return -1;
	else
	{
		rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
		if (rc < 0) return -1;
	}
	return 0;
}

/*! \brief Send command to un-enroll zone
	\param 	zigbeeSocket	socket already opened
	\param 	shortAddress	4-digit hexadecimal address
	\return true when successful, else false
	
	020C0207026ECF03000400AD                           (23) * req 0c02 ZCL, Len  7, IAS Zone Un-Enroll: 
		AddrMode Short Address 02, DstAddr 6ECF, DstEndpoint 03, ClusterId Groups (0x0004), ZoneID 00, FCS 0A
 */
int zbPollZoneUnEnroll(int zigbeeSocket, 
		       unitInfoRec * unit, 
		       int dstAddr,
		       int dstEndPoint,
		       int zoneId,
		       char * cBuffer,
		       int usec)
{
	int iLength = 0;
	int retval, resp, rc;
	char zigBeePktStr[128];
	char zbStr[128];

	/* Create Zigbee command string using short address */
	snprintf(zigBeePktStr, sizeof(zigBeePktStr), "020C020702%04X%02X0004%02X", dstAddr, dstEndPoint, zoneId);
	retval = zbSendCheck(zigBeePktStr, zbStr);
	if (retval)
	{
		printf("Packet check failed\n");
		return -1;
	}
	if (trace) printf("\tSend command to un-enroll zone for nwk %04x endpoint %02x zone %02X with command %s\n", dstAddr, dstEndPoint, zoneId, zbStr);

	/* Length of data in Asci HEX Bytes ie. 2 Chars per byte */
	iLength = strlen(zbStr);
	if (! iLength) return -1;
	if (trace) dumpTXdata("\tZigBee TX", zbStr, iLength);
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zbStr, iLength, "<tx>\n");
	}

	/* Send Zigbee command */
	retval = zbSocketWrite(zigbeeSocket, zbStr, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollGetIeee FAIL LEN:%i\n", iLength);
		return -1;
	}
			
	/* Wait for reply */
	usleep(usec);
	resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
	if (resp < 0) 
		return -1;
	else
	{
		rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
		if (rc < 0) return -1;
	}
	return 0;
}

/*! \brief Send command to reset alarm log
	\param 	zigbeeSocket	socket already opened
	\param 	shortAddress	4-digit hexadecimal address
	\return true when successful, else false
	
	4.40 Reset Alarm Log (Alarm)
	Cmd=0x0C03	Len=0x06	AddrMode	DstAddr	DstEndPoint	Cluster ID
		AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits GroupAddress (Addrmode = 0x01)
		DstAddr – 2 bytes –network address of the destination address.
		DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
		Cluster ID – 2 bytes –Alarm Cluster ID.

 */
static int zbPollResetLog(int zigbeeSocket, 
			  unitInfoRec * unit, 
			  int dstAddr,
			  int dstEndPoint,
			  int clusterId,
			  char * cBuffer)
{
	int iLength = 0;
	int retval;
	char zigBeePktStr[128];
	char zbStr[64];

	/* Create Zigbee command string using short address */
	snprintf(zigBeePktStr, sizeof(zigBeePktStr), "020C030602%04X%02X%04X", dstAddr, dstEndPoint, clusterId);
	retval = zbSendCheck(zigBeePktStr, zbStr);
	if (retval)
	{
		printf("Packet check failed\n");
		return -1;
	}
	if (trace) printf("\tSend command to reset alarm log for nwk %04x endpoint %02x cluster %02X with command %s\n", dstAddr, dstEndPoint,clusterId , zbStr);

	/* Length of data in Asci HEX Bytes ie. 2 Chars per byte */
	iLength = strlen(zbStr);
	if (! iLength) return -1;
	if (trace) dumpTXdata("\tZigBee TX", zbStr, iLength);
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zbStr, iLength, "<tx>\n");
	}

	/* Send Zigbee command */
	retval = zbSocketWrite(zigbeeSocket, zbStr, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollResetLog FAIL LEN:%i\n", iLength);
		return -1;
	}
	return 0;
}

/*! \brief Send command to read IEEE address
	\param 	zigbeeSocket	socket already opened
	\param 	shortAddress	4-digit hexadecimal address
	\return true when successful, else false
 */
static int zbPollGetIeee(int zigbeeSocket, 
			 unitInfoRec * unit, 
			 int shortAddress,
			 char * cBuffer, 
			 int usec)
{
	int iLength = 0;
	int retval, resp, rc;
	char zigBeePktStr[64];
	char zbStr[64];

	/* Create Zigbee command string using short address */
	if (shortAddress == 0)					/* Coordinator */
		strcpy(zigBeePktStr, "020A03060200000100000C");
	else
		snprintf(zigBeePktStr, sizeof(zigBeePktStr), "020A030602%04X000000", shortAddress);
	if (trace) printf("\tSend command to read IEEE address\n");

	retval = zbSendCheck(zigBeePktStr, zbStr);
	if (retval)
	{
		printf("Packet check failed\n");
		return -1;
	}
	if (trace) printf("\tGet IEEE address for %04x with command %s\n", shortAddress, zbStr);

	/* Length of data in Asci HEX Bytes ie. 2 Chars per byte */
	iLength = strlen(zbStr);
	if (! iLength) return -1;
	if (trace) dumpTXdata("\tZigBee TX", zbStr, iLength);
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zbStr, iLength, "<tx>\n");
	}

	/* Send Zigbee command */
	retval = zbSocketWrite(zigbeeSocket, zbStr, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollGetIeee FAIL LEN:%i\n", iLength);
		return -1;
	}
			
	/* Wait for reply */
	usleep(usec);
	resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
	if (resp < 0) 
		return -1;
	else
	{
		rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
		if (rc < 0) return -1;
	}
	return 0;
}

/*! \brief Send command to read user description
	\param 	zigbeeSocket	socket already opened
	\param 	shortAddress	4-digit hexadecimal address
	\return true when successful, else false
 */
static int zbPollGetUser(int zigbeeSocket, 
			 unitInfoRec * unit, 
			 int shortAddress,
			 char * cBuffer, 
			 int usec)
{
	int iLength = 0;
	int retval, resp, rc;
	char zigBeePktStr[64];
	char zbStr[64];

	/* Create Zigbee command string using short address 
	 Cmd=0x0A0A       Len=0x06        AddrMode    DstAddr NWKAddrOfIneterest   SecuritySuite

		AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
			GroupAddress (Addrmode = 0x01)
		DstAddr – 16 bit – NWK address of the device generating the inquiry..
		NWKAddrOfInterest – 16 bit - NWK address of the destination device being queried.
		SecuritySuite - 1 byte – Security options.
	 */

	snprintf(zigBeePktStr, sizeof(zigBeePktStr), "020A0A0602%04X%04X00", shortAddress, shortAddress);

	retval = zbSendCheck(zigBeePktStr, zbStr);
	if (retval)
	{
		printf("Packet check failed\n");
		return -1;
	}
	if (trace) printf("\tGet user description for %04x with command %s\n", shortAddress, zbStr);

	/* Length of data in Asci HEX Bytes ie. 2 Chars per byte */
	iLength = strlen(zbStr);
	if (! iLength) return -1;
	if (trace) dumpTXdata("\tZigBee TX", zbStr, iLength);
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zbStr, iLength, "<tx>\n");
	}

	/* Send Zigbee command */
	retval = zbSocketWrite(zigbeeSocket, zbStr, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollGetIeee FAIL LEN:%i\n", iLength);
		return -1;
	}
			
	/* Wait for reply */
	usleep(usec);
	resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
	if (resp < 0) 
		return -1;
	else
	{
		rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
		if (rc < 0) return -1;
	}
	
	return 0;
}

/*! \brief Send command to read short address
	\param 	zigbeeSocket	socket already opened
	\param 	zbStr		Zigbee command string
	\return 0 when successful, else false
 */
static int zbPollGetShort(int zigbeeSocket, 
			  unitInfoRec * unit, 
			  char * ieeeAddress,
			  char * cBuffer,
			  int usec)
{
	int iLength = 0;
	int retval, resp, rc;
	char zigBeePktStr[64];
	char zbStr[128];

	if (! ieeeAddress)
	{
		if (trace) fprintf(stderr, "\tIEEE address not known\n");
		return -1;
	}

	if (strlen(ieeeAddress) != 16)
	{
		fprintf(stderr, "Invalid IEEE address '%s'\n", ieeeAddress);
		return -1;
	}

	/* Create Zigbee command string using IEEE address */
	snprintf(zigBeePktStr, sizeof(zigBeePktStr), "020A020C03%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c000000", 
		 ieeeAddress[14], ieeeAddress[15], ieeeAddress[12], ieeeAddress[13], 
		 ieeeAddress[10], ieeeAddress[11], ieeeAddress[8], ieeeAddress[9],
		 ieeeAddress[6], ieeeAddress[7], ieeeAddress[4], ieeeAddress[5], 
		 ieeeAddress[2], ieeeAddress[3], ieeeAddress[0], ieeeAddress[1]);

	retval = zbSendCheck(zigBeePktStr, zbStr);
	if (retval)
	{
		printf("Packet check failed\n");
		return -1;
	}

	/* Length of data in Asci HEX Bytes ie. 2 Chars per byte */
	iLength = strlen(zbStr);
	if (! iLength) return -1;
	if (trace) dumpTXdata("\tZigBee TX", zbStr, iLength);
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zbStr, iLength, "<tx>\n");
	}
	if (trace) printf("\tGet short address for %s : %s\n", ieeeAddress, zbStr);

	/* Send Zigbee command */
	retval = zbSocketWrite(zigbeeSocket, zbStr, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollGetShort FAIL LEN:%i\n", iLength);
	}
			
	/* Wait for reply */
	usleep(usec);
	resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
	if (resp < 0) 
		return -1;
	else
	{
		rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
		if (rc < 0) return -1;
	}
	return 0;
}

/*! \brief Send Zigbee command to sound alarm
	\param	zigbeeSocket	socket already opened
	\param	shortAddress	short Zigbee address
	\return 0 when successful, else -1
 */
static int zbPollSendPkt(int zigbeeSocket,
			 unitInfoRec * unit, 
			 char * zbPkt, 
			 char * cBuffer,
			 int usec)
{
	int iLength = 0;
	int retval, resp, rc;

	if (trace) printf("\tSend Zigbee command packet: %s\n", zbPkt);
	iLength = strlen(zbPkt);	
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zbPkt, iLength, "\n");
	}
	retval = zbSocketWrite(zigbeeSocket, zbPkt, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollSendPkt FAIL LEN:%i\n", iLength);
		return -1;
	}
			
	/* Wait for reply */
	usleep(usec);
	resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
	if (resp < 0) 
		return -1;
	else
	{
		rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
		if (rc < 0) return -1;
	}

	return 0;
}

/*! \brief Set up heartbeat
*/
static int zbPollSetHeartbeat(int zigbeeSocket,
			      unitInfoRec * unit,
			      char * cBuffer,
			      int usec)
{
	int iLength = 0;
	int retval, resp, rc;
	char zbStr[128], zbValStr[8], zigBeePktStr[128];
	int heartBeatPrd, heartBeatLoss;
	
	heartBeatPrd = unit->hbeatmins;
	if (! heartBeatPrd) heartBeatPrd = 60;
	sprintf(zbValStr, "%04X", heartBeatPrd);
	
	heartBeatLoss = unit->hbeatloss;
	if (! heartBeatLoss) heartBeatLoss = 3;
	
	if (trace) printf("\tSet Zigbee heartbeat to %d minutes, loss %d\n", heartBeatPrd, heartBeatLoss);
	snprintf(zbStr, sizeof(zbStr), "020C0809020000010501%c%c%c%c%02X",
		 zbValStr[2], zbValStr[3], zbValStr[0], zbValStr[1], heartBeatLoss);
	retval = zbSendCheck(zbStr, zigBeePktStr);
	if (retval)
	{
		printf("Packet check failed\n");
		return -1;
	}

	if (trace) printf("\tSend Zigbee command packet: %s\n", zigBeePktStr);
	iLength = strlen(zigBeePktStr);	
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zigBeePktStr, iLength, "\n");
	}
	retval = zbSocketWrite(zigbeeSocket, zigBeePktStr, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollSendPkt FAIL LEN:%i\n", iLength);
		return -1;
	}
			
	/* Wait for reply */
	usleep(usec);
	resp = zbSocketReadPacket(zigbeeSocket, cBuffer);
	if (resp < 0) 
		return -1;
	else
	{
		rc = readBuffer(zigbeeSocket, unit, cBuffer, resp, usec);
		if (rc < 0) return -1;
	}
	unit->hbeatreq = time(NULL);

	return 0;
}
