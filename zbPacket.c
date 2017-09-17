/*
 * $Author: johan $
 * $Date: 2011/06/13 21:57:26 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: zbPacket.c,v $
 * Revision 1.2  2011/06/13 21:57:26  johan
 * Update
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 *
 *
 *
 */

#include <stdio.h>
#include <string.h>

#include "zbSend.h"
#include "zbDisplay.h"

extern int verbose;
extern int trace;

/* Predefined Zigbee packets, complete with checksum */
char PktSysPing[] 	= "0200070007";
char PktSysExt[] 	= "0200110011";
char PktDisarm[] 	= "020C0007020000010501000C";
char PktArm[]    	= "020C0007020000010501030F";
char PktArmDay[]    	= "020C0007020000010501010D";
char PktArmNite[]    	= "020C0007020000010501020E";
char PktSysInfo[] 	= "0200140014";
char PktGetArmMode[] 	= "020C0D0602000001050100";
char PktSoundAlarm[] 	= "020C02060200000105010F";
char PktSoundFire[] 	= "020C03060200000105010E";
char PktSoundPanic[] 	= "020C040602000001050109";
char PktGetLqi[]	= "020A0F040200000003";
char PktCoordIeee[]	= "020A03060200000000000D";
char PktGetAlarm[]	= "020C02060200000105000E";
char PktGetRecentAlrm[]	= "020c0b0602000001050106";
char PktReadHeartbeat[]	= "020C0A0602000001050107";
char PktPermitJoin[]	= "020A160502FFFF3C0027";
char PktPing[]          = "02XX00";			/*!< As used by ZigButler */

/*! \brief Send Zigbee command to sound squawk
	\param	iZigbeeSocket	socket already opened
	\param	shortAddress	short Zigbee address
	\return 0 when successful, else -1
 */
int zbPollSquawkAlarm(int iZigbeeSocket, 
		      char * shortAddress)
{
	int iLength = 0;
	int retval;
	char zigBeePktStr[64];
	char zbStr[64];

	if ((! shortAddress) || (strlen(shortAddress) != 4))
	{
		fprintf(stderr, "Invalid short address %s\n", shortAddress?shortAddress:"<UNKNOWN>");
		return -1;
	}
	/* Create Zigbee command string using short address e.g. 02 0C01 07 02E6CA0A050200 29 */
	snprintf(zigBeePktStr, sizeof(zigBeePktStr), "020C010702%s0A050200", shortAddress);
	if (trace) printf("\tSend Zigbee command to sound squawk\n");

	retval = zbSendCheck(zigBeePktStr, zbStr);
	if (retval)
	{
		printf("Packet check failed\n");
		return -1;
	}
	if (trace) printf("\tGenerate squawk for %s : %s\n", shortAddress, zbStr);

	/* Lemgth of data in Asci HEX Bytes ie. 2 Chars per byte */
	iLength = strlen(zbStr);
	if (! iLength) return -1;
	if (trace) dumpTXdata("\tZigBee TX", zbStr, iLength);
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zbStr, iLength, "<tx>\n");
	}

	if (trace) printf("\tSend command to sound squawk: %s\n", zbStr);

	if (trace) printf("\tSend squawk: %s\n", zbStr);
	iLength = strlen(zbStr);	
	if (verbose)
	{
		DateTimeStamp();
		dispBuffer(zbStr, iLength, "<tx>\n");
	}
	retval = zbSocketWrite(iZigbeeSocket, zbStr, iLength);
	if (retval < 0)
	{
		if (trace) fprintf(stderr, "zbPollSquawkAlarm FAIL LEN:%i\n", iLength);
		return -1;
	}

	return 0;
}