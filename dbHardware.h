#ifndef dbHardware_h
#define dbHardware_h

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: dbHardware.h,v $
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

int getUnitIp(char * ipadr);

int dbHardware_AlarmLog(struct ConnectionRec * connctn, char * rxbuf);

#endif
