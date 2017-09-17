/*! \file dbRs232.c
	\brief	Utilities for database interaction
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libpq-fe.h>

#include "updateDb.h"
#include "dbConnect.h"
#include "tcpServer.h"
#include "dbRs232.h"

extern int verbose;
extern int trace;

/*! \brief Get current date and time
 * 	\return Address of buffer
 */
char * updateDbGetTime()
{
	static char dtbuff[64];
	struct tm * dtm;
	time_t ts;

	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);		
	snprintf(dtbuff, sizeof(dtbuff), "%04d-%02d-%02d %02d:%02d:%02d",
		dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
	return dtbuff;
}

/*! \brief Display current date and time
 * 	\param	outf	File pointer for output
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
 * 	\param	ats	Time in seconds since epoch (use -1 for current time)
 * 	\return Address of buffer
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

/*! \brief Connect to database
 * 	\return Connection pointer or NULL
 */
PGconn * Rs232DbConnect()
{
	PGconn * conn;
	const char *conninfo;

	/* Make a connection to the database */
	conninfo = connection_str();
	conn = PQconnectdb(conninfo);
	
	/* Check to see that the backend connection was successfully made */
	if (PQstatus(conn) != CONNECTION_OK)
	{
		printf("%s: ERROR : Connection to database failed: %s", DateTimeStampStr(-1), PQerrorMessage(conn));
		PQfinish(conn);
		return NULL;
	}
	return conn;
}

/*! \brief Log alarm message 
 * 	\param 	conn 	Connection record
 * 	\param	unitnr	Unit number
 * 	\param 	rxbuf 	Message received
 * 	\param	atype	Alarm type (usually 'E' or 'R')
 * 	\param	acode	Alarm code number
 *	\return	Zero when successful, otherwise non-zero
 */
int dbRs232_AlarmLog(PGconn * conn, 
		     int unitnr, 
		     char * rxbuf, 
		     char atype, 
		     int acode,
		     char * action)
{
	char sqlStr[1024];
	PGresult * res;
	struct tm * dtm;
	time_t ts;

	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	
	if (verbose) printf("\tAdd %d : %s to event log\n", unitnr, rxbuf);

	snprintf(sqlStr, sizeof(sqlStr), 
		 "insert into alarmlog (unit, modbip, msg, moment, type, code, action) "
		 "values ('%d', '%s', '%s', "
		 "'%04d-%02d-%02d %02d:%02d:%02d',"
		 "'%c', '%d', '%s'"
		")",
		 unitnr, "0.0.0.0", rxbuf,
		 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
		 atype, acode, action);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	res = PQexec(conn, sqlStr);
	Rs232Result(res);

	PQclear(res);
	return 0;
}

/*! \brief Add record to event log
 * 	\param	conn		Database connection info
 * 	\param	unitNum		Unit number
 * 	\param	name		Stand or occupant name
 * 	\param	descr		Stand description
 * 	\param	eventtype	Event type number
 * 	\param	zone		Zone number from alarm system
 *	\return	Zero when successful, otherwise non-zero
 */
int Rs232LogEvent(PGconn * conn, 
		  int unitNum,
		  char * name,
		  char * descr, 
		  int eventtype, 
		  int zone)
{
	char sqlStr[1024];
	PGresult * res;
	struct tm * dtm;
	time_t ts;
	
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	
	if (verbose) printf("\tAdd %s %s to event log\n", name, descr);

	snprintf(sqlStr, sizeof(sqlStr), 
		 "insert into eventlog (name, eventtype, boxip, unit, loc, moment, zone) "
		 "values ('%s at %d (%s)', '%d', '%s', %d, %d, "
		 "'%04d-%02d-%02d %02d:%02d:%02d', %d)",
		 descr, unitNum, name, 6, "0.0.0.0", unitNum, zone,
		 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec, zone);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	
	res = PQexec(conn, sqlStr);
	Rs232Result(res);
	PQclear(res);
	
	return 0;
}

