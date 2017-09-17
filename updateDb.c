/*! \file updateDb.c
	\brief	Utilities for database interaction
*/

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.3 $
 * $State: Exp $
 *
 * $Log: updateDb.c,v $
 * Revision 1.3  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.2  2011/06/15 22:56:26  johan
 * Send message when alamr goes off
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.8  2011/02/24 16:20:55  johan
 * Store user descriptor in database
 *
 * Revision 1.7  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.6  2011/02/16 20:42:05  johan
 * Add user description field
 *
 * Revision 1.5  2011/02/15 10:27:18  johan
 * Add zone un-enroll message
 *
 * Revision 1.4  2011/02/10 15:20:40  johan
 * Log errors in PIR messages
 *
 * Revision 1.3  2011/02/10 11:09:43  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:16  johan
 * Add CVS stuff
 *
 *
 *
 */

#define _XOPEN_SOURCE 500 /* glibc2 needs this */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libpq-fe.h>
#include <sys/types.h>
#include <unistd.h>

#include "unitDb.h"
#include "updateDb.h"
#include "zbDisplay.h"
#include "zbPoll.h"
#include "zbData.h"
#include "dbConnect.h"

extern int verbose;
extern int trace;

/*! \brief Stop Postgres and exit
	\param	conn	Postgres connection
 */
void updateDbError(PGconn * conn)
{
	if (trace) printf("\tReopen database connection '%s'\n", connection_str());
	PQfinish(conn);
	conn = PQconnectdb(connection_str());
	if (PQstatus(conn) == CONNECTION_BAD) 
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "%s Connection to %s failed, %s", DateTimeStampStr(-1), connection_str(),
			PQerrorMessage(conn));
		return;
	}
	if (verbose) printf("%s Reopened database connection\n", DateTimeStampStr(-1));
	/*? exit(1); ?*/
}

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

/*! Display unit status according to value
	\param	sts	numerical value for status
 */
void dispUnitStatus(int sts)
{
	switch (sts)
	{
		case ZBSTATUS_UNKNOWN  		: printf("Unknown\n"); break;
		case ZBSTATUS_ALARM  		: printf("Alarm\n"); break;
		case ZBSTATUS_FIRE  		: printf("Fire\n"); break;
		case ZBSTATUS_PANIC  		: printf("Panic\n"); break;
		case ZBSTATUS_ARMED 		: printf("Armed\n"); break;
		case ZBSTATUS_ARM_DAY 		: printf("Day\n"); break;
		case ZBSTATUS_ARM_NITE 		: printf("Night\n"); break;
		case ZBSTATUS_DISARMED 		: printf("Disarmed\n"); break;
		case ZBSTATUS_LOST_COMMS  	: printf("Lost comms\n"); break;
		case ZBSTATUS_INIT  		: printf("Initializing\n"); break;
		default : 
			if ((sts > 1e6) && (sts & ZBSTATUS_STAY)) printf("Stay\n");
			else printf("Undefined %d\n", sts);
	}
}

/*! \brief Check result of database operation 
	\param	result	Database result
*/
int updateDbResult(PGresult * result)
{
	int rc=0;

	if (!result) 
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "PQexec command failed, no error code\n");
		rc = -1;
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
				DateTimeStampFile(stderr);
				fprintf(stderr, "Command failed with code %s, error message %s\n",
					PQresStatus(PQresultStatus(result)), PQresultErrorMessage(result));
				rc = -1;
				break;
		}
		PQclear(result);
	}
	return rc;
}

/*! \brief Get unit status info, i.e. armed, disarmed, etc.
	\param	unit	Data record to be populated
	\return		Zero when successful, otherwise non-zero
 */
