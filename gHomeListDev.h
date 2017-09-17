/*! \file gHomeListDev
	\brief List devices on Zigbee network
 */

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: gHomeListDev.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.1.1.1  2011/05/28 09:00:03  johan
 * gHome logging server
 *
 * Revision 1.3  2011/02/10 11:09:41  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:14  johan
 * Add CVS stuff
 *
 *
 *
 */

#ifndef GHOMELISTDEV_H
#define GHOMELISTDEV_H

#define MAX_ENDPOINTS 32

/*! \struct DeviceInfoStruct device information
 */
struct DeviceInfoStruct
{
	char shrtAddr[8];
	char ieeeAddr[32];
	int loctn;
	char endPoint[MAX_ENDPOINTS][8];
	char attributes[MAX_ENDPOINTS][128];
};

typedef struct DeviceInfoStruct DeviceInfo;

int GatewayInfo(int skt, char * txBuffer, char * rxBuffer, DeviceInfo * gwDev, int rdTimeout);

int CoordinatorInfo(int skt, char * txBuffer, char * rxBuffer, DeviceInfo * coordDev, int rdTimeout);

int DeviceList(int version, int skt, 
	        char * sAddr, char * txBuffer, char * rxBuffer, DeviceInfo ** coordDev,  
	        int devCount, int rdTimeout);

int GetEndPoints(int skt, int idx, char * txBuffer, char * rxBuffer, DeviceInfo * Dev, int rdTimeout);

int ReadAttributes(int skt, int idx, char * txBuffer, char * rxBuffer, DeviceInfo * Dev, int rdTimeout);

int StartWarning(int skt, char * txBuffer, char * rxBuffer, DeviceInfo * sysDev, int rdTimeout);

#endif
