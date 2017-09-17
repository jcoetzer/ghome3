#ifndef UNITLQI_H
#define UNITLQI_H

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: unitLqi.h,v $
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

#include <time.h>

#include <libpq-fe.h>

#define LQI_POLL 30

#define LQI_INTVL 300
#define LQI_QUICK_INTVL 90

#define MAX_DEVICES 32
#define MAX_ENDPOINTS 32

struct unitLqiStruct
{
	int unitNo;
	int srcAddr;
	char ieeeAddr[32];
	int logcltyp;
	time_t lqiReq;
	time_t lqiResp;
	time_t ieeeReq;
	time_t endpReq;
	time_t nodeReq;
	time_t attrReq;
	time_t ieeeResp;
	time_t nodeResp;
	time_t endpResp;
	time_t attrResp;
	int destAddr[MAX_DEVICES];
	int rxQuality[MAX_DEVICES];
	int endPoint[MAX_ENDPOINTS];
	char attributes[MAX_ENDPOINTS][128];
};

/*? typedef struct unitLqiStruct unitLqiRec; ?*/

void unitLqiInit(struct unitLqiStruct * lqiRec, int unitNo, int update);

void unitLqiItem(PGconn * DbConn, int unitNo, char * lqiPan, int srcAddr, struct unitLqiStruct * lqiRec, char * lqiBuff, int version);

int sendLqi(int iZigbeeSocket, int shortAddress);
void unitLqiAdd(struct unitLqiStruct * lqiRec, int srcAddr, int nwk, int rxQual, int txQual);
void unitLqiDisp(struct unitLqiStruct * lqiRec);

int unitLqiCheckXml(char * OpfDir, int unitnr, int lqintvl);
int unitLqiDispXml(struct unitLqiStruct * lqiRec, char * OpfDir, int unitnr);

int unitLqiEndPoints(struct unitLqiStruct * lqiRec, char * rxBuff);
int unitLqiAttributes(struct unitLqiStruct * lqiRec, char * rxBuffer);

int unitLqiComplete(struct unitLqiStruct * lqiRec, int endpoint);

#endif
