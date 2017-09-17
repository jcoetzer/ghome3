/*! \file zbDisplay.h
	\brief Display Zigbee data packets
*/

#ifndef ZBDISPLAY_H
#define ZBDISPLAY_H

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbDisplay.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
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

#include "updateDb.h"

void DateTimeStamp(void);

void dumpTXdata(char * sDescr, char * sData, int iLen);
void dumpRXdata(char * sDescr, char * sData, int iLen);

void dispCommand(char * cBuffer);

void readIeeeAddr(char * buf, char * ieeeBuf);

void dispBuffer(char * cBuffer, int cLen, char * suffix);

void dispClusterId(char * buf);

int Status(char * status);
void dispIeeeAddr(char * buf, int reverse);
int readAlarmDeviceNotify(char * cBuffer, unitInfoRec * unitRec);

#endif
