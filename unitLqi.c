/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: unitLqi.c,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.4  2011/02/10 11:09:42  johan
 * Take out CVS Id tag
 *
 * Revision 1.3  2011/02/09 19:56:18  johan
 * Clean up
 *
 * Revision 1.2  2011/02/03 08:38:15  johan
 * Add CVS stuff
 *
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "unitLqi.h"
#include "lqiDb.h"
#include "zbSend.h"
#include "zbData.h"
#include "zbDisplay.h"

extern int trace;
extern int verbose;

static int unitLqiCheckSrc(struct unitLqiStruct * lqiRec, int srcAddr);
static int unitLqiAddSrc(struct unitLqiStruct * lqiRec, int srcAddr);

void unitLqiInit(struct unitLqiStruct * lqiRec, 
		 int unitNo,
		 int update)
{
	int i;
	time_t curt;
	
	curt = time(NULL);
	
	lqiRec->unitNo = unitNo;

	lqiRec->srcAddr = -1;
	
	if (! update)
	{
		/* Subtract random interval from request time to spread out updates */
		lqiRec->lqiReq = curt - ((rand()%LQI_INTVL));
		lqiRec->ieeeReq = curt - ((rand()%LQI_INTVL));
		lqiRec->nodeReq = curt - ((rand()%LQI_INTVL));
		lqiRec->endpReq = curt - ((rand()%LQI_INTVL));
		lqiRec->attrReq = curt - ((rand()%LQI_INTVL));
	}
	else
	{
		lqiRec->lqiReq = 0;
		lqiRec->ieeeReq = 0;
		lqiRec->nodeReq = 0;
		lqiRec->endpReq = 0;
		lqiRec->attrReq = 0;
	}

	lqiRec->lqiResp = 0;
	lqiRec->ieeeResp = 0;
	lqiRec->nodeResp = 0;
	lqiRec->endpResp = 0;
	lqiRec->attrResp = 0;

	lqiRec->ieeeAddr[0] = 0;

	lqiRec->logcltyp = -1;

	for (i=0; i<MAX_DEVICES; i++)
	{
		lqiRec->destAddr[i] = -1;
		lqiRec->rxQuality[i] = -1;
	}
	for (i=0; i<MAX_ENDPOINTS; i++) lqiRec->endPoint[i] = 0;
	for (i=0; i<MAX_ENDPOINTS; i++) lqiRec->attributes[i][0] = 0;
}

int unitLqiUpdate(PGconn * DbConn,
		  int unitNo,
		  char * lqiPan,
		  struct unitLqiStruct * lqiRec, 
		  char * zbBuff, 
		  int version)
{
	char * lqiBuff;
	char countStr[8];
	int i, count, lqilen, srcAddr;
	int st;
	
	if (trace) printf("> LQI response: ");
	
	lqiBuff = zbBuff+14;

	/* Source address */
	strncpy(countStr, &zbBuff[8], 4);
	countStr[4] = 0;
	srcAddr = strtol(countStr, NULL, 16);
	if (trace) printf("SrcAddr %04x, ", srcAddr);
	
	/* Status */
	if (isxdigit(zbBuff[12]) && isxdigit(zbBuff[13]))
	{
		strncpy(countStr, &zbBuff[12], 2);
		countStr[2] = 0;
		st = strtol(countStr, NULL, 16);
		if (trace) printf("Status %d, ", st);
		if (st) return -1;
	}
	else
	{
		if (trace) printf("Invalid status '%c%c'\n", zbBuff[12], zbBuff[13]);
		return -1;
	}
	
	switch (version)
	{
		case 0 : lqilen = 12; break;
		case 1 : lqilen = 24; break;
		default : lqilen = 24;
	}

	if (trace) printf("Entries %c%c, ", lqiBuff[0], lqiBuff[1]);
	if (trace) printf("StartIndex %c%c, ", lqiBuff[2], lqiBuff[3]);
	strncpy(countStr, &lqiBuff[4], 2);
	countStr[2] = 0;
	count = strtol(countStr, NULL, 16);
	if (trace) printf("Count %d, ", count);
	
	for (i=0; i<count; i++) 
	{
		if (trace) printf("LQI%d: ", i+1);
		unitLqiItem(DbConn, unitNo, lqiPan, srcAddr, lqiRec, &lqiBuff[6+i*lqilen], version);
	}
	if (trace) printf("\n");
	return 0;
}

