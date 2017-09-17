#ifndef ZBSEND_H
#define ZBSEND_H

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbSend.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
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

#include "zbSocket.h"

int zbSendInit(char * sHostName, int sHostPort, int rdTimeout);

int zbSendCheck(char * pktStr, char * txBuffer);

int zbSendGetReply(int iZigbeeSocket, char * rxData, int rdTimeout);

int zbSendString(int iZigbeeSocket, char * sData);

int zbPollSendReq(int iZigbeeSocket, char * cBuffer, char * descrptn);

#endif
