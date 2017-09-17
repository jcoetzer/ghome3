/*! \file zbPoll.h
 	@brief ZigBee command
	@author Intelipower

Basic ZigBee implimentation

*/
#ifndef ZBPOLL_H
#define ZBPOLL_H

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbPoll.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.8  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.7  2011/02/16 21:39:38  johan
 * Read user description from message
 *
 * Revision 1.6  2011/02/15 13:05:06  johan
 * Add network discovery command
 *
 * Revision 1.5  2011/02/15 10:27:19  johan
 * Add zone un-enroll message
 *
 * Revision 1.4  2011/02/13 19:49:09  johan
 * Turn temperature poll
 *
 * Revision 1.3  2011/02/10 11:09:43  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:17  johan
 * Add CVS stuff
 *
 *
 *
 */

#include <time.h>

#include "zbSocket.h"
#include "updateDb.h"
#include "gHomeConf.h"

void zbPollRun(unitInfoRec * unit, int rdTimeout, char * zbCmd, gHomeConfRec * cfg);
void zbPollRunPollD(int AlarmDelay, unitInfoRec * unit, int SleepTime, int ConnectionTimer, int rdTimeout);

int zbPollTemperature(int iZigbeeSocket, unitInfoRec * unit, char * cBuffer, int usec);

int checkBuffer(int skt, void * unit, char * cBuffer, int cLen, int usec);

int zbPollZoneUnEnroll(int iZigbeeSocket, unitInfoRec * unit, int dstAddr, int dstEndPoint, int zoneId, char * cBuffer, int usec);
int zbPollManageDiscovery(int iZigbeeSocket, unitInfoRec * unit, int dstAddr, char * cBuffer, int usec);

#endif