int unitLqiUpdateIeee(char * lqiPan,
		      struct unitLqiStruct * lqiRec, 
		      char * zbBuff, 
		      int version)
{
	int addrMode, i, n, nwk;
	char nStr[8];
	char ieeeAd[64];
	
	if (trace) printf("> IEEE: "); 
	
	addrMode = AddrMode(&zbBuff[8]);
	switch(addrMode)
	{
		case 2 : sprintf(nStr, "%c%c%c%c", zbBuff[22], zbBuff[23], zbBuff[24], zbBuff[25]); break;
		case 3 : sprintf(nStr, "%c%c%c%c", zbBuff[10], zbBuff[11], zbBuff[12], zbBuff[13]); break;
		default : printf("SrcAddr Unknown address mode, "); return -1;
	}
	nwk = strtol(nStr, NULL, 16);
	
	n = Status(&zbBuff[26]);
	if (n) return -1;

	strncpy(ieeeAd, &zbBuff[28], 16);
	ieeeAd[16] = 0;
	if (trace) printf("Network address %04x, IEEE address %s\n", nwk, ieeeAd);
	n = 0;
	for (i=0; i<MAX_DEVICES; i++)
	{
		if (lqiRec[i].srcAddr == nwk)
		{
			if (trace) printf("\tUpdate LQI %d: set IEEE to %s\n", i, ieeeAd);
			strcpy(lqiRec[i].ieeeAddr, ieeeAd);
			lqiRec[i].ieeeResp = time(NULL);
			n = 1;
			break;
		}
	}
	if ((!n) && trace) printf("\tNetwork address %04x is unknown\n", nwk);
	return 0;
}

int unitLqiUpdateNode(char * lqiPan,
		      struct unitLqiStruct * lqiRec, 
		      char * zbBuff, 
		      int version)
{
	int i, n, nwk, ltyp;
	char nStr[8];

	printf("Node descriptor: ");
	n = Status(&zbBuff[8]);
	SrcAddr(&zbBuff[10], 2);
	NWKAddrOfInterest(&zbBuff[14]);
	if (n) return -1;

	if (trace) printf("> LogicalType ");
	switch(zbBuff[19])
	{
		case '0' : ltyp=0; if (trace) printf("Coordinator, "); break;
		case '1' : ltyp=1; if (trace) printf("Router, "); break;
		case '2' : ltyp=2; if (trace) printf("End Device, "); break;
		case '3' :
		case '4' :
		case '5' :
		case '6' :
		case '7' :
			ltyp = 3;
			if (trace) printf("Reserved, "); break;
		default :
			ltyp = 0xff;
			if (trace) printf("Unknown");
	}
	if (trace) printf("\n");
	
	sprintf(nStr, "%c%c%c%c", zbBuff[14], zbBuff[15], zbBuff[16], zbBuff[17]);
	nwk = strtol(nStr, NULL, 16);
	
	n = 0;
	for (i=0; i<MAX_DEVICES; i++)
	{
		if (lqiRec[i].srcAddr == nwk)
		{
			if (trace) printf("\tUpdate LQI %d: set logical type to %d\n", i, n);
			lqiRec[i].logcltyp = ltyp;
			lqiRec[i].nodeResp = time(NULL);
			n = 1;
			break;
		}
	}
	if ((!n) && trace) printf("\tNetwork address %04x is unknown\n", nwk);

	return 0;
}

