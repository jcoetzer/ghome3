
/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: deviceCheck.c,v $
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "unitLqi.h"
#include "zbSend.h"
#include "zbData.h"
#include "zbDisplay.h"
#include "deviceCheck.h"

extern int trace;
extern int verbose;

int deviceCheckLqi(int skt, 
		   struct unitLqiStruct * lqiRec,
		   int lqintvl)
{
	int i, nwk;
	time_t curt, intvl;

	lqintvl /= 2;
	curt = time(NULL);
	for (i=0; i<MAX_DEVICES; i++)
	{
		nwk = lqiRec[i].srcAddr;
		if (nwk < 0) break;
		intvl = curt-lqiRec[i].lqiReq;
		/*?
		if (nwk < 0) break;
		{
			if (trace) printf("\tNo LQI for %04x\n", nwk);
			lqiRec[i].lqiReq = curt;
			sendLqi(skt, nwk);
			
		}
		else 
		?*/  
		if (lqiRec[i].destAddr[0] < 0 && intvl > lqintvl)
		{
			if (trace) printf("\tGet LQI for %04x\n", nwk);
			lqiRec[i].lqiReq = curt;
			sendLqi(skt, nwk);
			break;
		}
		else if (lqiRec[i].logcltyp==2)
		{
			if (trace) printf("\tDevice %04x is an end device - no need for LQI request\n", nwk);
		}
		else if (intvl > lqintvl)
		{
			if (trace) printf("\tUpdate LQI for %04x after %d seconds\n", nwk, (int) intvl);
			lqiRec[i].lqiReq = curt;
			sendLqi(skt, nwk);
			break;
		}
		else 
		{
			if (trace) printf("\tLQI last request for %04x was %d seconds ago\n", nwk, (int) intvl);
		}
	}
	return 0;
}

int deviceCheckIeee(int skt, 
		     struct unitLqiStruct * lqiRec, 
		     char * txBuffer, 
		     char * rxBuffer,
		   int lqintvl)
{
	int i, nwk;
	time_t curt, intvl;

	curt = time(NULL);
	for (i=0; i<MAX_DEVICES; i++)
	{
		nwk = lqiRec[i].srcAddr;
		if (nwk < 0) break;
		intvl = curt-lqiRec[i].ieeeReq;
		if (lqiRec[i].ieeeAddr[0]==0 && intvl > lqintvl)
		{
			if (trace) printf("\tGet IEEE address for %04x\n", nwk);
			lqiRec[i].ieeeReq = curt;
			deviceGetIeee(skt, nwk, txBuffer, rxBuffer, 0);
			break;
		}
		else if (intvl > lqintvl)
		{
			if (trace) printf("\tUpdate IEEE address for %04x after %d seconds\n", nwk, (int) intvl);
			lqiRec[i].ieeeReq = curt;
			deviceGetIeee(skt, nwk, txBuffer, rxBuffer, 0);
			break;
		}
		else 
		{
			if (trace) printf("\tIEEE last request for %04x was %d seconds ago: result '%s'\n", nwk, (int) intvl, lqiRec[i].ieeeAddr);
		}
	}
	return 0;
}

int deviceCheckLogicalType(int skt, 
			   struct unitLqiStruct * lqiRec, 
			   char * txBuffer, 
			   char * rxBuffer,
			   int lqintvl)
{
	int i, nwk;
	time_t curt, intvl;

	curt = time(NULL);
	for (i=0; i<MAX_DEVICES; i++)
	{
		nwk = lqiRec[i].srcAddr;
		if (nwk < 0) break;
		intvl = curt-lqiRec[i].nodeReq;
		if ((lqiRec[i].logcltyp <= 0) || (intvl > lqintvl))
		{
			if (trace) printf("\tGet logical type for %04x after %d seconds\n", nwk, (int) intvl);
			lqiRec[i].nodeReq = curt;
			deviceGetNode(skt, nwk, txBuffer, rxBuffer);
			break;
		}
		else if ((intvl > lqintvl))
		{
			if (trace) printf("\tUpdate logical type for %04x after %d seconds\n", nwk, (int) intvl);
			lqiRec[i].nodeReq = curt;
			deviceGetNode(skt, nwk, txBuffer, rxBuffer);
			break;
		}
		else 
		{
			if (trace) printf("\tNode last request for %04x was %d seconds ago: result %d\n", nwk, (int) intvl, lqiRec[i].logcltyp);
		}
	}
	return 0;
}

