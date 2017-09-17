/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.1 $
 * $State: Exp $
 *
 * $Log: lqiDb.c,v $
 * Revision 1.1  2011/11/19 16:31:44  johan
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

/*! \brief Update PIR event in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int lqiDbInsertInfo(PGconn * DbConn,
		    int unitNo, 
		    int nwaddr,
		    int rxqual)
{
	int rc;
	char sqlStr[1024];
	PGresult * result;
	struct tm * dtm;
	time_t ts;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	
	if (DbConn == NULL) 
	{
		  printf("No DB connection!");
		  return -1;
	}

	if (trace) printf("\tSet RX quality info for %04x: %d\n", nwaddr, rxqual);

	snprintf(sqlStr, sizeof(sqlStr), 
		 "insert into lqi_check (unit, moment, nwaddr, rxqual) values (%d, '%04d-%02d-%02d %02d:%02d:%02d', x'%04x'::int, %d)",
		 unitNo, 
		 dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday, dtm->tm_hour, dtm->tm_min, dtm->tm_sec, nwaddr, rxqual);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(DbConn, sqlStr);
	rc = updateDbResult(result);

	return rc;
}

/*! \brief Update PIR event in current status
	\param	eventtype	Event type
	\param	unitNum		Unit number
	\return	Zero when successful, otherwise non-zero
 */
int lqiDbUpdateHwInfo(PGconn * DbConn,
		      int unitNo, 
		      int nwaddr,
		      int logcltyp)
{
	int rc;
	char sqlStr[1024];
	PGresult * result;
	struct tm * dtm;
	time_t ts;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);

	if (trace) printf("\tUpdate hardware info for %04x: set logical type to %d\n", nwaddr, logcltyp);

	snprintf(sqlStr, sizeof(sqlStr), 
			 "update hwinfo set logcltyp = %d where unit=%d and nwaddr=%d", logcltyp,
			 unitNo, nwaddr);
	if (verbose) printf("%s %s\n", DateTimeStampStr(-1), sqlStr);

	result = PQexec(DbConn, sqlStr);
	rc = updateDbResult(result);

	return rc;
}
