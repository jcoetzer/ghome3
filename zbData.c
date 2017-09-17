/*! \file zbData.c 
	\brief Utilities used to display Zigbee data packets
*/

/*
 * $Author: johan $
 * $Date: 2011/06/13 21:57:26 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: zbData.c,v $
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
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#include "zbData.h"
#include "zbDisplay.h"

extern int trace;

/*! \brief Arm Mode
	\param Str	data buffer

– byte –The Arm Mode field shall have one of the values shown following:
	0x00           →   Disarm
	0x01           →   Arm Day/Home Zones Only
	0x02           →   Arm Night/Sleep Zones Only
	0x03           →   Arm All Zones
	0x04-0xff      →   Reserved
 */
void ArmMode(char * amStr)
{
	char nan[8];
	int num;

	strncpy(nan, amStr, 2);
	nan[2] = 0;
	num = (int) strtol(nan, NULL, 16);
	printf("ArmMode ");
	switch (num)
	{
		case 0 : printf("All zones DisArm, "); break;
		case 1 : printf("Arm Day/Home zones, "); break;
		case 2 : printf("Arm Night/Sleep zones, "); break;
		case 3 : printf("Arm All Zones, "); break;
		default : printf("Reserved %d, ", num);
	}
}

/*! \brief Arm Mode
	\param Str	data buffer

– byte –The Arm Mode field shall have one of the values shown following:
	0x00           →   Disarm
	0x01           →   Arm Day/Home Zones Only
	0x02           →   Arm Night/Sleep Zones Only
	0x03           →   Arm All Zones
	0x04-0xff      →   Reserved
 */
int getArmMode(char * amStr)
{
	char nan[8];
	int num;

	strncpy(nan, amStr, 2);
	nan[2] = 0;
	num = (int) strtol(nan, NULL, 16);
	return num;
}

/*! \brief Read attribute list
	\param Str	data buffer
	\return Number of bytes read

Attribute reporting configuration record:
Attribute ID  Direction    dataType    Minimum reporting interval  Maximum reporting interval   Timeout period    Reportable change

	Number – 1 byte –number of attributes in the list.

	Attribute ID1...n –List –The attribute identifier field is 16-bits in length and shall contain the identifier of the
		attribute that is to be configured.
	Direction – byte –The direction field specifies whether values of the attribute are be reported, or whether
		reports of the attribute are to be received.
		If this value is set to 0x00, then the minimum reporting interval field and maximum reporting interval field
			are included in the payload, and the timeout period field is omitted. The record is sent to a cluster server
			(or client) to configure how it sends reports to a client (or server) of the same cluster.
		If this value is set to 0x01, then the timeout period field is included in the payload, and the minimum
			reporting interval field and maximum reporting interval field are omitted. The record is sent to a cluster
			client (or server) to configure how it should expect reports from a server (or client) of the same cluster.
	Data type – byte –See “Data Type Table”
	Minimum reporting interval – 2 byte –contain the minimum interval, in seconds, between issuing reports for
		each of the attributes specified in the attribute identifier list field.
		If this value is set to 0x0000, then there is no minimum limit, unless one is imposed by the specification of
		the cluster using this reporting mechanism or by the applicable profile.
	Maximum reporting interval – 2 byte – contain the maximum interval, in seconds, between issuing reports for
		each of the attributes specified in the attribute identifier list field.
		If this value is set to 0x0000, then the device shall not issue a report for the attributes supplied in the
		attribute identifier list.
	Timeout period – 2 byte –contain the maximum expected time, in seconds,between received reports for each
		of the attributes specified in the attribute identifier list field. If more timethan this elapses between reports, this
		may be an indication that there is a problem with reporting.
		If this value is set to 0x0000, reports of the attributes in the attribute identifier list field are not subject to
		timeout.
	Reportable change – variable length—contain the minimum change to the attribute that will result in a report
		being issued.

 */