int deviceCheckEndPoints(int skt, 
			 struct unitLqiStruct * lqiRec, 
			 char * txBuffer, 
			 char * rxBuffer,
			 int lqintvl)
{
	int i, nwk;
	time_t curt, intvl;

	curt = time(NULL);
	for (i=0; i<MAX_DEVICES; i++)
	{
		nwk = lqiRec[i].srcAddr;
		if (nwk < 0) break;
		intvl = curt-lqiRec[i].nodeReq;
		if ((lqiRec[i].endPoint[0]<=0) && (intvl > lqintvl))
		{
			if (trace) printf("\tGet endpoints for %04x after %d seconds\n", nwk, (int) intvl);
			lqiRec[i].nodeReq = curt;
			deviceGetEndPoints(skt, nwk, txBuffer, rxBuffer);
			break;
		}
		else if ((intvl > lqintvl))
		{
			if (trace) printf("\tUpdate endpoints for %04x after %d seconds\n", nwk, (int) intvl);
			lqiRec[i].nodeReq = curt;
			deviceGetEndPoints(skt, nwk, txBuffer, rxBuffer);
			break;
		}
		else 
		{
			if (trace) printf("\tEndpoint last request for %04x was %d seconds ago: result %d\n", nwk, (int) intvl, lqiRec[i].endPoint[0]);
		}
	}
	return 0;
}

int deviceCheckAttributes(int skt, 
			  struct unitLqiStruct * lqiRec, 
			  char * txBuffer, 
			  char * rxBuffer,
			  int lqintvl)
{
	int i, nwk;
	time_t curt, intvl;

	curt = time(NULL);
	for (i=0; i<MAX_DEVICES; i++)
	{
		nwk = lqiRec[i].srcAddr;
		if (nwk < 0) break;
		intvl = curt-lqiRec[i].attrReq;
		if ((lqiRec[i].attributes[0][0]==0) && (intvl > lqintvl))
		{
			if (trace) printf("\tGet attributes for %04x after %d seconds\n", nwk, (int) intvl);
			lqiRec[i].nodeReq = curt;
			deviceGetAttributes(skt, nwk,lqiRec[i].endPoint[0], 0, txBuffer, rxBuffer);
			break;
		}
		else if ((intvl > lqintvl))
		{
			if (trace) printf("\tUpdate attributes for %04x after %d seconds\n", nwk, (int) intvl);
			lqiRec[i].nodeReq = curt;
			deviceGetAttributes(skt, nwk,lqiRec[i].endPoint[0], 0, txBuffer, rxBuffer);
			break;
		}
		else 
		{
			if (trace) printf("\tAttribute last request for %04x was %d seconds ago: result '%s'\n", nwk, (int) intvl, lqiRec[i].attributes[0]);
		}
	}
	return 0;
}

/*! \brief Get active endpoints
	\param	skt		Socket ID
	\param	idx		Device index number
	\param	txBuffer	Transmit buffer
	\param	rxBuffer	Receive buffer
	\param	sysDev		Device info
	\param	rdTimeout	Timeout in seconds
 */
int deviceDiscovery(int skt, 
		    int shortAddress, 
		    char * txBuffer, 
		    char * rxBuffer)
{
	int rc;

	if (trace) printf("\n\n");
	sprintf(rxBuffer, "020A0E0902%04X07FFF8001000", shortAddress);
	rc = zbSendCheck(rxBuffer, txBuffer);
	if (rc)
	{
		fprintf(stderr, "Packet check failed\n");
		return -1;
	}

	if (trace) printf("\tDevice discovery: %s\n", txBuffer);
	zbSendString(skt, txBuffer);

	rc = zbSendGetReply(skt, rxBuffer, 0);
	if (rc < 0) return -1;
	
	return 0;
}

/*! \brief Get simple descriptor request
	\param	skt		Socket ID
	\param	idx		Device index number
	\param	txBuffer	Transmit buffer
	\param	rxBuffer	Receive buffer
	\param	sysDev		Device info
	\param	rdTimeout	Timeout in seconds
 */