int updateDbStatusInfo(unitInfoRec * unit, 
		       char * updTime)
{
	char sqlStr[512];
	int nFields, nTuples;
	int i, j;
	PGconn * conn;
	PGresult * res;
	char * dbVal;
	struct tm dtm;

	conn = unit->dbConnctn; 

	if (trace) printf("\tGet unit status info\n");

	/* Start a transaction block */
	res = PQexec(conn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return -1;
	}

	/* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
	PQclear(res);

	/* Fetch rows from pg_database, the system catalog of databases */
	snprintf(sqlStr, sizeof(sqlStr), 
		 "select moment, status, momentarm, extract(epoch from readmsg) rdmsg from statuscurr where unit=%d",
		 unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return -1;
	}

	/* first, print out the attribute names */
	nFields = PQnfields(res);
	nTuples = PQntuples(res);

	/* next, print out the rows */
	for (i = 0; i < PQntuples(res); i++)
	{
		if (trace) 
		{
			printf("\t");
			for (j = 0; j < nFields; j++) printf("\t'%-15s'", PQgetvalue(res, i, j));
			printf("\n");
		}
		dbVal = PQgetvalue(res, i, 0);					/*!< moment */
		strcpy(updTime, dbVal);
		dbVal = PQgetvalue(res, i, 1);					/*!< status */
		unit->ustatus = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 2);					/*!< momentarm */
		if (strlen(dbVal) && strncmp(dbVal, "    ", 4))
		{
			strncpy(sqlStr, dbVal, 19);
			sqlStr[19] = 0;
			strptime(sqlStr, "%Y-%m-%d %H:%M:%S", &dtm);
			unit->momentarm = mktime(&dtm); 
			if (trace) 
				printf("\tRearm time is %d '%04d-%02d-%02d %02d:%02d:%02d'\n", (int) unit->momentarm,
					dtm.tm_year+1900, dtm.tm_mon+1, dtm.tm_mday,
					dtm.tm_hour, dtm.tm_min, dtm.tm_sec);
		}
		dbVal = PQgetvalue(res, i, 3);					/*!< readmsg */
		unit->readmsg = atoi(dbVal);
		if (verbose)
		{
			DateTimeStamp();
			printf("\tStatus of %s is %d: ", unit->appt, unit->ustatus);
			dispUnitStatus(unit->ustatus);
		}
	}
	PQclear(res);

	/* end the transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tUnit status read from database\n");
	if (trace) dbUnitDisplayPirInfo(unit);
	return 0;
}

/*! \brief Add record to event log
	\param	descr		Description string
	\param	name		Name string
	\param	eventtype	Event type
	\param	gwip		IP address of Zigbee gateway
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int updateDbLogEvent(unitInfoRec * unit,
		     char * descr, 
		     int eventtype, 
		     int loc)
{
	char sqlStr[1024];
	PGconn * conn;
	PGresult * result;
	struct tm * dtm;
	time_t ts;
	char * name;
	char * gwip;
	int unitNum;

	name = unit->oname;
	gwip = unit->gwip;
	unitNum = unit->unitNo;
	conn = unit->dbConnctn;

	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	
	if (trace) printf("\tAdd %s %s to event log\n", name, descr);

	snprintf(sqlStr, sizeof(sqlStr), 
		 "insert into eventlog (name, eventtype, boxip, unit, loc, moment) "
		 "values ('%s at %d (%s)', '%d', '%s', %d, %d, "
		 "'%04d-%02d-%02d %02d:%02d:%02d')",
		 descr, unitNum, name, 6, gwip, unitNum, loc,
		 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec);

	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	result = PQexec(conn, sqlStr);

	if (!result) 
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "PQexec command failed, no error code\n");
		return -1;
	}
	else 
	{
		switch (PQresultStatus(result)) 
		{
			case PGRES_COMMAND_OK:
				if (trace) printf("\tCommand executed OK, %s rows affected\n", PQcmdTuples(result));
				break;
			case PGRES_TUPLES_OK:
				if (trace) printf("\tQuery may have returned data\n");
				break;
			default:
				DateTimeStampFile(stderr);
				fprintf(stderr, "Command failed with code %s, error message %s\n",
				       PQresStatus(PQresultStatus(result)), PQresultErrorMessage(result));
				break;
		}
		PQclear(result);
	}

	return 0;
}

/*! \brief Add record to event log
	\param	descr		Description string
	\param	name		Name string
	\param	eventtype	Event type
	\param	gwip		IP address of Zigbee gateway
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int updateDbAlarmMsg(unitInfoRec * unit,
		     char * descr, 
		     int eventtype, 
		     int loc)
{
	char sqlStr[1024];
	char msgStr[99];
	PGconn * conn;
	PGresult * result;
	struct tm * dtm;
	time_t ts;
	char* name;
	char * gwip;
	int unitNum, msgid;
	char * dbVal;
	
	name = unit->oname;
	gwip = unit->gwip;
	unitNum = unit->unitNo;
	conn = unit->dbConnctn;

	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	
	if (trace) printf("\tSend message to %s : '%s'\n", name, descr);

	snprintf(msgStr, sizeof(msgStr), 
		 "At %04d-%02d-%02d %02d:%02d:%02d (unit %d location %d)",
		 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		 dtm->tm_hour, dtm->tm_min, dtm->tm_sec, unitNum, loc);
	
	snprintf(sqlStr, sizeof(sqlStr),
		 "insert into messages (subject, message, time, expire, catgry, deliver, urgent) values "
                 "('%s','%s',CURRENT_TIMESTAMP,CURRENT_TIMESTAMP + interval '30 days', 0, 1, 1) RETURNING id AS msgid", 
		 descr, msgStr);

	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	result = PQexec(conn, sqlStr);

	if (!result) 
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "PQexec command failed, no error code\n");
		return -1;
	}
	else 
	{
		switch (PQresultStatus(result)) 
		{
			case PGRES_COMMAND_OK:
				if (trace) printf("\tCommand executed OK, %s rows affected\n", PQcmdTuples(result));
				break;
			case PGRES_TUPLES_OK:
				if (trace) printf("\tQuery may have returned data\n");
				break;
			default:
				DateTimeStampFile(stderr);
				fprintf(stderr, "Command failed with code %s, error message %s\n",
				       PQresStatus(PQresultStatus(result)), PQresultErrorMessage(result));
				PQclear(result);
				return -1;
		}
	}
	dbVal = PQgetvalue(result, 0, 0);
	msgid = atoi(dbVal);
	if (trace) printf("\tMessage ID is %d\n", msgid);

	sprintf(sqlStr, "insert into msgread (uid, mid) values (%d,%d)", unitNum, msgid);

	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	result = PQexec(conn, sqlStr);

	if (!result) 
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "PQexec command failed, no error code\n");
		return -1;
	}
	else 
	{
		switch (PQresultStatus(result)) 
		{
			case PGRES_COMMAND_OK:
				if (trace) printf("\tCommand executed OK, %s rows affected\n", PQcmdTuples(result));
				break;
			case PGRES_TUPLES_OK:
				if (trace) printf("\tQuery may have returned data\n");
				break;
			default:
				DateTimeStampFile(stderr);
				fprintf(stderr, "Command failed with code %s, error message %s\n",
				       PQresStatus(PQresultStatus(result)), PQresultErrorMessage(result));
				return -1;
		}
		PQclear(result);
	}

	return 0;
}

/*! \brief Add record to status log
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int updateDbLogStatus(unitInfoRec * unit,
		      int eventtype, 
		      int loc)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;
	int unitNum;

	unitNum = unit->unitNo;
	conn = unit->dbConnctn;

	if (trace) printf("\tAdd to status log\n");

	snprintf(sqlStr, sizeof(sqlStr), 
		"insert into statuslog (status, moment, unit, loc) "
		"values ('%d', '%s', %d, %d)",
		eventtype, updateDbGetTime(), unitNum, loc);

	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief Update current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int updateDbStatus(int eventtype,
		   unitInfoRec * unit)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;
	int unitNum;
	struct tm * dtm;
	time_t ts;

	conn = unit->dbConnctn;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);

	unitNum = unit->unitNo;
	
	if (trace) printf("\tUpdate current status\n");
	unit->ustatus = eventtype;

	snprintf(sqlStr, sizeof(sqlStr), 
		"update statuscurr set (status, moment) "
		"= ('%d', '%04d-%02d-%02d %02d:%02d:%02d') where unit=%d",
		eventtype, 
		dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
		unitNum);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	unit->sysping = time(NULL);

	return 0;
}

/*! \brief Update current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int updateDbPirError(unitInfoRec * unit)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;
	int unitNum;
	struct tm * dtm;
	time_t ts;

	if (trace) printf("\tUpdate PIR error\n");

	conn = unit->dbConnctn;

	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);

	unitNum = unit->unitNo;

	++(unit->pir0err);
	unit->pir0moment = ts;

	snprintf(sqlStr, sizeof(sqlStr), 
		"update statuscurr set (pir0err, pir0moment) "
		"= (%d, '%04d-%02d-%02d %02d:%02d:%02d') where unit=%d", 
		unit->pir0err,
		dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
		unitNum);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief Update process ID in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int updateDbPid(unitInfoRec * unit)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;
	int unitNum;
	int pid;

	conn = unit->dbConnctn;
	 
	pid = (int) getpid();

	unitNum = unit->unitNo;
	unit->pid = pid;
	
	if (trace) printf("\tUpdate current process ID to %d\n", pid);

	snprintf(sqlStr, sizeof(sqlStr), 
		"update statuscurr set (pid) = ('%d') where unit=%d", pid, unitNum);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief	Update event notice
	\param	conn	database connection info
	\param	logno	log number
	\return	Zero when successful, otherwise non-zero
 */