int AttReportList(char * cBuffer)
{
	char numSt[64];
	char nan[8];
	int i, dlen, num, sp, n, dtype, bch, dir;

	strncpy(nan, cBuffer, 2);
	nan[2] = 0;
	num = (int) strtol(nan, NULL, 16);
	printf("Number %02d, ", num);
	sp = 2;
	for (n=0; n<num; n++)
	{
		printf("AttributeId%d %c%c%c%c, ", n+1, cBuffer[sp], cBuffer[sp+1], cBuffer[sp+2], cBuffer[sp+3]);
		sp += 4;
		strncpy(nan, cBuffer+4, 2);
		nan[2] = 0;
		dir = (int) strtol(nan, NULL, 16);
		printf("Direction %d, ", dir);
		sp += 2;
		dlen = dataType(&cBuffer[sp], numSt, &dtype);
		sp += 2;
		switch (dir)
		{
			case 0 :
				printf("Minimum %c%c%c%c, ", cBuffer[sp], cBuffer[sp+1], cBuffer[sp+2], cBuffer[sp+3]);
				printf("Maximum %c%c%c%c, ", cBuffer[sp+4], cBuffer[sp+5], cBuffer[sp+6], cBuffer[sp+7]);
				sp += 8;
				break;
			case 1:
				printf("Timeout %c%c%c%c, ", cBuffer[sp], cBuffer[sp+1], cBuffer[sp+2], cBuffer[sp+3]);
				sp += 4;
				break;
			default :
				printf("\nERROR: Invalid direction %d\n", dir);
				return 0;
		}
		printf("Reportable ");
		if (dtype == 0x42)
		{
			printf(" %d bytes '", dlen);
			for (i=1; i<dlen; i++)
			{
				strncpy(nan, &cBuffer[sp+i*2], 2);
				nan[2] = 0;
				bch = strtol(nan, NULL, 16);
				printf("%c", bch);
			}
			printf("'");
			sp += 2*dlen;
		}
		else if (dtype == 0x21)
		{
			unsigned long int numVal;

			dlen *= 2;
			for (i=0; i<dlen; i++)
			{
				numSt[i] = cBuffer[sp+i];
			}
			numSt[i] = 0;
			sp += dlen;
			numVal = strtoul(numSt, NULL, 16);
			printf("%05ld", numVal);
		}
		else if (dtype == 0x29)
		{
			long int numVal;

			dlen *= 2;
			for (i=0; i<dlen; i++)
			{
				numSt[i] = cBuffer[sp+i];
			}
			numSt[i] = 0;
			sp += dlen;
			numVal = strtol(numSt, NULL, 16);
			printf("%05ld", numVal);
		}
		else if (dlen == -8)
		{
			dispIeeeAddr(&cBuffer[sp], 1);
			sp += 16;
		}
		else if (dlen == 0)
		{
			sp += dlen + 4;
		}
		else
		{
			printf(" %d. bytes ", dlen);
			dlen *= 2;
			for (i=0; i<dlen; i++)
				printf("%c", cBuffer[sp+i]);
			sp += dlen + 8;
		}
		printf(", ");
	}
	return sp-2;
}

/*! \brief Get status value
	\param	statStr		String value
 */
int getStatus(char *statStr)
{
	int rc;
	char statVal[8];
	
	if ((! isxdigit(statStr[0])) || (! isxdigit(statStr[1]))) return -1;
	
	strncpy(statVal, statStr, 2);
	statVal[2] = 0;
	
	rc = strtol(statVal, NULL, 16);
	return rc;
}

/*! \brief Status of operation
	\param	status	Numeric value
*/
void dispStatus(int status)
{
	switch (status)
	{
		case SUCCESS : printf("Operation was successful"); break;
		case FAILURE : printf("Operation was not successful"); break;
		case MALFORMED_COMMAND : printf("The command appears to contain the wrong fields"); break;
		case UNSUP_CLUSTER_COMMAND : printf("Specified general ZCL command is not supported on the device"); break;
		case UNSUP_GENERAL_COMMAND : printf("Specified cluster command is not supported on the device"); break;
		case UNSUP_MANUF_CLUSTER_COMMAND : printf("Cluster specific command has unknown manufacturer code"); break;
		case UNSUP_MANUF_GENERAL_COMMAND : printf("ZCL specific command has unknown manufacturer code"); break;
		case INVALID_FIELD : printf("At least one field of the command contains an incorrect value"); break;
		case UNSUPPORTED_ATTRIBUTE : printf("The specified attribute does not exist on the device"); break;
		case INVALID_VALUE : printf("Out of range error, or set to a reserved value"); break;
		case READ_ONLY : printf("Attempt to write a read only attribute"); break;
		case INSUFFICIENT_SPACE : printf("An attempt to create an entry in a table failed due to insufficient free space available"); break;
		case DUPLICATE_EXISTS : printf("An attempt to create an entry in a table failed due to a duplicate entry already being present in the table."); break;
		case NOT_FOUND : printf("The requested information (e.g. table entry) could not be found"); break;
		case UNREPORTABLE_ATTRIBUTE : printf("Periodic reports cannot be issued for this attribute"); break;
		case INVALID_DATA_TYPE : printf("The data type given for an attribute is incorrent. Command Not carried out"); break;
		case INVALID_SELECTOR : printf("The selector for an attribute is incorrect."); break;
		case WRITE_ONLY : printf("A request has been made to read an attribute that the Requestor is not authorized to read. No action taken."); break;
		case INCONSISTENT_STARTUP_STATE : printf("Setting the requested values would put the device in an inconsistent state on startup. No action taken."); break;
		case DEFINED_OUT_OF_BAND : printf("An attempt has been made to write an attribute that is present but is defined using an out-ofband method and not over the air"); break;
		case HARDWARE_FAILURE : printf("An operation was unsuccessful due to a hardware failure"); break;
		case SOFTWARE_FAILURE : printf("An operation was unsuccessful due to a software failure"); break;
		case CALIBRATION_ERROR : printf("An error occurred during calibration"); break;
		default : printf("Reserved");
	}
}