int deviceGetDescriptor(int skt, 
			int shortAddress, 
			int endp, 
			char * txBuffer, 
			char * rxBuffer)
{
	int rc;

	if (trace) printf("\n\n");
	sprintf(rxBuffer, "020A060702%04X%04X%02X00", shortAddress, shortAddress, endp);
	rc = zbSendCheck(rxBuffer, txBuffer);
	if (rc)
	{
		fprintf(stderr, "Packet check failed\n");
		return -1;
	}

	if (trace) printf("\tDevice discovery: %s\n", txBuffer);
	zbSendString(skt, txBuffer);

	rc = zbSendGetReply(skt, rxBuffer, 0);
	if (rc < 0) return -1;
	
	return 0;
}

/*! \brief Get active endpoints
	\param	skt		Socket ID
	\param	idx		Device index number
	\param	txBuffer	Transmit buffer
	\param	rxBuffer	Receive buffer
	\param	sysDev		Device info
	\param	rdTimeout	Timeout in seconds
 */
int deviceGetEndPoints(int skt, 
		       int shortAddress, 
		       char * txBuffer, 
		       char * rxBuffer)
{
	int rc;

	if (trace) printf("\n\n");
	sprintf(rxBuffer, "020A070602%04X%04X00", shortAddress, shortAddress);
	rc = zbSendCheck(rxBuffer, txBuffer);
	if (rc)
	{
		fprintf(stderr, "Packet check failed\n");
		return -1;
	}

	if (trace) printf("\tGet EndPoints: %s\n", txBuffer);
	zbSendString(skt, txBuffer);

	rc = zbSendGetReply(skt, rxBuffer, 0);
	if (rc < 0) return -1;
	
	return 0;
}

/*! \brief Obtain IEEE address
	\param	skt		Socket ID
	\param	idx		Device index number
	\param	txBuffer	Transmit buffer
	\param	rxBuffer	Receive buffer
	\param	sysDev		Device info
	\param	rdTimeout	Timeout in seconds
 */
int deviceGetIeee(int skt, 
		   int nwk,
		   char * txBuffer, 
		   char * rxBuffer, 
		   int rdTimeout)
{
	int rc;

	sprintf(rxBuffer, "020A030602%04X000000", nwk);
	rc = zbSendCheck(rxBuffer, txBuffer);
	if (rc)
	{
		fprintf(stderr, "Packet check failed\n");
		return -1;
	}

	if (trace) printf("\tGet IEEE Address: %s\n", txBuffer);
	rc = zbSendString(skt, txBuffer);
	if (rc < 0) return -1;
	
	return 0;
}

int deviceGetNode(int skt, 
		  int nwk, 
		  char * txBuffer, 
		  char * rxBuffer)
{
	int rc;

	sprintf(rxBuffer, "020a040702%04X%04X0000", nwk, nwk);

	rc = zbSendCheck(rxBuffer, txBuffer);
	if (rc)
	{
		fprintf(stderr, "Packet check failed\n");
		return -1;
	}

	if (trace) printf("\tGet Node descriptor: %s\n", txBuffer);
	rc = zbSendString(skt, txBuffer);
	if (rc < 0) return -1;
	
	return 0;
}

/*! \brief Read Attributes
	\param	skt		Socket ID
	\param	idx		Device index number
	\param	txBuffer	Transmit buffer
	\param	rxBuffer	Receive buffer
	\param	sysDev		Device info
	\param	rdTimeout	Timeout in seconds
 */
int deviceGetAttributes(int skt, 
			int nwk,
			int endp, 
			int idx,
			char * txBuffer, 
			char * rxBuffer)
{
	int rc;

	if (endp <= 0)
	{
		if (trace) printf("\tNo endpoint for device %04x\n", nwk);
		endp = 1;
	}

	if (trace) printf("\tGet attribute %d for device %04x endpoint %02x\n", idx, nwk, endp);

	/* Attributes 7, 5 and 6 */
	sprintf(rxBuffer, "020D000D02%04X%02X000003000700050006", nwk, endp);
	rc = zbSendCheck(rxBuffer, txBuffer);
	if (rc)
	{
		fprintf(stderr, "Packet check failed\n");
		return -1;
	}

	rc = zbSendString(skt, txBuffer);
	if (rc < 0) return -1;

	return 0;
}