int updateDbResetRearm(unitInfoRec * unit)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;
	struct tm * dtm;
	time_t ts;

	conn = unit->dbConnctn;
	 
	ts = time(NULL);
	/* Add 10 minutes */
	ts += 600;
	dtm = (struct tm *)localtime(&ts);
	
	if (trace) printf("\tReset rearm time for unit %d\n", unit->unitNo);

	snprintf(sqlStr, sizeof(sqlStr), 
		"update statuscurr set (momentarm) = (null) where unit=%d",
		unit->unitNo);

	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	if (!result) 
	{
		DateTimeStampFile(stderr);
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
				DateTimeStampFile(stderr);
				fprintf(stderr, "Command failed with code %s, error message %s\n",
					PQresStatus(PQresultStatus(result)),
							PQresultErrorMessage(result));
				break;
		}
		PQclear(result);
	}
	unit->momentarm = 0;

	return 0;
}

/*! \brief Update logger heartbeat in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int updateDbLoggerCheck(unitInfoRec * unit)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;
	int unitNum;
	struct tm * dtm;
	time_t ts;

	conn = unit->dbConnctn;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);

	unitNum = unit->unitNo;

	if (trace) printf("\tUpdate logger heartbeat\n");

	snprintf(sqlStr, sizeof(sqlStr), 
		"update statuscurr set (logchk0, logchk) = (logchk, '%04d-%02d-%02d %02d:%02d:%02d') where unit=%d",
		dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		dtm->tm_hour, dtm->tm_min, dtm->tm_sec, 
		unitNum);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief Update heartbeat in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int updateDbHeartbeat(unitInfoRec * unit, 
		      int idx)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;
	struct tm * dtm;
	time_t ts;

	conn = unit->dbConnctn;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);

	if (trace) printf("\tUpdate heartbeat for PIR %d\n", idx+1);

	snprintf(sqlStr, sizeof(sqlStr), 
		"update statuscurr set (pir%dhbeat) = ('%04d-%02d-%02d %02d:%02d:%02d') where unit=%d", idx+1,
		dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
		unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief Update coordinator check in current status
	\param	unit		Unit record
	\param	val		Status value
	\return	Zero when successful, otherwise non-zero
 */
