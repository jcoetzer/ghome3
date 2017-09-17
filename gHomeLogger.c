/*! \file gHomeLogger.c
	\brief Listen to Zigbee messages and log events
 */

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.3 $
 * $State: Exp $
 *
 * $Log: gHomeLogger.c,v $
 * Revision 1.3  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.2  2011/06/13 21:57:26  johan
 * Update
 *
 * Revision 1.1.1.1  2011/05/28 09:00:03  johan
 * gHome logging server
 *
 * Revision 1.7  2011/02/18 06:43:46  johan
 * Streamline polling
 *
 *
 *
 *
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <popt.h>
#include <libgen.h>
#include <sys/types.h>  /* for open, etc */
#include <sys/stat.h>   /* for open, etc */
#include <fcntl.h>      /* for open, etc */
#include <unistd.h>     /* for close, etc */

#include "gHomeLogger.h"
#include "zbDisplay.h"
#include "zbPoll.h"
#include "updateDb.h"
#include "zbData.h"
#include "dbConnect.h"
#include "gHomeConf.h"

#include "gversion.h"

int verbose = 0;
int ShowVersion = 0;
int AlarmDelay = 0;
int ReadTemp = 0;
int UnEnroll = 0;
int trace = 0;
int live = 0;

int UnitNum = -1;
char * UnitName = NULL;
char * ZigbeeStart = NULL;

unsigned short GatewayPort = 0;     		/* Echo server port */
char * GatewayIp = NULL;                    	/* Server IP address (dotted quad) */
int SleepTime = 2000;
int ConnectionTimer = 150;
int Heartbeat = 0;