void unitLqiItem(PGconn * DbConn,
		 int unitNo,
		 char * lqiPan,
		 int srcAddr,
		 struct unitLqiStruct * lqiRec, 
		 char * lqiBuff, 
		 int version)
{
	char countStr[8];
	int i, nwk, rxQual, txQual;
	
	/* TODO not sure what version is actually in use */
	switch(version)
	{
		case 0 :
			/* 2-byte address in reverse order */
			if (trace) printf("PAN ");
			for (i=2; i>=0; i-=2) { if (trace) printf("%c%c", lqiBuff[i], lqiBuff[i+1]); }
			strncpy(countStr, &lqiBuff[4], 4);
			countStr[4] = 0;
			nwk = strtol(countStr, NULL, 16);
			if (trace) printf(", Address %04x, ", nwk);
			strncpy(countStr, &lqiBuff[8], 2);
			countStr[2] = 0;
			rxQual = strtol(countStr, NULL, 16);
			if (trace) printf("RxQuality %d, ", rxQual);
			strncpy(countStr, &lqiBuff[10], 2);
			countStr[2] = 0;
			txQual = strtol(countStr, NULL, 16);
			if (trace) printf("TxQuality %d, ", txQual);
			break;
		case 1 :
			/* 8-byte address in reverse order */
			if (trace) printf("PAN ");
			for (i=0; i<16; i++) { if (trace) printf("%c", lqiBuff[i]); }
			strncpy(countStr, &lqiBuff[16], 4);
			countStr[4] = 0;
			nwk = strtol(countStr, NULL, 16);
			if (trace) printf(", Address %04x, ", nwk);
			strncpy(countStr, &lqiBuff[20], 2);
			countStr[2] = 0;
			rxQual = strtol(countStr, NULL, 16);
			if (trace) printf("RxQuality %d, ", rxQual);
			strncpy(countStr, &lqiBuff[22], 2);
			countStr[2] = 0;
			txQual = strtol(countStr, NULL, 16);
			if (trace) printf("TxQuality %d, ", txQual);
			break;
		default :
			printf("Invalid LQI version 1.%d, ", version);
			return;
	}
	if (trace) printf("\n");
	unitLqiAdd(lqiRec, srcAddr, nwk, rxQual, txQual);
	lqiDbInsertInfo(DbConn, unitNo, nwk, txQual);
}

void unitLqiAdd(struct unitLqiStruct * lqiRec, 
		int srcAddr, 
		int destAddr, 
		int rxQual, 
		int txQual)
{
	int i, j, iq, jq;
	
	if (trace) printf("\tAdd source address %04x (dest %04x, rx quality %d, tx quality %d)\n", srcAddr, destAddr, rxQual, txQual);
	iq = 0;
	for (i=0; i<MAX_DEVICES; i++)
	{
		if (lqiRec[i].srcAddr < 0) break;
		if (srcAddr == lqiRec[i].srcAddr)
		{
			if (trace) printf("\tUpdate nwk addr #%d (%04x)\n", i, srcAddr);
			iq = 1;
			jq = 0;
			for (j=0; j<MAX_DEVICES; j++)
			{
				if (lqiRec[i].destAddr[j] < 0) break;
				if (lqiRec[i].destAddr[j] == destAddr)
				{
					if (trace) printf("\tUpdate dest addr %d: %04x\n", j, destAddr);
					lqiRec[i].rxQuality[j] = rxQual;
					lqiRec[i].lqiResp = time(NULL);
					jq = 1;
					break;
				}
			}
			if (!jq)
			{
				if (trace) printf("\tAdd dest addr #%d (%04x)\n", j, destAddr);
				lqiRec[i].destAddr[j] = destAddr;
				lqiRec[i].rxQuality[j] = rxQual;
				lqiRec[i].lqiResp = time(NULL);
				if (unitLqiCheckSrc(lqiRec, destAddr))
				{
					unitLqiAddSrc(lqiRec, destAddr);
				}
			}
		}
	}
	if (!iq)
	{
		if (trace) printf("\tAdd src addr %d: %04x, dest addr %04x\n", i, srcAddr, destAddr);
		lqiRec[i].srcAddr = srcAddr;
		lqiRec[i].destAddr[0] = destAddr;
		lqiRec[i].rxQuality[0] = rxQual;
		lqiRec[i].lqiResp = time(NULL);
		lqiRec[i+1].srcAddr = destAddr;
	}
}

