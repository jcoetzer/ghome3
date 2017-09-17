/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: gHomeTraffic.c,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.4  2011/02/18 06:43:47  johan
 * Streamline polling
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

#include <stdio.h>
#include <string.h>
#include <popt.h>
#include <signal.h>
#include <libgen.h>
#include <stdlib.h>
#include <ctype.h>

#include "zbNumber.h"
#include "zbData.h"
#include "zbDisplay.h"
#include "zbPacket.h"
#include "gversion.h"

#define RX_BUFFER_SIZE 10240
#define FILTER_CMDS 8

int ShowVersion = 0;
int trace = 0;
int verbose = 0;
int force = 0;
int rdstdin = 0;
int FilterCmd[FILTER_CMDS] = { 0 };
char * FilterCmdStr = NULL;

char * TrafficFileName = NULL;

int checkData(char * txBuffer);
int istimeval(char * sdata);

/*! \struct Command line parsing data structure */
struct poptOption optionsTable[] =
{
	{"file", 'F', POPT_ARG_STRING, &TrafficFileName, 0,  "Traffic dump file",  "<string>"},
	{"force", 'f', POPT_ARG_NONE, &force, 0, "Force reading of invalid packets", NULL},
	{"stdin", 'i', POPT_ARG_NONE, &rdstdin, 0, "Read standard input", NULL},
	{"trace", 't', POPT_ARG_NONE, &trace, 0, "Trace/debug output", NULL},
	{"verbose", 'v', POPT_ARG_NONE, &verbose, 0, "Verbose output", NULL},
	{"filter", 'C', POPT_ARG_STRING, &FilterCmdStr, 0,  "Filter command (default is off)", "<integer>"},
	{"version", 	'V', POPT_ARG_NONE, 	&ShowVersion, 0, "Display version information", NULL},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

/* Local functions */
void sighandle(int signum);
int checkData(char * txBuffer);
int checkBuffer(int skt, void * unit, char * cBuffer, int cLen, int usec);
int readTrafficFile(char * fname);
int readTcpDump(void);
int readTcpDumpStr(char * tcpData);

/*! \brief M A I N
	\return	Zero when successful, otherwise non-zero
 */
int main(int argc, 
	 char *argv[])
{
	ssize_t rc;
	poptContext optcon;        /* Context for parsing command line options */

	signal(SIGPIPE, sighandle);

	optcon = poptGetContext(argv[0],argc,(const char **)argv,optionsTable,0);
	rc = poptGetNextOpt(optcon);
	if (rc < -1)
	{
		printf("%s: %s - %s\n", basename(argv[0]), poptBadOption(optcon,0),
		       poptStrerror(rc));
		poptPrintUsage(optcon, stderr, 0);
		poptFreeContext(optcon);
		return -1;
	}
	if (ShowVersion) displayVersion(argv[0]);

	if (rdstdin)
		readTcpDump();
	else if (TrafficFileName)
		readTrafficFile(TrafficFileName);
	else
	{
		fprintf(stderr, "No Traffic File Name given\n");
		poptPrintUsage(optcon, stderr, 0);
		poptFreeContext(optcon);
		return -1;
	}
	
	poptFreeContext(optcon);
	return 0;
}

int readTrafficFile(char * fname)
{
	ssize_t rc;
	int i, j, k, rxlen, chk, Cmd = 0, fcnt = 0;
	size_t buflen;
	char * dataBuffer;
	char rxBuffer[RX_BUFFER_SIZE];
	char timestr[16];
	FILE * trafficFile;
	unsigned char cByte = 0;
	unsigned char cByteL = 0;
	unsigned char cByteH = 0;
	int usec = 2e6;

	trafficFile = fopen(fname, "r");
	if (! trafficFile)
	{
		fprintf(stderr, "Could not read Traffic File %s\n", fname);
		return -1;
	}

	if (FilterCmdStr)
	{
		char * fctok = strtok(FilterCmdStr, ",");
		i = 0;
		while (fctok)
		{
			FilterCmd[i] = (int) strtol(fctok, NULL, 16);
			if (trace) printf("\tDisplay command %x\n", FilterCmd[i]);
			i++;
			fctok = strtok(NULL, ",");
		}
		fcnt = i;
		if (trace) printf("\tFound %d command(s)\n", fcnt);
	}

	dataBuffer = malloc(RX_BUFFER_SIZE);
	buflen = RX_BUFFER_SIZE;

	/* Read input file */
	Cmd = 0;
	strcpy(timestr, "xx:xx:xx");
	memset(dataBuffer, 0, RX_BUFFER_SIZE);
	rc = getline(&dataBuffer, &buflen, trafficFile);
	while (rc > 0)
	{
		if (trace) printf("\t>%s", dataBuffer);
		if (0 == strncmp(dataBuffer, "02", 2))
		{
			if (fcnt)
			{
				chk = 1;
				Cmd = getCmd(&rxBuffer[i]);
				for (k=0; k<fcnt; k++)
				{
					if (FilterCmd[k] == Cmd)
					{
						chk = 0;
						break;
					}
				}
			}
			else
				chk = 0;
			if (! chk)
			{
				printf("%s ", timestr);
				rxlen = strlen(dataBuffer)-1;
				dataBuffer[rxlen] = 0;
				readBuffer(0, NULL, dataBuffer, rxlen, usec);
			}
		}
		else if (0 == strncmp(dataBuffer, "|0   |", 6)) 
		{
			rxlen = strlen(dataBuffer)-2;
			j = 0;
			for (i=6; i<rxlen; i+=3)
			{
				cByteL = zbNumberHex2dec(dataBuffer[i+1]);
				cByteH = zbNumberHex2dec(dataBuffer[i]);
			
				cByte = cByteL + (unsigned char)(cByteH*0x10);
				
				if (! isprint(cByte)) cByte = '.';
				rxBuffer[j++] = cByte;
			}
			rxBuffer[j] = 0;
			if (trace) printf("\t%s\n", rxBuffer);
			rxlen = j-1;
			for (i=j; i>=0; i--)
			{
				if (rxBuffer[i] == '.') break;
			}
			i++;
			if (trace) printf("\t%s\n", &rxBuffer[i]);
			rc = rxlen-i+1;
			if (rc >= 6)
			{
				if (fcnt)
				{
					chk = 1;
					Cmd = getCmd(&rxBuffer[i]);
					for (k=0; k<fcnt; k++)
					{
						if (FilterCmd[k] == Cmd)
						{
							chk = 0;
							break;
						}
					}
				}
				else
					chk = 0;
				if (! chk)
				{
					printf("%s ", timestr);
					readBuffer(0, NULL, &rxBuffer[i], rc, usec);
				}
			}
		}
		else if (istimeval(dataBuffer))
		{
			strncpy(timestr, dataBuffer, 8);
			timestr[8] = 0;
		}
		memset(dataBuffer, 0, RX_BUFFER_SIZE);
		rc = getline(&dataBuffer, &buflen, trafficFile);
	}
	fclose(trafficFile);

	return 0;
}

int readTcpDump(void)
{
	int rc;
	size_t buflen;
	char * dataBuffer;
	char rxBuffer[RX_BUFFER_SIZE];

	dataBuffer = malloc(RX_BUFFER_SIZE);
	buflen = RX_BUFFER_SIZE;

	rxBuffer[0] = 0;
	rc = getline(&dataBuffer, &buflen, stdin);
	while (rc > 0)
	{
		rc = strlen(dataBuffer);
		--rc;
		if (! isprint(dataBuffer[rc])) dataBuffer[rc]=0;
		--rc;
		if (! isprint(dataBuffer[rc])) dataBuffer[rc]=0;
		if (trace) printf("\t<%s>\n", dataBuffer);
		if (rc < 4)
		{
			strcat(rxBuffer, dataBuffer);
		}
		else if (0==strncmp(dataBuffer, "IP ", 3) && isdigit(dataBuffer[3]))
		{
			readTcpDumpStr(rxBuffer);
			rxBuffer[0] = 0;
		}
		else
		{
			strcat(rxBuffer, dataBuffer);
		}
		rc = getline(&dataBuffer, &buflen, stdin);
	}
	return 0;
}

int readTcpDumpStr(char * tcpData)
{
	int dlen;
	int usec = 2000;
	
	dlen = strlen(tcpData);
	if (dlen < 38) 
	{
		if (verbose) printf("Incomplete data '%s'\n", tcpData);
		return -1;
	}
	if ('E' != tcpData[0])
	{
		if (verbose) printf("Invalid data \t'%s'\n", tcpData);
		return -1;
	}
	
	if (!strncmp(&tcpData[38], "02", 2))
	{
		if (trace) printf("\t%s\n", &tcpData[38]);
		readBuffer(0, NULL, &tcpData[38], dlen-38, usec);
	}
	else if (!strncmp(&tcpData[37], "02", 2))
	{
		if (trace) printf("\t%s\n", &tcpData[37]);
		readBuffer(0, NULL, &tcpData[37], dlen-38, usec);
	}
	else
	{
		if (trace) printf("\tNon-Zigbee data \t'%s'\n", tcpData);
		return -1;
	}
/*
E..("......,.......-...Q......P...1...020C0D
E..P5......w.......E...g......P..V\...03060200000100000C020A030602F9CB0100003E
E..C......&j.........-.....Q.3P....\..021C0008C68E01050011640528Z
E.........:+.........6..Z...PP...!c..020A8B363BDC00040004D87F0043B29F195A0000FD00D87F0043B29F195AF9CBFF00D87F0043B29F195AB198FF00D87F0043B29F195AC68EFF0001Z
E...5......9.......E...g......P..V....030602F9CB0100003E020A0F040200000003020A0E0902F9CB07FFF80010002D020A0E0902F9CB07FFF80010002D
*/
	return 0;
}

/*! \brief Signal handler
	\param	signum	signal number
 */
void sighandle(int signum)
{
	printf("Shutting down after signal %d\n", signum);
	exit(1);
}

/*! \brief	Check that string is a valid time in format HH:MM:SS
	\param	sdata		data buffer
	\return	1 if string is valid otherwise 0
 */
int istimeval(char * sdata)
{
	if (strlen(sdata)<8) return 0;
	if (isdigit(sdata[0]) && isdigit(sdata[1]) && sdata[2]==':' &&
		   isdigit(sdata[3]) && isdigit(sdata[4]) && sdata[5]==':' &&
		   isdigit(sdata[6]) && isdigit(sdata[7])) 
		return 1;
	return 0;
}

/*! \brief Read and display buffer
	\param	skt		open socket
	\param	unit		unit info
	\param	cBuffer		data buffer
	\param	cLen		buffer size
	\param	unitName	unit name
	\param	unitIp		IP address
	\param	unitNum		unit number
	\return	Zero when successful, otherwise non-zero
 */
int checkBuffer(int skt,
		void * unit,
		char * cBuffer, 
		int cLen,
		int usec)
{
	int rc;
	
	rc = checkData(cBuffer);
	
	if (force) dispBuffer(cBuffer, cLen, "\n");
	else if (! rc) dispBuffer(cBuffer, cLen, "\n");
	return 0;
}

/*! \brief Prepare data string
	\param	txBuffer		data buffer
	\return	Zero when successful, otherwise non-zero
 */
int checkData(char * txBuffer)
{
	int rc, fcs = -1, dlen, blen;
	char zbStr[512];
	
	blen = strlen(txBuffer);
	if (trace) printf("\tCheck string %s (%d bytes)\n", txBuffer, blen);
	
	if (strncmp(txBuffer, "02", 2))
	{
		if (trace) printf("\tStart of packet is not '02'\n");
		return -1;
	}

	/* Get length of data */
	zbStr[0] = txBuffer[6];
	zbStr[1] = txBuffer[7];
	zbStr[2] = 0;
	dlen = strtol(zbStr, NULL, 16);
	if (trace) printf("\tData length is %d bytes\n", dlen);
	rc = dlen * 2;
	
	/* Check length of data */
	if (blen == rc + 8 && force)
	{
		printf("Checksum is missing\n");
		return -1;
	}
	else if (blen != rc + 10 && blen != rc + 11 && force)
	{
		printf("* %s * ", txBuffer);
		dispCommand(&txBuffer[2]);
		if (rc) printf("Data string is %d bytes, should be %d\n", blen, rc + 10);
		else printf("Data string is %d bytes, should be %d (no length, data, FCS)\n", blen, rc + 10);
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
	if (trace) printf("\tData is '%s'\n", zbStr);
	
	if (force)
	{
		/* Calculate checksum */
		strncpy(zbStr, &txBuffer[2], rc+6);
		zbStr[rc+6] = 0;
		rc = zbNumberComputeFCS(zbStr);
		if (trace) printf("\tCalculated checksum for '%s' is %02x\n", zbStr, rc);
		
		if (rc != fcs)
		{
			printf("Checksum for '%s' is %02x, not %02x\n", zbStr, rc, fcs);
			return -1;
		}
	}

	return 0;
}
