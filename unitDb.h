#ifndef UNITDB_H
#define UNITDB_H

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: unitDb.h,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.6  2011/02/24 16:20:55  johan
 * Store user descriptor in database
 *
 * Revision 1.5  2011/02/23 16:18:03  johan
 * Takoe out sorting of PIR records
 *
 * Revision 1.4  2011/02/10 11:09:42  johan
 * Take out CVS Id tag
 *
 * Revision 1.3  2011/02/09 19:31:50  johan
 * Stop core dump during polling
 *
 * Revision 1.2  2011/02/03 08:38:15  johan
 * Add CVS stuff
 *
 *
 *
 */

#include <time.h>
#include <libpq-fe.h>

#include "unitLqi.h"
#include "unitRec.h"

int unitDbListAll(unitInfoRec * unit);
int unitDbGetInfo(unitInfoRec * unit);
int unitDbUpdateInfo(unitInfoRec * unit);
int unitDbGetPirHardware(unitInfoRec * unit);
time_t unitDbGetHardwareModif(unitInfoRec * unit);
time_t unitDbGetAdomisModif(unitInfoRec * unit);
int unitDbGetPirTemp(unitInfoRec * unit);
void unitDbInit(unitInfoRec * unit, int unitNo, int update);
int unitDbReloadPir(unitInfoRec * unit);
void unitDbCpy(unitInfoRec * dest, unitInfoRec * unit);
int unitDbCmp(unitInfoRec * dest, unitInfoRec * unit);

int unitDbSetPirStatus(unitInfoRec * unit);
int unitDbGetPirStatus(unitInfoRec * unit);

int unitDbUpdateLocation(unitInfoRec * unit);
int unitDbPirMoment(unitInfoRec * unit);
int unitDbFindNwk(unitInfoRec * unit, char * nwkAdr);

void updateDbExit(PGconn *conn);
int unitDbGetPirInfo(unitInfoRec * unit);

char * unitDbGetLocStr(int loc);

int unitDbSetLocation(unitInfoRec * unit);
int unitDbGetLocation(int loc, char * descrptn);

void unitDbShowInfo(unitInfoRec * unit);

int unitDbPirEvent(unitInfoRec * unit, int idx);
int unitDbUpdateAddr(unitInfoRec * unit, int idx);
void dBUnitGetLocation(unitInfoRec * unit);
int unitDbSetPirStatus(unitInfoRec * unit);

void dbUnitDisplayPirInfo(unitInfoRec * unit);
int unitDbUpdateHwInfo(unitInfoRec * unit, int nwaddr, char * ieeeaddr, char * udescrptn);
int unitDbInsertHwInfo(unitInfoRec * unit, int nwaddr, char * ieeeaddr, char * udescrptn);

#endif