/*! \brief Display entire transmit buffer
	\param Str	description
	\param sData	data buffer
	\param iLen	data buffer length
*/
void dumpTXdata(char * sDescr, 
		char * sData, 
		int iLen)
{
	int i, iEnd;
	
	printf("%s %02d bytes :", sDescr, iLen);
	printf("%c%c %c%c%c%c %c%c [", 
		sData[0], sData[1], 				/* SOP */
		sData[2], sData[3], sData[4], sData[5], 	/* Command */
		sData[6], sData[7]); 				/* Length */
	iEnd = iLen-8-2;
	for (i=0; i<iEnd; i++)
	{
		if (i && ((i%10)==0)) printf(" ");
		printf("%c", sData[i+8]);
	}
	printf("] %c%c\n", sData[iLen-2], sData[iLen-1]);
}

/*! \brief Display entire receive buffer
	\param Str	description
	\param sData	data buffer
	\param iLen	data buffer length
 */
void dumpRXdata(char * sDescr, 
		char * sData, 
		int iLen)
{
	int i, iEnd;
	
	printf("%s %02d bytes :", sDescr, iLen);
	printf("%c%c %c%c%c%c %c%c [", 
	       sData[0], sData[1], 				/* SOP */
	sData[2], sData[3], sData[4], sData[5], 	/* Command */
 sData[6], sData[7]); 				/* Length */

	iEnd = iLen-8-3;
	for (i=0; i<iEnd; i++)
	{
		if (i && ((i%10)==0)) printf(" ");
		printf("%c", sData[i+8]);
	}
	printf("] %c%c %c\n", sData[iLen-3], sData[iLen-2], sData[iLen-1]);
}

