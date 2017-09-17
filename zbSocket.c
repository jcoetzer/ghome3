/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbSocket.c,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.8  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.7  2011/02/16 21:39:38  johan
 * Read user description from message
 *
 * Revision 1.6  2011/02/16 20:42:05  johan
 * Add user description field
 *
 * Revision 1.5  2011/02/15 15:03:37  johan
 * Remove command parameter in message read
 *
 * Revision 1.4  2011/02/10 11:09:43  johan
 * Take out CVS Id tag
 *
 * Revision 1.3  2011/02/09 19:56:18  johan
 * Clean up
 *
 * Revision 1.2  2011/02/03 08:38:17  johan
 * Add CVS stuff
 *
 *
 *
 */

#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>

extern int trace;

#include "zbNumber.h"
#include "zbData.h"
#include "zbSocket.h"
#include "zbDisplay.h"
#include "zbPacket.h"

/*! \brief Open Client Socket Connection
 *	\param  sHostName IP Address or DNS reference
 *	\param	sHostPort	port number
 *	\return Socket number, -1 upon error
 */
int zbSocketOpen(char *sHostName, 
	         int sHostPort)
{
	int iZigbeeSocket;
	int flag;
	int ret;
	struct hostent* host;
	struct sockaddr_in serverAddress;
	
	/* Retrieve my ip address.  Assuming the hosts file in
	   in /etc/hosts contains my computer name.
	   DNS Resolve the server name to an IP address 
	 */
	if (trace) printf("\tResolving host: %s Port:%i \n",sHostName, sHostPort);
	host = gethostbyname((const char *)sHostName);
	if (host ==NULL)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "%s ERROR zbSocketOpen gethostbyname: %s\n", DateTimeStampStr(-1), strerror(h_errno));
		return -1;
	}

	/* Create a socket */
	iZigbeeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* TCP_NODELAY */
	if (iZigbeeSocket==-1)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "%s ERROR zbSocketOpen: %s\n", DateTimeStampStr(-1), strerror(errno));
		return -1;
	}

	/* Disable the Nagle (TCP No Delay) algorithm 
	   More tuning ideas http://www.ibm.com/developerworks/linux/library/l-hisock.html
	   We need to get realtime performance with immediate sends 
	 */
	flag = 1;
	ret = setsockopt(iZigbeeSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
	if (ret==-1)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "%s ERROR zbSocketOpen: %s\n", DateTimeStampStr(-1), strerror(errno));
		return -1;
	}

	/* Initialise structures */
	memcpy(&serverAddress.sin_addr, host->h_addr, host->h_length);
	serverAddress.sin_family=AF_INET; 		/*!< AF_INET - ARP TCP/IP */
	
	/* Convert socket to network byte order */
	serverAddress.sin_port=htons(sHostPort);
 
	/* Connect socket to the appropriate port and interface */
	if (connect(iZigbeeSocket,(const struct sockaddr *)&serverAddress,(socklen_t)sizeof(serverAddress))==-1)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "%s ERROR zbSocketOpen: %s\n", DateTimeStampStr(-1), strerror(errno));
		/* Close on failure */
		close(iZigbeeSocket); 
		iZigbeeSocket = 0;
		return -1;	
	}
	if (trace) printf("\tClient Socket connection started Socket:%X\n",iZigbeeSocket);
	/* Return successfull */
	return iZigbeeSocket;
}

/*! \brief   Close client socket
 *	\param	iZigbeeSocket	socket already open
 */
int zbSocketClose(int iZigbeeSocket)
{
	/* Close the main socket connection */
	if(iZigbeeSocket > 0)
	{
		if (trace) printf("\tClosing Socket\n");
		close (iZigbeeSocket);
		iZigbeeSocket = 0;
		return 0;
	}
	return -1;
}

/*! \brief Read from Socket
 *	\param	iZigbeeSocket	socket already open
 *	\param	cBuffer		data buffer
 *	\param	piLength 	length of block
 *	\param	rdTimeout	timeout period in seconds
 *	\return Number of bytes sent, -1 on error
 */
int zbSocketRead(int iZigbeeSocket, 
		 unsigned char * cBuffer,
		 int * piLength)
{
	int retval;
	fd_set readfds;
	struct timeval timeout;

	/* Zero the structures */
	FD_ZERO(&readfds);

	/* Load socket descriptor to structure */
	FD_SET(iZigbeeSocket,&readfds);

	/* Set up timeout parametres to POLL Operation */
	timeout.tv_sec = 0;     /*!< Seconds  */
	timeout.tv_usec = 0;	/*!< uSeconds */
		
	/* Block here until data is available to be read
	   This stops an exception that sometimes occurs if there is a problem with the connection */
	retval = select(iZigbeeSocket+1, &readfds, NULL, NULL, &timeout);
	/* retval == 0 if timed out */

	/* Process error if any */
	if (retval == -1) 
		printf("%s ERROR zbSocketRead: %s\n", DateTimeStampStr(-1), strerror(errno));
	else
	{
		/* Look for connections to be read */
		if (retval > 0)
		{
		
			retval = recv(iZigbeeSocket,(void *) cBuffer,*piLength - 1,0); /* MSG_WAITALL */

			*piLength = retval;
			/* Process error if any */
			if (retval == -1)
			{
				printf("%s ERROR zbSocketRead: %s\n", DateTimeStampStr(-1), strerror(errno));
				return -1;
			}
			/* Cater for a disconected Client */
			else if(retval == 0)
			{
				*piLength = retval;
				return 0;
			}
			else
			{
				/* Return with data */
				*piLength = retval;
				return retval;
			}
		}
		else
		{
			/* Timeout nothing to read */
			*piLength = retval;
			return 0;
		}
		
	}
	/* Problem with socket */
	*piLength = retval;
	return retval;
}

