/*
 * 
 * PostgreSQL LISTEN/NOTIFY C code template
 *
 * Portions Copyright (c) 2010-Present, Ross Cameron <ross.cameron@unix.net> for
 *                                      InteliPower CC <roger@intelipower.co.za>
 * 
 * Portions Copyright (c) 1996-Present, The PostgreSQL Global Development Group
 * 
 * Portions Copyright (c) 1994, The Regents of the University of California
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written agreement
 * is hereby granted, provided that the above copyright notice and this
 * paragraph and the following two paragraphs appear in all copies.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON AN
 * "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 * 
 * ----------------------------------------------------------------------------
 *
 */

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: gHomeNotify.c,v $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <popt.h>
#include <time.h>
#include <libgen.h>

#include "libpq-fe.h"
#include "zbSocket.h"
#include "zbSend.h"
#include "zbDisplay.h"
#include "zbPacket.h"
#include "gversion.h"
#include "dbConnect.h"

int trace = 0;
int verbose = 0;

int getNotices(PGconn     *conn, unsigned short ServerPort, int rdTimeout);
int sendDisarm(PGconn * conn, char * ServerIP, unsigned short ServerPort, int logno, int rdTimeout);
int updateEventLog(PGconn * conn, int logno);
int updateStatusCurr(PGconn * conn, char * boxip);

int ShowVersion = 0;
unsigned short ServerPort = 0;     /* Gateway port */

/*! \struct optionsTable	Command line parsing data structure */
struct poptOption optionsTable[] =
{
	{"port", 'P', POPT_ARG_INT, &ServerPort, 0,  "Gateway port(default is 5000)", "<integer>"},
 	{"trace", 't', POPT_ARG_NONE, &trace, 0, "Trace/debug output", NULL},
	{"verbose", 'v', POPT_ARG_NONE, &verbose, 0, "Trace output", NULL},
	{"version", 	'V', POPT_ARG_NONE, 	&ShowVersion, 0, "Display version information", NULL},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

/*! \brief Stop Postgres and exit
	\param	conn	Postgres connection
 */
static void exit_nicely(PGconn *conn)
{
    PQfinish(conn);
    exit(1);
}

/*! \brief Signal handler
	\param	signum	signal number
 */
void sighandle(int signum)
{
	fprintf(stderr, "Shutting down after signal %d\n", signum);
	exit(1);
}

/*! \brief	Main routine
 */
int main(int argc, 
	 char ** argv)
{
	const char *conninfo;
	PGconn     *conn;
	PGresult   *res;
	PGnotify   *notify;
	int         rc, nnotifies;
	poptContext optcon;        /*!< Context for parsing command line options */
	int rdTimeout = 0;

	signal(SIGPIPE, sighandle);

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

	if (! ServerPort) ServerPort = ZIGBEE_GATEWAY_PORT;

	conninfo = connection_str();
	
	/* Make a connection to the database */
	if (trace) printf("\tConnect to database: %s\n", conninfo);
	conn = PQconnectdb(conninfo);
	
	/* Check to see that the backend connection was successfully made */
	if (PQstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database failed: %s",
			PQerrorMessage(conn));
		exit_nicely(conn);
	}
	
	/*
	* Issue LISTEN command to enable notifications from the rule's NOTIFY.
	*/
	res = PQexec(conn, "LISTEN tarfu");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "LISTEN command failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	
	/*
	 * should PQclear PGresult whenever it is no longer needed to avoid memory
	 * leaks
	 */
	if (trace) printf("\tClear result\n");
	PQclear(res);
	
