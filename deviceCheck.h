#ifndef DEVICECHECK_H
#define DEVICECHECK_H

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: deviceCheck.h,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.1.1.1  2011/05/28 09:00:03  johan
 * gHome logging server
 *
 * Revision 1.3  2011/02/10 11:09:40  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:13  johan
 * Add CVS stuff
 *
 *
 *
 */
#include <libpq-fe.h>

int deviceCheckLqi(int skt, struct unitLqiStruct * lqiRec, int lqintvl);
int deviceCheckIeee(int skt, struct unitLqiStruct * lqiRec, char * txBuffer, char * rxBuffer, int lqintvl);
int deviceCheckLogicalType(int skt, struct unitLqiStruct * lqiRec, char * txBuffer, char * rxBuffer, int lqintvl);
int deviceCheckEndPoints(int skt, struct unitLqiStruct * lqiRec, char * txBuffer, char * rxBuffer, int lqintvl);
int deviceCheckAttributes(int skt, struct unitLqiStruct * lqiRec, char * txBuffer, char * rxBuffer, int lqintvl);

int unitLqiUpdate(PGconn * DbConn, int unitNo, char * lqiPan, struct unitLqiStruct * lqiRec, char * zbBuff, int version);
int unitLqiUpdateIeee(char * lqiPan, struct unitLqiStruct * lqiRec, char * zbBuff, int version);
int unitLqiUpdateNode(char * lqiPan, struct unitLqiStruct * lqiRec, char * zbBuff, int version);

int deviceDiscovery(int skt, int shortAddress, char * txBuffer, char * rxBuffer);
int deviceGetDescriptor(int skt, int shortAddress, int endp, char * txBuffer, char * rxBuffer);

int deviceGetIeee(int skt, int nwk, char * txBuffer, char * rxBuffer, int rdTimeout);
int deviceGetNode(int skt, int nwk, char * txBuffer, char * rxBuffer);
int deviceGetEndPoints(int skt, int shortAddress, char * txBuffer, char * rxBuffer);
int deviceGetAttributes(int skt, int nwk, int endp, int idx, char * txBuffer, char * rxBuffer);

#endif
