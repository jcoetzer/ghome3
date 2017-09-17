/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.3 $
 * $State: Exp $
 *
 * $Log: unitDb.c,v $
 * Revision 1.3  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.2  2011/06/13 21:57:26  johan
 * Update
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 *
 *
 *
 *
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libpq-fe.h>
#include <sys/types.h>
#include <signal.h>

#include "unitDb.h"
#include "zbData.h"
#include "zbPoll.h"
#include "updateDb.h"
#include "dbConnect.h"

extern int verbose;
extern int trace;

char LocatnDescrptn[MAX_LOC][128];

/*! \brief Set unit location 
	\return		Zero when successful, otherwise non-zero
 */
int unitDbSetLocation(unitInfoRec * unit)
{
	char sqlStr[512];
	int nFields, nTuples;
	int i, j, loc;
	PGresult * res;
	char * dbVal;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tSet unit location\n");

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
	snprintf(sqlStr, sizeof(sqlStr), "select loc, descrptn from locatn");
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
	if (trace) printf("\tDatabase contains %d locations\n", PQntuples(res));
	for (i = 0; i < PQntuples(res); i++)
	{
		if (trace) 
		{
			printf("\t");
			for (j = 0; j < nFields; j++) printf("\t'%-15s'", PQgetvalue(res, i, j));
			printf("\n");
		}
		loc = atoi(PQgetvalue(res, i, 0));				/*!< loc */
		dbVal = PQgetvalue(res, i, 1);					/*!< descrptn */
		if (trace) printf("\tLocation %d is %s\n", loc, dbVal);
		if (loc < MAX_LOC) strcpy(LocatnDescrptn[loc], dbVal);
		else printf("ERROR : ignore value for location %d (%s)\n", loc, dbVal);
	}
	PQclear(res);

	/* end the transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	return 0;
}

/*! \brief Get unit info 
 */
int unitDbGetLocation(int loc, 
		      char * descrptn)
{
	if (loc < MAX_LOC) 
		strcpy(descrptn, LocatnDescrptn[loc]);
	else
	{
		printf("ERROR : no value for location %d\n", loc);
		descrptn[0] = 0;
		return -1;
	}
	if (trace) printf("\tLocation %d is %s\n", loc, descrptn);
	return 0;
}

/*! \brief Read data for PIRs in unit record
	\param	unit	Data record to be populated
*/
int unitDbReloadPir(unitInfoRec * unit)
{
	int i, rc;

	/* Initialize PIRs in unit record */
	for (i=0; i<MAX_PIR; i++)
	{
		unit->piraddr[i] = 0;		/*!< Short address of PIR */
		unit->pirieee[i][0] = 0;	/*!< IEEE address of PIR */
		unit->pirdesc[i][0] = 0;	/*!< Location string */
		unit->piruser[i][0] = 0;	/*!< User description string */
		
		unit->pirtime[i] = 0;		/*!< Last activity time */
		unit->pirbeat[i] = 0;		/*!< Last heartbeat time */	
		unit->pirzone[i] = 0;		/*!< Day (1) or night (0) zone */
		unit->pirloc[i] = 0;		/*!< Location value */
		unit->pirtemp[i] = 0;		/*!< Temperature */
		unit->tmptime[i] = 0;		/*!< Last temperature time */
	}

	/* Read PIR info from table hardware */
	rc = unitDbGetPirHardware(unit);
	if (rc)
	{
		fprintf(stderr, "PIR info not found in database\n");
		return -1;
	}
	return 0;
}

/*! \brief Get unit info 
 */
char * unitDbGetLocStr(int loc)
{
	char * rc;
	if (loc < MAX_LOC)
		rc = LocatnDescrptn[loc];
	else
	{
		printf("ERROR : no value for location %d\n", loc);
		return NULL;
	}
	if (trace) printf("\tLocation %d: %s\n", loc, rc);
	return rc;
}

/*! \brief Get unit info 
	\param	unit	Data record to be populated
	\return		Zero when successful, otherwise non-zero
*/
int unitDbGetPirHardware(unitInfoRec * unit)
{
	char sqlStr[640];
	int nFields, nTuples;
	int i, j;
	PGresult * res;
	char * dbVal;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tGet unit hardware info\n");

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
		"select "
		"pir1addr,pir2addr,pir3addr,pir4addr,pir5addr,pir6addr,pir7addr,pir8addr,pir9addr,pir10addr, "	/* 0 - 9 */
		"pir1ieee,pir2ieee,pir3ieee,pir4ieee,pir5ieee,pir6ieee,pir7ieee,pir8ieee,pir9ieee,pir10ieee, "	/* 10 - 19 */
		"pir1loc,pir2loc,pir3loc,pir4loc,pir5loc,pir6loc,pir7loc,pir8loc,pir9loc,pir10loc, "	        /* 20 - 29 */
		"pir1zone,pir2zone,pir3zone,pir4zone,pir5zone,pir6zone,pir7zone,pir8zone,pir9zone,pir10zone "	/* 30 - 39 */
		"from hardware where unit=%d",
		unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return -1;
	}

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
		dbVal = PQgetvalue(res, i, 0);					/*!< pir1addr */
		if (strlen(dbVal)) unit->piraddr[0] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 1);					/*!< pir2addr */
		if (strlen(dbVal)) unit->piraddr[1] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 2);					/*!< pir3addr */
		if (strlen(dbVal)) unit->piraddr[2] = atoi(dbVal);
		dbVal = PQgetvalue(res, i,  3);					/*!< pir4addr */
		if (strlen(dbVal)) unit->piraddr[3] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 4);					/*!< pir5addr */
		if (strlen(dbVal)) unit->piraddr[4] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 5);					/*!< pir6addr */
		if (strlen(dbVal)) unit->piraddr[5] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 6);					/*!< pir7addr */
		if (strlen(dbVal)) unit->piraddr[6] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 7);					/*!< pir8addr */
		if (strlen(dbVal)) unit->piraddr[7] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 8);					/*!< pir9addr */
		if (strlen(dbVal)) unit->piraddr[8] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 9);					/*!< pir10addr */
		if (strlen(dbVal)) unit->piraddr[9] = atoi(dbVal);
		
		dbVal = PQgetvalue(res, i, 10);					/*!< pir1ieee */
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pirieee[0], dbVal);
		dbVal = PQgetvalue(res, i, 11);					/*!< pir2ieee */
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pirieee[1], dbVal);
		dbVal = PQgetvalue(res, i, 12);					/*!< pir3ieee */
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pirieee[2], dbVal);
		dbVal = PQgetvalue(res, i, 13);					/*!< pir4ieee */
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pirieee[3], dbVal);
		dbVal = PQgetvalue(res, i, 14);					/*!< pir5ieee */
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pirieee[4], dbVal);
		dbVal = PQgetvalue(res, i, 15);					/*!< pir6ieee */
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pirieee[5], dbVal);
		dbVal = PQgetvalue(res, i, 16);					/*!< pir7ieee */
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pirieee[6], dbVal);
		dbVal = PQgetvalue(res, i, 17);					/*!< pir8ieee */
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pirieee[7], dbVal);
		dbVal = PQgetvalue(res, i, 18);					/*!< pir9ieee */
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pirieee[8], dbVal);
		dbVal = PQgetvalue(res, i, 19);					/*!< pir10ieee */
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pirieee[9], dbVal);
		
		unit->pirloc[0] = atoi(PQgetvalue(res, i, 20));			/*!< pir1loc */
		unitDbGetLocation(unit->pirloc[0], unit->pirdesc[0]);
		unit->pirloc[1] = atoi(PQgetvalue(res, i, 21));			/*!< pir2loc */
		unitDbGetLocation(unit->pirloc[1], unit->pirdesc[1]);
		unit->pirloc[2] = atoi(PQgetvalue(res, i, 22));			/*!< pir3loc */
		unitDbGetLocation(unit->pirloc[2], unit->pirdesc[2]);
		unit->pirloc[3] = atoi(PQgetvalue(res, i, 23));			/*!< pir4loc */
		unitDbGetLocation(unit->pirloc[3], unit->pirdesc[3]);
		unit->pirloc[4] = atoi(PQgetvalue(res, i, 24));			/*!< pir5loc */
		unitDbGetLocation(unit->pirloc[4], unit->pirdesc[4]);
		unit->pirloc[5] = atoi(PQgetvalue(res, i, 25));			/*!< pir6loc */
		unitDbGetLocation(unit->pirloc[5], unit->pirdesc[5]);
		unit->pirloc[6] = atoi(PQgetvalue(res, i, 26));			/*!< pir7loc */
		unitDbGetLocation(unit->pirloc[6], unit->pirdesc[6]);
		unit->pirloc[7] = atoi(PQgetvalue(res, i, 27));			/*!< pir8loc */
		unitDbGetLocation(unit->pirloc[7], unit->pirdesc[7]);
		unit->pirloc[8] = atoi(PQgetvalue(res, i, 28));			/*!< pir9loc */
		unitDbGetLocation(unit->pirloc[8], unit->pirdesc[8]);
		unit->pirloc[9] = atoi(PQgetvalue(res, i, 29));			/*!< pir10loc */
		unitDbGetLocation(unit->pirloc[9], unit->pirdesc[9]);
		
		unit->pirzone[0] = atoi(PQgetvalue(res, i, 30));			/*!< pir1zone */
		unit->pirzone[1] = atoi(PQgetvalue(res, i, 31));			/*!< pir2zone */
		unit->pirzone[2] = atoi(PQgetvalue(res, i, 32));			/*!< pir3zone */
		unit->pirzone[3] = atoi(PQgetvalue(res, i, 33));			/*!< pir4zone */
		unit->pirzone[4] = atoi(PQgetvalue(res, i, 34));			/*!< pir5zone */
		unit->pirzone[5] = atoi(PQgetvalue(res, i, 35));			/*!< pir6zone */
		unit->pirzone[6] = atoi(PQgetvalue(res, i, 36));			/*!< pir7zone */
		unit->pirzone[7] = atoi(PQgetvalue(res, i, 37));			/*!< pir8zone */
		unit->pirzone[8] = atoi(PQgetvalue(res, i, 38));			/*!< pir9zone */
		unit->pirzone[9] = atoi(PQgetvalue(res, i, 39));			/*!< pir10zone */
	}
	PQclear(res);

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tUnit PIR data read\n");
	if (trace) dbUnitDisplayPirInfo(unit);
	return 0;
}