	/* Quit after four notifies are received. */
	nnotifies = 0;
	while (1)
	{
		/* Sleep until something happens on the connection.  We use select(2)
		   to wait for input, but you could also use poll() or similar
		   facilities. */
		int         sock;
		fd_set      input_mask;
		
		if (trace) printf("\tWaiting for %d.\n", nnotifies+1);
		
		sock = PQsocket(conn);
		
		if (sock < 0)
			break;              /* shouldn't happen */
		
		FD_ZERO(&input_mask);
		FD_SET(sock, &input_mask);
		
		if (select(sock + 1, &input_mask, NULL, NULL, NULL) < 0)
		{
			fprintf(stderr, "select() failed: %s\n", strerror(errno));
			exit_nicely(conn);
		}
		
		/* Now check for input */
		if (trace) printf("\tcheck for input\n");
		PQconsumeInput(conn);
		while ((notify = PQnotifies(conn)) != NULL)
		{
			if (trace) 
				printf("\tASYNC NOTIFY of '%s' received from backend pid %d\n",
					notify->relname, notify->be_pid);
			PQfreemem(notify);
			getNotices(conn, ServerPort, rdTimeout);
			nnotifies++;
		}
	}

	if (trace) printf("\tDone.\n");

	/* close the connection to the database and cleanup */
	PQfinish(conn);

	return 0;
}

/*! \brief	Get notice info 
	\param	conn		database connection info
	\param	ServerPort	port number
	\param	rdTimeout	read timeout in seconds
	\return	Zero when successful, otherwise non-zero
 */
