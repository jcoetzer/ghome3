/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: unitRec.c,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.9  2011/02/23 16:27:15  johan
 * Store time of last heartbeat request
 *
 * Revision 1.8  2011/02/23 16:18:03  johan
 * Takoe out sorting of PIR records
 *
 * Revision 1.7  2011/02/23 15:48:49  johan
 * Reset alarm log
 *
 * Revision 1.6  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.5  2011/02/16 20:42:05  johan
 * Add user description field
 *
 * Revision 1.4  2011/02/10 15:20:39  johan
 * Log errors in PIR messages
 *
 * Revision 1.3  2011/02/10 11:09:42  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:16  johan
 * Add CVS stuff
 *
 *
 *
 */

#include <string.h>
#include <stdlib.h>

extern int trace;

#include "unitRec.h"
#include "zbSocket.h"

/*! \brief Initialize unit record
 *	\param	unit	Data record to be populated
 */
void unitDbInit(unitInfoRec * unit, 
		int unitNo,
		int update)
{
	int i;

	if (unitNo) unit->unitNo = unitNo;
	unit->pid = 0;
	unit->dbConnctn = NULL;
	unit->ustatus = ZBSTATUS_UNKNOWN;
	unit->dlay = 0;
	unit->live = 0;
	unit->pan[0] = 0;
	unit->appt[0] = 0;
	unit->gwip[0] = 0;
	unit->dinip[0] = 0;
	unit->oname[0] = 0;
	unit->edelay = 0;
	unit->gwport = ZIGBEE_GATEWAY_PORT;
	unit->hbeatreq = 0;
	unit->readmsg = 0;
	unit->hbeatmins = 0;
	unit->hbeatloss = 0;
	unit->cfedit = 0;
	unit->hwedit = 0;
	
	unit->currnt_hour = -1;
	
	for (i=0; i<24; i++)
	{
		unit->pirevnt[i] = 0;
	}
	
	for (i=0; i<MAX_DEVICES; i++)
		unitLqiInit(&unit->lqi[i], unit->unitNo, update);

	unit->pircount = 0;
	for (i=0; i<MAX_PIR; i++)
	{
		unit->piraddr[i] = 0;
		unit->pirieee[i][0] = 0;
		unit->pirdesc[i][0] = 0;
		unit->piruser[i][0] = 0;
		unit->pirloc[i] = 0;
		unit->pirtime[i] = 0;
		unit->pirbeat[i] = 0;
		unit->pirzone[i] = 1;
		unit->pirtemp[i] = 0.0;
		unit->tmptime[i] = 0;
	}

	unit->alrmaddr = 0;
	unit->alrmieee[0] = 0;
	unit->alrmreset = 0;
	unit->gwaddr = 0;
	unit->gwieee[0] = 0;
	unit->cordieee[0] = 0;
	unit->sysping = 0;
	unit->getalarm = 0;
	unit->momentarm = 0;
	unit->loglevel = 0;
	unit->pir0err = 0;
	unit->pir0moment = 0;
}

/*! \brief Copy unit record
 */
void unitDbCpy(unitInfoRec * dest, 
	       unitInfoRec * src)
{
	int i;

	dest->dbConnctn = src->dbConnctn;
	dest->ustatus = src->ustatus;
	dest->dlay = src->dlay;
	strcpy(dest->appt, src->appt);
	strcpy(dest->gwip, src->gwip);
	strcpy(dest->dinip, src->dinip);
	strcpy(dest->pan, src->pan);
	strcpy(dest->oname, src->oname);
	dest->edelay = src->edelay;
	dest->gwport = src->gwport;
	dest->cfedit = src->cfedit;
	dest->hwedit = src->hwedit;

	dest->pircount = src->pircount;
	for (i=0; i<MAX_PIR; i++)
	{
		dest->piraddr[i] = src->piraddr[i];
		strcpy(dest->pirieee[i], src->pirieee[i]);
		strcpy(dest->pirdesc[i], src->pirdesc[i]);
		strcpy(dest->piruser[i], src->piruser[i]);
		dest->pirloc[i] = src->pirloc[i];
		dest->pirtime[i] = src->pirtime[i];
	}

	dest->hbeatreq = src->hbeatreq;
	dest->alrmaddr = src->alrmaddr;
	dest->alrmreset = src->alrmreset;
	strcpy(dest->alrmieee, src->alrmieee);
	dest->gwaddr = src->gwaddr;
	strcpy(dest->gwieee, src->gwieee);
	strcpy(dest->cordieee, src->cordieee);
	dest->momentarm = src->momentarm;
	dest->loglevel = src->loglevel;
}

/*! \brief Compare unit records
 */
int unitDbCmp(unitInfoRec * dest, 
	      unitInfoRec * src)
{
	int i;
	
	if (dest->dlay != src->dlay) return 1;
	if (dest->pircount != src->pircount) return 1;
	if (dest->cfedit != src->cfedit) return 1;
	if (dest->hwedit != src->hwedit) return 1;
	for (i=0; i<MAX_PIR; i++)
	{
		if (dest->piraddr[i] != src->piraddr[i]) return 1;
		if (0 != strcmp(src->pirieee[i], dest->pirieee[i])) return 1;
	}
	return 0 ;
}

/*! \brief Search unit record for PIR
 *	\param	unit	Data record
 *	\param  nwkAdr	Network address of PIR unit
 *	\return index of PIR, -1 if not found
 */
int unitDbFindNwk(unitInfoRec * unit, 
		  char * nwkAdr)
{
	int i;
	int nwk;
	
	nwk = (int) strtol(nwkAdr, NULL, 16);
	if (0==nwk) return -1;
	for (i=0; i<MAX_PIR; i++)
	{
		if (unit->piraddr[i] == nwk)
		{
			if (trace) printf("\tPIR %d matches network address %04x\n", i+1, nwk);
			return i;
		}
	}
	if (trace) printf("\tPIR with network address %04x not in list\n", nwk);
	return -1;
}