/*! \brief Get unit info 
	\param	unit	Data record to be populated
	\return		Zero when successful, otherwise non-zero
*/
time_t unitDbGetHardwareModif(unitInfoRec * unit)
{
	char sqlStr[640];
	int nFields, nTuples;
	int i, j;
	PGresult * res;
	char * dbVal;
	time_t lastmodif=0;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tGet unit hardware last modify time\n");

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
		"select extract(epoch from lastmodif) hwedit "
		"from hardware where unit=%d",
		unit->unitNo);
	if (trace) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return -1;
	}

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
		dbVal = PQgetvalue(res, i, 0);					/*!< hwedit */
		if (strlen(dbVal)) lastmodif = atoi(dbVal);
	}
	PQclear(res);

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tUnit hardware last modify time read\n");
	if (trace) dbUnitDisplayPirInfo(unit);
	return lastmodif;
}

/*! \brief Get unit info 
	\param	unit	Data record to be populated
	\return		Last modification time when successful, otherwise zero
*/
time_t unitDbGetAdomisModif(unitInfoRec * unit)
{
	char sqlStr[640];
	int nFields, nTuples;
	int i, j;
	PGresult * res;
	char * dbVal;
	time_t lastmodif=0;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tGet unit configuration last modify time\n");

	/* Start a transaction block */
	res = PQexec(conn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return 0;
	}

	/* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
	PQclear(res);

	/* Fetch rows from pg_database, the system catalog of databases */
	snprintf(sqlStr, sizeof(sqlStr), 
		"select extract(epoch from lastmodif) hwedit "
		"from adomis_box where unit=%d",
		unit->unitNo);
	if (trace) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return 0;
	}

	/* Print out the attribute names */
	nFields = PQnfields(res);
	nTuples = PQntuples(res);

	/* Print out the rows, there should only be one */
	for (i = 0; i < PQntuples(res); i++)
	{
		if (trace)
		{
			for (j = 0; j < nFields; j++) printf("\t'%-15s'", PQgetvalue(res, i, j));
			printf("\n");
		}
		dbVal = PQgetvalue(res, i, 0);					/*!< hwedit */
		if (strlen(dbVal)) lastmodif = atoi(dbVal);
	}
	PQclear(res);

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tUnit configuration last modify time read\n");
	if (trace) dbUnitDisplayPirInfo(unit);
	return lastmodif;
}