int getNotices(PGconn * conn, 
	       unsigned short ServerPort,
	       int rdTimeout)
{
	int i;
	char * boxip;
	int logno;

	PGresult * res;

	/* Our test case here involves using a cursor, for which we must be inside
	   a transaction block.  We could do the whole thing with a single
	   PQexec() of "select * from pg_database", but that's too trivial to make
	   a good example. */

	/* Start a transaction block */
	res = PQexec(conn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	/* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
	PQclear(res);

	/* Fetch rows from pg_database, the system catalog of databases */
	if (trace) printf("\tselect eventlog_no, boxip from eventnotice where acktime is null and boxip is not null\n");
	res = PQexec(conn, "DECLARE myportal CURSOR FOR select eventlog_no, boxip from eventnotice "
			"where acktime is null and boxip is not null");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "DECLARE CURSOR failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

	res = PQexec(conn, "FETCH ALL in myportal");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "FETCH ALL failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

#if 0
	/* first, print out the attribute names */
	nFields = PQnfields(res);
	if (trace) 
	{
		for (i = 0; i < nFields; i++)
			printf("\t%-15s", PQfname(res, i));
		printf("\n");
	}
#endif

	/* next, print out the rows */
	for (i = 0; i < PQntuples(res); i++)
	{
		logno = atoi(PQgetvalue(res, i, 0));
		boxip = PQgetvalue(res, i, 1);
		if (trace) printf("\tEvent %d on IP address %s", logno, boxip);
		sendDisarm(conn, boxip, ServerPort, logno, rdTimeout);
	}

	PQclear(res);

	/* close the portal ... we don't bother to check for errors ... */
	res = PQexec(conn, "CLOSE myportal");
	PQclear(res);

	/* end the transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	return 0;
}

/*! \brief	Send disarm signal to alarm 
	\param	conn		database connection info
	\param	ServerIP	IP address
	\param	ServerPort	port number
	\param	logno		log number
	\param	rdTimeout	timeout period in seconds
	\return	Zero when successful, otherwise non-zero
 */
int sendDisarm(PGconn * conn, 
	       char * ServerIP,
	       unsigned short ServerPort, 
	       int logno,
	       int rdTimeout)
{
	char * zigBeePktStr = NULL;
	
	int skt, rc;
	char txBuffer[512];
	char rxBuffer[512];
	
	memset(txBuffer, 0, sizeof(txBuffer));
	memset(rxBuffer, 0, sizeof(rxBuffer));

	skt = zbSendInit(ServerIP, ServerPort, rdTimeout);
	if (skt <= 0)
	{
		fprintf(stderr, "Initialize for %s:%d failed\n", ServerIP, ServerPort);
		return -1;
	}

	zigBeePktStr = PktDisarm;
	
	rc = zbSendCheck(zigBeePktStr, txBuffer);
	if (rc)
	{
		fprintf(stderr, "Packet check failed\n");
		return -1;
	}
	
	zbSendString(skt, txBuffer);
	
	zbSendGetReply(skt, rxBuffer, rdTimeout);
	
	updateEventLog(conn, logno);
	/* updateStatusCurr(conn, ServerIP); */

	zbSocketClose(skt);

	return 0;
}

/*! \brief	Update event notice
	\param	conn	database connection info
	\param	logno	log number
	\return	Zero when successful, otherwise non-zero
 */
int updateStatusCurr(PGconn * conn, 
		     char * boxip)
{
	char sqlStr[1024];
	PGresult *result;
	struct tm * dtm;
	time_t ts;
	 
	ts = time(NULL);
	/* Add 10 minutes */
	ts += 600;
	dtm = (struct tm *)localtime(&ts);

	snprintf(sqlStr, sizeof(sqlStr), 
		"update statuscurr set (momentarm) "
		"= ('%04d-%02d-%02d %02d:%02d:%02d') where unit="
		"(select unit from adomis_box where ip='%s')",
		dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
		boxip);

	if (trace) printf("\tExecute %s\n", sqlStr);

	result = PQexec(conn, sqlStr);
	if (!result) 
	{
		fprintf(stderr, "PQexec command failed, no error code\n");
		return -1;
	}
	else 
	{
		switch (PQresultStatus(result)) 
		{
			case PGRES_COMMAND_OK:
				if (trace) printf("\tCommand executed OK, %s rows affected\n",PQcmdTuples(result));
				break;
			case PGRES_TUPLES_OK:
				if (trace) printf("\tQuery may have returned data\n");
				break;
			default:
				fprintf(stderr, "Command failed with code %s, error message %s\n",
					PQresStatus(PQresultStatus(result)),
							PQresultErrorMessage(result));
				break;
		}
		PQclear(result);
	}

	return 0;
}

/*! \brief	Update event notice
	\param	conn	database connection info
	\param	logno	log number
	\return	Zero when successful, otherwise non-zero
 */
int updateEventLog(PGconn * conn, 
		   int logno)
{
	char sqlStr[1024];
	PGresult *result;

	snprintf(sqlStr, sizeof(sqlStr), "update eventnotice set acktime = CURRENT_TIMESTAMP where eventlog_no = %d", logno);

	if (trace) printf("\tExecute %s\n", sqlStr);

	result = PQexec(conn, sqlStr);
	if (!result) 
	{
		fprintf(stderr, "PQexec command failed, no error code\n");
	}
	else 
	{
		switch (PQresultStatus(result)) 
		{
			case PGRES_COMMAND_OK:
				if (trace) printf("\tCommand executed OK, %s rows affected\n",PQcmdTuples(result));
				break;
			case PGRES_TUPLES_OK:
				if (trace) printf("\tQuery may have returned data\n");
				break;
			default:
				fprintf(stderr, "Command failed with code %s, error message %s\n",
					PQresStatus(PQresultStatus(result)),
							PQresultErrorMessage(result));
				break;
		}
		PQclear(result);
	}

	return 0;
}

/*! \brief Read and display buffer
	\param	skt		open socket
	\param	unit 		unit information
	\param	cBuffer		buffer
	\param	cLen		buffer size
	\param	unitName	unit name
	\param	unitIp		IP address of gateway
	\param	unitNum		unit number
	\return	Zero when successful, otherwise non-zero
*/
int checkBuffer(int skt,
		void * unit, 
		char * cBuffer, 
		int cLen,
		int usec)
{
	int rc = 0;

	if (0 == strncmp(cBuffer, "021C0519", 8) && 0 == strncmp(&cBuffer[56], "01", 2))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<ALARM>\n");
		rc = 3;
	}
	else if (0 == strncmp(cBuffer, "021C0205", 8))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<PANIC>\n");
	}
	else if (0 == strncmp(cBuffer, "021C0305", 8))
	{
		DateTimeStamp();
		dispBuffer(cBuffer, cLen, "<FIRE>\n");
	}
	else
	{
		if (verbose)
		{
			DateTimeStamp();
			dispBuffer(cBuffer, cLen, "\n");
		}
	}
	return 0;
}
