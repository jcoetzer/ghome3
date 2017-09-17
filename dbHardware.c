/*! \file dbHardware.c
	\brief	Utilities for database interaction
 */

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: dbHardware.c,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.1.1.1  2011/05/28 09:00:03  johan
 * gHome logging server
 *
 * Revision 1.3  2011/02/10 11:09:40  johan
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
#include <time.h>
#include <libpq-fe.h>

#include "updateDb.h"
#include "dbConnect.h"
#include "dbHardware.h"
#include "tcpServer.h"

extern int verbose;
extern int trace;

/*! \brief Display current date and time
*/
void DateTimeStampFile(FILE * outf)
{
	struct tm * dtm;
	time_t ts;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	fprintf(outf, "%04d-%02d-%02d %02d:%02d:%02d ",
		dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
}

/*! \brief Display current date and time
*/
char * DateTimeStampStr(int ats)
{
	struct tm * dtm;
	time_t ts;
	static char dts[32];
	 
	if (ats > 0) ts = (time_t) ats;
	else if (ats < 0) ts = time(NULL);
	else
	{
		strcpy(dts,"                   ");
		return dts;
	}

	dtm = (struct tm *)localtime(&ts);
	sprintf(dts, "%04d-%02d-%02d %02d:%02d:%02d",
	        dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
	        dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
	return dts;
}

/*! \brief Get unit info 
	\param	ipadr	IP address 
	\return		Unit number when successful, otherwise zero or less
*/
int getUnitIp(char * ipadr)
{
	char sqlStr[640];
	int nFields, nTuples;
	int i, j;
	int unum=0;
	PGresult * res;
	char * dbVal;
	PGconn * conn;
	const char *conninfo;
	
	/* Make a connection to the database */
	conninfo = connection_str();
	if (trace) printf("\tConnect to database: %s\n", conninfo);
	conn = PQconnectdb(conninfo);
	
	/* Check to see that the backend connection was successfully made */
	if (PQstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database failed: %s",
			PQerrorMessage(conn));
		PQfinish(conn);
		return -1;
	}

	if (trace) printf("\tGet unit hardware last modify time\n");

	/* Start a transaction block */
	res = PQexec(conn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}

	/* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
	PQclear(res);

	/* Fetch rows from pg_database, the system catalog of databases */
	snprintf(sqlStr, sizeof(sqlStr), 
		"select unit from adomis_box where modbip='%s'",
		ipadr);
	if (trace) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}

	/* Print out the attribute names */
	nFields = PQnfields(res);
	nTuples = PQntuples(res);

	/* Print out the rows */
	unum = 0;
	for (i = 0; i < PQntuples(res); i++)
	{
		if (trace)
		{
			for (j = 0; j < nFields; j++) printf("\t'%-15s'", PQgetvalue(res, i, j));
			printf("\n");
		}
		dbVal = PQgetvalue(res, i, 0);					/*!< unit */
		unum = atoi(dbVal);
	}

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	/* close the connection to the database and cleanup */
	PQfinish(conn);

	if (verbose) printf("\tUnit number for %s is %d\n", ipadr, unum);
	return unum;
}

/*! \brief Log alarm message 
 * 	\param connctn Connection record
 * 	\param rxbuf Message received
 */
int dbHardware_AlarmLog(struct ConnectionRec * connctn, char * rxbuf)
{
	char sqlStr[1024];
	PGconn * conn;
	PGresult * res;
	struct tm * dtm;
	time_t ts;
	const char *conninfo;
	
	/* Make a connection to the database */
	conninfo = connection_str();
	if (trace) printf("\tConnect to database: %s\n", conninfo);
	conn = PQconnectdb(conninfo);
	
	/* Check to see that the backend connection was successfully made */
	if (PQstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database failed: %s",
			PQerrorMessage(conn));
		PQfinish(conn);
		return -1;
	}

	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	
	if (trace) printf("\tAdd %s : %s to event log\n", connctn->IpAddr, rxbuf);

	snprintf(sqlStr, sizeof(sqlStr), 
		 "insert into alarmlog (unit, modbip, msg, moment) "
		 "values ('%d', '%s', '%s', "
		 "'%04d-%02d-%02d %02d:%02d:%02d')",
		 connctn->Unit, connctn->IpAddr, rxbuf,
		 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec);

	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);

	if (!res) 
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "PQexec command failed, no error code\n");
		PQfinish(conn);
		return -1;
	}

	switch (PQresultStatus(res)) 
	{
		case PGRES_COMMAND_OK:
			if (trace) printf("\tCommand executed OK, %s rows affected\n", PQcmdTuples(res));
			break;
		case PGRES_TUPLES_OK:
			if (trace) printf("\tQuery may have returned data\n");
			break;
		default:
			DateTimeStampFile(stderr);
			fprintf(stderr, "Command failed with code %s, error message %s\n",
				PQresStatus(PQresultStatus(res)), PQresultErrorMessage(res));
			break;
	}
	
	PQclear(res);
	PQfinish(conn);

	return 0;
}
