/*
 * $Author$
 * $Date$
 * $Revision$
 * $State$
 *
 * $Log$
 *
 *
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
#include <libpq-fe.h>

#include "gversion.h"
#include "dbConnect.h"

int ShowVersion = 0;
char * EmailAddr = NULL;
int verbose = 0;
int trace = 0;

/*! \struct Command line parsing data structure */
struct poptOption optionsTable[] =
{
 	{"trace", 	't', POPT_ARG_NONE, 	&trace, 	0, "Trace/debug output", NULL},
	{"verbose", 	'v', POPT_ARG_NONE, 	&verbose, 	0, "Verbose output", NULL},
	{"version", 	'V', POPT_ARG_NONE, 	&ShowVersion, 	0, "Display version information", NULL},
	{"email",	'E', POPT_ARG_STRING, 	&EmailAddr, 	0, "Email address",  "<string>"},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

/* Local functions */
void sighandle(int signum);

int gNetReport(PGconn * dbConn, FILE * mailq);
int gSendMail(char * ema, PGconn * dbConn);

/* ==========================================================================
 * M A I N
 * ========================================================================== 
 */
int main(int argc, 
	 char *argv[])
{
	int rc;
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
		poptFreeContext(optcon);
		return -1;
	}
	if (ShowVersion) displayVersion(argv[0]);
	
	if (EmailAddr == NULL)
	{
		printf("Email address not supplied\n");
		poptPrintUsage(optcon, stderr, 0);
		poptFreeContext(optcon);
		return -1;
	}

	poptFreeContext(optcon);

	dbConn = PQconnectdb(connection_str());
	if (PQstatus(dbConn) == CONNECTION_BAD) 
	{
		fprintf(stderr, "Connection to %s failed, %s", connection_str(),
			PQerrorMessage(dbConn));
		return -1;
	}

	rc = gSendMail(EmailAddr, dbConn);
	
	PQfinish(dbConn);
	
	return rc;
}

/*! \brief Signal handler
 *	\param	signum	signal number
 */
void sighandle(int signum)
{
	printf("Shutting down after signal %d\n", signum);
	exit(1);
}

/*! \brief Send email
 * 
 */
int gSendMail(char * ema, 
	      PGconn * dbConn)
{
	int rc = 0;
	FILE * mailq;
	struct tm * dtm;
	time_t ts;
	char mailp[256];
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	
	if (verbose) printf("Send mail to %s\n", ema);
	
	sprintf(mailp, "/usr/bin/esmtp %s", ema);
	
	mailq = popen(mailp, "w");
	if (mailq == NULL)
	{
		printf("Could not open email");
		return -1;
	}
	/* : */
	
	fprintf(mailq, "Subject: Test\n");
	fprintf(mailq, "Reply-to: jcoetzer@gmail.com\n");
	fprintf(mailq, "To: %s\n", ema);
	fprintf(mailq, "Content-type: text/plain\n\n");
	
	fprintf(mailq, "This is a test\n");
	fprintf(mailq, "Run on %04d-%02d-%02d %02d:%02d:%02d\n",
		 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec);

	//? rc = gNetReport(dbConn, mailq);
	
	if (verbose) printf("Close mail\n");

	pclose(mailq);
	
	if (verbose) printf("Mail sent to %s\n", ema);
	
	return rc;
}

int gNetReport(PGconn * dbConn, 
	       FILE * mailq)
{
	int i, n;
	int nFields, nTuples;
	PGresult * res;
	char ncmd[1024];
	
	sprintf(ncmd, "select id, mac, ip, appt, name, dinip, a.unit unt, wapip, "
			"snote,  extract(epoch from nstart) startn, extract(epoch from nend) endn, extract(epoch from readmsg) rdmsg "
			"from adomis_box a, statuscurr s where a.unit = s.unit order by a.unit");

	/* Start a transaction block */
	res = PQexec(dbConn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(mailq, "BEGIN command failed: %s", PQerrorMessage(dbConn));
		PQclear(res);
		return -1;
	}

	/* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
	if (verbose) printf("%s\n", ncmd);
	PQclear(res);
	res = PQexec(dbConn, ncmd);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(mailq, "Database select failed: %s\n", PQerrorMessage(dbConn));
		PQclear(res);
		return -1;
	}

	/* Print out the attribute names */
	nFields = PQnfields(res);
	nTuples = PQntuples(res);

	/* Print out the rows */
	n = PQntuples(res);
	if (verbose) printf("%d rows\n", n);
	fprintf(mailq, "%d records\n", n);
	for (i = 0; i < n; i++)
	{
		if (verbose) printf("%d : \n", i);
	}

	PQfinish(dbConn);
	return 0;
	
}
