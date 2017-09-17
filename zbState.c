/*! \file zbState.c 
	\brief Display Zigbee state name
*/

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbState.c,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
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

#include <stdio.h>

#include "zbState.h"

void dispEvent(char * pf, 
	       zbEvent eVal, 
	       char * ps)
{
	printf("%sEvent ", pf);
	switch (eVal)
	{
		case UNKNOWN : printf("UNKNOWN"); break;
		case ALARM : printf("ALARM"); break;
		case ALARMcontinue : printf("ALARMcontinue"); break;
		case ANNCE : printf("ANNCE"); break;
		case ARM : printf("ARM"); break;
		case ARM_DAY : printf("ARM_DAY"); break;
		case ARM_NITE : printf("ARM_NITE"); break;
		case ARM_STAY : printf("ARM_STAY"); break;
		case DISARM : printf("DISARM"); break;
		case FIRE : printf("FIRE"); break;
		case GET_ALARM : printf("GET_ALARM"); break;
		case HBEAT : printf("HBEAT"); break;
		case HEARTBEAT : printf("HEARTBEAT"); break;
		case IEEE : printf("IEEE"); break;
		case LQI : printf("LQI"); break;
		case PANIC : printf("PANIC"); break;
		case PIR : printf("PIR"); break;
		case SHORT : printf("SHORT"); break;
		case SYSDEV : printf("SYSDEV"); break;
		case SYSPING : printf("SYSPING"); break;
		case TEMP : printf("TEMP"); break;
		default : printf("Undefined %d", eVal);
	}
	printf("%s", ps);
}
