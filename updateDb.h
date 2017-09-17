/*! \file updateDb.h
	\brief	Utilities for database interaction
 */
#ifndef UPDATEDB_H
#define UPDATEDB_H

/*
 * $Author: johan $
 * $Date: 2011/06/15 22:59:04 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: updateDb.h,v $
 * Revision 1.2  2011/06/15 22:59:04  johan
 * Send message when alarm goes off
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.5  2011/02/24 16:20:55  johan
 * Store user descriptor in database
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

#include "unitDb.h"

void updateDbError(PGconn * conn);
void dispUnitStatus(int sts);

int updateDbLoggerCheck(unitInfoRec * unit);
int updateDbStatusInfo(unitInfoRec * unit, char * updTime);
int updateDbLogEvent(unitInfoRec * unit, char * descr, int eventtype, int loc);
int updateDbLogStatus(unitInfoRec * unit, int eventtype, int loc);
int updateDbStatus(int eventtype, unitInfoRec * unit);
int updateDbTemperature(unitInfoRec * unit, int idx, float temp);
int updateDbSetPirHardware(unitInfoRec * unit);
int updateDbHeartbeat(unitInfoRec * unit, int idx);
int updateDbGateway(unitInfoRec * unit);
int updateDbCoordinatorIeee(unitInfoRec * unit);
int updateDbCoordCheck(unitInfoRec * unit);
int updateDbResetRearm(unitInfoRec * unit);
int updateDbPid(unitInfoRec * unit);
int updateDbPirError(unitInfoRec * unit);
int updateDbAlarmMsg(unitInfoRec * unit, char * descr, int eventtype, int loc);

int updateDbResult(PGresult * result);

#endif
