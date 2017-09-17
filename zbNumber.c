/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbNumber.c,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.5  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.4  2011/02/15 15:03:36  johan
 * Remove command parameter in message read
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "updateDb.h"
#include "zbNumber.h"

extern int trace;

/*! \brief Parse character as hex digit.
	\param	hexadecimal character
	\return integer value, -1 on error. 
 */
int zbNumberHex2dec(char hex)
{
	if (hex >= '0' && hex <= '9')
		return hex - '0';
	if (hex >= 'A' && hex <= 'F')
		return 10 + hex - 'A';
	if (hex >= 'a' && hex <= 'f')
		return 10 + hex - 'a';
	return -1;
}

/*! \brief Check buffer
	\param	skt		open socket
	\param	unit		unit info
	\param	cBuffer		data buffer
	\param	cLen		buffer size
	\param	unitName	unit name
	\param	unitIp		IP address
	\param	unitNum		unit number
 */
int readBuffer(int skt, 
	       void * unit,
	       char * rxBuffer,
	       int bufLen,
	       int usec)
{
	static char datastr1[512];
	char lenstr[8];
	int rc, pkt = 1, dlen, pktlen, rem;
	unitInfoRec * unitRec = (unitInfoRec *) unit;		/*!< For debugging only? */
	
	if (! bufLen) return 0;
	if (bufLen < 0)
	{
		return -1;
	}
	
	if (trace > 1) printf("\tRX <%s> %d bytes\n", rxBuffer, bufLen);

	strcpy(lenstr, "0X");
	strncat(lenstr, &rxBuffer[6], 2);
	lenstr[4] = 0;
	dlen = (int) strtol(lenstr, NULL, 16);

	pktlen = 11 + dlen*2;

	rem = bufLen - pktlen;

	while (rem > 0)
	{
		strncpy(datastr1, &rxBuffer[pktlen], rem);
		datastr1[rem] = 0;
		rxBuffer[pktlen] = 0;
		checkBuffer(skt, unit, rxBuffer, pktlen, usec);
		bufLen -= pktlen;

		strcpy(rxBuffer, datastr1);

		strcpy(lenstr, "0X");
		strncat(lenstr, &rxBuffer[6], 2);
		lenstr[4] = 0;
		dlen = (int) strtol(lenstr, NULL, 16);
		pktlen = 11 + dlen*2;

		rem = bufLen-pktlen;

		++pkt;
	}
	rc = checkBuffer(skt, unitRec, rxBuffer, bufLen, usec);
	return rc;
}

/*! \brief Compute XOR checksum, at ASCI byte level.
	\param sData  data buffer, stops at Null or Z character
	\return code FCS

              Packet:   SOP 0C 00 09 02 3C B9 0C 05 02 01 05 00 FCS
                        Only the CMD onwards is computed for FCS 
*/
int zbNumberComputeFCS(char * sData)
{
	int iPtr = 0;
	unsigned char cByte = 0;
	unsigned char cByteL = 0;
	unsigned char cByteH = 0;
	unsigned char bFCS = 0; 

	while(sData[iPtr] != (char)0 && sData[iPtr] != (char)0x5A) 
	{
		cByteL = zbNumberHex2dec(sData[iPtr+1]);
		cByteH = zbNumberHex2dec(sData[iPtr]);
			
		cByte = cByteL + (unsigned char)(cByteH*0x10);
			
		iPtr+=2;
		
		bFCS ^= cByte;
		
		if (iPtr > 500)
		{
			printf("ERROR zigbeeComputeFCS Loop limit Data:%s\n",sData);
			break;
		}
	}
	return bFCS;
}