/*! \brief Add record to status log
 * 	\param	conn		Database connection info
	\param	eventtype	Event type
	\param	unitNum		Unit number
 * 	\param	eventtype	Event type number
 * 	\param	zone		Zone number from alarm system
	\return	Zero when successful, otherwise non-zero
 */
int Rs232LogStatus(PGconn * conn, 
		   int unitNum,
		   int eventtype, 
		   int zone)
{
	char sqlStr[1024];
	PGresult *res;

	if (verbose) printf("\tAdd to status log\n");

	snprintf(sqlStr, sizeof(sqlStr), 
		"insert into statuslog (status, moment, unit, loc) "
		"values ('%d', '%s', %d, %d)",
		eventtype, updateDbGetTime(), unitNum, zone);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	res = PQexec(conn, sqlStr);
	Rs232Result(res);
	
	return 0;
}

/*! \brief Update current status
 * 	\param	conn		Database connection info
 * 	\param	eventtype	Event type number
 *	\param	unitNum		Unit number
 *	\return	Zero when successful, otherwise non-zero
 */
int Rs232Status(PGconn * conn, 
		int eventtype,
		int unitNum)
{
	char sqlStr[1024];
	PGresult *res;
	struct tm * dtm;
	time_t ts;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	
	if (verbose) printf("\tUpdate current status\n");

	snprintf(sqlStr, sizeof(sqlStr), 
		"update statuscurr set (status, moment) "
		"= ('%d', '%04d-%02d-%02d %02d:%02d:%02d') where unit=%d",
		eventtype, 
		dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
		unitNum);
	if (trace) printf("\t%s\n", sqlStr);

	res = PQexec(conn, sqlStr);
	Rs232Result(res);
	PQclear(res);
	
	return 0;
}

/*! \brief Add record to event log
 * 	\param	conn		Database connection info
 *	\param	unitNum		Unit number
 * 	\param	name		Stand or occupant name
 * 	\param	descr		Stand description
 * 	\param	eventtype	Event type number
 * 	\param	zone		Zone number from alarm system
	\return	Zero when successful, otherwise non-zero
 */
int Rs232AlarmMsg(PGconn * conn,
		  int unitNum,
		  char* name,
		  char * descr, 
		  int eventtype, 
		  int zone)
{
	char sqlStr[1024];
	char msgStr[99];
	PGresult * res;
	struct tm * dtm;
	time_t ts;
	int msgid, rc;
	char * dbVal;
	
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	
	if (verbose) printf("\tSend message to %s : '%s'\n", name, descr);

	snprintf(msgStr, sizeof(msgStr), 
		 "At %04d-%02d-%02d %02d:%02d:%02d (unit %d zone %d)",
		 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		 dtm->tm_hour, dtm->tm_min, dtm->tm_sec, unitNum, zone);
	
	snprintf(sqlStr, sizeof(sqlStr),
		 "insert into messages (subject, message, time, expire, catgry, deliver, urgent) values "
                 "('%s','%s',CURRENT_TIMESTAMP,CURRENT_TIMESTAMP + interval '30 days', 0, 1, 1) RETURNING id AS msgid", 
		 descr, msgStr);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	res = PQexec(conn, sqlStr);
	rc = Rs232Result(res);
	if (rc) 
	{
		PQclear(res);
		return -1;
	}

	dbVal = PQgetvalue(res, 0, 0);
	msgid = atoi(dbVal);
	if (trace) printf("\tMessage ID is %d\n", msgid);

	sprintf(sqlStr, "insert into msgread (uid, mid) values (%d,%d)", unitNum, msgid);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	res = PQexec(conn, sqlStr);

	rc = Rs232Result(res);
	PQclear(res);
	
	return rc;
}