/*! \brief Get unit info 
	\param	unit	Data record to be populated
	\return		Zero when successful, otherwise non-zero
*/
int unitDbGetPirTemp(unitInfoRec * unit)
{
	char sqlStr[640];
	int nFields, nTuples;
	int i, j;
	PGresult * res;
	char * dbVal;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tGet unit temperature info\n");

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
		"select "
		"temp1,temp2,temp3,temp4,temp5,temp6,temp7,temp8,temp9,temp10,"
		"EXTRACT(EPOCH FROM temp1moment) t1,EXTRACT(EPOCH FROM temp2moment) t2,"
		"EXTRACT(EPOCH FROM temp3moment) t3,EXTRACT(EPOCH FROM temp4moment) t4,"
		"EXTRACT(EPOCH FROM temp5moment) t5,EXTRACT(EPOCH FROM temp6moment) t6,"
		"EXTRACT(EPOCH FROM temp7moment) t7,EXTRACT(EPOCH FROM temp8moment) t8,"
		"EXTRACT(EPOCH FROM temp9moment) t9,EXTRACT(EPOCH FROM temp10moment) t10, "
		"pan, pid, pir0err, EXTRACT(EPOCH FROM pir0moment) p0 "
		"from statuscurr where unit=%d",
		unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return -1;
	}

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
		dbVal = PQgetvalue(res, i, 0);					/*!< temp1 */
		if (strlen(dbVal)) unit->pirtemp[0] = atof(dbVal);
		dbVal = PQgetvalue(res, i, 1);					/*!< temp2 */
		if (strlen(dbVal)) unit->pirtemp[1] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 2);					/*!< temp3 */
		if (strlen(dbVal)) unit->pirtemp[2] = atoi(dbVal);
		dbVal = PQgetvalue(res, i,  3);					/*!< temp4 */
		if (strlen(dbVal)) unit->pirtemp[3] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 4);					/*!< temp5 */
		if (strlen(dbVal)) unit->pirtemp[4] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 5);					/*!< temp6 */
		if (strlen(dbVal)) unit->pirtemp[5] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 6);					/*!< temp7 */
		if (strlen(dbVal)) unit->pirtemp[6] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 7);					/*!< temp8 */
		if (strlen(dbVal)) unit->pirtemp[7] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 8);					/*!< temp9 */
		if (strlen(dbVal)) unit->pirtemp[8] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 9);					/*!< temp10 */
		if (strlen(dbVal)) unit->pirtemp[9] = atoi(dbVal);
		
		dbVal = PQgetvalue(res, i, 10);					/*!< pir1addr */
		if (strlen(dbVal)) unit->tmptime[0] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 11);					/*!< pir2addr */
		if (strlen(dbVal)) unit->tmptime[1] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 12);					/*!< pir3addr */
		if (strlen(dbVal)) unit->tmptime[2] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 13);					/*!< pir4addr */
		if (strlen(dbVal)) unit->tmptime[3] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 14);					/*!< pir5addr */
		if (strlen(dbVal)) unit->tmptime[4] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 15);					/*!< pir6addr */
		if (strlen(dbVal)) unit->tmptime[5] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 16);					/*!< pir7addr */
		if (strlen(dbVal)) unit->tmptime[6] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 17);					/*!< pir8addr */
		if (strlen(dbVal)) unit->tmptime[7] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 18);					/*!< pir9addr */
		if (strlen(dbVal)) unit->tmptime[8] = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 19);					/*!< pir10addr */
		if (strlen(dbVal)) unit->tmptime[9] = atoi(dbVal);
		
		dbVal = PQgetvalue(res, i, 20);					/*!< pan */
		if (trace) printf("\tPAN address is '%s'\n", dbVal);
		if (strlen(dbVal) && dbVal[0] != ' ') strcpy(unit->pan, dbVal);
		
		dbVal = PQgetvalue(res, i, 21);					/*!< pid */
		unit->pid = (pid_t) atoi(dbVal);
		if (trace) printf("\tProcess ID is %d\n", unit->pid);

		dbVal = PQgetvalue(res, i, 22);					/*!< pir0err */
		unit->pir0err = atoi(dbVal);
		dbVal = PQgetvalue(res, i, 23);					/*!< pir0moment */
		unit->pir0moment = atoi(dbVal);
	}
	PQclear(res);

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tUnit PIR data read\n");
	if (trace) dbUnitDisplayPirInfo(unit);
	return 0;
}

