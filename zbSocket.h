#ifndef ZBSOCKET_H
#define ZBSOCKET_H

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbSocket.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.5  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.4  2011/02/15 15:03:37  johan
 * Remove command parameter in message read
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

#define ZIGBEE_GATEWAY_PORT 5000

int zbSocketOpen(char *sHostName, int sHostPort);
int zbSocketClose(int gizigbeeSocket);
int zbSocketClear(int gizigbeeSocket, int rdTimeout);
int zbSocketRead(int gizigbeeSocket, unsigned char * cBuffer,int * piLength);
int zbSocketWrite(int gizigbeeSocket, void * cBuffer,int iLength);

int zbSocketSendPing(int iZigbeeSocket);

int zbSocketReadPacket(int gizigbeeSocket, char * cBuffer);

int readBuffer(int skt, void * unit, char * rxBuffer, int rxLen, int usec);
int checkBuffer(int skt, void * unit, char * rxBuffer, int rxLen, int usec);

#endif