/*! \brief Get unit info 
 *	\param	unitNum		Unit number
 * 	\param	conn		Database connection info
 * 	\param	oname		Occupant name
 * 	\param 	appt		Stand name
 * 	\return	Zero when successful, otherwise non-zero
*/
int Rs232GetInfo(int unitNo, PGconn * conn, char * oname, char *appt)
{
	char sqlStr[640];
	int nFields, nTuples;
	int i, j;
	PGresult * res;

	if (verbose) printf("\tUpdate unit info\n");

	/* Start a transaction block */
	res = PQexec(conn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		DateTimeStampFile(stderr);
		printf("%s: ERROR : BEGIN command failed: %s", DateTimeStampStr(-1), PQerrorMessage(conn));
		PQclear(res);
		return -1;
	}

	/* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
	PQclear(res);

	/* Fetch rows from pg_database, the system catalog of databases */
	snprintf(sqlStr, sizeof(sqlStr), 
		"select appt,name "			/* 0 1 */
		"from adomis_box a where a.unit=%d",
		unitNo);
	if (verbose) printf("\t%s\n", sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		printf("%s: ERROR : Database select failed: %s\n", DateTimeStampStr(-1), PQerrorMessage(conn));
		PQclear(res);
		return -1;
	}
	
	oname[0] = appt[0] = 0;

	/* Print out the attribute names */
	nFields = PQnfields(res);
	nTuples = PQntuples(res);

	/* Print out the rows */
	for (i = 0; i < PQntuples(res); i++)
	{
		if (trace)
		{
			for (j = 0; j < nFields; j++) printf("\t'%-15s'", PQgetvalue(res, i, j));
			printf("\n");
		}
		strcpy(appt, PQgetvalue(res, i, 0));			/*!< appt */
		strcpy(oname, PQgetvalue(res, i, 1));			/*!< name */
	}
	PQclear(res);

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tUnit info read : '%s' '%s'\n", oname, appt);
	return 0;
}

/*! \brief Check result of database operation 
	\param	result	Database result
 * 	\return	Zero when successful, otherwise non-zero
*/
int Rs232Result(PGresult * res)
{
	int rc=0;

	if (!res) 
	{
		DateTimeStampFile(stderr);
		printf("PQexec command failed, no error code\n");
		rc = -1;
	}
	else
	{
		switch (PQresultStatus(res)) 
		{
		case PGRES_COMMAND_OK:
			if (trace) printf("\tCommand executed OK, %s rows affected\n",PQcmdTuples(res));
			break;
		case PGRES_TUPLES_OK:
			if (verbose) printf("\tQuery may have returned data\n");
			break;
		default:
			DateTimeStampFile(stderr);
			printf("%s: ERROR : Command failed with code %s, error message %s\n", DateTimeStampStr(-1),
				PQresStatus(PQresultStatus(res)), PQresultErrorMessage(res));
			rc = -1;
			break;
		}
	}
	
	return rc;
}

/*! \brief Update PIR event in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int Rs232PirEvent(int unitNo, 
		  PGconn * conn)
{
	char sqlStr[1024];
	PGresult * result;
	struct tm * dtm;
	time_t ts;
	static int phour = -1;
	int chour;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	chour = dtm->tm_hour;
	if (trace) printf("\tUpdate event for hour %d(previous %d)\n", chour, phour);
	
	if (chour != phour)
	{
		snprintf(sqlStr, sizeof(sqlStr), 
			"update statuscurr set (pir0moment, pirevnt[%d]) = ('%04d-%02d-%02d %02d:%02d:%02d', 1) where unit=%d", 
			chour,
			dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec, 
			unitNo);
	}
	else
	{
		snprintf(sqlStr, sizeof(sqlStr), 
			"update statuscurr set (pir0moment, pirevnt[%d]) = ('%04d-%02d-%02d %02d:%02d:%02d', pirevnt[%d]+1) where unit=%d", 
			chour, 
			dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec, 
			chour, unitNo);
	}

	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	PQclear(result);
	
	phour = chour;
	
	return 0;
}