/*! \brief Get unit info 
	\param	unit	Data record to be populated
	\return		Zero when successful, otherwise non-zero
*/
int unitDbListAll(unitInfoRec * unit)
{
	char sqlStr[640];
	int nFields, nTuples;
	int i, j;
	pid_t pid;
	PGresult * res;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tRead units info\n");

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
		"select ip,appt,name,delay,"			/* 0 1 2 3 */
		"alrmaddr,alrmieee,gwayieee,coordieee,"		/* 4 5 6 7 */
		"delay,gwayaddr,hbeatmins,hbeatloss, "		/* 8 9 10 11 */
		"a.mac, a.unit, pid, udescrptn, "		/* 12 13 14 15 */
		"dinip "					/* 16 */
		"from adomis_box a,hardware h, statuscurr s, unitstatus u "
		"where a.unit=h.unit and a.unit=s.unit and status=ustatus "
		"order by a.unit");
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return -1;
	}

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
		strcpy(unit->gwip, PQgetvalue(res, i, 0));			/*!< ip  */
		strcpy(unit->appt, PQgetvalue(res, i, 1));			/*!< appt */
		strcpy(unit->oname, PQgetvalue(res, i, 2));			/*!< name */
		unit->edelay = atoi( PQgetvalue(res, i, 3) );			/*!< delay */
		
		unit->alrmaddr = atoi(PQgetvalue(res, i, 4));			/*!< alrmaddr */

		strcpy(unit->alrmieee, PQgetvalue(res, i, 5));			/*!< alrmieee */
		if (trace) printf("\tAlarm IEEE address is '%s'\n", unit->alrmieee);

		strcpy(unit->gwieee, PQgetvalue(res, i, 6));			/*!< gwayieee */
		if (trace) printf("\tGateway IEEE address is '%s'\n", unit->gwieee);

		strcpy(unit->cordieee, PQgetvalue(res, i, 7));			/*!< coordieee */
		if (trace) printf("\tCoordinator IEEE address is '%s'\n", unit->cordieee);

		unit->dlay = atoi(PQgetvalue(res, i, 8));			/*!< delay */
		unit->gwaddr = atoi(PQgetvalue(res, i, 9));			/*!< gwayaddr */
		unit->hbeatmins = atoi(PQgetvalue(res, i, 10));			/*!< hbeatmins */
		unit->hbeatloss = atoi(PQgetvalue(res, i, 11));			/*!< hbeatloss */
		unit->unitNo = atoi(PQgetvalue(res, i, 13));			/*!< unit */
		pid = (pid_t) atoi(PQgetvalue(res, i, 14));			/*!< pid */
		if (kill(pid, 0)) pid = 0;
		
		strcpy(unit->dinip, PQgetvalue(res, i, 16));			/*!< dinip  */
		
		printf("%d\t%-20s\t%d\t%s\t%16s\t%17s\t%-15s\n", 
		       unit->unitNo, PQgetvalue(res, i, 15), pid, unit->gwip, unit->gwieee, PQgetvalue(res, i, 12),
		       unit->dinip);
	}
	PQclear(res);

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tUnits read from database\n");
	return 0;
}

/*! \brief Get unit info 
	\param	unit	Data record to be populated
	\return		Zero when successful, otherwise non-zero
*/
int unitDbGetInfo(unitInfoRec * unit)
{
	char sqlStr[640];
	int nFields, nTuples;
	int i, j;
	PGresult * res;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tGet unit info\n");

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
		"select ip,appt,name,delay,"			/* 0 1 2 3 */
		"alrmaddr,alrmieee,gwayieee,coordieee,"		/* 4 5 6 7 */
		"delay,gwayaddr,hbeatmins,hbeatloss, "		/* 8 9 10 11 */
		"EXTRACT(EPOCH FROM h.lastmodif) hwedit, "	/* 12 */
		"EXTRACT(EPOCH FROM a.lastmodif) cfedit, "	/* 13 */
		"loglevel, dinip "				/* 14 15 */
		"from adomis_box a,hardware h where a.unit=%d and a.unit=h.unit",
		unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return -1;
	}

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
		strcpy(unit->gwip, PQgetvalue(res, i, 0));			/*!< ip  */
		strcpy(unit->appt, PQgetvalue(res, i, 1));			/*!< appt */
		strcpy(unit->oname, PQgetvalue(res, i, 2));			/*!< name */
		unit->edelay = atoi( PQgetvalue(res, i, 3) );			/*!< delay */
		
		unit->alrmaddr = atoi(PQgetvalue(res, i, 4));			/*!< alrmaddr */

		strcpy(unit->alrmieee, PQgetvalue(res, i, 5));			/*!< alrmieee */
		if (unit->alrmieee[0] == ' ') unit->alrmieee[0] = 0;
		if (trace) printf("\tAlarm IEEE address is '%s'\n", unit->alrmieee);

		strcpy(unit->gwieee, PQgetvalue(res, i, 6));			/*!< gwayieee */
		if (unit->gwieee[0] == ' ') unit->gwieee[0] = 0;
		if (trace) printf("\tGateway IEEE address is '%s'\n", unit->gwieee);

		strcpy(unit->cordieee, PQgetvalue(res, i, 7));			/*!< coordieee */
		if (unit->cordieee[0] == ' ') unit->cordieee[0] = 0;
		if (trace) printf("\tCoordinator IEEE address is '%s'\n", unit->cordieee);

		unit->dlay = atoi(PQgetvalue(res, i, 8));			/*!< delay */
		unit->gwaddr = atoi(PQgetvalue(res, i, 9));			/*!< gwayaddr */
		unit->hbeatmins = atoi(PQgetvalue(res, i, 10));			/*!< hbeatmins */
		unit->hbeatloss = atoi(PQgetvalue(res, i, 11));			/*!< hbeatloss */
		unit->hwedit = atoi(PQgetvalue(res, i, 12));			/*!< hwedit */
		unit->cfedit = atoi(PQgetvalue(res, i, 13));			/*!< cfedit */
		unit->loglevel = atoi(PQgetvalue(res, i, 14));			/*!< loglevel */
		strcpy(unit->dinip, PQgetvalue(res, i, 15));			/*!< dinip  */
	}
	PQclear(res);

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tUnit info read from database\n");
	if (trace) dbUnitDisplayPirInfo(unit);
	return 0;
}