void unitLqiDisp(struct unitLqiStruct * lqiRec)
{
	int i, j;

	printf("> %s Device data\n", DateTimeStampStr(-1));
	for (i=0; i<MAX_DEVICES; i++)
	{
		if (lqiRec[i].srcAddr < 0) break;
		printf("> Network address %d (%04x) %16s ", i, lqiRec[i].srcAddr, lqiRec[i].ieeeAddr);
		switch(lqiRec[i].logcltyp)
		{
			case 0 : printf("Coordinator\t "); break;
			case 1 : printf("Router\t "); break;
			case 2 : printf("End Device\t "); break;
			case 3 :
			case 4 :
			case 5 :
			case 6 :
			case 7 :
				printf("Reserved\t "); break;
			default :
				printf("Unknown\t ");
		}
		printf("%s ", DateTimeStampStr((int)lqiRec[i].lqiResp));
		for (j=0; j<MAX_DEVICES; j++)
		{
			if (lqiRec[i].destAddr[j] < 0) break;
			printf("\t%04x(rxq %d)", lqiRec[i].destAddr[j], lqiRec[i].rxQuality[j]);
		}
		printf("\tEndpoints: ");
		for (j=0; j<MAX_ENDPOINTS; j++)
		{
			if (lqiRec[i].endPoint[j] <= 0) break;
			printf("\t%02x ", lqiRec[i].endPoint[j]);
		}
		printf("\n");
	}
}

int unitLqiCheckXml(char * OpfDir,
		    int unitnr,
		    int lqintvl)
{
	int rc;
	char OpfName[256];
	struct stat buf;
	time_t curt, intvl;

	curt = time(NULL);
	
	snprintf(OpfName, sizeof(OpfName), "%s/unit%d.xml", OpfDir, unitnr);
	
	rc = stat(OpfName, &buf);
	if (rc) 
	{
		if (trace) printf("\tFile %s does not exist\n", OpfName);
		return -1;
	}
	
	rc = buf.st_size;
	intvl = curt - buf.st_mtim.tv_sec;
	if (trace) printf("\tFile %s: %d bytes, modified %d seconds ago\n", OpfName, rc, (int) intvl);
	if (! rc) return -1;
	if (intvl > lqintvl) return -1;
	
	return 0;
}

int unitLqiDispXml(struct unitLqiStruct * lqiRec, 
		   char * OpfDir,
		   int unitnr)
{
	int i, j;
	FILE * opf;
	char OpfName[256];
	
	snprintf(OpfName, sizeof(OpfName), "%s/unit%d.xml", OpfDir, unitnr);
	opf = fopen(OpfName, "w");
	if (! opf)
	{
		fprintf(stderr, "%s ERROR: could not write file %s\n", DateTimeStampStr(-1), OpfName);
		return -1;
	}
	if (trace) printf("\tWrite file %s\n", OpfName);

	fprintf(opf, "<?xml version=\"1.0\"?>\n");
	fprintf(opf, "<Unit>\n");
	fprintf(opf, "<Lqi>%s</Lqi>\n", DateTimeStampStr(-1));
	fprintf(opf, "<Devices>\n");
	for (i=0; i<MAX_DEVICES; i++)
	{
		if (lqiRec[i].srcAddr < 0) break;
		fprintf(opf, "\t<Device>\n");
		fprintf(opf, "\t\t<NetworkAddress>%04x</NetworkAddress>\n", lqiRec[i].srcAddr);
		fprintf(opf, "\t\t<IeeeAddress>%16s</IeeeAddress>\n", lqiRec[i].ieeeAddr);
		fprintf(opf,"\t\t<LogicalType>");
		switch(lqiRec[i].logcltyp)
		{
			case 0 : fprintf(opf, "Coordinator"); break;
			case 1 : fprintf(opf, "Router"); break;
			case 2 : fprintf(opf, "End Device"); break;
			case 3 :
			case 4 :
			case 5 :
			case 6 :
			case 7 :
				fprintf(opf, "Reserved"); break;
			default :
				fprintf(opf, "Unknown");
		}
		fprintf(opf,"</LogicalType>\n");
		fprintf(opf, "\t\t<LqiUpdate>%s</LqiUpdate>\n", DateTimeStampStr((int)lqiRec[i].lqiResp));
		fprintf(opf, "\t\t<AssociatedDevices>\n");
		for (j=0; j<MAX_DEVICES; j++)
		{
			if (lqiRec[i].destAddr[j] < 0) break;
			fprintf(opf, "\t\t\t<AssociatedDevice>\n");
			fprintf(opf, "\t\t\t\t<NwkAddr>%04x</NwkAddr>\n", lqiRec[i].destAddr[j]);
			fprintf(opf, "\t\t\t\t<RxQuality>%d</RxQuality>\n", lqiRec[i].rxQuality[j]);
			fprintf(opf, "\t\t\t</AssociatedDevice>\n");
		}
		fprintf(opf, "\t\t</AssociatedDevices>\n");
		fprintf(opf, "\t\t<Attributes>\n");
		for (j=0; j<MAX_DEVICES; j++)
		{
			if (lqiRec[i].attributes[j][0] == 0) break;
			fprintf(opf, "\t\t\t<Attribute>%s</Attribute>\n", lqiRec[i].attributes[j]);
		}
		fprintf(opf, "\t\t</Attributes>\n");
		fprintf(opf, "\t\t<Endpoints>\n");
		for (j=0; j<MAX_ENDPOINTS; j++)
		{
			if (lqiRec[i].endPoint[j] <= 0) break;
			fprintf(opf, "\t\t\t<Endpoint>%02x</Endpoint>\n", lqiRec[i].endPoint[j]);
		}
		fprintf(opf, "\t\t</Endpoints>\n");
		fprintf(opf, "\t</Device>\n");
	}
	fprintf(opf, "</Devices>\n");
	fprintf(opf, "</Unit>\n");
	
	fclose(opf);
	
	return 0;
}

