#ifndef ZBSTATE_H
#define ZBSTATE_H

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbState.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.11  2011/02/23 15:48:49  johan
 * Reset alarm log
 *
 * Revision 1.10  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.9  2011/02/16 21:39:38  johan
 * Read user description from message
 *
 * Revision 1.8  2011/02/16 20:42:06  johan
 * Add user description field
 *
 * Revision 1.7  2011/02/15 13:05:07  johan
 * Add network discovery command
 *
 * Revision 1.6  2011/02/10 11:09:44  johan
 * Take out CVS Id tag
 *
 * Revision 1.5  2011/02/03 09:02:24  johan
 * Check again that CVS works
 *
 * Revision 1.4  2011/02/03 08:57:54  johan
 * Check again that CVS works
 *
 * Revision 1.3  2011/02/03 08:41:10  johan
 * Check that CVS works
 *
 * Revision 1.2  2011/02/03 08:38:17  johan
 * Add CVS stuff
 *
 *
 */

/*! \enum zbState Enumerated Event Types */
enum zbState 
{
	zigbeeDOWN,
	zigbeeCONNECT,
	zigbeeINITGW,
	zigbeeSYSPING,
	zigbeeARMMODE,
	zigbeeUP,
	zigbeePING,
	zigbeeSTART,
	zigbeeERROR,
	zigBeePOLL,
	zigbeeARMED,
	zigbeeALARM,
	zigbeeHEARTBEAT,
	zigbeeGETALARM,
	zigBeeUPDATE,
	zigbeeDISCOVER,
	zigbeePIR,
	zigbeeTEMP,
	zigbeeCMD,
	zigbeeRESETLOG
};

enum zbEventEnum
{
	UNKNOWN = 0,
	ALARM,
	ALARMcontinue,
	ANNCE,
	ARM,
	ARM_DAY,
	ARM_NITE,
	ARM_STAY,
	DISARM,
	FIRE,
	GET_ALARM,
	HBEAT,
	HEARTBEAT,
	IEEE,
	NODE,
	LQI,
	PANIC,
	PIR,
	SHORT,
	SYSDEV,
	SYSPING,
	TEMP,
	ENDPOINT,
	ATTRIB,
	USER
};
typedef enum zbEventEnum zbEvent;

void dispEvent(char * pf, zbEvent eVal, char * ps);

#endif