int updateDbCoordCheck(unitInfoRec * unit)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;
	int unitNum;
	struct tm * dtm;
	time_t ts;

	conn = unit->dbConnctn;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);

	unitNum = unit->unitNo;

	if (trace) printf("\tUpdate coordinator check\n");

	if (unit->pan[0])
		snprintf(sqlStr, sizeof(sqlStr), 
			"update statuscurr set (pan,coordchk0, coordchk, coordstatus) = ("
			"'%s',coordchk, '%04d-%02d-%02d %02d:%02d:%02d', %d) where unit=%d",
			unit->pan,
			dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
			dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
			1, unitNum);
	else
		snprintf(sqlStr, sizeof(sqlStr), 
			"update statuscurr set (coordchk0, coordchk, coordstatus) = ("
			"coordchk, '%04d-%02d-%02d %02d:%02d:%02d', %d) where unit=%d",
			dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
			dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
			1, unitNum);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief Update database with PIR addresses
	\param	unit	structure with addresses
	\return	Zero when successful, otherwise non-zero
*/
int updateDbSetPirHardware(unitInfoRec * unit)
{
	char sqlStr[1024], sqlStr2[128];
	PGconn *conn;
	PGresult *result;
	int i, cCount = 0;

	conn = unit->dbConnctn;
	
	if (trace) printf("\tUpdate PIR address(es) in hardware database\n");

	for (i=0; i<MAX_PIR; i++)
	{
		if (unit->piraddr[i] || unit->pirieee[i][0]) ++cCount;
	}
	if (trace) printf("\tUnit %d contains %d PIRs (actual %d)\n", unit->unitNo, unit->pircount, cCount);

	strcpy(sqlStr, "update hardware set (");
	for (i=0; i<MAX_PIR; i++)
	{
		if (i) strcat(sqlStr, ", ");
		snprintf(sqlStr2, sizeof(sqlStr2), "pir%daddr,pir%dieee,pir%dloc,pir%dzone", i+1, i+1, i+1, i+1);
		strcat(sqlStr, sqlStr2);
	}
	strcat(sqlStr, ") = (");
	for (i=0; i<MAX_PIR; i++)
	{
		if (i) strcat(sqlStr, ", ");
		if (unit->piraddr[i] || unit->pirieee[i][0])
			snprintf(sqlStr2, sizeof(sqlStr2), "x'%x'::int, '%s', %d, %d", 
				 unit->piraddr[i], unit->pirieee[i][0]?unit->pirieee[i]:"", unit->pirloc[i], unit->pirzone[i]);
		else
			snprintf(sqlStr2, sizeof(sqlStr2), "0,NULL,0,0");
		strcat(sqlStr, sqlStr2);
	}
	snprintf(sqlStr2, sizeof(sqlStr2), ") where unit=%d", unit->unitNo);
	strcat(sqlStr, sqlStr2);

	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief Update database with coordinator address
	\param	unit	structure with addresses
	\return	Zero when successful, otherwise non-zero
 */
int updateDbCoordinatorIeee(unitInfoRec * unit)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;

	conn = unit->dbConnctn;
	
	if (trace) printf("\tUpdate coordinator address\n");

	snprintf(sqlStr, sizeof(sqlStr), 
		"update hardware set (coordieee) = ('%s') where unit=%d",
		unit->cordieee, unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief Update database with gateway addresses
	\param	unit	structure with addresses
	\return	Zero when successful, otherwise non-zero
 */
int updateDbGateway(unitInfoRec * unit)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;

	conn = unit->dbConnctn;
	
	if (trace) printf("\tUpdate gateway address\n");

	snprintf(sqlStr, sizeof(sqlStr), 
		"update hardware set (gwayaddr, gwayieee) = (x'%04x'::int, '%s') where unit=%d",
		unit->gwaddr, unit->gwieee, unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief update Temperature
	\param	unitNum		unit number
	\param	temp		temperature in Celcius
	\return	Zero when successful, otherwise non-zero
*/
int updateDbTemperature(unitInfoRec * unit,
			int idx,
		        float temp)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;
	struct tm * dtm;
	time_t ts;

	conn = unit->dbConnctn;

	if (trace) printf("\tUpdate temperature %d to %.2f\n", idx, temp);

	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	
	unit->pirtemp[idx] = temp;
	unit->tmptime[idx] = ts;

	snprintf(sqlStr, sizeof(sqlStr), 
		 "update statuscurr set (temp%d, temp%dmoment) = ('%.2f', '%04d-%02d-%02d %02d:%02d:%02d') where unit=%d",
		 idx+1, idx+1, temp, 
		 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		 dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
		 unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