int unitLqiComplete(struct unitLqiStruct * lqiRec,
		    int endpoint)
{
	int i;
	
	if (lqiRec[0].srcAddr < 0) return -1;

	for (i=0; i<MAX_DEVICES; i++)
	{
		if (lqiRec[i].srcAddr < 0) break;
		if (trace) printf("\tCheck data completeness for %04x\n", lqiRec[i].srcAddr);
		if (0==lqiRec[i].ieeeAddr[0]) return -1;
		if (lqiRec[i].logcltyp < 0) return -1;
		if (lqiRec[i].logcltyp!=2)
		{
			if (lqiRec[i].lqiResp==0) return -1;
			if (lqiRec[i].destAddr[0] < 0) return -1;
		}
		if (endpoint && lqiRec[i].endPoint[0] <= 0) return -1;
	}
	if (trace) printf("\tLink quality indicators complete\n");
	return 0;
}

static int unitLqiCheckSrc(struct unitLqiStruct * lqiRec, 
			   int srcAddr)
{
	int i;

	for (i=0; i<MAX_DEVICES; i++)
	{
		if (lqiRec[i].srcAddr == srcAddr) return 0;
	}
	return -1;
}

static int unitLqiAddSrc(struct unitLqiStruct * lqiRec, 
			 int srcAddr)
{
	int i;

	for (i=0; i<MAX_DEVICES; i++)
	{
		if (lqiRec[i].srcAddr < 0) break;
	}
	if (i==MAX_DEVICES)
	{
		printf("%s ERROR: maximum number of devices (%d) reached\n", DateTimeStampStr(-1), i);
		return -1;
	}
	if (trace) printf("\tAdd source address %d: %04x\n", i, srcAddr);
	lqiRec[i].srcAddr = srcAddr;
	return 0;
}

/*! \brief Update for gateway IEEE
	\param	unit	Record with addresses
	\param	rxBuff	Data received from Zigbee
*/
int unitLqiGateway(unitInfoRec * unit, 
		   char * rxBuff)
{
	char shortAd[8];
	char ieeeAd[64];
	int sAdr;

	strncpy(shortAd, &rxBuff[26], 4);
	shortAd[4] = 0;
	sAdr = (int) strtol(shortAd, NULL, 16);
	
	strncpy(ieeeAd, &rxBuff[10], 16);
	ieeeAd[16] = 0;

	if (trace) printf("\tIEEE for gateway %04x is %s\n", sAdr, ieeeAd);

	strcpy(unit->gwieee, ieeeAd);
	unit->gwaddr = sAdr;
	
	return 0;
}