/*! \brief Get unit info 
	\param	unit	Data record to be populated
	\return		Zero when successful, otherwise non-zero
*/
int unitDbUpdateInfo(unitInfoRec * unit)
{
	char sqlStr[640];
	int nFields, nTuples;
	int i, j;
	PGresult * res;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tUpdate unit info\n");

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
		"select appt,name,delay,"			/* 0 1 2  */
		"EXTRACT(EPOCH FROM lastmodif) cfedit, "	/* 3 */
		"loglevel,dinip "				/* 4 5 */
		"from adomis_box a where a.unit=%d",
		unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return -1;
	}

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
		strcpy(unit->appt, PQgetvalue(res, i, 0));			/*!< appt */
		strcpy(unit->oname, PQgetvalue(res, i, 1));			/*!< name */
		unit->edelay = atoi( PQgetvalue(res, i, 2) );			/*!< delay */
		unit->cfedit = atoi(PQgetvalue(res, i, 3));			/*!< cfedit */
		unit->loglevel = atoi(PQgetvalue(res, i, 4));			/*!< loglevel */
		strcpy(unit->dinip, PQgetvalue(res, i, 5));			/*!< dinip */
	}
	PQclear(res);

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tUnit info updated from database\n");
	if (trace) dbUnitDisplayPirInfo(unit);
	return 0;
}

/*! \brief Update unit info 
	\param	unit	Data record to be updated
	\return		Zero when successful, otherwise non-zero
*/
int unitDbUpdateLocation(unitInfoRec * unit)
{
	char sqlStr[640];
	int nFields, nTuples;
	int i, j;
	PGresult * res;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tUpdate unit PIR location info\n");

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
		 "select pir1loc,pir2loc,pir3loc,pir4loc,pir5loc,pir6loc,pir7loc,pir8loc,pir9loc,pir10loc "
		 "from hardware where unit=%d",
		 unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return -1;
	}

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
		unit->pirloc[0] = atoi(PQgetvalue(res, i, 0));			/*!< pir1loc */
		unitDbGetLocation(unit->pirloc[0], (unit->pirdesc[0]));
		unit->pirloc[1] = atoi(PQgetvalue(res, i, 1));			/*!< pir2loc */
		unitDbGetLocation(unit->pirloc[1], (unit->pirdesc[1]));
		unit->pirloc[2] = atoi(PQgetvalue(res, i, 2));			/*!< pir3loc */
		unitDbGetLocation(unit->pirloc[2], (unit->pirdesc[2]));
		unit->pirloc[3] = atoi(PQgetvalue(res, i, 3));			/*!< pir4loc */
		unitDbGetLocation(unit->pirloc[3], (unit->pirdesc[3]));
		unit->pirloc[4] = atoi(PQgetvalue(res, i, 4));			/*!< pir5loc */
		unitDbGetLocation(unit->pirloc[4], (unit->pirdesc[4]));
		unit->pirloc[5] = atoi(PQgetvalue(res, i, 5));			/*!< pir6loc */
		unitDbGetLocation(unit->pirloc[5], (unit->pirdesc[5]));
		unit->pirloc[6] = atoi(PQgetvalue(res, i, 6));			/*!< pir7loc */
		unitDbGetLocation(unit->pirloc[6], (unit->pirdesc[6]));
		unit->pirloc[7] = atoi(PQgetvalue(res, i, 7));			/*!< pir8loc */
		unitDbGetLocation(unit->pirloc[7], (unit->pirdesc[7]));
		unit->pirloc[8] = atoi(PQgetvalue(res, i, 8));			/*!< pir9loc */
		unitDbGetLocation(unit->pirloc[8], (unit->pirdesc[8]));
		unit->pirloc[8] = atoi(PQgetvalue(res, i, 9));			/*!< pir10loc */
		unitDbGetLocation(unit->pirloc[9], (unit->pirdesc[9]));
	}

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tUnit PIR info read from database\n");
	if (trace) dbUnitDisplayPirInfo(unit);
	return 0;
}