/*! \brief Display current date and time
*/
void DateTimeStamp(void)
{
	struct tm * dtm;
	time_t ts;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	printf("%04d-%02d-%02d %02d:%02d:%02d ",
	       dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
	dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
}

/*! \brief Display current date and time
*/
char * DateTimeStampStr(int ats)
{
	struct tm * dtm;
	time_t ts;
	static char dts[32];
	 
	if (ats > 0) ts = (time_t) ats;
	else if (ats < 0) ts = time(NULL);
	else
	{
		strcpy(dts,"                   ");
		return dts;
	}

	dtm = (struct tm *)localtime(&ts);
	sprintf(dts, "%04d-%02d-%02d %02d:%02d:%02d",
	        dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
	        dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
	return dts;
}

/*! \brief Display current date and time
*/
void DateTimeStampFile(FILE * outf)
{
	struct tm * dtm;
	time_t ts;
	 
	ts = time(NULL);
	dtm = (struct tm *)localtime(&ts);
	fprintf(outf, "%04d-%02d-%02d %02d:%02d:%02d ",
		dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
}

/*! \brief Display current date and time
*/
void getDateTimeStamp(time_t * ts, 
		      char * buff, 
		      int bsize)
{
	struct tm * dtm;
	 
	dtm = (struct tm *)localtime(ts);
	snprintf(buff, bsize, "%04d-%02d-%02d %02d:%02d:%02d ",
		dtm->tm_year+1900, dtm->tm_mon+1, dtm->tm_mday,
		dtm->tm_hour, dtm->tm_min, dtm->tm_sec);
}

/*! \brief Display command name for empty packets (should not happen but does)
	\param sData	data buffer
*/
void dispCommand(char * cBuffer)
{
	char CmdStr[8];
	int cmd, dLen;
		
	strncpy(CmdStr, cBuffer, 4);
	CmdStr[4] = 0;
	cmd = (int) strtol(CmdStr, NULL, 16);
	
	strncpy(CmdStr, &cBuffer[6], 2);
	CmdStr[2] = 0;
	dLen = (int) strtol(CmdStr, NULL, 16);

	switch (cmd)
	{
		case 0x0D00 : printf("Read Attribute Len=var"); break;
		case 0x0007 : printf("System Ping request Len=0x00"); break;
		case 0x1007 : printf("System Ping response Len=0x02)");  break;
		case 0x1014 : printf("System Get Device Information Rsp Len = var "); break;
		case 0x0A02 : printf("Network Address Request Len = 0x0C");  break;
		case 0x0A80 : printf("Network Address Respose Len=var");  break;
		case 0x0A03 : printf("IEEE Request (Len=0x06)"); break;
		case 0x0A81 : printf("IEEE response Len=0x24");  break;
		case 0x0A06 : printf("Node descriptor request Len=0x07");  break;
		case 0x0A82 : printf("Node descriptor response Len=21");  break;
		case 0x0A05 : printf("Power descriptor request Len=0x07");  break;
		case 0x0A83 : printf("Power descriptor response Len=0x09");  break;
		case 0x0A84 : printf("Simple descriptor request Len=var\t"); break;
		case 0x0A07 : printf("Active EndPoint request Len=0x06");  break;
		case 0x0A85 : printf("Active EndPoint response Len=var"); break;
		case 0x0A0A : printf("User Description request Len=0x06");   break;
		case 0x0A8F : printf("User Description response Len=var");  break;
		case 0x0A11 : printf("Manage Bind request Len=0x04"); break;
		case 0x0A8D : printf("Manage Bind response Len=var");   break;
		case 0x0A0F : printf("Manage LQI request Len=0x04");  break;
		case 0x0A8B : printf("Manage LQI response Len=var");  break;
		case 0x0AF0 : printf("End Device Annce Len=0x0B");  break;
		case 0x1C00  : 
			switch (dLen)
			{
				case 6: printf("Arm response Len=0x06"); break;
				case 7: printf("Zone Status Change Notification Len=0x07"); break;
				case 8: printf("Add Group response Len=0x08"); break;
				default : printf("Other command %04x Len=%d", cmd, dLen);
			}
			break;
		case 0x1C01 : 
			switch (dLen)
			{
				case 6: printf("Zone Un-Eroll response Len=0x06"); break;
				case 8: printf("Get Zone ID Map response Len=0x08"); break;
				case 9: printf("Zone Eroll Req Len=0x09"); break;
				case 13: printf("Get Alarm response Len=13"); break;
				default: printf("View Group response Len=var");
			}
			break;
		case 0x1C02 : 
			switch (dLen)
			{
				case 0x10 : printf("Get Zone Information response Len=0x10"); break;
				case 9: printf("Remove Scene response Len=0x09"); break;
				default : printf("Other command %04x Len=%d", cmd, dLen);
			}
			break;
		case 0x1C03 : 
			switch (dLen)
			{
				case 8: printf("Remove Group response Len=0x08"); break;
				case 7: printf("Get Zone Trouble Map response Len=0x07"); break;
				default : printf("Other command %04x Len=%d", cmd, dLen);
			}
			break;
		case 0x1C04 : 
			switch (dLen)
			{
				case 9: printf("Store Scene response Len=0x09"); 
				case 8: printf("Read Heart Beat Period response Len=8");
				default : printf("Other command %04x Len=%d", cmd, dLen);
			}
			break;
		case 0x0C00  :
			switch (dLen)
			{
				case 7: printf("Arm     Len=0x07"); break;
				case 8: printf("Zone Eroll Rsp Len=0x08"); break;
				case 9: printf("Start Warning Len=0x09\t"); break;
			}
			break;
		case 0x0C01 : 
			switch (dLen)
			{
				case 7: printf("Squawk Len=0x07"); break;
				default : printf("Bypass Len=var");
			}
			break;
		case 0x0C02 : 
			switch (dLen)
			{
				case 7: printf("Zone Un-Eroll Request Len=0x07"); break;
				case 6: printf("Emergency Len=0x06\t"); break;
				default : printf("Other command %04x Len=%d", cmd, dLen);
			}
			break;
		case 0x0C03 : printf("Fire Len=0x06"); break;
		case 0x0C04 : 
			switch (dLen)
			{
				case 6: printf("Panic Len=0x06"); break;
				case 9: printf("Store Scene Len=0x09"); break;
				default : printf("Other command %04x Len=%d", cmd, dLen);
			}
			break;
		case 0x0C05 : printf("Get Zone ID Map Len=0x06");
		case 0x0C06 : printf("Get Zone Information Len=0x07 "); break;
		case 0x0C07 : printf("Un-Bypass   Len=var"); break;
		case 0x0C09 : printf("Get Zone Trouble Map Len=0x06"); break;
		case 0x0C0A : printf("Read Heart Beat period Len=0x06"); break;
		case 0x1C05 : 
			switch (dLen)
			{
				case 25: printf("Alarm Device Notify Len=25"); break;
				default : printf("Blind Node Data response Len=var"); 
			}
			break;
		case 0x0C0B : printf("Get Recent Alarm Information Len=0x06"); break;
		case 0x1C06 : printf("Get Recent Alarm Information response   Len=Var"); break;
		case 0x0C0C : printf("Set Day-Night Type Len=0x08"); break;
		case 0x0C0D : printf("Get Arm Mode Len=0x06"); break;
		case 0x1C08 : printf("Get Arm Mode response Len=Var"); break;
		case 0x0D01 : printf("Read Attribute response Len=var"); break;
		case 0x0D06 : printf("Configure reporting Len=var"); break;
		case 0x0D0A : printf("Report attributes Len=var"); break;
		default : printf("Other command %04X Len=%d", cmd, dLen);
	}
}

/*! \brief Get value of command
	\param sData	data buffer
 */
int getCmd(char * cBuffer)
{
	char CmdStr[8];
	long Cmd;
	
	strncpy(CmdStr, &cBuffer[2], 4);
	CmdStr[4] = 0;
	Cmd = strtol(CmdStr, NULL, 16);
	
	return Cmd;
}

/*! \brief CMD (Command ID)
	\param sData	data buffer

This is a two byte field (MSB transmitted first) with a value denoting the Command
Identification (ID) for this message.
The Command ID field is a 16 bit field broken up in to the following 2 sub fields:
Command Type is a most significant 4 bits of the CMD field and is used to route messages. Each bit (3-0) of
this sub field is defined as:
	Bit 12 – Response Bit – if set, this bit indicates a response message (each response message is
		described below).
	Bit 13 ~ 15 - Reserved
		Command Number is the least significant 12 bits of the CMD field. The unique number range of this field is
		broken down into the following:
		0x000 – 0x07F System Commands (RAM R/W, Debug, Trace, etc.)
		0x080 – 0x0FF MAC Interface Commands
		0x100 – 0x3FF NWK Interface Commands
		0x400 – 0x4FF PHY Interface Commands
		0x500 – 0x5FF SPI Interface Commands
		0x600 – 0x6FF Sequence Interface Commands
		0x800 – 0x8FF APS Interface Commands
		0x900 – 0x9FF AF Interface Commands
		0xA00 – 0xAFF ZDO Interface Commands
		0xB00 – 0xBFF Device Interface Commands
		0xC00 – 0xCFF ZCL Interface Commands
		0xD00 – 0xDFF Attribute Interface Commands
*/
void CommandId(char * cmdBuf)
{
	char cmdStr[8];
	int cmd;
	
	strncpy(cmdStr, cmdBuf, 4);
	cmdStr[4] = 0;
	cmd = strtol(cmdStr, NULL, 16);
	
	if (cmd & 0x1000)
		printf("RSP %04x ", cmd);
	else
		printf("req %04x ", cmd);
	cmd &= 0x0fff;
	if (cmd >= 0xD00) printf("Att");
	else if (cmd >= 0xC00) printf("ZCL");
	else if (cmd >= 0xB00) printf("Dev");
	else if (cmd >= 0xA00) printf("ZDO");
	else if (cmd >= 0x900) printf("AF");
	else if (cmd >= 0x800) printf("APS");
	else if (cmd >= 0x600) printf("Seq");
	else if (cmd >= 0x500) printf("SPI");
	else if (cmd >= 0x400) printf("PHY");
	else if (cmd >= 0x100) printf("NWK");
	else if (cmd >= 0x080) printf("MAC");
	else printf("Sys");
	printf(", ");
}

/*! \brief Address Mode 
	\param sData	data buffer
	\return address mode value

indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
GroupAddress (Addrmode = 0x01),IEEE Address Addrmode = 0x03).
*/
int AddrMode(char * buf)
{
	int addrMode;
	char valStr[8];
	
	printf("AddrMode ");

	snprintf(valStr, sizeof(valStr), "%c%c", buf[0], buf[1]);
	addrMode = strtol(valStr, NULL, 16);
	switch (addrMode) 
	{
		case 0 : printf("No Address"); break;
		case 1 : printf("Group Address"); break;
		case 2 : printf("Short Address"); break;
		case 3 : printf("IEEE Address"); break;
		case 0xF : printf("Broadcast Address"); break;
		default : printf("Reserved"); break;
	}
	
	printf(" %c%c, ", buf[0], buf[1]);
	return addrMode;
}

/*! \brief Data type 
	\param	dtStr	data string
	\param	nan	not-a-number
	\param	dtype	data type
	\return	length of data in bytes
 */
int dataType(char * dtStr, 
	     char * nan, 
	     int * dtype)
{
	int rval = 0;
	int dtVal;
	char dtValStr[8];
	
	strncpy(dtValStr, dtStr, 2);
	dtValStr[2] = 0;
	dtVal = strtol(dtValStr, NULL, 16);

	printf("DataType ");
	switch(dtVal)
	{
		case 0x00 : printf("No data "); rval = 0; break;
		case 0x08 : printf("8-bit data "); rval = 1; break;
		case 0x09 : printf("16-bit data "); rval = 2; break;
		case 0x0a : printf("24-bit data "); rval = 3; break;
		case 0x0b : printf("32-bit data "); rval = 4; break;
		case 0x10 : printf("Boolean "); rval = 1; strcpy(nan, "0xff"); break;
		case 0x18 : printf("8-bit bitmap "); rval = 1; break;
		case 0x19 : printf("16-bit bitmap "); rval = 2; break;
		case 0x1a : printf("24-bit bitmap "); rval = 3; break;
		case 0x1b : printf("32-bit bitmap "); rval = 4; break;
		case 0x20 : printf("Unsigned 8-bit integer "); rval = 1; strcpy(nan, "0xff"); break;
		case 0x21 : printf("Unsigned 16-bit integer "); rval = 2; strcpy(nan, "0xffff"); break;
		case 0x22 : printf("Unsigned 24-bit integer "); rval = 3; strcpy(nan, "0xffffff"); break;
		case 0x23 : printf("Unsigned 32-bit integer "); rval = 4; strcpy(nan, "0xffffffff"); break;
		case 0x28 : printf("Signed 8-bit integer "); rval = 1; strcpy(nan, "0x80"); break;
		case 0x29 : printf("Signed 16-bit integer "); rval = 2; strcpy(nan, "0x8000"); break;
		case 0x2a : printf("Signed 24-bit integer "); rval = 3; strcpy(nan, "0x800000"); break;
		case 0x2b : printf("Signed 32-bit integer "); rval = 4; strcpy(nan, "0x80000000"); break;
		case 0x30 : printf("8-bit enumeration "); rval = 1; strcpy(nan, "0xff"); break;
		case 0x31 : printf("16-bit enumeration "); rval = 2; strcpy(nan, "0xffff"); break;
		case 0x38 : printf("Semi-precision "); rval = 2; break; 	/* Not a Number */
		case 0x39 : printf("Single precision "); rval = 4; break; 	/* Not a Number */
		case 0x3a : printf("Double precision "); rval = 8; break; 	/* Not a Number */
		case 0x41 :
			printf("Octet string "); 
			/* Defined in first octet 0xff in first octet */
			strncpy(dtValStr, &dtStr[2], 2);
			dtValStr[2] = 0;
			rval = strtol(dtValStr, NULL, 16);
			break;
		case 0x42 :
			printf("Character string ");
			/* Defined in first octet 0xff in first octet */
			strncpy(dtValStr, &dtStr[2], 2);
			dtValStr[2] = 0;
			rval = strtol(dtValStr, NULL, 16);
			break;
		case 0xe0 : printf("Time of day "); rval = 4; strcpy(nan, "0xffffffff"); break;
		case 0xe1 : printf("Date "); rval = 4; strcpy(nan, "0xffffffff"); break;
		case 0xe8 : printf("Cluster ID "); rval = 2; strcpy(nan, "0xffff"); break;
		case 0xe9 : printf("Attribute ID "); rval = 2; strcpy(nan, "0xffff"); break;
		case 0xea : printf("BACnet OID "); rval = 4; strcpy(nan, "0xffffffff"); break;
		case 0xf0 : printf("IEEE address "); rval = -8; strcpy(nan, "0xffffffffffffffff"); break;
		case 0xff : printf("Unknown "); rval = 0; break;
		default : printf("Reserved 0x%02x", dtVal); rval = -1;
	}
	*dtype = dtVal;
	return rval;
}

/*! \brief network address of device being identified
	\param 	buf		Zigbee data
	\param	addrMode	Address mode
*/
void SrcAddr(char * buf, 
	     int addrMode)
{
	int i;
	switch(addrMode)
	{
		case 1 :  /* 16 bits GroupAddress */
			printf("SrcAddr ");
			for (i=0; i<4; i++) printf("%c", buf[i]);
			break;
		case 2 :  /* 16 bits ShortAddress */
			printf("SrcAddr ");
			for (i=0; i<4; i++) printf("%c", buf[i]);
			break;
		case 3 : /* IEEE Address */
			printf("SrcAddr ");
			for (i=0; i<16; i++) printf("%c", buf[i]);
			break;
		default :
			printf("SrcAddr Unknown address mode");
			return;
	}
	printf(", ");
}

/*! \brief short network address of device 
 */
void ShortAddr(char * buf)
{
	printf("ShortAddr %c%c%c%c, ", buf[0], buf[1], buf[2], buf[3]);
}

/*! \brief source EndPoint. represents the application endpoint the data. 
 */
void SrcEndpoint(char * buf)
{	
	printf("SrcEndpoint %c%c, ", buf[0], buf[1]);
}

/*! \brief source EndPoint. represents the application endpoint the data. 
 */
int GetSrcEndpoint(char * buf)
{
	int epVal;
	char epValStr[8];

	strncpy(epValStr, buf, 2);
	epValStr[2] = 0;
	epVal = strtol(epValStr, NULL, 16);

	if (trace) printf("\tSrcEndpoint %02x\n", epVal);
	return epVal;
}

/*! \brief Destination address 
 */
void DstAddr(char * buf)
{
	printf("DstAddr %c%c%c%c, ", buf[0], buf[1], buf[2], buf[3]);
}

/*! \brief	Display status
 */
int Status(char * status)
{
	char statusStr[8];
	int st;
	
	printf("Status %c%c ", status[0], status[1]);
	if (isxdigit(status[0]) && isxdigit(status[1]))
	{
		sprintf(statusStr, "%c%c", status[0], status[1]);
		st = strtol(statusStr, NULL, 16);
		if (st) dispStatus(st);
		else printf("OK");
	}
	else
	{
		printf("Invalid");
		st = -1;
	}
	printf(", ");
	
	return st;
}

/*! \brief	Get IEEE address in reverse byte order
	\param	buf	input buffer
	\param	ieeeBuf	output buffer
 */
void readIeeeAddr(char * buf,
		  char * ieeeBuf)
{
	int i, j=0;

	for (i=14; i>=0; i-=2) 
	{
		ieeeBuf[j++] = buf[i];
		ieeeBuf[j++] = buf[i+1];
	}
	ieeeBuf[j] = 0;
}

/*! \brief IEEE address 
	\param	buf	input buffer
	\param	reverse	flag to indicate reverse byte order
 */
void dispIeeeAddr(char * buf,
		  int reverse)
{
	int i;

	printf("IEEEaddr ");
	if (reverse != 0)
		for (i=14; i>=0; i-=2) printf("%c%c", buf[i], buf[i+1]);
	else
		for (i=0; i<16; i++) printf("%c", buf[i]);
	printf(", ");
}

/*! \brief Display cluster name and description
	\param	buf	input buffer
 */
void ClusterId(char * buf)
{
	int cid;
	char cidStr[8];
	
	printf("ClusterId ");
	
	sprintf(cidStr, "%c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
	cid = strtol(cidStr, NULL, 16);
	
	switch(cid)
	{
		case 0x0000 : 
			/* Attributes for determining basic information about a device, setting user device information such as location, and enabling a device. */
			printf("Basic");
			break;
		case 0x0001 : 
			/* Attributes for determining more detailed information about a device’s power source(s), and for configuring under/over voltage alarms. */
			printf("Power configuration");
			break;
		case 0x0002 : 
			/* Attributes for determining information about a device’s internal temperature, and for configuring under/over temperature alarms. */
			printf("Device Temperature Configuration");
			break;
		case 0x0003 :
			/* Attributes and commands for putting a device into Identification mode (e.g. flashing a light) */
			printf("Identify");
			break;
		case 0x0004 : 
			/* Attributes and commands for group configuration and manipulation */
			printf("Groups");
			break;
		case 0x0005 : 
			/* Attributes and commands for scene configuration and manipulation. */
			printf("Scenes");
			break;
		case 0x0006 : 
			/* Attributes and commands for switching devices between ‘On’ and ‘Off’ states. */
			printf("On/off");
			break;
		case 0x0007 : 
			/* Attributes and commands for configuring On/Off switching devices */
			printf("On/off Switch Configuration");
			break;
		case 0x0008 : 
			/* Attributes and commands for controlling devices that can be set to a level between fully ‘On’ and fully ‘Off’. */
			printf("Level Control");
			break;
		case 0x0009 : 
			/* Attributes and commands for sending notifications and configuring alarm functionality. */
			printf("Alarms");
			break;
		case 0x000a : 
			/* Attributes and commands that provide a basic interface to a real-time clock. */
			printf("Time");
			break;
		case 0x000b :
			/* Attributes and commands that provide a means for exchanging location information and channel parameters among devices. */
			printf("RSSI Location");
			break;
		case 0x0100 :
			/* Attributes and commands for configuring a shade. */
			printf("Shade Configuration");
			break;
		case 0x0200 : 
			/* An interface for configuring and controlling pumps.and Control */
			printf("Pump Configuration"); 
			break; 
		case 0x0201 : 
			/* An interface for configuring and controlling the functionality of a thermostat. */
			printf("Thermostat"); 
			break;     
		case 0x0202 :
			/* An interface for controlling a fan in a heating / cooling system. */
			printf("Fan Control"); 
			break;
		case 0x0203 :
			/* An interface for controlling dehumidification. */
			printf("Dehumidification Control"); 
			break;
		case 0x0204 :
			/* An interface for configuring the user interface of a thermostat (which may be remote from the thermostat) */
			printf("Thermostat User"); 
			break;
		case 0x0300 :
			/* Attributes and commands for controlling the hue and saturation of a color-capable light */
			printf("Color control"); 
			break;
		case 0x0301 :
			/* Attributes and commands for configuring a lighting ballast */
			printf("Ballast"); 
			break;
		case 0x0400 : 
			/* Attributes and commands for configuring the measurement of
			   illuminance, and reporting illuminance measurements. */
			printf("Illuminance");
			break;
		case 0x0401 : 
			/* Attributes and commands for configuring the sensing of illuminance
			   levels, and reporting whether illuminance is above, below, or on target. */
			printf("Illuminance level");
			break;
		case 0x0402 : 
			/* Attributes and commands for configuring the measurement of
			   temperature, and reporting temperature measurements. */
			printf("Temperature");
			break;
		case 0x0403 : 
			/* Attributes and commands for configuring the measurement of
			   pressure, and reporting pressure measurements. */
			printf("Pressure");
			break;
		case 0x0404 : 
			/* Attributes and commands for configuring the measurement of flow,
			   and reporting flow rates. */
			printf("Flow");
			break;
		case 0x0405 : 
			/* Attributes and commands for configuring measurement humidity, and
			   reporting humidity status. */
			printf("Humidity");
			break;
		case 0x0406 : 
			/* Attributes and commands for configuring occupancy the of sensing,
			   and reporting occupancy measurements. */
			printf("Occupancy sensing");
			break;
		case 0x0500 :
			/* IAS Zone Attributes and commands for IAS security zone devices. */
			printf("IAS security zone device");
			break;
		case 0x0501 :
			/* IAS ACE Attributes and commands for IAS Ancillary Control Equipment. */
			printf("IAS Ancillary Control Equipment");
			break;
		case 0x0502 :
			/* IAS WD Attributes and commands for IAS Warning Devices. */
			printf("IAS Warning Devices");
			break;
		case 0x0600 :
			/* Attributes and commands for interfacing to BACnet */
			printf("BACnet interface");
			break;
		/* Smart Energy */
		case 0x0700 : 
			printf("Price"); 
			break;
		case 0x0701 :
			printf("Demand Response and Load Control");
			break;
		case 0x0702 :	
			printf("Simple Metering");
			break;
		case 0x0703 :
			printf("Message");
			break;
		case 0x0704 :
			printf("Smart Energy Tunneling (Complex Metering)");
			break;
		case 0x0705 :
			printf("Pre-Payment");
			break;
		default : 
			printf("Reserved");
	}
	printf(" (0x%04x), ", cid);
}

/*! \brief (reverse) ID of Group to be added 
	\param	buf	input buffer
 */
void GroupID(char * idBuf)
{
	printf("GroupID %c%c%c%c, ", idBuf[2], idBuf[3], idBuf[0], idBuf[1]);
}

/*! \brief Destination EndPoint 
	\param	buf	input buffer
 */
void DstEndpoint(char * idBuf)
{
	printf("DstEndpoint %c%c, ", idBuf[0], idBuf[1]);
}

/*! \brief EndPoint 
	\param	buf	input buffer
 */
void Endpoint(char * idBuf)
{
	printf("Endpoint %c%c, ", idBuf[0], idBuf[1]);
}

/*! \brief Request Type
	\param	buf	input buffer

– byte – following options:
		0	→      Single device response
		1	→      Extended – include associated devices
*/
void ReqType(char * dtStr)
{
	int dtVal;
	char dtValStr[8];
	
	strncpy(dtValStr, dtStr, 2);
	dtValStr[2] = 0;
	dtVal = strtol(dtValStr, NULL, 16);
	printf("ReqType ");
	switch (dtVal)
	{
		case 0 : printf("Single device"); break;
		case 1 : printf("Extended"); break;
		default : printf("Unknown");
	}
	printf(" %02x, ", dtVal);
}

/*! \brief Warning Mode 
	\param	buf	input buffer
 */
void WarningMode(char * wBuff)
{
	printf("WarningMode ");
	if (wBuff[0] == '0')
	{
		switch(wBuff[1])
		{
			case '0' : printf("No Warning"); break;
			case '1' : printf("Burglar"); break;
			case '2' : printf("Fire"); break;
			case '3' : printf("Emergency"); break;
			case 'E' : printf("Device Trouble"); break;
			case 'F' : printf("DoorBell"); break;
			default  : printf("Reserved 0%c", wBuff[1]);
		}
		printf(", ");
	}
	else
		printf(" Reserved %c%c, ", wBuff[0], wBuff[1]);
}

/*! \brief Zone ID 
	\param	buf	input buffer
 */
void ZoneID(char * zBuff)
{
	printf("ZoneID %c%c, ", zBuff[0], zBuff[1]);
}

/*! \brief Zone Type 
 */
void ZoneType(char * zBuff)
{
	printf("ZoneType %c%c%c%c, ", zBuff[2], zBuff[3], zBuff[0], zBuff[1]);
}

/*! \brief Zone status 
	\param	buf	input buffer
 */
void ZoneStatus(char * zBuff)
{
	printf("ZoneStatus %c%c%c%c, ", zBuff[2], zBuff[3], zBuff[0], zBuff[1]);
}

/*! \brief Extended Zone status 
	\param	buf	input buffer
 */
void ExtendedStatus(char * zBuff)
{
	printf("ExtendedStatus %c%c, ", zBuff[0], zBuff[1]);
}

/*! \brief Networf address of interest 
	\param	buf	input buffer
 */
void NWKAddrOfInterest(char * nBuff)
{
	printf("NWKAddrOfInterest %c%c%c%c, ", nBuff[0], nBuff[1], nBuff[2], nBuff[3]);
}

/*! \brief Security suite
	\param	buf	input buffer
 */
void SecuritySuite(char * nBuff)
{
	printf("SecuritySuite %c%c, ", nBuff[0], nBuff[1]);
}

/*! \brief Zone ID Map
	\param	buf	input buffer
 */
void ZoneIdMap(char * zBuff)
{
	printf("ZoneIDMap %c%c, ", zBuff[0], zBuff[1]);
}