int unitLqiEndPoints(struct unitLqiStruct * lqiRec,
		   char * rxBuff)
{
	char nStr[8];
	int i, j, iq, n, nwk;

	printf("Get EndPoints response: %s\n", rxBuff);
	strncpy(nStr, &rxBuff[18], 2);
	nStr[2] = 0;
	n = strtol(nStr, NULL, 16);
		
	sprintf(nStr, "%c%c%c%c, ", rxBuff[14], rxBuff[15], rxBuff[16], rxBuff[17]);
	nwk = strtol(nStr, NULL, 16);
	
	if (trace) printf("\tEndpoints for %04x\n", nwk);

	iq = 0;
	for (i=0; i<MAX_DEVICES; i++)
	{
		if (lqiRec[i].srcAddr < 0) break;
		if (nwk == lqiRec[i].srcAddr)
		{
			if (trace) printf("\tActiveEndpointCount %d, ActiveEndpointList ", n);
			for (j=0; j<n; j++) 
			{
				if (trace) printf("%s%c%c, ", j?" ":"", rxBuff[20+j*2], rxBuff[21+j*2]);
				/*? strncpy(Dev->endPoint[i], &rxBuff[20+i*2], 2);
				Dev->endPoint[i][2] = 0; ?*/
				strncpy(nStr, &rxBuff[20+j*2], 2);
				nStr[2] = 0;
				lqiRec[i].endPoint[j] = strtol(nStr, NULL, 16);
				lqiRec[i].endpResp = time(NULL);
			}
			iq = 1;
			break;
		}
		if (trace) printf("\n");
	}
	if (! iq) 
	{
		if (trace) printf("Network address %04x is not known\n", nwk);
		return -1;
	}
	
	return 0;
}

int unitLqiAttributes(struct unitLqiStruct * lqiRec, 
		      char * rxBuffer)
{
	int bch;
	int i , iq, n, nwk, endp, num, dlen, sp, sts, dtype;
	char nan[64], nStr[8];

	sprintf(nStr, "%c%c%c%c, ", rxBuffer[8], rxBuffer[9], rxBuffer[10], rxBuffer[11]);
	nwk = strtol(nStr, NULL, 16);

	iq = -1;
	for (i=0; i<MAX_DEVICES; i++)
	{
		if (lqiRec[i].srcAddr < 0) break;
		if (nwk == lqiRec[i].srcAddr)
		{
			iq = i;
			break;
		}
	}

	if (iq < 0)
	{
		printf("Network address %04x not known\n", nwk);
		return -1;
	}
	endp = GetSrcEndpoint(&rxBuffer[12]);
	if (trace) printf("\tProcess attributes for %04x endpoint %02x\n", nwk, endp);
	if (lqiRec[iq].endPoint[0] <= 0) 
	{
		lqiRec[iq].endPoint[0] = endp;
		if (verbose) printf("Add endpoint for device %d: %04x (%02x)\n", iq, nwk, endp);
	}

	strncpy(nan, &rxBuffer[18], 2);
	nan[2] = 0;
	num = (int) strtol(nan, NULL, 16);
	if (trace) printf("\tNumber %02d, ", num);
	sp = 20;
	for (n=0; n<num; n++)
	{
		if (trace) printf("\tAttributeId%d %c%c%c%c, ", n+1, rxBuffer[sp], rxBuffer[sp+1], rxBuffer[sp+2], rxBuffer[sp+3]);
		sts = Status(&rxBuffer[sp+4]);
		if (sts == 0)
		{
			dlen = dataType(&rxBuffer[sp+6], nan, &dtype);
			if (dtype == 0x42)
			{
				printf(" %d bytes '", dlen);
				for (i=1; i<=dlen; i++)
				{
					strncpy(nan, &rxBuffer[sp+8+i*2], 2);
					nan[2] = 0;
					bch = strtol(nan, NULL, 16);
					printf("%c", bch);
					lqiRec[iq].attributes[0][i-1] = bch;
				}
				lqiRec[iq].attributes[0][dlen] = 0;
				printf("'");
				sp += 2*dlen + 8;
			}
			else if (dlen == -8)
			{
				dispIeeeAddr(&rxBuffer[sp+8], 1);
				sp += 16;
			}
			else
			{
				if (trace) printf(" %d bytes ", dlen);
				dlen *= 2;
				for (i=0; i<dlen; i++)
					printf("%c", rxBuffer[sp+8+i]);
				sp += dlen + 8;
			}
			printf(", ");
		}
		else
		{
			if (trace) printf("\tIgnore status %02x\n", sts);
		}
	}
	printf("\n");
	if (trace) printf("\n");

	return 0;
}
