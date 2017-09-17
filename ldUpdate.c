/*! \file updateDb.c
	\brief	Utilities for database interaction
*/

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: ldUpdate.c,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.3  2011/02/10 11:09:42  johan
 * Take out CVS Id tag
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
#include <time.h>
#include <libpq-fe.h>

#include "updateDb.h"
#include "zbData.h"
#include "zbDisplay.h"
#include "zbPoll.h"
#include "dbConnect.h"

extern int verbose;
extern int trace;

/*! \brief Update alarm address
	\param	unitNum		Unit number
	\param	shortAddr	Zigbee alarm device short address
	\param	ieeeAddr	Zigbee alarm device IEEE address
	\return	Zero when successful, otherwise non-zero
 */
int updateDbAlarmDev(int unitNum, 
		     char * shortAddr, 
		     char * ieeeAddr)
{
	char sqlStr[1024];
	PGconn *conn;
	PGresult *result;

	if (trace) printf("\tUpdate logger heartbeat\n");
	conn = PQconnectdb(connection_str());
	if (PQstatus(conn) == CONNECTION_BAD) 
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Connection to %s failed, %s", connection_str(),
			PQerrorMessage(conn));
		return -1;
	}

	snprintf(sqlStr, sizeof(sqlStr), 
		"update hardware set (alrmaddr, alrmieee) = (x'%s'::int, '%s') where unit=%d", 
		 shortAddr, ieeeAddr, unitNum);
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
					PQresStatus(PQresultStatus(result)),
							PQresultErrorMessage(result));
				break;
		}
		PQclear(result);
	}

	PQfinish(conn);
	return 0;
}