/*! \struct optionsTable Command line parsing data structure */
struct poptOption optionsTable[] =
{
	{"delay", 	'D', POPT_ARG_INT, 	&AlarmDelay, 0, "Display version information", NULL},
	{"gateway", 	'G', POPT_ARG_STRING, 	&GatewayIp, 0,  "Zigbee gateway address",  "<ip_address>"},
	{"name", 	'N', POPT_ARG_STRING,	&UnitName, 0, "Unit name",  "<string>"},
	{"port", 	'P', POPT_ARG_INT, 	&GatewayPort, 0,  "Gateway port (default is 5000)", "<integer>"},
	{"sleep", 	'S', POPT_ARG_INT, 	&SleepTime, 0, "Sleep time (in milliseconds, default is 2000)",  "<integer>"},
	{"timer", 	'T', POPT_ARG_INT, 	&ConnectionTimer, 0, "Number of poll cycles between updates (default is 150)",  "<integer>"},
	{"unit", 	'U', POPT_ARG_INT, 	&UnitNum, 0, "Unit number",  "<integer>"},
	{"version", 	'V', POPT_ARG_NONE, 	&ShowVersion, 0, "Display version information", NULL},
	{"unenroll", 	'u', POPT_ARG_NONE, 	&UnEnroll, 0, "Force un-enroll for PIRs", NULL},
	{"zigbee", 	'Z', POPT_ARG_STRING, 	&ZigbeeStart, 0, "Zigbee command on startup",  "<string>"},
	{"live", 	'l', POPT_ARG_NONE, 	&live, 0, "Live update", NULL},
        {"heartbeat",   'b', POPT_ARG_NONE,     &Heartbeat, 0, "Activate heartbeat for PIRs", NULL},
	{"temp", 	'l', POPT_ARG_NONE, 	&ReadTemp, 0, "Read temperature from PIRs", NULL},
 	{"trace", 	't', POPT_ARG_NONE, 	&trace, 0, "Trace/debug output", NULL},
	{"verbose",	'v', POPT_ARG_NONE, 	&verbose, 0, "Trace output", NULL},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

unitInfoRec unitInfo;

/*! \brief Signal handler
*/
void sighandle(int signum)
{
	switch (signum)
	{
		case SIGUSR1 :
		{
			printf("\n\n%s ***** Restart after signal %d *****\n", DateTimeStampStr(-1), signum);
			LoggerStart(1, ReadTemp);
			break;
		}
		case SIGUSR2 :
		{
			printf("\n\n%s ***** Restart after signal %d (keep command line options) *****\n", DateTimeStampStr(-1), signum);
			LoggerStart(0, ReadTemp);
			break;
		}
		case SIGPIPE :
		{
			printf("\n\n%s ***** Restart after signal SIGPIPE *****\n", DateTimeStampStr(-1));
			LoggerStart(0, ReadTemp);
			break;
		}
		default :
		{
			printf("\n%s Shutting down after signal %d\n", DateTimeStampStr(-1), signum);
			fflush(stdout);
			fflush(stderr);
			exit(1);
		}
	}
}

/*! \brief Main routine
 */
int main(int argc, 
	 char *argv[])
{
	int rc, loopc=0;
	poptContext optcon;        /* Context for parsing command line options */

	optcon = poptGetContext(argv[0],argc,(const char **)argv,optionsTable,0);
	rc = poptGetNextOpt(optcon);
	if (rc < -1)
	{
		fprintf(stderr, "%s: %s - %s\n", basename(argv[0]), poptBadOption(optcon,0),
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

	signal(SIGHUP, sighandle);
	signal(SIGINT, sighandle);
	signal(SIGQUIT, sighandle);
	signal(SIGILL, sighandle);
	signal(SIGTRAP, sighandle);
	signal(SIGABRT, sighandle);
	signal(SIGIOT, sighandle);
	signal(SIGBUS, sighandle);
	signal(SIGFPE, sighandle);
	signal(SIGUSR1, sighandle);
	signal(SIGPIPE, sighandle);
	signal(SIGUSR2, sighandle);
	
	srand(time(NULL));

	/* Endless task loop */
	while(1)
	{
		++loopc;
		if (verbose) printf("%s Start logger %d\n", DateTimeStampStr(-1), loopc);
		rc = LoggerStart(0, ReadTemp);
		if (rc)
		{
			poptPrintUsage(optcon, stderr, 0);
			return -1;
		}
		sleep(10);
	}
	poptFreeContext(optcon);
	return 0;
}

/*! \brief Initialize logging
 */
int LoggerStart(int n,
		int readTemp)
{
	int rc;
	int rdTimeout = 0;
	PGconn * dbConn;
	gHomeConfRec cfg;
	
	gHomeConfInit(&cfg);
	unitDbInit(&unitInfo, UnitNum, 0);

	dbConn = PQconnectdb(connection_str());
	if (PQstatus(dbConn) == CONNECTION_BAD) 
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Connection to %s failed, %s", connection_str(),
			PQerrorMessage(dbConn));
		return -1;
	}
	unitInfo.dbConnctn = dbConn;
	
	unitDbSetLocation(&unitInfo);

	unitInfo.unitNo = UnitNum;

	if (LoggerCheckPid(unitInfo.pid))
	{
		PQfinish(dbConn);
		exit(1);
	}

	/* Read unit info from tables adomis_box and hardware */
	rc = unitDbGetInfo(&unitInfo);
	if (rc)
	{
		fprintf(stderr, "Unit info not found in database\n");
		PQfinish(dbConn);
		return -1;
	}
	if (0==verbose && 0==trace)
	{
		printf("%s Log level is %d\n", DateTimeStampStr(-1), unitInfo.loglevel);
		switch(unitInfo.loglevel)
		{
			case 2 : verbose = 1; trace = 0; break;
			case 3 : verbose = 1; trace = 1; break;
		}
	}

	/* Read PIR info from table hardware */
	rc = unitDbGetPirHardware(&unitInfo);
	if (rc)
	{
		fprintf(stderr, "PIR info not found in database\n");
		PQfinish(dbConn);
		return -1;
	}

	/* Read PIR temperature data from table statuscurr */
	if (readTemp)
	{
		rc = unitDbGetPirTemp(&unitInfo);
		if (rc)
		{
			fprintf(stderr, "PIR temperature data not found in database\n");
			PQfinish(dbConn);
			return -1;
		}
	}
	/* Read PIR events and heartbeats from table statuscurr */
	unitDbPirMoment(&unitInfo);
	if (rc)
	{
		fprintf(stderr, "PIR event data not found in database\n");
		PQfinish(dbConn);
		return -1;
	}
	
	/* Read options from command line */
        if (! n)
	{
		if (live) unitInfo.live = 1;
		if (NULL != UnitName) strcpy(unitInfo.appt, UnitName);
		if (NULL != GatewayIp) strcpy(unitInfo.gwip, GatewayIp);
		if (0 != GatewayPort) unitInfo.gwport = GatewayPort;
		else unitInfo.gwport = ZIGBEE_GATEWAY_PORT;
	}
	/* Check name */
	if (0 == unitInfo.appt[0])
	{
		printf("WARNING: No name for unit\n");
		sprintf(unitInfo.appt, "Unit %d", unitInfo.unitNo);
	}
	/* Check IP address for Zigbee gateway */
	if (0 == unitInfo.gwip[0])
	{
		printf("No gateway IP address for unit\n");
		PQfinish(dbConn);
		return -1;
	}
	
	/* Sort PIR records by IEEE address */
	if (trace) dbUnitDisplayPirInfo(&unitInfo);

	if (trace) unitDbShowInfo(&unitInfo);

	updateDbPid(&unitInfo);

#if 0
	if (AlarmDelay)
	{
		if (verbose) printf("%s Start logger for '%s' at '%s' with %d sec delay\n", DateTimeStampStr(-1), unitInfo.appt, unitInfo.gwip, AlarmDelay);
		zbPollRunPollD(AlarmDelay, &unitInfo, SleepTime, ConnectionTimer, rdTimeout);
	}
	else
#endif
	{
		if (verbose) printf("%s Start logger for '%s' at '%s'\n", DateTimeStampStr(-1), unitInfo.appt, unitInfo.gwip);
		zbPollRun(&unitInfo, rdTimeout, ZigbeeStart, &cfg);
	}
	PQfinish(dbConn);
	return 0;
}

int LoggerCheckPid(pid_t pid)
{
	int fd;				/*!< File descriptor to read command line */
	char filename[255];		/*!< Filename where command line is written to on /proc */
	char buf[255];			/*!< Buffer read from filename (command line) */
	
	if (trace) printf("\tCheck if process ID %d is running\n", pid);

	/* Check if provided PID is valid */
	if (pid < 1 || pid > 65535)
	{
		if (trace) printf("\tProcess ID %d is not valid\n", pid);
		return 0;
	}

	/* Build the filename */
	snprintf(filename, sizeof(filename), "/proc/%d/cmdline", pid);

	/* Open filename for reading only */
	if (trace) printf("\tReading file: %s\n", filename);
	fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		if (trace) printf("\tCannot open filename: %s. Process is probably not running.\n", filename);
		close(fd);
		return 0;
	}

	/* Try to read contents of the file */
	if (read(fd, buf, sizeof(buf)) > 0)
	{
		fprintf(stderr, "%s ERROR: PID %d already running: %s\n", DateTimeStampStr(-1), pid, buf);
		close(fd);
		return -1;
	}
	if (verbose) printf("%s Process ID %d not found in file: %s\n", DateTimeStampStr(-1), pid, filename);
	close(fd);
	return 0;
}
