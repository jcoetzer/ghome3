/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: gHomeUnit.c,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.5  2011/02/23 16:18:03  johan
 * Takoe out sorting of PIR records
 *
 * Revision 1.4  2011/02/10 11:09:42  johan
 * Take out CVS Id tag
 *
 * Revision 1.3  2011/02/09 19:31:49  johan
 * Stop core dump during polling
 *
 * Revision 1.2  2011/02/03 08:38:15  johan
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
#include "gversion.h"
#include "unitDb.h"
#include "dbConnect.h"

#include "zbState.h"

int ShowVersion = 0;
int trace = 0;
int verbose = 0;
int AllUnits = 0;

int UnitNum = -1;

/*! \struct Command line parsing data structure */
struct poptOption optionsTable[] =
{
	{"all", 	'a', POPT_ARG_NONE, 	&AllUnits, 	0, "List all units", NULL},
	{"unit", 	'U', POPT_ARG_INT, 	&UnitNum, 	0, "Unit number",  "<integer>"},
 	{"trace", 	't', POPT_ARG_NONE, 	&trace, 	0, "Trace/debug output", NULL},
	{"verbose", 	'v', POPT_ARG_NONE, 	&verbose, 	0, "Verbose output", NULL},
	{"version", 	'V', POPT_ARG_NONE, 	&ShowVersion, 0, "Display version information", NULL},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

/* Local functions */
void sighandle(int signum);
int pollReply(void * unit, int skt, char * txBuffer, char * rxBuffer, int rxSize, int rdTimeout);
void zbPollAlarmPoll(char * gwip,
		   int gwport,
		   char * almaddr,
		   int SleepTime,
		   int ConnectionTimer,
		   int rdTimeout,
		   int onTime);

/* ==========================================================================
   M A I N
   ========================================================================== */
int main(int argc, 
	 char *argv[])
{
	int rc;
	unitInfoRec unitInfo;
	poptContext optcon;        /*!< Context for parsing command line options */
	PGconn * dbConn;

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

	if (AllUnits == 0 && UnitNum < 0)
	{
		fprintf(stderr, "Unit number not specified\n");
		poptPrintUsage(optcon, stderr, 0);
		return -1;
	}

	poptFreeContext(optcon);

	unitDbInit(&unitInfo, 0, 1);

	dbConn = PQconnectdb(connection_str());
	if (PQstatus(dbConn) == CONNECTION_BAD) 
	{
		fprintf(stderr, "Connection to %s failed, %s", connection_str(),
			PQerrorMessage(dbConn));
		return -1;
	}
	unitInfo.dbConnctn = dbConn;
	
	if (AllUnits)
	{
		unitDbListAll(&unitInfo);
	}
	else
	{
		unitDbSetLocation(&unitInfo);
		
		unitInfo.unitNo = UnitNum;
		rc = unitDbGetInfo(&unitInfo);
		if (rc)
		{
			fprintf(stderr, "Unit info not found in database\n");
			return -1;
		}
		rc = unitDbGetPirHardware(&unitInfo);
		if (rc)
		{
			fprintf(stderr, "PIR info not found in database\n");
			return -1;
		}
		rc = unitDbGetPirTemp(&unitInfo);
		if (rc)
		{
			fprintf(stderr, "PIR temperature data not found in database\n");
			return -1;
		}
		unitDbPirMoment(&unitInfo);
		unitDbShowInfo(&unitInfo);
	}

	PQfinish(dbConn);
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