/*! \brief Update PIR update info 
	\param	unit	Data record to be updated
	\return		Zero when successful, otherwise non-zero
*/
int unitDbPirMoment(unitInfoRec * unit)
{
	static char sqlStr[1024], sqlBufStr[512];
	int nFields, nTuples;
	int i, j;
	PGresult * res;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tUpdate unit PIR update info\n");

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
		 "select "
		 "EXTRACT(EPOCH FROM pir1moment) t1,EXTRACT(EPOCH FROM pir2moment) t2,"
		 "EXTRACT(EPOCH FROM pir3moment) t3,EXTRACT(EPOCH FROM pir4moment) t4,"
		 "EXTRACT(EPOCH FROM pir5moment) t5,EXTRACT(EPOCH FROM pir6moment) t6,"
		 "EXTRACT(EPOCH FROM pir7moment) t7,EXTRACT(EPOCH FROM pir8moment) t8,"
		 "EXTRACT(EPOCH FROM pir9moment) t9, EXTRACT(EPOCH FROM pir10moment) t10,");
	/* String is too long to generate in one pass */
	snprintf(sqlBufStr,sizeof(sqlBufStr),
		 "EXTRACT(EPOCH FROM pir1hbeat) h1,EXTRACT(EPOCH FROM pir2hbeat) h2,"
		 "EXTRACT(EPOCH FROM pir3hbeat) h3,EXTRACT(EPOCH FROM pir4hbeat) h4,"
		 "EXTRACT(EPOCH FROM pir5hbeat) h5,EXTRACT(EPOCH FROM pir6hbeat) h6,"
		 "EXTRACT(EPOCH FROM pir7hbeat) h7,EXTRACT(EPOCH FROM pir8hbeat) h8,"
		 "EXTRACT(EPOCH FROM pir9hbeat) h9,EXTRACT(EPOCH FROM pir10hbeat) h10 " 
		 "from statuscurr where unit=%d",
		 unit->unitNo);
	strcat(sqlStr, sqlBufStr);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);
	res = PQexec(conn, sqlStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		updateDbError(conn);
		return -1;
	}

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
		unit->pirtime[0] = atoi(PQgetvalue(res, i, 0));
		unit->pirtime[1] = atoi(PQgetvalue(res, i, 1));
		unit->pirtime[2] = atoi(PQgetvalue(res, i, 2));
		unit->pirtime[3] = atoi(PQgetvalue(res, i, 3));
		unit->pirtime[4] = atoi(PQgetvalue(res, i, 4));
		unit->pirtime[5] = atoi(PQgetvalue(res, i, 5));
		unit->pirtime[6] = atoi(PQgetvalue(res, i, 6));
		unit->pirtime[7] = atoi(PQgetvalue(res, i, 7));
		unit->pirtime[8] = atoi(PQgetvalue(res, i, 8));
		unit->pirtime[9] = atoi(PQgetvalue(res, i, 9));
		
		unit->pirbeat[0] = atoi(PQgetvalue(res, i, 10));
		unit->pirbeat[1] = atoi(PQgetvalue(res, i, 11));
		unit->pirbeat[2] = atoi(PQgetvalue(res, i, 12));
		unit->pirbeat[3] = atoi(PQgetvalue(res, i, 13));
		unit->pirbeat[4] = atoi(PQgetvalue(res, i, 14));
		unit->pirbeat[5] = atoi(PQgetvalue(res, i, 15));
		unit->pirbeat[6] = atoi(PQgetvalue(res, i, 16));
		unit->pirbeat[7] = atoi(PQgetvalue(res, i, 17));
		unit->pirbeat[8] = atoi(PQgetvalue(res, i, 18));
		unit->pirbeat[9] = atoi(PQgetvalue(res, i, 19));
	}
	PQclear(res);

	/* End transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	if (trace) printf("\tPIR update info read from database\n");
	if (trace) dbUnitDisplayPirInfo(unit);
	return 0;
}

/*! \brief Display unit info
	\param	unit		Record with addresses etc.
*/
void unitDbShowInfo(unitInfoRec * unit)
{
	int i;
	char dtbuff[64];
	time_t ptime;
	
	printf("Unit number : %d\n", unit->unitNo);
	printf("Unit name : %s\n", unit->oname);
	printf("Description : %s\n", unit->appt);
	printf("\tPAN address : %s\n", unit->pan ? unit->pan : "Unknown");
	printf("\tCurrent status (armed, disarmed, etc.) : %d\n", unit->ustatus);
	printf("\tIP address of Zigbee gateway : %s\n", unit->gwip);
	printf("\tPort of Zigbee gateway : %d\n", unit->gwport);
	printf("\tIP address of Android panel : %s\n", unit->dinip);
	printf("\tEntry delay : %d sec\n", unit->dlay);
	
	for (i=0; i<MAX_PIR; i++)
	{
		if (verbose || unit->piraddr[i] || unit->pirieee[i][0])
		{
			printf("\t\tPIR %d:\n", i+1);

			if (unit->piraddr[i]) printf("\t\t\tNetwork address\t: %04X\n", unit->piraddr[i]);
			else printf("\t\t\tNetwork address\t: Unknown\n");

			printf("\t\t\tIEEE address\t: '%s'\n", unit->pirieee[i][0] ? unit->pirieee[i]  : "Unknown");

			printf("\t\t\tDescription\t: %s (%d)\n", unit->pirdesc[i], unit->pirloc[i]);
			
			dtbuff[0] = 0;
			ptime = unit->pirtime[i];
			if (ptime) getDateTimeStamp(&ptime, dtbuff, sizeof(dtbuff));
			printf("\t\t\tLast activity\t: %s\n", dtbuff);
			
			dtbuff[0] = 0;
			ptime = unit->tmptime[i];
			if (ptime) getDateTimeStamp(&ptime, dtbuff, sizeof(dtbuff));
			else strcpy(dtbuff, "never");
			printf("\t\t\tTemperature\t: %.2f deg C update %s\n", unit->pirtemp[i], dtbuff);
			
			printf("\t\t\tZone\t\t: %s\n", unit->pirzone[i] ? "day" : "night");
			
			dtbuff[0] = 0;
			ptime = unit->pirbeat[i];
			if (ptime) getDateTimeStamp(&ptime, dtbuff, sizeof(dtbuff));
			printf("\t\t\tHeartbeat\t: %s\n", dtbuff);
		}
		else
		{
			printf("\t\tPIR %d:\n", i+1);
		}
	}

	if (unit->alrmaddr) printf("\tShort address of alarm \t\t: %04x\n", unit->alrmaddr);
	else printf("\tShort address of alarm \t\t: Unknown\n");
	printf("\tIEEE address of alarm \t\t: '%s'\n", unit->alrmieee[0] ? unit->alrmieee : "Unknown");
	printf("\tNetwork address of gateway \t: %04x\n", unit->gwaddr);
	printf("\tIEEE address of gateway \t: '%s'\n", unit->gwieee[0] ? unit->gwieee : "Unknown");
	printf("\tIEEE address of coordinator \t: '%s'\n", unit->cordieee[0] ? unit->cordieee : "Unknown");
	if (unit->momentarm)
	{
		ptime = unit->momentarm;
		getDateTimeStamp(&ptime, dtbuff, sizeof(dtbuff));
		printf("\tTime to arm again \t: %s\n", dtbuff);
	}
	dtbuff[0] = 0;
	ptime = unit->cfedit;
	if (ptime) getDateTimeStamp(&ptime, dtbuff, sizeof(dtbuff));
	printf("\tConfiguration update \t\t: %s\n", dtbuff);
	dtbuff[0] = 0;
	ptime = unit->hwedit;
	if (ptime) getDateTimeStamp(&ptime, dtbuff, sizeof(dtbuff));
	printf("\tHardware update \t\t: %s\n", dtbuff);
}