/*! \brief	Clear all messages from the socket Socket
 *	\param	iZigbeeSocket	socket already open
 *	\param	rdTimeout	timeout period in seconds
 *	\return	true - error, false - ok
 */
int zbSocketClear(int iZigbeeSocket, 
		  int rdTimeout)
{
	static unsigned char cBuffer[512];
	int iLength = 512;
	int rc;

	do 
	{
		rc = zbSocketRead(iZigbeeSocket, (unsigned char *) cBuffer,&iLength);
		if (trace) printf("\tClear socket: read %d bytes\n", rc);
	} while(rc > 0);
	return 0;
}

/*! \brief	Send on socket connection
 *	\param	iZigbeeSocket	socket already open
 *	\param	cBuffer		data buffer
 *	\param	iLength		size of buffer
 *	\return Number of bytes sent, -1 on error
 */
int zbSocketWrite(int iZigbeeSocket, 
		  void * cBuffer, 
		  int iLength)
{
	int retval;

	/* Write message block to client */
	retval = send( iZigbeeSocket,(char *)cBuffer,iLength,MSG_CONFIRM   );
	if(retval == -1) /* MSG_DONTROUTE */
	{		
		printf("%s ERROR zbSocketWrite: %s\n", DateTimeStampStr(-1), strerror(errno));
		return -1;
	}
	return retval;
}

/*! \brief	Format zigbee message and send it
 *	\param	iZigbeeSocket	socket already open
 *	\return Number of bytes sent, -1 on error
 */
int zbSocketSendPing(int iZigbeeSocket)
{
	int retval;
	
	if (trace) printf("\tPing %s\n", PktPing);
		
	retval = zbSocketWrite(iZigbeeSocket, PktPing, strlen(PktPing));
	if (retval < 0)
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "ZigBee Ping FAIL : %s\n", PktPing);
		return -1;
	}
	
	return retval;
}

/*! \brief	Read anything on the line
 *	\param	iZigbeeSocket	socket already open
 *	\param  iCMD		command
 *	\param	rxBuffer	data buffer
 *	\param	rdTimeout	timeout period in seconds
 *	\return Number of bytes read, -1 on error
 */
int zbSocketReadPacket(int iZigbeeSocket, 
		       char * rxBuffer)
{
	static char cBuffer[512];
	int iLen = 512, rc=0, iCMD;
	int iFCS = 0 ;
	int iPacketFCS;
	
	cBuffer[0] = 0;
	rxBuffer[0] = 0;

	/* Get a message if any */
	if(zbSocketRead(iZigbeeSocket, cBuffer, &iLen) < 0)
	{
		DateTimeStampFile(stderr);
		if (trace) fprintf(stderr, "zigbeeReadPacket read Socket failed len %i \n", iLen);
		return -1;
	}

	/* No message */
	if (iLen == 0)
	{
		return 0;
	}
	rc = iLen;

	if (trace >= 2) dumpRXdata("\tZigBee RX", cBuffer, iLen);
	strncpy(rxBuffer, cBuffer, iLen);
	rxBuffer[iLen] = 0;

	/* Start of packet (SOP) */
	if (!(cBuffer[0] == 0x30 && cBuffer[1] == 0x32))
	{
		DateTimeStampFile(stderr);
		fprintf(stderr, "Msg fails Start Of Packet %02X%02X \n", cBuffer[0],cBuffer[1]);
		return -1;
	}

	/* Command (CMD) */
	iCMD = zbNumberHex2dec(cBuffer[5]) + zbNumberHex2dec(cBuffer[4])*0x10 + zbNumberHex2dec(cBuffer[3])* 0x100 + zbNumberHex2dec(cBuffer[2]) * 0x1000;

	/* Length (LEN) */
	iLen = zbNumberHex2dec(cBuffer[7]) + zbNumberHex2dec(cBuffer[6])*0x10;

	/* Frame check sequence (FCS) */
	iPacketFCS = zbNumberHex2dec(cBuffer[8+ (iLen*2)+1]) + zbNumberHex2dec(cBuffer[8+ (iLen*2)])*0x10;

	/* FCS Check - Flag end of CS area so ComputeFCS will stop */
	cBuffer[8 + ( iLen * 2)] = 0;
	iFCS = zbNumberComputeFCS(&cBuffer[2]);

	if ((iFCS ^ iPacketFCS) == 0 ) return rc;

	return -1;
}
