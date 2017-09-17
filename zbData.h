/*! \file zbData.h
	\brief Utilities used to display Zigbee data packets
 */

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbData.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.3  2011/02/10 11:09:43  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:16  johan
 * Add CVS stuff
 *
 *
 *
 */


#ifndef ZBDATA_H
#define ZBDATA_H

#define SUCCESS 0x00
#define FAILURE  0x01
#define NOT_AUTHORIZED  0x7e
#define RESERVED_FIELD_NOT_ZERO   0x7f
#define MALFORMED_COMMAND  0x80
#define UNSUP_CLUSTER_COMMAND  0x81
#define UNSUP_GENERAL_COMMAND  0x82
#define UNSUP_MANUF_CLUSTER_COMMAND  0x83
#define UNSUP_MANUF_GENERAL_COMMAND  0x84
#define INVALID_FIELD  0x85
#define UNSUPPORTED_ATTRIBUTE  0x86
#define INVALID_VALUE  0x87
#define READ_ONLY  0x88
#define INSUFFICIENT_SPACE  0x89
#define DUPLICATE_EXISTS  0x8a
#define NOT_FOUND  0x8b
#define UNREPORTABLE_ATTRIBUTE  0x8c
#define INVALID_DATA_TYPE  0x8d
#define INVALID_SELECTOR  0x8e
#define WRITE_ONLY  0x8f
#define INCONSISTENT_STARTUP_STATE  0x90
#define DEFINED_OUT_OF_BAND  0x91
#define HARDWARE_FAILURE  0xc0
#define SOFTWARE_FAILURE  0xc1
#define CALIBRATION_ERROR  0xc2

int AddrMode(char * buf);
void ArmMode(char * aBuff);
int AttReportList(char * cBuffer);

void CommandId(char * cmdBuf);
void ClusterId(char * buf);

void GroupID(char * idBuf);

int dataType(char * dtStr, char * nan, int * dtype);
void dispStatus(int status);
void DstAddr(char * buf);
void DstEndpoint(char * idBuf);
void DateTimeStampFile(FILE * outf);
char * DateTimeStampStr(int ats);

void Endpoint(char * idBuf);
void ExtendedStatus(char * zBuff);

int getArmMode(char * amStr);
int getCmd(char * cBuffer);
int getStatus(char *statStr);
void getDateTimeStamp(time_t *ts, char * buff, int bsize);
int GetSrcEndpoint(char * buf);

void NWKAddrOfInterest(char * nBuff);

void ReqType(char * dtStr);

void SecuritySuite(char * nBuff);
void SrcAddr(char * buf, int addrMode);
void SrcEndpoint(char * buf);
void ShortAddr(char * buf);

void WarningMode(char * wBuff);

void ZoneID(char * zBuff);
void ZoneIdMap(char * zBuff);
void ZoneStatus(char * zBuff);
void ZoneType(char * zBuff);

#endif