/*! \brief Update PIR event in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int unitDbSetPirStatus(unitInfoRec * unit)
{
	static char sqlStr[1024], sqlStr2[128];
	PGresult *result;
	struct tm * dtm;
	time_t ts;
	int i, j;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tUpdate event for all PIRs\n");

	snprintf(sqlStr, sizeof(sqlStr), "update statuscurr set (");
	j = 0;
	for (i=0; i<MAX_PIR; i++)
	{
		if (unit->pirtime[i] && unit->piraddr[i])
		{
			if (j) strcat(sqlStr,",");
			sprintf(sqlStr2, "pir%dmoment,pir%dhbeat,temp%d,temp%dmoment", i+1, i+1, i+1, i+1);
			strcat(sqlStr, sqlStr2);
			j++;
		}
	}
	if (! j) /* Nothing to do */
	{
		if (trace) printf("\tNo PIR event data available\n");
		return 0;
	}

	strcat(sqlStr, ") = (");
	j = 0;
	for (i=0; i<MAX_PIR; i++)
	{
		/* Activity detected timestamp */
		ts = unit->pirtime[i];
		if (ts && unit->piraddr[i])
		{
			if (j) strcat(sqlStr,",");
			dtm = (struct tm *)localtime(&ts);
			snprintf(sqlStr2, sizeof(sqlStr), "'%04d-%02d-%02d %02d:%02d:%02d',",
				 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
				 dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
			strcat(sqlStr, sqlStr2);
			/* Heartbeat timestamp */
			ts = unit->pirbeat[i];
			if (ts)
			{
				dtm = (struct tm *)localtime(&ts);
				snprintf(sqlStr2, sizeof(sqlStr), "'%04d-%02d-%02d %02d:%02d:%02d',",
					dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
					dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
				strcat(sqlStr, sqlStr2);
			}
			else
				strcat(sqlStr, "NULL,");
			/* Temperature reading */
			sprintf(sqlStr2, "%.2f,", unit->pirtemp[i]);
			strcat(sqlStr, sqlStr2);
			/* Time of temperature reading */
			ts = unit->tmptime[i];
			if (ts)
			{
				dtm = (struct tm *)localtime(&ts);
				snprintf(sqlStr2, sizeof(sqlStr), "'%04d-%02d-%02d %02d:%02d:%02d'",
					dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
					dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
				strcat(sqlStr, sqlStr2);
			}
			else
				strcat(sqlStr, "NULL");
			j++;
		}
	}
	sprintf(sqlStr2, ") where unit=%d", unit->unitNo);
	strcat(sqlStr, sqlStr2);
	
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);
	
	snprintf(sqlStr, sizeof(sqlStr), "update statuscurr set (");
	for (i=0; i<24; i++)
	{
		snprintf(sqlStr2, sizeof(sqlStr2), "pirevnt[%d]", i);
		strcat(sqlStr, sqlStr2);
		if (i<23) strcat(sqlStr, ",");
	}
	strcat(sqlStr, ") = (");
	for (i=0; i<24; i++)
	{
		if (unit->pirevnt[i] > 0)
		{
			snprintf(sqlStr2, sizeof(sqlStr2), "%d", unit->pirevnt[i]);
			strcat(sqlStr, sqlStr2);
		}
		else
			strcat(sqlStr, "0");
		if (i<23) strcat(sqlStr, ",");
	}
	sprintf(sqlStr2, ") where unit=%d", unit->unitNo);
	strcat(sqlStr, sqlStr2);
	
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief Update PIR event in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int unitDbGetPirStatus(unitInfoRec * unit)
{
	static char sqlStr[1024], sqlStr2[128];
	PGresult *result;
	int i;
	PGconn * conn = unit->dbConnctn;

	if (trace) printf("\tRead events for all PIRs\n");
	
	snprintf(sqlStr, sizeof(sqlStr), "select ");
	for (i=0; i<24; i++)
	{
		snprintf(sqlStr2, sizeof(sqlStr2), "pirevnt[%d]", i);
		strcat(sqlStr, sqlStr2);
		if (i<23) strcat(sqlStr, ",");
	}
	sprintf(sqlStr2, " from statuscurr where unit=%d", unit->unitNo);
	strcat(sqlStr, sqlStr2);
	
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	if (PQresultStatus(result) != PGRES_TUPLES_OK)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Database select failed: %s\n", PQerrorMessage(conn));
		PQclear(result);
		updateDbError(conn);
		return -1;
	}
	
	if (PQntuples(result))
	{
		for (i = 0; i < 24; i++)
		{
			unit->pirevnt[i] = atoi(PQgetvalue(result, 0, i));
			if (trace) printf("\tPIR count %d = %d\n", i, unit->pirevnt[i]);
		}
	}

	PQclear(result);

	return 0;
}

/*! \brief Update PIR event in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int unitDbPirEvent(unitInfoRec * unit, 
		   int idx)
{
	char sqlStr[1024];
	PGresult * result;
	struct tm * dtm;
	time_t ts;
	int chour;
	PGconn * conn = unit->dbConnctn;
	
	if (idx<0 || idx>=MAX_PIR)
	{
		fprintf(stderr, "Invalid PIR number %d", idx+1);
		return -1;
	}
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);

	if (trace) printf("\tUpdate event for PIR %d\n", idx+1);

	chour = dtm->tm_hour;
	if (chour != unit->currnt_hour)
	{
		unit->currnt_hour = chour;
		unit->pirevnt[chour] = 0;
	}
	++unit->pirevnt[chour];
	if (trace) printf("\tEvents for hour %d = %d\n", chour, unit->pirevnt[chour]);
	
	if (unit->live)
	{
		snprintf(sqlStr, sizeof(sqlStr), 
			"update statuscurr set (pir%dmoment, pirevnt[%d]) = ('%04d-%02d-%02d %02d:%02d:%02d', %d) where unit=%d", idx+1, chour,
			dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
			dtm->tm_hour, dtm->tm_min, dtm->tm_sec, unit->pirevnt[chour],
			unit->unitNo);
		if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

		result = PQexec(conn, sqlStr);
		updateDbResult(result);
	}

	return 0;
}

/*! \brief Update PIR event in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int unitDbInsertHwInfo(unitInfoRec * unit, 
		       int nwaddr,
		       char * ieeeaddr,
		       char * udescrptn)
{
	int rc;
	char sqlStr[1024];
	PGresult * result;
	struct tm * dtm;
	time_t ts;
	PGconn * conn = unit->dbConnctn;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);

	if (trace) printf("\tSet hardware info for %04x: '%s'\n", nwaddr, udescrptn);

	if (ieeeaddr)
		snprintf(sqlStr, sizeof(sqlStr), 
			 "insert into hwinfo (unit, nwaddr, descrptn, ieeeaddr, moment) values (%d, x'%04x'::int, '%s', '%s', '%04d-%02d-%02d %02d:%02d:%02d')",
			 unit->unitNo, nwaddr, udescrptn, ieeeaddr,
			 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
	else
		snprintf(sqlStr, sizeof(sqlStr), 
			 "insert into hwinfo (unit, nwaddr, descrptn, moment) values (%d, x'%04x'::int, '%s', '%04d-%02d-%02d %02d:%02d:%02d')",
			 unit->unitNo, nwaddr, udescrptn,
			 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	rc = updateDbResult(result);

	return rc;
}

/*! \brief Update PIR event in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int unitDbUpdateHwInfo(unitInfoRec * unit, 
		       int nwaddr,
		       char * ieeeaddr,
		       char * udescrptn)
{
	int rc;
	char sqlStr[1024];
	PGresult * result;
	struct tm * dtm;
	time_t ts;
	PGconn * conn = unit->dbConnctn;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);

	if (trace) printf("\tUpdate hardware info for %04x: '%s'\n", nwaddr, udescrptn);

	if (ieeeaddr)
		snprintf(sqlStr, sizeof(sqlStr), 
			 "update hwinfo set (descrptn, ieeeaddr, moment) = ('%s', '%s', '%04d-%02d-%02d %02d:%02d:%02d') where unit=%d and nwaddr=%d", 
			 udescrptn, ieeeaddr,
			 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
			 unit->unitNo, nwaddr);
	else
		snprintf(sqlStr, sizeof(sqlStr), 
			 "update hwinfo set (descrptn, moment) = ('%s', '%04d-%02d-%02d %02d:%02d:%02d') where unit=%d and nwaddr=%d", 
			 udescrptn,
			 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec,
			 unit->unitNo, nwaddr);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	rc = updateDbResult(result);

	return rc;
}

/*! \brief Update PIR network and IEEE addresses in hardware table
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int unitDbUpdateAddr(unitInfoRec * unit, 
		     int idx)
{
	char sqlStr[1024];
	PGresult *result;
	struct tm * dtm;
	time_t ts;
	PGconn * conn;
	
	conn = unit->dbConnctn;
	
	if (idx<0 || idx>=MAX_PIR)
	{
		fprintf(stderr, "Invalid PIR number %d", idx+1);
		return -1;
	}
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);

	if (verbose) printf("%s Update addresses for PIR %d : %04x, %s\n", DateTimeStampStr(-1), idx+1, unit->piraddr[idx], unit->pirieee[idx]);

	snprintf(sqlStr, sizeof(sqlStr), 
		"update hardware set (pir%daddr, pir%dieee) = (x'%04x'::int,'%s') where unit=%d", idx+1, idx+1, 
		unit->piraddr[idx], unit->pirieee[idx], unit->unitNo);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(conn, sqlStr);
	updateDbResult(result);

	return 0;
}

/*! \brief Display PIRs in unit record
	\param	unit		Record with addresses
 */
void dbUnitDisplayPirInfo(unitInfoRec * unit)
{
	int i;

	if (! unit) return;
	for (i=0; i<MAX_PIR; i++)
	{
		if ((! unit->piraddr[i]) && (! unit->pirieee[i][0])) continue;
		printf("\tPIR %d : \t%04x (%s)\t%s\t%s\t%s\n", i+1,
			unit->piraddr[i], 
			unit->pirieee[i][0] ? unit->pirieee[i] : "unknown",
			DateTimeStampStr(unit->pirtime[i]),
			unit->pirdesc[i],
			unit->piruser[i]);
	}
}
