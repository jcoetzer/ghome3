#ifndef ZBPACKET_H
#define ZBPACKET_H

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbPacket.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.4  2011/02/16 20:42:05  johan
 * Add user description field
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

extern char PktSysPing[];
extern char PktSysExt[];
extern char PktDisarm[];
extern char PktArm[];
extern char PktArmDay[];
extern char PktArmNite[];
extern char PktSysInfo[];
extern char PktLqit[];
extern char PktGetArmMode[];
extern char PktSoundAlarm[];
extern char PktSoundFire[];
extern char PktSoundPanic[];
extern char PktCoordIeee[];
extern char PktGetAlarm[];
extern char PktGetLqi[];
extern char PktGetRecentAlrm[];
extern char PktReadHeartbeat[];
extern char PktPermitJoin[];
extern char PktPing[];

int zbPollSquawkAlarm(int iZigbeeSocket, int shortAddress);

#endif
