/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbSend.c,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.5  2011/02/15 15:03:37  johan
 * Remove command parameter in message read
 *
 * Revision 1.4  2011/02/15 13:58:01  johan
 * Check for error after sending
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <popt.h>
#include <libgen.h>
#include <unistd.h>

#include "zbNumber.h"
#include "zbData.h"
#include "zbSend.h"
#include "zbSocket.h"
#include "zbDisplay.h"

extern int verbose;
extern int trace;

/*! \brief	Initialize socket
	\param	sHostName	host name
	\param	sHostPort	port number
	\param	rdTimeout	timeout period in seconds
	\return	socket number opened
 */
int zbSendInit(char * sHostName,
	       int sHostPort,
	       int rdTimeout)
{
	int iZigbeeSocket = -1;
	iZigbeeSocket = zbSocketOpen(sHostName, sHostPort);
	if (iZigbeeSocket < 0)
	{
		DateTimeStampFile(stderr);
		if (trace) fprintf(stderr, "zigbee Connection CONNECT-->ERROR\n");
		return -1;
	}
	/* Make sure there are no buffered messages sitting there */
	zbSocketClear(iZigbeeSocket, rdTimeout);

	if (trace) printf("\tzigbee Connection CONNECT-->UP\n");
	return iZigbeeSocket;
}

/*! \brief	Prepare send string
	\param	pktStr		input packet string
	\param	txBuffer	packet string for transmission
	\return zero on success
 */
int zbSendCheck(char * pktStr,
		char * txBuffer)
{
	int i, rc, fcs = -1, dlen, blen;
	char zbStr[512];
	
	/* Remove character '-' used for readability */
	rc = strlen(pktStr);
	blen=0;
	for (i=0; i<rc; i++)
	{
		if (pktStr[i] != '-') txBuffer[blen++] = pktStr[i];
	}
	txBuffer[blen] = 0;
	
	if (strncmp(txBuffer, "02", 2))
	{
		printf("Start of packet is not '02'\n");
		return -1;
	}

	/* Get length of data */
	zbStr[0] = txBuffer[6];
	zbStr[1] = txBuffer[7];
	zbStr[2] = 0;
	dlen = strtol(zbStr, NULL, 16);
	rc = dlen * 2;

	/* Check length of data */
	if (blen == rc + 8)
	{
		fcs = -1;
	}
	else if (blen != rc + 10)
	{
		printf("Data length is %d bytes, should be %d\n", blen, rc + 10);
		return -1;
	}
	else
	{
		/* Get checksum */
		zbStr[0] = txBuffer[rc+8];
		zbStr[1] = txBuffer[rc+9];
		zbStr[2] = 0;
		fcs = strtol(zbStr, NULL, 16);
		if (trace) printf("\tChecksum is %02x\n", fcs);
	}
	
	/* Get data */
	strncpy(zbStr, &txBuffer[8], rc);
	zbStr[rc] = 0;
	
	/* Calculate checksum */
	strncpy(zbStr, &txBuffer[2], rc+6);
	zbStr[rc+6] = 0;
	rc = zbNumberComputeFCS(zbStr);
	if (fcs < 0) 
	{
		fcs = rc;
		sprintf(zbStr, "%02X", fcs);
		strcat(txBuffer, zbStr);
		if (trace) printf("\tAdd missing checksum : string now %s\n", txBuffer);
	}
	
	if (rc != fcs)
	{
		printf("Checksum for '%s' is %02x, not %02x\n", zbStr, rc, fcs);
		return -1;
	}

	return 0;
}

/*! \brief Wait for response
	\param	iZigbeeSocket	socket already open
	\param	rxData		data buffer
	\param	rdTimeout	timeout period in seconds
	\return	number of bytes read
 */
int zbSendGetReply(int iZigbeeSocket,
		   char * rxData, 
		   int rdTimeout)
{
	int resp, count;

	if (trace) printf("\tGet Reply (timeout %d seconds)\n", rdTimeout);

	count = 10;
	while (count)
	{
		rxData[0] = 0;
		resp = zbSocketReadPacket(iZigbeeSocket, rxData);
		if (resp == 0);
		else if (resp == -1)
		{
			DateTimeStampFile(stderr);
			if (trace) fprintf(stderr, "zigbeeConnection stop alarm-->ERROR\n");
			return -1;
		}
		else
		{
			if (trace) dumpRXdata("\tPoll reply", rxData, resp);
			break;
		}
		sleep(1);
		--count;
	}

	return resp;
}

/*! \brief Send zigbee message
	\param	iZigbeeSocket	socket already open
	\param  sData		data buffer
	\return true - Error, false - OK
 */
int zbSendString(int iZigbeeSocket,
			char * sData)
{
	int iLength =0 ;
	int retval;
	
	/* Length of data in Asci HEX Bytes ie. 2 Chars per byte */
	iLength = strlen(sData);
	
	if (verbose)
	{
		dispBuffer(sData, iLength, "\n");
	}
	
	retval = zbSocketWrite(iZigbeeSocket, sData, iLength);
	if (retval < 0)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Send command '%s' FAIL: %i\n", sData, iLength);
		return -1;
	}
	if (trace) printf("\tSend command '%s' OK: length %d\n", sData, retval);

	return retval;
}

/*! \brief Send Zigbee command to get arm mode
	\param iZigbeeSocket	socket already opened
	\return 0 when successful, else -1
 */
int zbPollSendReq(int iZigbeeSocket, 
			 char * cBuffer, 
			 char * descrptn)
{
	int iLength = 0;
	int retval;

	if (trace) printf("\tZigbee request (%s): %s\n", descrptn, cBuffer);
	iLength = strlen(cBuffer);
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(cBuffer, iLength, "<tx>\n");
	}
	retval = zbSocketWrite(iZigbeeSocket, cBuffer, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollSendReq FAIL LEN:%i\n", iLength);
		return -1;
	}

	return 0;
}

