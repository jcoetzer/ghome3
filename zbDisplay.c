/*! \file zbDisplay.c 
	\brief Display Zigbee data packets

 References below are from Netvox document 'Home Automation Profile Command' Version 3.0, Jan, 2009 

*/

/*
 * $Author: johan $
 * $Date: 2011/06/13 21:57:26 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: zbDisplay.c,v $
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

static void LqiList(char * lqiBuff, int version);
static void LqiListItem(char * lqiBuff, int version);

extern int verbose;
extern int trace;

/*! \brief 2.1 System Ping request:
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0007     Len=0x00
    	PING requests to verify if a device is active and check the capability of the device.
*/
static void SystemPing(char * cBuffer, 
		       int * ddata)
{
	printf("System Ping request: ");
	*ddata = 0;
}

/*! \brief 2.2 System Ping response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1007     Len=0x02      Capabilities
	Capabilities –2 byte – represents the interfaces that this device can handle (compiled into the device).
	Bit weighted and defined as:
		MT_CAP_SYS                     →    0x0001
		MT_CAP_MAC                     →    0x0002
		MT_CAP_NWK                     →    0x0004
		MT_CAP_AF                      →    0x0008
		MT_CAP_ZDO                     →    0x0010
		MT_CAP_SAPI                    →    0x0020
		MT_CAP_UTIL                    →    0x0040
		MT_CAP_DEBUG                   →    0x0080
		MT_CAP_APP                     →    0x0100
		MT_CAP_ZOAD                    →    0x1000
*/
static void SystemPingRsp(char * cBuffer, 
			  int * ddata)
{
	int c;
	char numStr[8];

	printf("System Ping response: ");
	strncpy(numStr, &cBuffer[8], 4);
	numStr[4] = 0;
	c = strtol(numStr, NULL, 16);
	printf("Capabilities %04x", c);
	if (c & 0x0001) printf(" MT_CAP_SYS");
	if (c & 0x0002) printf(" MT_CAP_MAC");
	if (c & 0x0004) printf(" MT_CAP_NWK");
	if (c & 0x0008) printf(" MT_CAP_AF");
	if (c & 0x0010) printf(" MT_CAP_ZDO");
	if (c & 0x0020) printf(" MT_CAP_SAPI");
	if (c & 0x0040) printf(" MT_CAP_UTIL");
	if (c & 0x0080) printf(" MT_CAP_DEBUG");
	if (c & 0x0100) printf(" MT_CAP_APP");
	if (c & 0x1000) printf(" MT_CAP_ZOAD");
	printf(", ");
	*ddata = 0;
}

/*! \brief 2.12 System Get Device Information Rsp
	\param	buf	input buffer
	\param	ddata	data display flag

CMD = 0x1014 Len = var Status IEEEAddr ShortAddress DeviceType DeviceState | NumAssocDevices | AssocDevicesList

	Status is a one byte field and is either success(0) or fail(1). The fail status is returned if the address value in the
		command message was not within the valid range.
	IEEEAddress is an 8 byte field.
	ShortAddress is a 2 byte field.
	DeviceType is a 1 byte field indicating device type, where bits 0 to 2 indicate the capability for the device to
		operate as a coordinator, router, or end device, respectively.
	DeviceState is a 1 byte field indicating the state of the device with different possible states as shown below:
		Description 						Value
		-----------						-----
		Initialized - not started automatically 		0x00
		Initialized - not connected to anything 		0x01
		Discovering PAN's to join 				0x02
		Joining a PAN 						0x03
		ReJoining a PAN, only for end devices 			0x04
		Joined but not yet authenticated by trust center 	0x05
		Started as device after authentication 			0x06
		Device joined, authenticated and is a router 		0x07
		Starting as Zigbee Coordinator 				0x08
		Started as Zigbee Coordinator 				0x09
		Device has lost information about its parent 		0x0A
	NumAssocDevices is a 1 byte field specifying the number of devices being associated to the target device.
	AssocDevicesList is a variable length array of 16-bits specifying the network address(es) of device(s)
		associated to the target device.
*/
static void SysDevInfoRsp(char * cBuffer, 
			  int * ddata)
{
	int devState;
	char devStateStr[8];
	int i, num;
	char numStr[8];

	printf("System Get Device Information Rsp: ");

	Status(&cBuffer[8]);

	dispIeeeAddr(&cBuffer[10], 0);
	
	ShortAddr(&cBuffer[26]);

	/* device type */
	snprintf(devStateStr, sizeof(devStateStr), "%c%c", cBuffer[32], cBuffer[33]);
	devState = strtol(devStateStr, NULL, 16);
	printf(" DeviceType ");
	if (devState & 0x080) printf("coordinator, ");
	else if (devState & 0x040) printf("router, ");
	else if (devState & 0x020) printf("end device, ");
	else printf("%c%c, ", cBuffer[30], cBuffer[31]);
	
	snprintf(devStateStr, sizeof(devStateStr), "%c%c", cBuffer[32], cBuffer[33]);
	devState = strtol(devStateStr, NULL, 16);
	printf(" DeviceState %02x (", devState);
	switch(devState)
	{
		case 0x00 : printf("Initialized - not started automatically"); break;
		case 0x01 : printf("Initialized - not connected to anything"); break;
		case 0x02 : printf("Discovering PAN's to join"); break;
		case 0x03 : printf("Joining a PAN"); break;
		case 0x04 : printf("ReJoining a PAN, only for end devices"); break;
		case 0x05 : printf("Joined but not yet authenticated by trust center"); break;
		case 0x06 : printf("Started as device after authentication"); break;
		case 0x07 : printf("Device joined, authenticated and is a router"); break;
		case 0x08 : printf("Starting as Zigbee Coordinator"); break;
		case 0x09 : printf("Started as Zigbee Coordinator"); break;
		case 0x0A : printf("Device has lost information about its parent"); break;
		default : printf("Unknown device state");
	}
	sprintf(numStr, "%c%c, ", cBuffer[34], cBuffer[35]);
	num = strtol(numStr, NULL, 16);
	printf(") NumAssocDevices %d, ", num);
	for (i=0; i<num; i++)
	{
		printf("AssocDevice%d %c%c%c%c, ", i+1, cBuffer[36+i*4], cBuffer[37+i*4], cBuffer[38+i*4], cBuffer[39+i*4]);
	}
	*ddata = 0;
}

/*! \brief 3.1 Network Address Request
	\param	buf	input buffer
	\param	ddata	data display flag

CMD = 0x0A02         Len = 0x0C      AddrMode         IEEEAddr       ReqType       StartIndex     SecuritySuite
      
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
	GroupAddress (Addrmode = 0x01),IEEE Address Addrmode = 0x03).
	IEEEAddr(reverse)– 64 bits – IEEE address of the destination device.
	ReqType – byte – following options:
		0	→      Single device response
		1	→      Extended – include associated devices
	StartIndex – 1 byte – Starting index into the list of children. This is used to get more of the list if the list is too
		large for one message
	SecuritySuite - 1 byte – Security options
*/
static void NetworkAddress(char * cBuffer, 
			   int * ddata)
{
	
	printf("Network Address Request: ");
	AddrMode(&cBuffer[8]);
	dispIeeeAddr(&cBuffer[10], 1);
	ReqType(&cBuffer[26]);
	printf("ReqType %c%c, ", cBuffer[26], cBuffer[27]);
	printf("StartIndex %c%c, ", cBuffer[28], cBuffer[29]);
	printf("SecuritySuite %c%c, ", cBuffer[30], cBuffer[31]);
	*ddata = 0;
}

/*! \brief 3.2 Network Address Respose
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A80     Len=var   AddrMode    SrcAddr    Status  IEEEAddr     nwkAddr   NumAssocDev      StartIndex AssocDevList LQI

	AddrMode – byte – indicates that the SrcAddr is either 16 bits (2) or 64 bits (3)
	SrcAddr –16 bit or 64 bits – Source address, size is dependent on SrcAddrMode. when Addrmode is 2,the
		SrcAddr is 2 bytes and have ‘00’(6 bytes) before SrcAddr.( 00 00 00 00 00 00 SrcAddr(2 bytes))e
	Status – byte – this field indicates either SUCCESS or FAILURE.
	IEEEAddr – 8 bytes – 64 bit IEEE address of source device
	nwkAddr – 16 bit – short network address of responding device
	NumAssocDev – byte – the number of associated devices
	StartIndex – byte - Starting index into the list of associated devices for this report.
	AssocDevList – array of 16 bit short addresses – list of network address for associated devices. This list can
		be a partial list if the entire list doesn’t fit into a packet. If it is a partial list, the starting index is StartIndex.
	LQI -byte- Sender Link Quality
*/
static void NetworkAddressRsp(char * cBuffer, 
			      int * ddata)
{
	int i, amod, numdev;
	char numStr[8];

	printf("Network Address response: ");
	amod = AddrMode(&cBuffer[8]);
	if (amod == 2)
		SrcAddr(&cBuffer[22], 2);
	else
		dispIeeeAddr(&cBuffer[10], 0);
	Status(&cBuffer[26]);
	dispIeeeAddr(&cBuffer[28], 0);
	printf("nwkAddr %c%c%c%c, ", cBuffer[44], cBuffer[45], cBuffer[46], cBuffer[47]);
	strncpy(numStr, &cBuffer[48], 2);
	numStr[2] = 0;
	numdev = strtol(numStr, NULL, 16);
	printf("NumAssocDev %02x, ", numdev);
	printf("StartIndex %c%c, ", cBuffer[50], cBuffer[51]);
	/* add the rest */
	for (i=0; i<numdev; i++)
		dispIeeeAddr(&cBuffer[52+(i-1)*16], 0);
	*ddata = 0;
}

/*! \brief 3.3 IEEE Request request a device’s IEEE 64-bit address, include associated devices (ShortAddress)
	\param	buf	input buffer
	\param	ddata	data display flag

CMD = 0x0A03 Len = 0x06 AddrMode DstAddr ReqType StartIndex SecuritySuite
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
	GroupAddress (Addrmode = 0x01)
	DstAddr– 16 bits – short address of the destination device.
	ReqType – byte – following options:
		0 → Single device response
		1 → Extended – include associated devices
	StartIndex – 1 byte – Starting index into the list of children. This is used to get more of the list if the list is too
		largefor one message
	SecuritySuite - 1 byte – Security options
*/
static void IeeeRequest(char * cBuffer, 
			int * ddata)
{
	int addrMode;

	printf("IEEE Request: "); 

	addrMode = AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);

	if (strncmp(&cBuffer[14], "00", 2) == 0) printf("ReqType Single, ");
	else if (strncmp(&cBuffer[14], "01", 2) == 0) printf("ReqType Extended, ");
	else printf("ReqType %c%c, ", cBuffer[14], cBuffer[15]);

	printf("StartIndex %c%c, ", cBuffer[16], cBuffer[17]);
	printf("SecuritySuite %c%c, ", cBuffer[18], cBuffer[19]);
	*ddata = 0;
}

/*! \brief 3.4 IEEE response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A81 Len=0x24 AddrMode SrcAddr Status IEEEAddr NumAssocDev StartIndex AssocDevList
	AddrMode – byte – indicates that the SrcAddr is either 16 bits (Addrmode = 2) or 64 bits (Addrmode = 3)
	SrcAddr – 16 bit or 64 bits – Source address, size is dependent on SrcAddrMode. when Addrmode is 2,the
		SrcAddr is 2 bytes and have ‘00’(4 bytes) before SrcAddr.( 00 00 00 00 SrcAddr(2 bytes))
	Status – byte – this field indicates either SUCCESS(0x00) or FAILURE(0x01).
	IEEEAddr – 8 bytes – 64 bit IEEE address of source device
	NumAssocDev – byte – the number of associated devices
	StartIndex – byte - Starting index into the list of associated devices for this report
	AssocDevList – array of 16 bit short addresses – list of network address for associated devices. This list can
		be a partial list if the entire list doesn’t fit into a packet. If it is a partial list, the starting index is StartIndex.(Each
		times Max is 8 devices).
*/
static void IeeeResponse(char * cBuffer, 
			 int * ddata)
{
	int addrMode, i, n;
	char nStr[8];
	
	printf("IEEE response: "); 
	
	addrMode = AddrMode(&cBuffer[8]);
	switch(addrMode)
	{
		case 2 : SrcAddr(&cBuffer[22], 2); break;
		case 3 : SrcAddr(&cBuffer[10], 3); break;
		default : printf("SrcAddr Unknown address mode, "); return;
	}
	
	Status(&cBuffer[26]);
	dispIeeeAddr(&cBuffer[28], 0);
	sprintf(nStr, "%c%c, ", cBuffer[44], cBuffer[45]);
	n = strtol(nStr, NULL, 16);
	printf(" NumAssocDev %d, ", n);
	printf("StartIndex %c%c", cBuffer[46], cBuffer[47]);
	if (n) printf(", AssocDevList");
	for (i=0; i<n; i++)
	{
		printf(" %c%c%c%c", cBuffer[48+i*4], cBuffer[49+i*4], cBuffer[50+i*4], cBuffer[51+i*4]);
	}
	printf(", ");
	*ddata = 0;
}

/*! \brief 3.5 Node descriptor request
	\param	buf	input buffer
	\param	ddata	data display flag

    inquire about the Node Descriptor information of the destination device.
      Cmd=0x0A04           Len=0x07     AddrMode        DstAddr   NWKAddrOfIneterest          SecuritySuite
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes – NWK address of the device generating the inquiry.
	NWKAddrOfInterest –2 bytes - NWK address of the destination device being queried.
	SecuritySuite - 1 byte –. Security options.
*/
static void NodeDescriptor(char * cBuffer, 
			   int * ddata)
{
	printf("Node descriptor Request: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	NWKAddrOfInterest(&cBuffer[14]);
	printf("SecuritySuite %c%c, ", cBuffer[18], cBuffer[19]);
	*ddata = 0;
}

/*! \brief 3.6 Node descriptor response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A82            Len=21    Status     SrcAddr    NWKAddrOfIneterest   Logical Type    ComplexDescriptor Available
     	UserDescriptor Available APSFlags       Frequency Band    MACCapabilities Flags              Manufacturer Code
         MaxBuffer Size       MaxInTransfer Size   Server Mask  MaxOutTransferSize                 DescCapabilities

	Status – byte – this field indicates either SUCCESS or FAILURE.
	SrcAddr – 16 bit – the message’s source network address.
	NWKAddrOfInterest – 16 bits – Device’s short address of this Node descriptor
	LogicalType - 1 byte -
			Description                 Value
			ZigBee Coordinator          0
			ZigBee Router               1
			ZigBee End Device           2
			Resever                     3-7
	ComplexDescriptorAvailable – 1bytes- Indicates if complex descriptor is available for the node
	UserDescriptorAvailable – 1bytes- Indicates if User descriptor is available for the node
	APSFlags -1byte- Node Flags assigned for APS.For V1.0 all bits are reserved.
	FrequencyBand -1 byte- Identifies node frequency band capabilities
	MACCapabilities –1 byte – Capability flags stored for the MAC
			Description                          Value
			CAPINFO_DEVICETYPE_RFD               0x00
			CAPINFO_ALTPANCOORD                  0x01
			CAPINFO_DEVICETYPE_FFD               0x02
			CAPINFO_POWER_AC                     0x04
			CAPINFO_RCVR_ON_IDLE                 0x08
			CAPINFO_SECURITY_CAPABLE             0x40
			CAPINFO_ALLOC_ADDR                   0x80
	ManufacturerCode – 2 bytes – specifies a manufacturer code that is allocated by the ZigBee Alliance, relating
		to the manufacturer to the device.
	MaxBufferSize – byte - Indicates size of maximum NPDU. This field is used as a high level indication for
		management.
	MaxInTransferSize – 2 bytes – Indicates maximum size of Transfer up to 0x7fff (This field is reserved in
		version 1.0 and shall be set to zero ).
	ServerMask – 2 bytes – Specifies the system server capability. It is defined as follows:
		Bit Number         Assignment
		0                  Primary Trust Center
		1                  Backup Trust Center
		2                  Primary Binding Table Cache
		3                  Backup Binding Table Cache
		4                  Primary Discovery Cache
		5                  Backup Discovery Cache
		6– 15              Reserved
	MaxOutTransferSize -2bytes- Indicates maximum size of Transfer up to 0x7fff (This field is reserved in version
		1.0 and shall be set to zero).
	DescriptorCapabilities -1 byte- Specifies the Descriptor capabilities

*/
static void NodeDescriptorRsp(char * cBuffer, 
			      int * ddata)
{
	int st;

	printf("Node descriptor response: ");
	st = Status(&cBuffer[8]);
	SrcAddr(&cBuffer[10], 2);
	NWKAddrOfInterest(&cBuffer[14]);

	if (st) return;

	printf("LogicalType ");
	switch(cBuffer[19])
	{
		case '0' : printf("Coordinator, "); break;
		case '1' : printf("Router, "); break;
		case '2' : printf("End Device, "); break;
		case '3' :
		case '4' :
		case '5' :
		case '6' :
		case '7' :
			printf("Reserved, "); break;
		default :
			printf("Unknown logical type");
	}
	printf("ComplexDescriptorAvailable %c%c, ", cBuffer[20], cBuffer[21]);
	printf("UserDescriptorAvailable %c%c, ", cBuffer[22], cBuffer[23]);
	printf("APSFlags %c%c, ", cBuffer[24], cBuffer[25]);
	printf("FrequencyBand %c%c, ", cBuffer[26], cBuffer[27]);
	printf("MACCapabilities %c%c, ", cBuffer[28], cBuffer[29]);
	printf("ManufacturerCode %c%c%c%c, ", cBuffer[30], cBuffer[31], cBuffer[32], cBuffer[33]);
	printf("MaxBufferSize %c%c, ", cBuffer[34], cBuffer[35]);
	printf("MaxInTransferSize %c%c%c%c, ", cBuffer[36], cBuffer[37], cBuffer[38], cBuffer[39]);
	printf("ServerMask %c%c%c%c, ", cBuffer[40], cBuffer[41], cBuffer[42], cBuffer[43]);
	printf("MaxOutTransferSize %c%c%c%c, ", cBuffer[44], cBuffer[45], cBuffer[46], cBuffer[47]);
	printf("DescriptorCapabilities %c%c, ", cBuffer[48], cBuffer[49]);
	*ddata = 0;
}

/*! \brief 3.7 Power descriptor request
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A05      Len=0x07       AddrMode     DstAddr  NWKAddrOfIneterest   SecuritySuite
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes – NWK address of the device generating the inquiry.
	NWKAddrOfInterest –2 bytes - NWK address of the destination device being queried.
	SecuritySuite - 1 byte –. Security options.
*/
static void PowerDescriptor(char * cBuffer, 
			    int * ddata)
{
	printf("Power descriptor request: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	NWKAddrOfInterest(&cBuffer[14]);
	SecuritySuite(&cBuffer[18]);
	*ddata = 0;
}

/*! \brief 3.8 Power descriptor response
	\param	buf	input buffer
	\param	ddata	data display flag

      Cmd=0x0A83      Len=0x09       Status   SrcAddr   NWKAddrOfIneterest   CurrentPowe Mode
      AvailablePowerSources          CurrentPowerSource             CurrentPowerSourceLevel
      
	Status – byte – this field indicates either SUCCESS(0x00) or FAILURE(0x01).
	SrcAddr – 16 bit – the message’s source network address.
	NWKAddrOfInterest – 16 bits – Device’s short address that this response describes.
	CurrentPowerMode -1 byte - Current Power Mode;
		Value                 Description
		0x00                  Power permanently on
		0x01                  Power automatically comes on periodically
		0x02                  Power comes on when simulated
	AvailablePowerSources -1 byte - Available Power Sources;
		Value                 Description
		0x01                  Constant (Mains) power
		0x02                  Rechargeable Battery
		0x04                  Disposable Battery
	CurrentPowerSource -1 byte - Current Power Source;
		Value           Description
		0x01            Constant (Mains) power
		0x02            Rechargeable Battery
		0x04            Disposable Battery
	CurrentPowerSourceLevel -1 byte - Current Power Source Level;
		Value           Description
		0x00            Critical
		0x04            33%
		0x08            66%
		0x0C            100%

*/
static void PowerDescriptorRsp(char * cBuffer, 
			       int * ddata)
{
	*ddata = 0;
	printf("Power descriptor response: ");
	Status(&cBuffer[8]);
	SrcAddr(&cBuffer[10], 2);
	NWKAddrOfInterest(&cBuffer[14]);
	printf("CurrentPowerMode %c%c, ", cBuffer[18], cBuffer[19]);
	printf("AvailablePowerSources %c%c, ", cBuffer[20], cBuffer[21]);
	printf("CurrentPowerSource %c%c, ", cBuffer[22], cBuffer[23]);
	printf("CurrentPowerSourceLevel %c%c, ", cBuffer[24], cBuffer[25]);
}

/*! \brief 3.9 Simple descriptor request inquire as to the Simple Descriptor of the destination device’s EndPoint
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A06      Len=0x07       AddrMode     DstAddr  NWKAddrOfIneterest     EndPoint  SecuritySuite
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes – NWK address of the device generating the inquiry.
	NWKAddrOfInterest –2 bytes - NWK address of the destination device being queried.
	Endpoint – 1 byte – represents the application endpoint the data is from.
	SecuritySuite - 1 byte –. Security options.
*/
static void SimpleDescriptor(char * cBuffer, 
			     int * ddata)
{
	int addrMode;

	printf("Simple descriptor request: ");
	addrMode = AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	NWKAddrOfInterest(&cBuffer[14]);
	Endpoint(&cBuffer[18]);
	SecuritySuite(&cBuffer[20]);
	*ddata = 0;
}

/*! \brief 3.10 Simple descriptor response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A84     Len=var      Status   SrcAddr   NWKAddrOfIneterest        EndPoint      AppProfID  AppDevID
AppDevVer    AppFlags      AppInClusterCount    AppInClusterList  AppOutClusterCount    AppOutClusterList
	Status – byte – this field indicates either SUCCESS(0x00) or FAILURE(0x01).
	SrcAddr – 16 bit – the message’s source network address.
	NWKAddrOfInterest – 16 bits – Device’s short address that this response describes.
	Endpoint – 8 bits
	AppProfID – 16 bits – The profile ID for this endpoint.
	AppDevID – 16 bits – The Device Description ID for this endpoint.
	AppDevVer – 8 bits – Defined as the following format
		0              →       Version 1.00
		0x1~0xF        →       Reserved
	AppFlags – 8 bits – Defined as the following format
		0	→    Complex Descriptor Available Flag
		1~3       →    Reserved
	AppInClusterCount – byte – The number of input clusters in the AppInClusterList:
	AppInClusterList – byte array – List of input cluster IDs supported
	AppOutClusterCount – byte – The number of output clusters in the AppOutClusterList:
	AppOutClusterList – byte array – List of output cluster IDs supported
*/
static void SimpleDescriptorRsp(char * cBuffer, 
				int * ddata)
{
	int i, p, q;
	char nStr[8];

	*ddata = 0;
	printf("Simple descriptor response: ");
	p = Status(&cBuffer[8]);
	SrcAddr(&cBuffer[10], 2);
	NWKAddrOfInterest(&cBuffer[14]);
	printf("Endpoint %c%c, ", cBuffer[18], cBuffer[19]);
	if (p) return;

	printf("AppProfID %c%c%c%c, ", cBuffer[20], cBuffer[21], cBuffer[22], cBuffer[23]);
	printf("AppDevID %c%c%c%c, ", cBuffer[24], cBuffer[25], cBuffer[26], cBuffer[27]);
	printf("AppDevVer %c%c, ", cBuffer[28], cBuffer[29]);
	printf("AppFlags %c%c, ", cBuffer[30], cBuffer[31]);
	strncpy(nStr, &cBuffer[32], 2);
	nStr[2] = 0;
	p = strtol(nStr, NULL, 16);
	printf("AppInClusterCount %d, AppInClusterList ", p);
	for (i=0; i<p; i++) printf("%s%c%c", i?" ":"", cBuffer[34+i*2], cBuffer[35+i*2]);
	strncpy(nStr, &cBuffer[34+p*2], 2);
	nStr[2] = 0;
	q = strtol(nStr, NULL, 16);
	printf(", AppOutClusterCount %d, AppOutClusterList ", q);
	for (i=0; i<q; i++) printf("%s%c%c", i?" ":"", cBuffer[36+p*2+i*2], cBuffer[37+p*2+i*2]);
	printf(", ");
}

/*! \brief 3.11 Active EndPoint request request a list of active endpoint from the destination device.
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A07       Len=0x06   AddrMode    DstAddr    NWKAddrOfIneterest   SecuritySuite

	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 16 bit – NWK address of the device generating the request..
	NWKAddrOfInterest – 16 bit - NWK address of the destination device being queried.
	SecuritySuite - 1 byte – Security options.
*/
static void ActiveEndPoint(char * cBuffer, 
			   int * ddata)
{
	printf("Active EndPoint request: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	printf("NWKAddrOfInterest %c%c%c%c, ", cBuffer[14], cBuffer[15], cBuffer[16], cBuffer[17]);
	printf("SecuritySuite %c%c, ", cBuffer[18], cBuffer[19]);
	*ddata = 0;
}

/*! \brief 3.12 Active EndPoint response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A85     Len=var     Status  SrcAddr   NWKAddrOfIneterest ActiveEndpointCount ActiveEndpointList
	Status – byte – this field indicates either SUCCESS(0x00) or FAILURE(0x01).
	SrcAddr – 16 bit – the message’s source network address.
	NWKAddrOfInterest – 16 bits – Device’s short address that this response describes.
	ActiveEndpointCount – 8 bits – Number of active endpoint in the list
	ActiveEndpointList – byte array – Array of active endpoints on this device.

*/
static void ActiveEndPointRsp(char * cBuffer, 
			      int * ddata)
{
	int i, n;
	char nStr[8];

	printf("Active EndPoint response: ");
	Status(&cBuffer[8]);
	SrcAddr(&cBuffer[10], 2);
	printf("NWKAddrOfInterest %c%c%c%c, ", cBuffer[14], cBuffer[15], cBuffer[16], cBuffer[17]);
	strncpy(nStr, &cBuffer[18], 2);
	nStr[2] = 0;
	n = strtol(nStr, NULL, 16);
	printf("ActiveEndpointCount %d, ActiveEndpointList ", n);
	for (i=0; i<n; i++) printf("%s%c%c, ", i?" ":"", cBuffer[20+i*2], cBuffer[21+i*2]);
	*ddata = 0;
}

/*! \brief 3.15 User Description request
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A0A       Len=0x06        AddrMode    DstAddr NWKAddrOfIneterest   SecuritySuite

	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 16 bit – NWK address of the device generating the inquiry..
	NWKAddrOfInterest – 16 bit - NWK address of the destination device being queried.
	SecuritySuite - 1 byte – Security options.
*/
static void UserDescription(char * cBuffer, 
			    int * ddata)
{
	printf("User Description request: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	printf("NWKAddrOfInterest %c%c%c%c, ", cBuffer[14], cBuffer[15], cBuffer[16], cBuffer[17]);
	printf("SecuritySuite %c%c, ", cBuffer[18], cBuffer[19]);
	*ddata = 0;
}

/*! \brief 3.16 User Description response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A8F     Len=var      Status  SrcAddr   NWKAddrOfIneterest DescLen Descriptor
	Status – byte – this field indicates either SUCCESS(0x00) or FAILURE(0x01).
	SrcAddr – 16 bit – the message’s source network address.
	NWKAddrOfInterest – 16 bits – Device’s short address that this response describes.
	DescLen – 8 bits – Length, in bytes, of the user descriptor
	Descriptor – byte array – User descriptor array (can be up to 16 bytes).
*/
static void UserDescriptionRsp(char * cBuffer, 
			       int * ddata)
{
	int i, n, dc;
	char nStr[8];

	printf("User Description response: ");
	Status(&cBuffer[8]);
	SrcAddr(&cBuffer[10], 2);
	NWKAddrOfInterest(&cBuffer[14]);
	strncpy(nStr, &cBuffer[18], 2);
	nStr[2] = 0;
	n = strtol(nStr, NULL, 16);
	printf("DescLen %d, Descriptor '", n);
	for (i=0; i<n; i++)
	{
		sprintf(nStr, "%c%c", cBuffer[20+i*2], cBuffer[21+i*2]);
		dc = strtol(nStr, NULL, 16);
		if (isprint(dc)) printf("%c", dc);
		else printf(".");
	}
	printf("', ");
	*ddata = 0;
}

/*
 * 3.17 User Descriptor Set request
	Cmd=0x0A13	Len=var		AddrMode	DstAddr		NWKAddrOfInterest	DescLen		Descriptor	SecuritySuite
		AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits GroupAddress (Addrmode = 0x01)
		DstAddr – 16 bit – NWK address of the device generating the request..
		NWKAddrOfInterest – 16 bit - NWK address of the destination device being queried.
		DescLen – 1 byte –the length of the user descriptor
		Descriptor – byte array – User descriptor array (can be up to 16 bytes).
		SecuritySuite - 1 byte – Security options.
*/
static void UserDescriptorSet(char * cBuffer, 
			      int * ddata)
{
	int i, n, dc;
	char nStr[8];

	printf("User Description Set request: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	printf("NWKAddrOfInterest %c%c%c%c, ", cBuffer[14], cBuffer[15], cBuffer[16], cBuffer[17]);
	strncpy(nStr, &cBuffer[18], 2);
	nStr[2] = 0;
	n = strtol(nStr, NULL, 16);
	printf("DescLen %d, Descriptor '", n);
	for (i=0; i<n; i++)
	{
		sprintf(nStr, "%c%c", cBuffer[20+i*2], cBuffer[21+i*2]);
		dc = strtol(nStr, NULL, 16);
		if (isprint(dc)) printf("%c", dc);
		else printf(".");
	}
	printf("', SecuritySuite %c%c, ", cBuffer[20+n], cBuffer[21+n]);
	*ddata = 0;
}

/*
3.18 User Descriptor Set response
	Cmd=0x0A90	Len=0x03	Status		SrcAddr
		Status – byte – this field indicates either SUCCESS(0x00) or FAILURE(0x01).
		SrcAddr – 16 bit – the message’s source network address.
 */
static void UserDescriptorSetRsp(char * cBuffer, 
			         int * ddata)
{
	printf("User Description Set response: ");
	Status(&cBuffer[8]);
	SrcAddr(&cBuffer[10], 2);
	printf(", ");
	*ddata = 0;
}

/*! \brief 3.19 End device Bind request request an End Device Bind with the destination device.
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A0B        Len=Var       AddrMode    DstAddr   Location Coordinator     EndPoint      Profile ID
NumInClusters                 InClusterList             NumOutClusters       OutClusterList SecuritySuite
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
	GroupAddress (Addrmode = 0x01)
	DstAddr – 16 bits – NWK address of the device generating the request.
	LocalCoordinator – 16 bit – Local coordinator’s short address. In the case of source binding, it’s the short
	address of the source address.
	Endpoint – byte – Device’s Endpoint.
	Profile ID – 16 bit – Profile ID to match.
	NumInClusters – byte – Number of Cluster ID in the InClusterList.
	InClusterList – byte array – Array of input cluster ID – (NumInClusters * 2) long.
	NumOutClusters – byte – Number of Cluster ID in the OutClusterList.
	OutClusterList – byte array – Array of output cluster ID – (NumOutClusters * 2) long.
	SecuritySuite - 1 byte –. Security options.
 */
static void EndDeviceBind(char * cBuffer, 
			  int * ddata)
{
	printf("End device Bind request: ");
	/* TODO add the rest of this message */
	*ddata = 1;
}

/*! \brief 3.20 End device Bind Response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A87      Len=0x03        Status  SrcAddr
	Status – byte – this field indicates
		Description                Value
		SUCCESS                    0x00
		TIMEOUT                    0x85
		No Match                   0x86
		Table Full                 0x87
		No Entry                   0x88
	SrcAddr – 16 bit – the message’s source network address.
 */
static void EndDeviceBindRsp(char * cBuffer, 
			     int * ddata)
{
	printf("End device Bind Response: ");
	*ddata = 1;
}

/*! \brief 3.21 Bind request     request a Bind
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A0C   Len=var AddrMode DstAddr    SrcAddress SrcEndPoint
Cluster ID       DstAddrMode          DstAddress  DstEndpoint    SecuritySuite
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 16 bits –destination address of the device generating the bind request
	SrcAddress(Active) – 8 bytes – 64 bit Binding source IEEE address
	SrcEndpoint (Active)– 8 bits – Binding source endpoint.
	ClusterID – 2 byte – Cluster ID to match in messages.
	DstAddrMode(Passive) – 1 byte – Destination address mode:
		Mode                                 Value         Description
		ADDRESS_NOT_PRESENT                  0x00          Address Not Present
		GROUP_ADDRESS                        0x01          Group address
		ADDRESS_16_BIT                       0x02          Address 16 bit
		ADDRESS_64_BIT                       0x03          Address 64 bit
		BROADCAST                            0xFF          Broadcast
	DstAddress(Passive) – 8 bytes / 2bytes – Binding destination IEEE address. Not to be confused with DstAddr.
	DstEndpoint(Passive)– 8 bits / 0 byte – Binding destination endpoint. It is used only when DstAddrMode is 64
		bits extended address
	SecuritySuite - 1 byte – Security options.
 */
static void Bind(char * cBuffer, 
		 int * ddata)
{
	char nan[8];
	int dam;

	printf("Bind: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	printf("Src ");
	dispIeeeAddr(&cBuffer[14], 0);
	SrcEndpoint(&cBuffer[30]);
	ClusterId(&cBuffer[32]); 
	strncpy(nan, &cBuffer[36], 2);
	nan[2] = 0;
	dam = (int) strtol(nan, NULL, 16);
	printf("DstAddrMode ");
	switch (dam)
	{
		case 0 : 
			printf("Address Not Present, "); 
			SecuritySuite(&cBuffer[38]);
			break;
		case 1 : 
			printf("Group address, "); 
			DstAddr(&cBuffer[36]);
			SecuritySuite(&cBuffer[42]);
			break;
		case 2 : 
			printf("Address 16 bit, "); 
			DstAddr(&cBuffer[36]);
			SecuritySuite(&cBuffer[42]);
			break;
			case 3 : printf("Address 64 bit, "); 
			dispIeeeAddr(&cBuffer[38], 0);
			DstEndpoint(&cBuffer[54]);
			SecuritySuite(&cBuffer[56]);
			break;
			default : printf("Reserved %d, ", dam);
	}
	*ddata = 0;
}

/*! \brief 3.22 Bind response
	\param	buf	input buffer
	\param	ddata	data display flag

        Cmd=0x0A88      Len=0x03       Status  SrcAddr
     Status – byte – this field indicates status of the bind request with the following values:.
               Description                        Value
               SUCCESS                            0
               NOT SUPPORTED                      1
               TABLE FULL                         2
               Reserved                           0x03 – 0xFF
     SrcAddr – 16 bit – the message’s source network address.
 */
static void BindRsp(char * cBuffer,
		    int * ddata)
{
	char nan[8];
	int sts;

	printf("Bind response: ");
	strncpy(nan, &cBuffer[8], 2);
	nan[2] = 0;
	sts = (int) strtol(nan, NULL, 16);
	switch (sts)
	{
		case 0 : printf("SUCCESS, "); break;
		case 1 : printf("NOT SUPPORTED, "); break;
		case 2 : printf("TABLE FULL, "); break;
		default : printf("Reserved %02x, ", sts); break;
	}
	SrcAddr(&cBuffer[10], 2);
	*ddata = 0;
}

/*! \brief 3.23 Unbind request
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A0D       Len=0x19      AddrMode        DstAddr     SrcAddress       SrcEndPoint
Cluster ID          DstAddrMode           DstAddress    DstEndpoint      SecuritySuite
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 byte –destination address of the device generating the bind request
	SrcAddress(Active) – 8 bytes – 64 bit Binding source IEEE address
	SrcEndpoint(Active) – 1 byte – Binding source endpoint.
	ClusterID – 2 byte – Cluster ID to match in messages.
	DstAddrMode(Passive) – 1 byte – Destination address mode:
		Mode                                     Value         Description
		ADDRESS_NOT_PRESENT                      0x00          Address Not Present
		GROUP_ADDRESS                            0x01          Group address
		ADDRESS_16_BIT                           0x02          Address 16 bit
		ADDRESS_64_BIT                           0x03          Address 64 bit
		BROADCAST                                0xFF          Broadcast
	DstAddress(Passive) – 8/2 bytes – Binding destination IEEE address. Not to be confused with DstAddr.
	DstEndpoint(Passive) – 1 byte – Binding destination endpoint. It is used only when DstAddrMode is 64 bits
		extended address
	SecuritySuite - 1 byte – Security options.
 */
static void Unbind(char * cBuffer, 
		   int * ddata)
{
	char nan[8];
	int dam;

	printf("Unbind request: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	printf("Src ");
	dispIeeeAddr(&cBuffer[14], 1);
	SrcEndpoint(&cBuffer[30]);
	ClusterId(&cBuffer[32]); 
	strncpy(nan, &cBuffer[36], 2);
	nan[2] = 0;
	dam = (int) strtol(nan, NULL, 16);
	printf("DstAddrMode ");
	switch (dam)
	{
		case 0 : 
			printf("Address Not Present, "); 
			SecuritySuite(&cBuffer[38]);
			break;
		case 1 : 
			printf("Group address, "); 
			DstAddr(&cBuffer[38]);
			SecuritySuite(&cBuffer[42]);
			break;
		case 2 : 
			printf("Address 16 bit, "); 
			DstAddr(&cBuffer[38]);
			SecuritySuite(&cBuffer[42]);
			break;
		case 3 : printf("Address 64 bit, "); 
			dispIeeeAddr(&cBuffer[38], 0);
			DstEndpoint(&cBuffer[54]);
			SecuritySuite(&cBuffer[56]);
			break;
		default : printf("Reserved %d, ", dam);
	}
	*ddata = 0;
}

/*! \brief 3.24 Unbind response
	\param	buf	input buffer
	\param	ddata	data display flag

      Cmd=0x0A89      Len=0x03       Status   SrcAddr
     Status – byte – this field indicates status of the bind request with the following values:.
              Description                         Value
              SUCCESS                             0
              NOT SUPPORTED                       1
              NO ENTRY                            2
              Reserved                            0x03 – 0xFF
     SrcAddr – 16 bit – the message’s source network address.
 */
static void UnbindRsp(char * cBuffer, 
		      int * ddata)
{
	printf("Unbind response: ");
	Status(&cBuffer[12]);
	SrcAddr(&cBuffer[10], 2);
	*ddata = 0;
}

/*! \brief 3.25 Manage Network Discovery request
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A0E     Len=0x09       AddrMode      DstAddr    ScanChannels     ScanDuration   StartIndex
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr -2 bytes - Specifies the network address of the device performing the discovery.
	ScanChannels -4 bytes- Specifies the Bit Mask for channels to scan:
                   Channel                   Value
                   NONE                      0x00000000
                   ALL_CHANNELS              0x07FFF800
                   CHANNEL 11                0x00000800
                   CHANNEL 12                0x00001000
                   CHANNEL 13                0x00002000
                   CHANNEL 14                0x00004000
                   CHANNEL 15                0x00008000
                   CHANNEL 16                0x00010000
                   CHANNEL 17                0x00020000
                   CHANNEL 18                0x00040000
                   CHANNEL 19                0x00080000
                   CHANNEL 20                0x00100000
                   CHANNEL 21                0x00200000
                   CHANNEL 22                0x00400000
                   CHANNEL 23                0x00800000
                   CHANNEL 24                0x01000000
                   CHANNEL 25                0x02000000
                   CHANNEL 26                0x04000000
	ScanDuration – 1 byte - Specifies the scanning time.
	StartIndex – 1 byte –Specifies where to start in the response array list. The result may contain more entries
		than can be reported, so this field allows the user to retrieve the responses anywhere in the array list.
 */
static void ManageNetworkDiscovery(char * cBuffer, 
				   int * ddata)
{
	int count;
	char countStr[16];

	printf("Manage Network Discovery: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	sprintf(countStr, "%c%c%c%c%c%c%c%c", cBuffer[14], cBuffer[15], cBuffer[16], cBuffer[17], cBuffer[18], cBuffer[19], cBuffer[20], cBuffer[21]);
	count = strtol(countStr, NULL, 16);
	printf("ScanChannels ");
	switch (count)
	{
		case 0x00000000 : printf("NONE, "); break;
		case 0x07FFF800 : printf("ALL CHANNELS, "); break;
		case 0x00000800 : printf("CHANNEL 11, "); break;
		case 0x00001000 : printf("CHANNEL 12, "); break;
		case 0x00002000 : printf("CHANNEL 13, "); break;
		case 0x00004000 : printf("CHANNEL 14, "); break;
		case 0x00008000 : printf("CHANNEL 15, "); break;
		case 0x00010000 : printf("CHANNEL 16, "); break;
		case 0x00020000 : printf("CHANNEL 17, "); break;
		case 0x00040000 : printf("CHANNEL 18, "); break;
		case 0x00080000 : printf("CHANNEL 19, "); break;
		case 0x00100000 : printf("CHANNEL 20, "); break;
		case 0x00200000 : printf("CHANNEL 21, "); break;
		case 0x00400000 : printf("CHANNEL 22, "); break;
		case 0x00800000 : printf("CHANNEL 23, "); break;
		case 0x01000000 : printf("CHANNEL 24, "); break;
		case 0x02000000 : printf("CHANNEL 25, "); break;
		case 0x04000000 : printf("CHANNEL 26, "); break;
		default : printf("unknown channel %08x, ", count);
	}
	sprintf(countStr, "%c%c", cBuffer[22], cBuffer[23]);
	count = strtol(countStr, NULL, 16);
	printf("ScanDuration %d, ", count);
	sprintf(countStr, "%c%c", cBuffer[24], cBuffer[25]);
	count = strtol(countStr, NULL, 16);
	printf("StartIndex %d, ", count);
	*ddata = 0;
}

/*! \brief 3.26 Manage Network Discovery response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A8A      Len=var    SrcAddr   Status  NetworkCount StartIndex NetworkListCount NetworkList
	SrcAddr -2 byte - Source address of the message.
	Status -1 byte- This field indicates either SUCCESS or FAILURE.
	NetworkCount -1 byte - Total number of entries available in the device.
	StartIndex -1 byte - Where in the total number of entries this response starts.
	NetworkListCount -1 byte - Number of entries in this response.
	NetworkList – var - An array of NetworkList items. NetworkListCount contains the number of items in this table:
		Name                              Size			Description
		PAN ID/Extended PAN ID            8 bytes		PAN ID of the neighbor device
		Logical Channel                   1 byte		The current logical channel occupied by the network.
		Stack Profile / ZigBee Version    1 byte		StackProfile: bits 3-0
									ZigBeeVersion: bits 7-4
									A ZigBee stack profile identifier indicating the stack
									profile in use in the discovered network.The version of
									the ZigBee protocol in use in the discovered network.
		Beacon Order / Super frame Order  1 byte		BeaconOrder: bits 3-0
									SuperframeOrder: bits 7-4
		Permit Joining                    1 byte		Permit joining flag

 */
static void ManageNetworkDiscoveryRsp(char * cBuffer, 
				      int * ddata)
{
	int i, j, val, count, start;
	char countStr[16];

	printf("Manage Network Discovery response: ");
	SrcAddr(&cBuffer[10], 2);
	Status(&cBuffer[12]);
	sprintf(countStr, "%c%c", cBuffer[14], cBuffer[15]);
	count = strtol(countStr, NULL, 16);
	printf("NetworkCount %d, ", count);
	sprintf(countStr, "%c%c", cBuffer[16], cBuffer[17]);
	start = strtol(countStr, NULL, 16);
	printf("StartIndex %d, ", start);
	sprintf(countStr, "%c%c", cBuffer[18], cBuffer[19]);
	count = strtol(countStr, NULL, 16);
	printf("NetworkListCount %d, ", count);
	for (i=0; i<count; i++)
	{
		printf("NetworkList %d: ", start+i);
		printf("PANID ");
		for (j=0; j<16; j++) printf("%c", cBuffer[20+i*24+j]);
		sprintf(countStr, "%c%c", cBuffer[36+24*i], cBuffer[37+24*i]);
		val = strtol(countStr, NULL, 16);
		printf(", LogicalChannel %d, ", val);
		printf("StackProfile %c, ZigBeeVersion %c, ", cBuffer[39+24*i], cBuffer[38+24*i]);
		printf("BeaconOrder %c, SuperframeOrder %c, ", cBuffer[41+24*i], cBuffer[40+24*i]);
		sprintf(countStr, "%c%c", cBuffer[42+24*i], cBuffer[43+24*i]);
		val = strtol(countStr, NULL, 16);
		printf("PermitJoining %d, ", val);
	}
	*ddata = 0;
}

/*! \brief 3.31 Manage Bind request request the Binding Table of the destination device.
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A11       Len=0x04      AddrMode     DstAddr     StartIndex
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 16 bits –network address of the device being queried.
	StartIndex – 8 bits – Where to start in the response array list. The result may contain more entries than can be
		reported, so this field allows the user to retrieve the responses anywhere in the array list.
 */
static void ManageBind(char * cBuffer,
		       int * ddata)
{
	printf("Manage Bind request: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	printf("StartIndex %c%c, ", cBuffer[14], cBuffer[15]);
	*ddata = 0;
}


/*! \brief 3.32 Manage Bind response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd = 0x0A8D        Len=var    SrcAddr    Status    BindCount     StartIndex BindListCount     BindList
	SrcAddr – 16 bit – Source address of the message
	Status – byte – this field indicates either SUCCESS or FAILURE.
	BindCount – 1 byte – Total number of entries available in the device.
	StartIndex – 1 byte – Where in the total number of entries this response starts.
	BindListCount – 1 byte – Number of entries in this response.

BindList – list – an array of BindList items. BindListCount contains the number of items in this table.
                                                    Bind List Item
                    Name                 Size(byte)                       Description
               Source Address                8                Binding Entry’s source IEEE address
              Source Endpoint                1                   Binding Entry’s source endpoint
                  Cluster ID                 2                     Message ID in binding table
        Destination Address Mode             1       Address mode for binding entry’s destination address
            Destination Address              8             Binding Entry’s destination IEEE address
           Destination Endpoint              1                Binding Entry’s destination endpoint.

 */
static void ManageBindRsp(char * cBuffer, 
			  int * ddata)
{
	int i, count;
	char countStr[8];

	printf("Manage Bind response: ");
	SrcAddr(&cBuffer[10], 2);
	Status(&cBuffer[12]);
	printf("BindCount %c%c, ", cBuffer[14], cBuffer[15]);
	printf("StartIndex %c%c, ", cBuffer[16], cBuffer[17]);
	strncpy(countStr, &cBuffer[18], 2);
	countStr[2] = 0;
	count = strtol(countStr, NULL, 16);
	printf("BindListCount %d, ", count);
	for (i=0; i<count; i++)
	{
		dispIeeeAddr(&cBuffer[20+42*i], 0);
		SrcEndpoint(&cBuffer[36+42*i]);
		ClusterId(&cBuffer[40+42*i]);
		AddrMode(&cBuffer[42+42*i]);
		dispIeeeAddr(&cBuffer[44+42*i], 0);
		DstEndpoint(&cBuffer[60+42*i]);
	}
	*ddata = 0;
}

/*! \brief 3.33 Manage Permit Join request
	\param	buf	input buffer
	\param	ddata	data display flag

set the Permit Join for the destination device.
Cmd=0x0A16      Len=0x05       AddrMode      DstAddr    Duration   TC Significance
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination device whose Permit Join information is to be modified.
	Duration – 1 byte – The duration to permit joining. 0 = join disabled. 0xff = join enabled. 0x01-0xfe = number of
		seconds to permit joining.
	TC Significance – 1 byte - Trust Center Significance.
 */
static void ManagePermitJoin(char * cBuffer, 
			     int * ddata)
{
	int count;
	char countStr[8];

	printf("Manage Permit Join: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	sprintf(countStr, "%c%c", cBuffer[14], cBuffer[15]);
	count = strtol(countStr, NULL, 16);
	printf("Duration %d, ", count);
	sprintf(countStr, "%c%c", cBuffer[16], cBuffer[17]);
	count = strtol(countStr, NULL, 16);
	printf("TC Significance %d, ", count);
	*ddata = 0;
}

/*! \brief 3.34 Manage Permit Join response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A92      Len=0x03       Status    SrcAddr
	Status – byte – this field indicates either SUCCESS (0) or FAILURE (1).
	SrcAddr – 16 bit – Source address of the message
 */
static void ManagePermitJoinRsp(char * cBuffer, 
				int * ddata)
{
	printf("Manage Permit Join response: ");
	Status(&cBuffer[8]);
	SrcAddr(&cBuffer[10], 2);
	*ddata = 0;
}

/*! \brief 4.1 Reset to Factory Defaults (Basic)
	\param	buf	input buffer
	\param	ddata	data display flag
 
Cmd=0x0C00  Len=0x06  AddrMode  DstAddr  DstEndpoint  Cluster ID

	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
	GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the device being reset.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes – Basic Cluster ID
*/
static void FactoryDefaults(char * cBuffer, 
			    int * ddata)
{
	printf("Reset to Factory Defaults: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	*ddata = 0;
}

/*! 4.9 \brief	Get Group Membership (Group)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C02       Len=var    AddrMode      DstAddr    DstEndpoint   Cluster ID    GroupCount      Group ID List
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Group Cluster ID.
	GroupCount – byte - The number of the groups in the Group list. The group count field has a value of 0
		indicating that the device response all the groups of the DstEndpoint
	Group ID List(reverse) – Var- The group list field of the command frame contains at least one group of which
		the entity is a member. In this case the response frame will contain the identifiers of all such groups.
 */
static void GetGroupMembership(char * cBuffer, 
			       int * ddata)
{
	int i, count;
	char countStr[8];

	printf("Get Group Membership: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	strncpy(countStr, &cBuffer[18], 2);
	countStr[2] = 0;
	count = strtol(countStr, NULL, 16);
	printf("GroupCount %d, ", count);
	for (i=0; i<count; i++)
	{
		printf("GroupID%d %c%c%c%c, ", i+1, cBuffer[22+i*4], cBuffer[23+i*4], cBuffer[20+i*4], cBuffer[21+i*4]);
	}
	*ddata = 0;
}


/*! \brief 4.10 Get Group Membership Response (Group)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C02       Len=var    SrcAddr     SrcEndpoint    Cluster ID  Capacity   GroupCount       Group ID List
	SrcAddr – 2 bytes –network address of the device being identify.
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Group Cluster ID.
	Capacity – byte – The Capacity field shall contain the remaining capacity of the group table of the device. The
		following values apply:
		0                        No further groups may be added.
		0 < Capacity < 0xfe      Capacity holds the number of groups that may be added
		0xfe                     At least 1 further group may be added (exact number is unknown)
		0xff                     It is unknown if any further groups may be added
	GroupCount – byte - The number of the groups in the Group list.
	Group ID List(reverse) – Var- The group list field of the command frame contains at least one group of which
		the entity is a member. In this case the response frame will contain the identifiers of all such groups.
 */
static void GetGroupMembershipRsp(char * cBuffer, 
				  int * ddata)
{
	int i, count, cap;
	char countStr[8];

	printf("Get Group Membership Response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	strncpy(countStr, &cBuffer[18], 2);
	countStr[2] = 0;
	cap = strtol(countStr, NULL, 16);
	printf("Capacity %d ", cap);
	switch (cap)
	{
		case 0    : printf("No further groups, "); break;
		case 0xfe : printf("At least 1 further group, "); break;
		case 0xff : printf("unknown number of groups, "); break;
		default   : printf(" further groups, ");
	}
	strncpy(countStr, &cBuffer[20], 2);
	countStr[2] = 0;
	count = strtol(countStr, NULL, 16);
	printf("GroupCount %d, ", count);
	if (0==cap) return;
	for (i=0; i<count; i++)
	{
		printf("GroupID%d %c%c%c%c, ", i+1, cBuffer[24+i*4], cBuffer[25+i*4], cBuffer[22+i*4], cBuffer[23+i*4]);
	}
	*ddata = 0;
}

/*! \brief 4.20 Manage LQI request
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A0F Len=0x04 AddrMode DstAddr StartIndex
	AddrMode – byte – indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
	GroupAddress (Addrmode = 0x01)
	DstAddr – 16 bits – network address of the device being queried.
	StartIndex – 8 bits – Where to start in the response array list. The result may contain more entries
*/
static void ManageLqiReq(char * cBuffer, 
			 int * ddata)
{
	int addrMode;
	
	*ddata = 0;
	printf("Manage LQI request: ");
	addrMode = AddrMode(&cBuffer[8]);
	SrcAddr(&cBuffer[10], addrMode);
	printf("StartIndex %c%c, ", cBuffer[14], cBuffer[15]);
}

/*! \brief 4.21 Manage LQI response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0A8B Len=var SrcAddr Status NeighborLQIEntries StartIndex NeighborLQICount NeighborLqiList
	SrcAddr – 16 bit – Source address of the message.
	Status – byte – this field indicates either SUCCESS or FAILURE.
	NeighborLQIEntries – 1 byte – Total number of entries available in the device.
	StartIndex – 1 byte – Where in the total number of entries this response starts.
	NeighborLQICount – 1 byte – Number of entries in this response.
	NeighborLqiList – list – an array of NeighborLqiList items. NeighborLQICount contains the number of items in
		this table.
	NeighborLqiList Item
		Name 			Size(V1.0) 	Size(V1.1) 	Description
		PAN ID 			2 Bytes 	8 Bytes 	PAN ID of the neighbor device
		Network Address 	2 Bytes 	2 Bytes 	Network short address
		Rx LQI 			1 Byte 		1 Byte 		Receiver Link Quality
		Tx Quality 		1 Byte 		1 Byte 		Transmitter Quality factor
*/
static void ManageLqiRsp(int dLen,
			 char * cBuffer, 
			 int * ddata, 
			 int version)
{
	int st;

	*ddata = 0;
	printf("Manage LQI response: ");
	if (dLen == 1)
	{
		st = Status(&cBuffer[8]);
	}
	else
	{
		SrcAddr(&cBuffer[8], 2);
		st = Status(&cBuffer[12]);
		if (! st) LqiList(&cBuffer[14], version);
	}
}

static void LqiList(char * lqiBuff, 
		    int version)
{
	char countStr[8];
	int i, count, lqilen;
	
	switch (version)
	{
		case 0 : lqilen = 12; break;
		case 1 : lqilen = 24; break;
		default : lqilen = 24;
	}

	printf("Entries %c%c, ", lqiBuff[0], lqiBuff[1]);
	printf("StartIndex %c%c, ", lqiBuff[2], lqiBuff[3]);
	strncpy(countStr, &lqiBuff[4], 2);
	countStr[2] = 0;
	count = strtol(countStr, NULL, 16);
	printf("Count %d, ", count);
	
	for (i=0; i<count; i++) 
	{
		printf("LQI%d: ", i+1);
		LqiListItem(&lqiBuff[6+i*lqilen], version);
	}
}

static void LqiListItem(char * lqiBuff, 
			int version)
{
	char countStr[8];
	int i, count;
	
	/* TODO not sure what version is actually in use */
	switch(version)
	{
		case 0 :
			/* 2-byte address in reverse order */
			printf("PAN ");
			for (i=2; i>=0; i-=2) printf("%c%c", lqiBuff[i], lqiBuff[i+1]);
			printf(", Address %c%c%c%c, ", lqiBuff[4], lqiBuff[5], lqiBuff[6], lqiBuff[7]);
			strncpy(countStr, &lqiBuff[8], 2);
			countStr[2] = 0;
			count = strtol(countStr, NULL, 16);
			printf("RxQuality %d, ", count);
			strncpy(countStr, &lqiBuff[10], 2);
			countStr[2] = 0;
			count = strtol(countStr, NULL, 16);
			printf("TxQuality %d, ", count);
			break;
		case 1 :
			/* 8-byte address in reverse order */
			printf("PAN ");
			for (i=0; i<16; i++) printf("%c", lqiBuff[i]);
			printf(", Address %c%c%c%c, ", lqiBuff[16], lqiBuff[17], lqiBuff[18], lqiBuff[19]);
			strncpy(countStr, &lqiBuff[20], 2);
			countStr[2] = 0;
			count = strtol(countStr, NULL, 16);
			printf("RxQuality %d, ", count);
			strncpy(countStr, &lqiBuff[22], 2);
			countStr[2] = 0;
			count = strtol(countStr, NULL, 16);
			printf("TxQuality %d, ", count);
			break;
		default :
			printf("Invalid LQI version 1.%d, ", version);
	}
	printf(", ");
}

/*! \brief 4.30 End Device Annce
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0AF0 Len=0x0B DevAddr DeviceAddress Capabilities
	DevAddr – 16 bits – network address of the device generating the request.
	DeviceAddress – 8 bytes – The 64 bit IEEE Address of the device being announced.
	Capabilities – 8 bits – The operating capabilities of the device being directly joined. Bit weighted values
		follow:Can you please verify if the table below is accurate? I understand that for a router, the value should be
		0x8E, and for an ED, it’s 0x80, and I’m not sure if these jive with this table?
		Bit 	Description
		0 	Alternate PAN Coordinator
		1 	Device type:
			1 – ZigBee Router
			0 – End Device
		2 Power Source:
			1 – Mains powered
		3 Receiver on when idle
		4-5 Reserved
		6 Security capability
		7 Reserved
*/
static void EndDeviceAnnce(char * cBuffer, 
			   int * ddata)
{
	int atts;
	char attsStr[8];

	printf("End Device Annce: ");
	
	SrcAddr(&cBuffer[8], 2);
	dispIeeeAddr(&cBuffer[12], 1);
	sprintf(attsStr, "%c%c, ", cBuffer[28], cBuffer[29]);
	atts = strtol(attsStr, NULL, 16);
	switch(atts)
	{
		case 0 : printf("Capabilities Alternate PAN Coordinator, "); break;
		case 1 : printf("Capabilities Device type, "); break;
		/*
				1 – ZigBee Router
				0 – End Device
		*/
		case 2 : printf("Capabilities Power Source, "); break; /* 1 – Mains powered */
		case 3 : printf("Capabilities Receiver on when idle, "); break;
		case 4 :
		case 5 : printf("Capabilities Reserved, "); break;
		case 6 : printf("Capabilities Security capability, "); break;
		case 7 : printf("Capabilities Reserved, "); break;
		case 0x80 : printf("Capabilities End Device (ED), "); break;
		case 0x8e : printf("Capabilities Router, "); break;
		default : printf("Capabilities %c%c, ", cBuffer[28], cBuffer[29]);
	}
	*ddata = 0;
}


/*! \brief 4.6 Add Group response (Group)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C00      Len=0x08       SrcAddr    SrcEndpoint   Cluster ID   status   Group ID
	SrcAddr – 2 bytes –network address of the device being identify.
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Group Cluster ID.
	Status – byte – this field indicates either SUCCESS ,DUPLICATE_EXISTS or INSUFFICIENT_SPACE.
	Group ID (reverse)– 2 bytes –Group Address

This message is received when PIR triggers 
*/
static void AddGroupRsp(char * cBuffer, 
			int * ddata)
{
	printf("Add Group response: ");
	SrcAddr(&cBuffer[8], 2); 		/*network address of device being identify. */
	SrcEndpoint(&cBuffer[12]); 		/* source EndPoint. represents the application endpoint [of] the data. */
	ClusterId(&cBuffer[14]); 		/* Group Cluster ID. */
	Status(&cBuffer[18]); 			/* indicates either SUCCESS or FAILURE. */
	GroupID(&cBuffer[20]); 			/* (reverse) ID of Group to be added */
	*ddata = 0;
}

/*
 * 
 * 4.7 View Group (Group)
Cmd=0x0C01	Len=0x08	AddrMode	DstAddr		DstEndpoint	Cluster ID	Group ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Group Cluster ID.
	Group ID(reverse) – 2 bytes –Group Address.

 * 
 */
static void ViewGroup(char * cBuffer, 
		      int * ddata)
{
	printf("View Group: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	GroupID(&cBuffer[20]); 			/* (reverse) ID of Group to be added */
	*ddata = 0;
}

/*! \brief 4.36 Send Reset Alarm (Alarm)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C00     Len=9    AddrMode    DstAddr    DstEndPoint   Cluster ID Alarm Code    Alarm ClusterID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Alarm Cluster ID.
	Alarm Code –byte –The identifying Code for the cause of the alarm.as given in the specification of the cluster
		whose attribute generated this alarm.
	Alarm Cluster ID -2bytes- The identifier of the cluster whose attribute generated this alarm. (Power Cluster ID
		add so on)
*/
static void SendResetAlarm(char * cBuffer, 
			   int * ddata)
{
	printf("Send Reset Alarm: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);

	printf("AlarmCode %c%c, ", cBuffer[20], cBuffer[21]);
	printf("AlarmClusterId %c%c%c%c, ", cBuffer[22], cBuffer[23], cBuffer[24], cBuffer[25]);
	*ddata = 0;
}

/*! \brief 4.37 Reset All Alarm (Alarm)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C01       Len=0x06   AddrMode    DstAddr     DstEndPoint    Cluster ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Alarm Cluster ID.
 */
static void ResetAllAlarm(char * cBuffer, 
			  int * ddata)
{
	printf("Reset All Alarm: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	*ddata = 0;
}


/*! \brief 4.39 Get Alarm response (Alarm)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C01     Len=13    SrcAddr   SrcEndPoint  Cluster ID  Status  Alarm Code    AlarmClusterID  Time Stamp
	SrcAddr – 2 bytes –network address of the device alarming.
	SrcEndpoint –1 byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –The Identifier of the Cluster whose attribute generated the alarm.
	Status –1 byte –this field indicates either SUCCESS or NOT_FOUND.
	Alarm Code –1 byte –The identifying Code for the cause of the alarm.
	Alarm Cluster ID -2 bytes- The identifier of the cluster whose attribute generated this alarm. (Power Cluster ID
		add so on)
	Time Stamp -4 bytes- The time at which the alarm occurred.
		* If there is at least one alarm record in the alarm table then the status field is set to SUCCESS. The alarm
		  code, cluster identifier and time stamp fields shall all be present and shall take their values from the item in
		  the alarm table that they are reporting.
		* If there are no more alarms logged in the alarm table then the status field is set to NOT_FOUND and the
		  alarm code, cluster identifier and time stamp fields shall be omitted.
*/
static void GetAlarmRsp(char * cBuffer, 
			int * ddata)
{
	int status;
	
	*ddata = 0;
	
	printf("Get Alarm response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]); 	/* Group Cluster ID. */

	status = getStatus(&cBuffer[18]);
	/* indicates either SUCCESS or FAILURE. */
	if (status)
		dispStatus(status);
	else
	{
		printf(" Status OK, AlarmCode %c%c, ", cBuffer[20], cBuffer[21]);
		printf("AlarmClusterID %c%c%c%c, ", cBuffer[20], cBuffer[21], cBuffer[20], cBuffer[21]);
		printf("Time stamp %c%c%c%c, ", cBuffer[20], cBuffer[21], cBuffer[22], cBuffer[23]); 	/* (reverse) ID of Group to be added */
	}
}

/* Get Alarm response indicates FAILURE. */
static void GetAlarmFailRsp(char * cBuffer, 
			    int * ddata)
{
	int status;
	
	*ddata = 0;
	
	printf("Get Alarm response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]); 	/* Group Cluster ID. */

	status = getStatus(&cBuffer[18]);
	dispStatus(status);
}

/*! \brief 5.8 View Group response (Group)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C01 Len=var SrcAddr SrcEndpoint Cluster ID status Group ID Group name
	SrcAddr – 2 bytes –network address of the device being identify.
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Group Cluster ID.
	Status – byte – this field indicates either SUCCESS or FAILURE.
	Group ID(reverse) – 2 bytes –the ID of Group to be view
	Group name –byte array– the first byte is length of array.(MAX=16 bytes)
*/
static void ViewGroupRsp(char * cBuffer, 
			 int * ddata)
{
	int i, atts;
	char attsStr[8];

	printf("View Group response: ");
	SrcAddr(&cBuffer[8], 2); 		/*network address of device being identify. */
	SrcEndpoint(&cBuffer[12]); 		/* source EndPoint. represents the application endpoint [of] the data. */
	ClusterId(&cBuffer[14]); 		/* Group Cluster ID. */
	Status(&cBuffer[18]); 			/* indicates either SUCCESS or FAILURE. */
	GroupID(&cBuffer[20]); 			/* (reverse) ID of Group to be added */
	sprintf(attsStr, "%c%c, ", cBuffer[24], cBuffer[25]);
	atts = strtol(attsStr, NULL, 16);
	printf("Length %02x, ", atts);
	for (i=0; i<atts; i++)
		printf("%c%c, ", cBuffer[26+i*2], cBuffer[27+i*2]);
	printf(", ");
	*ddata = 0;
}

/*! \brief 5.12 Remove Group response (Group)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C03 Len=0x08 SrcAddr SrcEndpoint Cluster ID status Group ID
	SrcAddr – 2 bytes –network address of the device being removed.
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Group Cluster ID.
	Status – byte – this field indicates either SUCCESS or FAILURE.
	Group ID(reverse) – 2 bytes –the ID of Group to be removed.

This message is received when alarm is deactivated
*/
static void RemoveGroupRsp(char * cBuffer, 
			   int * ddata)
{
	printf("%-40s", "Remove Group response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	Status(&cBuffer[18]);
	GroupID(&cBuffer[20]);
	*ddata = 0;
}

/*! \brief 5.20 Remove Scene response (Scene)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C02 Len=0x09 SrcAddr SrcEndPoint Cluster ID Status Group ID Scene ID
	SrcAddr – 2 bytes –network address of the device being added with scene.
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Scene Cluster ID.
	Status –byte –this field indicates either SUCCESS or FAILURE.
	Group ID(reverse) – 2 bytes –The group ID for which this scene applies.
	Scene ID – byte – to be removed.
*/
static void RemoveSceneRsp(char * cBuffer, 
			   int * ddata)
{
	printf("%-40s\t * PANIC ", "Remove Scene response (Scene)");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	Status(&cBuffer[18]);
	GroupID(&cBuffer[20]);
	printf("SceneID %c%c, ", cBuffer[24], cBuffer[25]);
	*ddata = 0;
}

/*! \brief 5.23 Store Scene (Scene)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C04 Len=0x09 AddrMode DstAddr DstEndPoint Cluster ID Group ID Scene ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Scene Cluster ID.
	Group ID(reverse) – 2 bytes –The group ID for which this scene applies.
	Scene ID – byte –store scene.
*/
static void StoreSceneReq(char * cBuffer, 
			  int * ddata)
{
	printf("Store Scene: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	GroupID(&cBuffer[20]);
	printf("SceneId %c%c, ", cBuffer[24], cBuffer[25]);
	*ddata = 0;
}

/*! \brief 5.24 Store Scene response (Scene)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C04 Len=0x09 SrcAddr SrcEndPoint Cluster ID Status Group ID Scene ID
	SrcAddr – 2 bytes –network address of the device store scene.
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Scene Cluster ID.
	Status –byte –this field indicates either SUCCESS or FAILURE.
	Group ID(reverse) – 2 bytes – The group ID for which this scene applies.
	Scene ID – byte – store scene.
*/
static void StoreSceneRsp(char * cBuffer, 
			  int * ddata)
{
	printf("Store Scene response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	Status(&cBuffer[18]);
	GroupID(&cBuffer[20]);
	printf("SceneId %c%c, ", cBuffer[24], cBuffer[25]);
	*ddata = 0;
}

/*! \brief 5.30 Send Alarm Toggle (On/Off)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C02 Len=0x06 AddrMode DstAddr DstEndPoint Cluster ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
	GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –On/Off Cluster ID.
 
\brief 5.38 Get Alarm (Alarm)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C02 Len=0x06 AddrMode DstAddr DstEndPoint Cluster ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
	GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –Alarm Cluster ID.
*/
void GetAlarm(char * cBuffer, 
	      int * ddata)
{
	printf("Get Alarm: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]); 	/* Group Cluster ID. */
	*ddata = 0;
}

/*! \brief 5.52 Blind Node Data response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C05 Len=var SrcAddr SrcEndPoint ClusterID Num StartIdex IEEE1 LQI1 … IEEEn LQIn 
	SrcAddr – 2 bytes –network address of a device to announce its location information and channel parameters
	periodically.
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2byte –RSSI Location cluster ID.
*/
static void BlindNodeDataRsp(char * cBuffer, 
			     int * ddata) 				/* see 5.52 */
{
	printf("ALARM %s: ", "Blind Node Data response");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]); 	/* Group Cluster ID. */
	printf("Num %c%c%c%c, StartIdex %c%c, IEEE1: ", cBuffer[18], cBuffer[19], cBuffer[20], cBuffer[21], cBuffer[22], cBuffer[23]);
	/* IEEE address is in reverse byte order */
	dispIeeeAddr(&cBuffer[24], 1);
	printf("LQI1: ");
	LqiListItem(&cBuffer[40], 0);
	*ddata = 0;
}

/*! \brief 5.67 Get Zone Information (IAS ACE)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C06 Len=0x07 AddrMode DstAddr DstEndPoint Cluster ID Zone ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	Zone ID– byte –the IAS Zone ID.
*/
static void GetZoneInfo(char * cBuffer, 
			int * ddata)
{
	printf("Get Zone Information: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	ZoneID(&cBuffer[20]);
	*ddata = 0;
}

/*! \brief 5.68 Get Zone Information response(IAS ACE)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C02 Len=0x10 SrcAddr SrcEndPoint Cluster ID Zone ID Zone Type IEEE Address
	SrcAddr– 2 bytes –network address of the source address.
	SrcEndPoint– byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	Zone ID– byte –the IAS Zone ID.
	Zone Type – 2 byte –See” Zone Type attribute of IAS Zone”
	IEEE Address –8 bytes –IEEE Address of device get Zone Information.
*/
static void GetZoneInfoRsp(char * cBuffer,
			   int * ddata)
{
	printf("Get Zone Information response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	ZoneID(&cBuffer[18]);
	ZoneType(&cBuffer[20]);
	dispIeeeAddr(&cBuffer[24], 1);
	*ddata = 0;
}

/*! \brief 6.1 Read Attribute
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0D00 Len=var AddrMode DstAddr DstEndPoint Cluster ID Number Attribute ID1 … Attribute IDn
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the device being read attributes.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –All Cluster ID.
	Number – 1 byte –number of attributes in the list.
	Attribute ID1…n –List –The attribute identifier field is 16-bits in length and shall contain the identifier of the attribute
		that is to be read.
*/
static void ReadAttribute(char * cBuffer, 
			  int * ddata)
{
	int addrMode;
	int i, atts;
	char attsStr[8];
	
	*ddata = 0;
	printf("Read Attribute: ");
	addrMode = AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	sprintf(attsStr, "%c%c, ", cBuffer[20], cBuffer[21]);
	atts = strtol(attsStr, NULL, 16);
	printf("Number %02x, ", atts);
	for (i=0; i<atts; i++)
		printf("AttributeID%d %c%c%c%c, ", i+1, cBuffer[22+i*4], cBuffer[23+i*4], cBuffer[24+i*4], cBuffer[25+i*4]);
}

/*! \brief 7.1 Zone Eroll Rsp(IAS Zone Client) Sent by CIE
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C00      Len=0x08    AddrMode     DstAddr   DstEndPoint      Cluster ID    Eroll Code    ZoneID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS Zone cluster ID.
	Eroll Code -1bye-
		Code              Meaning               Details
		0x00              Success               Success
		0x01              Not supported         This specific Zone type is not known to the CIE and is not supported.
		0x02              No enroll permit      CIE does not permit new zones to enroll at this time.
		0x03              Too many zones        CIE reached its limit of number of enrolled zones
		0x04-0xfe         Reserved              -
	ZoneID -1byte- The Zone ID field is the index into the zone table of the CIE. This field is only relevant if the
 */
static void ZoneEnrollRsp(char * cBuffer, 
			  int * ddata)
{
	int ecode;
	char ecodeStr[8];

	printf("Zone Eroll Rsp: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	strncpy(ecodeStr, &cBuffer[20], 2);
	ecodeStr[2] = 0;
	ecode = (int) strtol(ecodeStr, NULL, 16);
	printf("EnrollCode ");
	switch (ecode)
	{
		case 0x00 : printf("Success, ");
		case 0x01 : printf("Not supported, ");
		case 0x02 : printf("No enroll permit, ");
		case 0x03 : printf("Too many zones, ");
		default : printf("Reserved %02x, ", ecode);
	}
	*ddata = 0;
}

/*! \brief 7.3 Zone Eroll Req(IAS Zone Client) CIE received.
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C01     Len=0x09      SrcAddr  SrcEndPoint    Cluster ID   Zone Type   Manufacturer Code
	SrcAddr – 2 byte –network address of the source address (device’s Zone status change and sent).
	SrcEndPoint– 1 byte –the source EndPoint. represents the application endpoint the data.
	Cluster ID– 2 byte –IAS Zone Cluster ID.
	Zone Type(reverse)-2byte- The Zone Type field shall be the current value of the ZoneType attribute.
	ManuFacturer Code(reverse) – 2bytes- The Manufacturer Code field shall be the manufacturer code as held in
		the node descriptor for the device. Manufacturer Codes are allocated by the ZigBee Alliance.
 */
static void ZoneEnroll(char * cBuffer, 
		       int * ddata)
{
	printf("Zone Enroll Request: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	ZoneID(&cBuffer[16]);
	ZoneType(&cBuffer[18]);
	printf("Manufacturer %c%c%c%c, ", cBuffer[24], cBuffer[25], cBuffer[22], cBuffer[23]);
	*ddata = 0;
}

/*! \brief 7.2 Zone Status Change Notification(IAS Zone Client)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C00      Len=0x07   SrcAddr    SrcEndPoint    Cluster ID  Zone Status     Extended Status
	SrcAddr – 2 byte –network address of the source address (device’s Zone status change and sent).
	SrcEndPoint– 1 byte –the source EndPoint. represents the application endpoint the data.
	Cluster ID– 2 byte –IAS Zone Cluster ID.
	Zone Status– 2 byte –See “ZoneStatus attribute of IAS Zone”
	Extended Status– 1 byte –The Extended Status field is reserved for additional status information and shall be
		set to zero.
*/
static void ZoneStatusChange(char * cBuffer, 
			     int * ddata)
{
	printf("Zone Status Change Notification: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	ZoneStatus(&cBuffer[18]);
	ExtendedStatus(&cBuffer[22]);
	*ddata = 0;
}

/*! \brief 7.4 Arm (IAS ACE)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C00      Len=0x07    AddrMode    DstAddr    DstEndPoint     Cluster ID Arm Mode
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	Arm Mode– byte –The Arm Mode field shall have one of the values shown following:
		0x00           →   Disarm
		0x01           →   Arm Day/Home Zones Only
		0x02           →   Arm Night/Sleep Zones Only
		0x03           →   Arm All Zones
		0x04-0xff      →   Reserved

*/
static void Arm(char * cBuffer, 
		int * ddata)
{
	printf("Arm: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	ArmMode(&cBuffer[20]);
	*ddata = 0;
}

/*! \brief 7.5 Arm response (IAS ACE)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C00      Len=0x06    SrcAddr   SrcEndPoint    Cluster ID   Arm Notification
	SrcAddr– 2 bytes –network address of the source address.
	SrcEndPoint– byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	Arm Notification– byte –The Arm Notification field shall have one of the values shown following:
		0x00           →   All Zones Disarm
		0x01           →   Only Day/Home Zones Armed
		0x02           →   Only Night/Sleep Zones Armed
		0x03           →   All Zones Armed
		0x04-0xff      →   Reserved

*/
static void ArmRsp(char * cBuffer,
		   int * ddata)
{
	printf("Arm response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	printf("ArmNotif ");
	if (cBuffer[18] == '0')
	{
		switch(cBuffer[19])
		{
			case '0' : printf("All Zones Disarm"); break;
			case '1' : printf("Only Day/Home Zones Armed"); break;
			case '2' : printf("Only Night/Sleep Zones Armed"); break;
			case '3' : printf("All Zones Armed"); break;
			default  : printf("Reserved"); break;
		}
	}
	else 
		printf("Reserved");
	printf(" (%c%c), ", cBuffer[18], cBuffer[19]);
	*ddata = 0;
}

/*! \brief 7.6 Bypass (IAS ACE)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C01  Len=var AddrMode DstAddr DstEndPoint Cluster ID Number of Zone Zone ID1 ... Zone IDn

	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	Number of Zone – byte –This is the number of Zone IDs included in the payload.
	Zone ID – byte –Zone ID is the index of the Zone in the CIE's zone table.

		Format of the Zone Table
	Field                    Valid range                Description
	Zone ID                  0x00 – 0xfe                The unique identifier of the zone
	Zone Type                0x0000 – 0xfffe            See “Zone Type attribute of IAS Zone”
	Zone Address             64bit IEEE                 Device address

 */
void Bypass(char * cBuffer, 
	    int * ddata)
{
	char nan[8];
	int i, num;

	printf("Bypass: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	strncpy(nan, &cBuffer[20], 2);
	nan[2] = 0;
	num = strtol(nan, NULL, 16);
	printf("Number %d, ", num);
	if (num)
	{
		printf("ZoneID");
		num *= 2;
		for (i=0; i<num; i+=2)
		{
			printf(" %c%c", cBuffer[22+i], cBuffer[23+i]);
		}
		printf(", ");
	}
	*ddata = 0;
}

/*! \brief 7.7 Emergency, Fire and Panic (IAS ACE)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=xxxx     Len=0x06    AddrMode     DstAddr   DstEndPoint    Cluster ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	Emergency(Cmd=0x0C02), Fire(Cmd=0x0C03),Panic(Cmd=0x0C04)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
 */
void Alarm(int Cmd, 
	   char * cBuffer, 
	   int * ddata)
{
	switch (Cmd)
	{
		case 0x0C02 : printf("Emergency: "); break;
		case 0x0C03 : printf("Fire: "); break;
		case 0x0C04 : printf("Panic: "); break;
		default : printf("Unknown alarm %04x: ", Cmd);
	}
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	*ddata = 0;
}

/*! \brief 7.8 Get Zone ID Map(IAS ACE)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C05        Len=0x06      AddrMode    DstAddr     DstEndPoint     Cluster ID

	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
*/
void GetZoneIdMap(char * cBuffer, 
		  int * ddata)
{
	printf("Get Zone ID Map: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	*ddata = 0;
}

/*! \brief 7.9 Get Zone ID Map response (IAS ACE)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C01     Len=0x08       SrcAddr  SrctEndPoint   Cluster ID   Zone ID Map (bit15)       Zone ID Map (bit0)
                                                                                            ...
	SrcAddr– 2 bytes –network address of the source address.
	SrcEndPoint– byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	Zone ID Map(reverse) -bit -The 16 fields of the payload indicate whether each of the Zone IDs from 0 to 0xff is
		allocated or not. If bit n of Zone ID Map section N is set to 1, then Zone ID (16 x N + n ) is allocated, else it is not
		allocated.
 */
void GetZoneIdMapRsp(char * cBuffer, 
		     int * ddata)
{
	printf("Get Zone ID Map response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	ZoneIdMap(&cBuffer[18]);
	*ddata = 0;
}

/*! \brief 7.12 Start Warning (IAS WD)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C00        Len=0x09     AddrMode      DstAddr                    DstEndPoint
Cluster ID        Strobe(bit4~5)             Warning mode(bit0~3)       Warning duration

	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS WD cluster ID.
	Warning mode/Strobe - byte
		Warning mode(bit0~3) The Warning Mode field is used as a 4-bit enumeration.Following:
			Warning             Mode Meaning
			0                   Stop (no warning)
			1                   Burglar
			2                   Fire
			3                   Emergency
			4-15                Reserved
		Strobe(bit4~5) The Strobe field is used as a 2-bit enumeration, and determines if the visual indication is
			required in addition to the audible siren, as indicated in Table 20. If the strobe field is "1" and the Warning Mode
			is "0" ("Stop") then only the strobe is activated.
			Value          Meaning
			0              No strobe
			1              Use strobe in parallel to warning
			2-3            Reserved
	Warning duration(reverse) –2 byte – Requested duration of warning, in seconds. If both Strobe and Warning
		Mode are "0" this field shall be ignored.
*/
static void StartWarning(char * cBuffer, 
			 int * ddata)
{
	int modeVal, mode, strob;
	char modeStr[8];

	printf("Start Warning (IAS WD): ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	strncpy(modeStr, &cBuffer[20], 2);
	modeStr[2] = 0;
	modeVal = strtol(modeStr, NULL, 16);
	mode = (modeVal&0xF0) >> 4;
	printf("Warning mode ");
	switch (mode)
	{
		case 0 :  printf("Stop (no warning)"); break;
		case 1 :  printf("Burglar"); break;
		case 2 :  printf("Fire"); break;
		case 3 :  printf("Emergency"); break;
		default : printf("Reserved");
	}
	printf(" %x, ", mode);
	strob = (modeVal&0x0C) >> 2;
	printf("Strobe ");
	switch (strob)
	{
		case 0 :  printf("No strobe"); break;
		case 1 :  printf("Use strobe in parallel to warning"); break;
		default : printf("Reserved");
	}
	printf(" %x, ", strob);
	printf("Warning duration %c%c%c%c, ", cBuffer[24], cBuffer[25], cBuffer[22], cBuffer[23]);
	*ddata = 0;
}

/*! \brief 7.13 Squawk (IAS WD)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C01      Len=0x07      AddrMode    DstAddr        DstEndPoint
     Cluster ID      Squawk level(bit6~7)      Strobe(bit4)   Squawk mode(bit0~3)

	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS WD cluster ID.
	Squawk mode(bit0~3) The Squawk Mode field is used as a 4-bit enumeration. Following
			Squawk Mode	Meaning
			0		Notification sound for "System is armed"
			1		Notification sound for "System is disarmed"
	Strobe(bit4) The strobe field is used as a boolean, and determines if the visual indication is also required in
		addition to the audible squawk.
			Value          Meaning
			0              No strobe
			1              Use strobe in parallel to warning
	Squawk level(bit6~7) The squawk level field is used as a 2-bit enumeration, and determines the intensity of
		audible squawk sound
			Value          Meaning
			0              Low level sound
			1              Medium level sound
			2              High level sound
			3              Very High level sound
 */
static void Squawk(char * cBuffer, 
		   int * ddata)
{
	char nan[8];
	int num, mode, strobe, level;

	printf("Squawk: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	strncpy(nan, &cBuffer[20], 2);
	nan[2] = 0;
	num = strtol(nan, NULL, 16);
	printf("Squawk %02x, ", num);
	mode = (num & 0xf0) >> 4;
	strobe = (num & 0x08) >> 3;
	level = num & 0x03;
	switch (mode)
	{
		case 0 : printf("Mode armed, "); break;
		case 1 : printf("Mode disarmed, "); break;
		default : printf("Mode %02x, ", mode);
	}
	switch (strobe)
	{
		case 0 : printf("Strobe off, "); break;
		case 1 : printf("Strobe on, "); break;
		default : printf("Strobe %02x, ", strobe);
	}
	switch (level)
	{
		case 0 : printf("Level Low, "); break;
		case 1 : printf("Level Medium, "); break;
		case 2 : printf("Level High, "); break;
		case 3 : printf("Level Very High, "); break;
		default : printf("Level %02x, ", level);
	}
	*ddata = 0;
}

/*! \brief 9.1 Zone Un-Eroll Request(IAS Zone ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C02     Len=0x07    AddrMode     DstAddr    DstEndPoint     Cluster ID ZoneID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS Zone cluster ID.
	ZoneID -1byte- The Zone ID field is the index into the zone table of the CIE.
*/
static void ZoneUnEnroll(char * cBuffer, 
			 int * ddata)
{
	printf("IAS Zone Un-Enroll: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	ZoneID(&cBuffer[20]);
	*ddata = 0;
}

/*! \brief 9.2 Zone Un-Eroll response(IAS Zone ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C01      Len=0x06    SrcAddr   SrcEndPoint  Cluster ID    Status
	SrcAddr – 2 bytes –network address of the source device
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –IAS Zone Cluster ID.
	Status -1byte- The status is a SUCCESS (0) or FAILURE
*/
static void ZoneUnEnrollRsp(char * cBuffer, 
			    int * ddata)
{
	printf("Zone Un-Enroll response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	Status(&cBuffer[18]);
	*ddata = 0;
}

/*! \brief 9.3 Un-Bypass(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

     Cmd=0x0C07    Len=var AddrMode     DstAddr DstEndPoint Cluster ID Number of Zone  Zone ID1 ...  Zone IDn

	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	Number of Zone – byte –This is the number of Zone IDs included in the payload.
	Zone ID – byte –Zone ID is the index of the Zone in the CIE's zone table.

		Format of the Zone Table
		------------------------
		Field                    Valid range                Description
		Zone ID                  0x00 – 0xfe                The unique identifier of the zone
		Zone Type                0x0000 – 0xfffe            See “Zone Type attribute of IAS Zone”
		Zone Address             64bit IEEE                 Device address
 */
static void UnBypass(char * cBuffer, 
		     int * ddata)
{
	char nan[8];
	int i, num;

	printf("Un-Bypass: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	strncpy(nan, &cBuffer[20], 2);
	nan[2] = 0;
	num = strtol(nan, NULL, 16);
	printf("Number %d, ", num);
	if (num)
	{
		printf("ZoneID");
		num *= 2;
		for (i=0; i<num; i+=2)
		{
			printf(" %c%c", cBuffer[22+i], cBuffer[23+i]);
		}
		printf(", ");
	}
	*ddata = 0;
}

/*! \brief 9.4 Set Heart Beat Period(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C08     Len=0x09  AddrMode     DstAddr  DstEndPoint    Cluster ID HeartBeatPeriod  MaxLossHeartBeat
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – 1 byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte – IAS ACE cluster ID.
	HeartBeatPeriod(reverse) - 2bytes - Set the device heart beat period.
	MaxLossHeartBeat - 1 byte - The maximum count of loss heart beat
 */
static void SetHeartBeatPeriod(char * cBuffer, 
			       int * ddata)
{
	int count;
	char countStr[8];

	printf("Set Heart Beat Period: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	/*?
	strncpy(countStr, &cBuffer[20], 2);
	countStr[2] = 0;
	?*/
	sprintf(countStr, "%c%c%c%c", cBuffer[22], cBuffer[23], cBuffer[20], cBuffer[21]);
	count = strtol(countStr, NULL, 16);
	printf("HeartBeatPeriod %d, ", count);
	strncpy(countStr, &cBuffer[24], 2);
	countStr[2] = 0;
	count = strtol(countStr, NULL, 16);
	printf("MaxLossHeartBeat %d, ", count);
	*ddata = 0;
}

/*! \brief 9.5 Set Heart Beat Period Response(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C07      Len=0x06    SrcAddr    SrcEndPoint    Cluster ID   Status
	SrcAddr – 2 bytes –network address of the source device
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 bytes –IAS Zone Cluster ID.
	Status -1byte- The status is a SUCCESS (0) or FAILURE
 */
static void SetHeartBeatPeriodRsp(char * cBuffer, 
				  int * ddata)
{
	printf("Set Heart Beat Period response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	Status(&cBuffer[18]);
	*ddata = 0;
}

/*! \brief 9.6 Get Zone Trouble Map(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C09     Len=0x06    AddrMode     DstAddr    DstEndPoint     Cluster ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
 */
void GetZoneTroubleMap(char * cBuffer, 
		       int * ddata)
{
	printf("Get Zone Trouble Map: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	*ddata = 0;
}

/*! \brief 9.7 Get Zone Trouble Map response(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C03     Len=0x07       SrcAddr  SrctEndPoint   Cluster ID   Zone ID Map (bit15)       Zone ID Map (bit0)
	SrcAddr– 2 bytes –network address of the source address.
	SrcEndPoint– byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	Zone ID Map(reverse) -bit -The 16 fields of the payload indicate whether each of the Zone IDs from 0 to 0xff is
		allocated or not. If bit n of Zone ID Map section N is set to 1, then Zone ID (16 x N + n ) is allocated, else it is not
		allocated.
*/
void GetZoneTroubleMapRsp(char * cBuffer, 
			  int * ddata, 
			  int dlen)
{
	printf("Get Zone Trouble Map response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	/* TODO undocumented territory here */
	if (dlen == 37)
	{
		ZoneIdMap(&cBuffer[18]);
	}
	*ddata = 0;
}

/*! \brief 9.8 Read Heart Beat period(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C0A       Len=0x06  AddrMode     DstAddr    DstEndPoint     Cluster ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
*/
void ReadHeartBeatPeriod(char * cBuffer, 
			 int * ddata)
{
	int addrMode;
	
	printf("Read Heart Beat Period: ");
	addrMode = AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	*ddata = 0;
}

/*! \brief 9.9 Read Heart Beat Period(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C04    Len=8   SrcAddr  SrcEndPoint  Cluster ID  HeartBeatPeriod  MaxLossHeartBeat
	SrcAddr– 2 bytes –network address of the source address.
	SrcEndPoint– byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte – IAS ACE cluster ID.
	HeartBeatPeriod(reverse) - 2 bytes - Set the device heart beat period.
	MaxLossHeartBeat - 1 byte - The maximum count of loss heart beat
*/
static void ReadHeartBeatPeriodRsp(char * cBuffer, 
				   int * ddata)
{
	char symStr[8];
	int sym;

	printf("Read Heart Beat Period response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	
	snprintf(symStr, sizeof(symStr), "%c%c%c%c", cBuffer[20], cBuffer[21], cBuffer[18], cBuffer[19]);
	sym = strtol(symStr, NULL, 16);
	printf("HeartBeatPeriod %d, ", sym);
	
	strncpy(symStr, &cBuffer[22], 2);
	symStr[2] = 0;
	sym = strtol(symStr, NULL, 16);
	printf("MaxLossHeartBeat %d, ", sym);
	*ddata = 0;
}

/*! \brief 9.10 Alarm Device Notify(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C05         Len=25      SrcAddr    SrcEndPoint    ClusterID    ZoneID ZoneType     IEEEAddress
		Zone nwkAddr    Zone EndPoint     ManufacturerCode       ZoneStatus    Symbol        Waring Mode
	SrcAddr– 2 bytes –network address of the source address.
	SrcEndPoint– byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	ZoneID -1byte - Zone ID is the index of the Zone in the CIE's zone table.
	ZoneType(reverse) – 2 byte –See” Zone Type attribute of IAS Zone”
	IEEE Address(reverse) –8 bytes –IEEE Address of Zone device
	Zone nwkAddr(reverse) -2bytes- ShortAddress of Zone device
	Zone EndPoint -1byte- EndPoint of Zone device
	ManufacturerCode(reverse) – 2bytes- The manufacturer code field is 16-bits in length 
		and specifies the ZigBee assigned manufacturer code for proprietary extensions 
		to a profile.
	ZoneStatus(reverse) – 2bytes- See “ZoneStatus attribute of IAS Zone”
	Symbol – byte- Record Zone Information
		Bit                Description
		Bit7               0 -> NO 1 -> presence
		Bit6               0 -> Unbypass 1 -> Bypass
		Bit5               0 -> night Zone 1 -> day Zone
		Bit4-bit0          HeartBeatCount
	Waring Mode – byte- Alarm type
		Value            Description
		0x00             No Waring
		0x01             BurgLar
		0x02             Fire
		0x03             Emergency
		0x04-0x0D        Reserved
		0x0E             Device Trouble
		0x0F             DoorBell
		0x10-0xFF        Reserved
*/
void AlarmDeviceNotify(char * cBuffer, 
		       int * ddata)
{
	char symStr[8];
	int sym;
	
	printf("Alarm Device Notify: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	ZoneID(&cBuffer[18]);
	printf("ZoneType %c%c%c%c, ", cBuffer[22], cBuffer[23], cBuffer[20], cBuffer[21]);
	dispIeeeAddr(&cBuffer[24], 1);
	printf("ZoneNwkAddr %c%c%c%c, ", cBuffer[42], cBuffer[43], cBuffer[40], cBuffer[41]);
	printf("ZoneEndPoint %c%c, ", cBuffer[44], cBuffer[45]);
	printf("Manuf %c%c%c%c, ", cBuffer[48], cBuffer[49], cBuffer[46], cBuffer[47]);
	ZoneStatus(&cBuffer[50]);
	
	/* Symbol */
	printf("Symbol %c%c ", cBuffer[54], cBuffer[55]);
	strncpy(symStr, &cBuffer[54], 2);
	symStr[2] = 0;
	sym = strtol(symStr, NULL, 16);
	if (sym & 0x80) printf("Presence ");
	else printf("No presence ");
	if (sym & 0x40) printf("Bypassed ");
	else printf("Unbypassed ");
	if (sym & 0x20) printf("day Zone ");
	else printf("night Zone ");
	printf("Heartbeat %2d, ", sym&0x1f);
	
	WarningMode(&cBuffer[56]);
	*ddata = 0;
}

int readAlarmDeviceNotify(char * cBuffer, 
			  unitInfoRec * unit)
{
	char symStr[8];
	int sym, warn, presence, bypass, zone, hbeat;
	char ieeeBuf[40];
	int i, loc=0;
	
	if (trace) printf("\tRead Alarm Notification: ");
	
	readIeeeAddr(&cBuffer[24], ieeeBuf);
	
	/* Symbol */
	strncpy(symStr, &cBuffer[54], 2);
	symStr[2] = 0;
	sym = strtol(symStr, NULL, 16);
	presence = sym & 0x80;
	bypass = sym & 0x40;
	zone = sym & 0x20;
	hbeat = sym&0x1f;
	
	strncpy(symStr, &cBuffer[56], 2);
	symStr[2] = 0;
	warn = strtol(symStr, NULL, 16);
	
	for (i=0; i<MAX_PIR; i++)
	{
		if (! strcmp(ieeeBuf, unit->pirieee[i]))
		{
			loc = unit->pirloc[i];
			break;
		}
	}
	
	if (trace)
	{
		printf("IEEE address %s, ", ieeeBuf);
		printf("Heartbeat %2d, ", hbeat);
		if (presence) printf("Presence ");
		else printf("No presence ");
		if (bypass) printf("Bypassed ");
		else printf("Unbypassed ");
		if (zone) printf("Day Zone ");
		else printf("Night Zone ");
		switch(warn)
		{
			case 0 : printf("No Warning "); break;
			case 1 : printf("Burglar "); break;
			case 2 : printf("Fire "); break;
			case 3 : printf("Emergency "); break;
			case 0xE : printf("Device Trouble "); break;
			case 0xF : printf("DoorBell "); break;
			default  : printf("Reserved ");
		}
		printf("(Location %d)\n", loc);
	}
	return loc;
}

/*! \brief 9.11 Get Recent Alarm Information(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C0B       Len=0x06  AddrMode     DstAddr    DstEndPoint     Cluster ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID. 
*/
void GetRecentAlarmInfo(char * cBuffer, 
			int * ddata)
{
	printf("Get Recent Alarm Information: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	*ddata = 0;
}

/*! \brief 9.12 Get Recent Alarm Information response(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C06    Len=Var  SrcAddr   SrcEndPoint ClusterID Number   ZoneID   WaringMode ......   ZoneID WaringMode
     SrcAddr– 2 bytes –network address of the source address.
     SrcEndPoint– byte – the destination EndPoint. represents the application endpoint the data.
     Cluster ID – 2 byte –IAS ACE cluster ID.
     Number-byte- the number of alarm information
     ZoneID -1byte - Zone ID is the index of the Zone in the CIE's zone table.
     WaringMode-byte-Alarm Type,See”Alarm device Notify”Command
*/
void GetRecentAlarmInfoRsp(char * cBuffer,
			   int * ddata)
{
	int i, count;
	char countStr[8];
	
	printf("Get Recent RecentAlarm Information response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	
	strncpy(countStr, &cBuffer[18], 2);
	countStr[2] = 0;
	count = strtol(countStr, NULL, 16);
	
	printf("Number %d, ", count);
	for (i=0; i<count; i++)
	{
		ZoneID(&cBuffer[20+i*2]);
		WarningMode(&cBuffer[22+i*2]);
	}
	*ddata = 0;
}

/*! \brief 9.13 Set Day-Night Type(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C0C       Len=0x08   AddrMode      DstAddr     DstEndPoint    Cluster ID ZoneID   Type
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	ZoneID-1byte- The Zone ID field is the index into the zone table of the CIE.
	Type-1byte- 0 night zone;1 day zone
*/
void SetDayNightType(char * cBuffer, 
		     int * ddata)
{
	char nan[8];
	long int num;

	printf("Set Day-Night Type: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	ZoneID(&cBuffer[20]);
	strncpy(nan, &cBuffer[22], 2);
	nan[2] = 0;
	num = strtol(nan, NULL, 16);
	switch (num)
	{
		case 0 : printf("Type night zone, "); break;
		case 1 : printf("Type day zone, "); break;
		default : printf("Type zone %ld, ", num);
	}
	*ddata = 0;
}

/*! \brief 9.14 Get Arm Mode(ACE ClusterID)
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0C0D       Len=0x06   AddrMode      DstAddr     DstEndPoint    Cluster ID
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the destination address.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
*/
void GetArmMode(char * cBuffer, 
		int * ddata)
{
	printf("Get Arm Mode: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	*ddata = 0;
}

/*! \brief 9.15 Get Arm Mode response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x1C08    Len=Var SrcAddr SrcEndPoint ClusterID ArmMode
	SrcAddr– 2 bytes –network address of the source address.
	SrcEndPoint– byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –IAS ACE cluster ID.
	ArmMode-byte- Described as follows
		Value             Description
		0x00              All zones DisArm
		0x01              Arm Day/Home zones only
		0x02              Arm Night/Sleep zones only
		0x03              Arm All Zones
		0x04-0xFF         Reserved
*/
void GetArmModeRsp(char * cBuffer,
		   int * ddata)
{
	printf("Get Arm Mode response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	ArmMode(&cBuffer[18]);
	*ddata = 0;
}

/*! \brief 12.2 Read Attribute response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0D01            Len=var      SrcAddr   SrcEndPonit     Cluster ID   Number
	Attribute ID1	Status	Data type	Attribute data
	Attribute IDn	Status	Data type	Attribute data
	
	SrcAddr – 2 bytes –network address of the device being read attributes.
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –All Cluster ID.
	Number – 1 byte –number of attributes in the list.
	Attribute ID–List –The attribute identifier field is 16-bits in length and shall contain the identifier of the attribute that
		is to be read.
	Status – byte – This field shall be set to SUCCESS or UNSUPPORTED_ATTRIBUTE.
	Data type – byte –See “Data Type Table”
	Attribute data The attribute data field is variable in length and shall contain the current value of this attribute. This
		field shall only be included if the associated status field contains a value of SUCCESS.
*/
static void ReadAttributeRsp(char * cBuffer, 
			     int * ddata)
{
	int i , n, num, dlen, sp, sts, bch, dtype;
	char nan[64];
	char numSt[64];
	long int numVal;
	
	*ddata = 0;
	printf("Read Attribute response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	strncpy(nan, &cBuffer[18], 2);
	nan[2] = 0;
	num = (int) strtol(nan, NULL, 16);
	printf("Number %02d, ", num);
	sp = 20;
	for (n=0; n<num; n++)
	{
		printf("AttributeId%d %c%c%c%c, ", n+1, cBuffer[sp], cBuffer[sp+1], cBuffer[sp+2], cBuffer[sp+3]);
		sts = Status(&cBuffer[sp+4]);
		if (sts == 0)
		{
			dlen = dataType(&cBuffer[sp+6], nan, &dtype);
			if (dtype == 0x42)
			{
				printf(" %d bytes '", dlen);
				for (i=1; i<=dlen; i++)
				{
					strncpy(nan, &cBuffer[sp+8+i*2], 2);
					nan[2] = 0;
					bch = strtol(nan, NULL, 16);
					printf("%c", bch);
				}
				printf("'");
				sp += 2*dlen + 8;
			}
			/*
			*/
			else if (dtype == 0x29)
			{
				printf(" %d. bytes ", dlen);
				dlen *= 2;
				for (i=0; i<dlen; i++)
				{
					numSt[i] = cBuffer[sp+8+i];
				}
				numSt[i] = 0;
				sp += dlen + 8;
				numVal = strtol(numSt, NULL, 16);
				printf(" %05ld", numVal);
			}
			else if (dlen == -8)
			{
				dispIeeeAddr(&cBuffer[sp+8], 1);
				sp += 16;
			}
			else
			{
				printf(" %d. bytes ", dlen);
				dlen *= 2;
				for (i=0; i<dlen; i++)
					printf("%c", cBuffer[sp+8+i]);
				sp += dlen + 8;
			}
			printf(", ");
		}
	}
}

/*! \brief 12.3 Write Attribute
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0D02        Len=var       AddrMode     DstAddr   DstEndPoint    Cluster ID     Number
Attribute ID1   DataType     Attribute data           Attribute IDn  DataType    Attribute data
                                                       ...
	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the device being read attributes.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –All Cluster ID.
	Number – 1 byte –number of attributes in the list.
	Attribute ID1...n –List –The attribute identifier field is 16-bits in length and shall contain the identifier of the
		attribute that is to be written.
	Data type – byte –See “Data Type Table”
	Attribute data (reverse)The attribute data field is variable in length and shall contain the actual value of the
		attribute that is to be written.
 */
static void WriteAttribute(char * cBuffer, 
			   int * ddata)
{
	int i, n, num, sp, dlen;
	int bch, dtype;
	char numSt[8];
	char nan[64];

	printf("Write Attribute: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	strncpy(numSt, &cBuffer[20], 2);
	numSt[2] = 0;
	num = strtol(numSt, NULL, 16);
	printf("Number %d, ", num);
	sp = 22;
	for (n=0; n<num; n++)
	{
		printf("AttributeId%d %c%c%c%c, ", n+1, cBuffer[sp], cBuffer[sp+1], cBuffer[sp+2], cBuffer[sp+3]);
		dlen = dataType(&cBuffer[sp+4], nan, &dtype);
		if (dtype == 0x42)
		{
			printf(" %d bytes '", dlen);
			for (i=1; i<=dlen; i++)
			{
				strncpy(nan, &cBuffer[sp+6+i*2], 2);
				nan[2] = 0;
				bch = strtol(nan, NULL, 16);
				printf("%c", bch);
			}
			printf("'");
			sp += 2*dlen + 8;
		}
		else if (dtype == 0x21)
		{
			unsigned long int numVal;

			dlen *= 2;
			for (i=0; i<dlen; i++)
			{
				numSt[i] = cBuffer[sp+6+i];
			}
			numSt[i] = 0;
			sp += dlen + 8;
			numVal = strtoul(numSt, NULL, 16);
			printf("%05ld", numVal);
		}
		else if (dtype == 0x29)
		{
			long int numVal;

			dlen *= 2;
			for (i=0; i<dlen; i++)
			{
				numSt[i] = cBuffer[sp+6+i];
			}
			numSt[i] = 0;
			sp += dlen + 8;
			numVal = strtol(numSt, NULL, 16);
			printf("%05ld", numVal);
		}
		else if (dlen == -8)
		{
			dispIeeeAddr(&cBuffer[sp+8], 1);
			sp += 16;
		}
		else
		{
			printf(" %d. bytes ", dlen);
			dlen *= 2;
			for (i=0; i<dlen; i++)
				printf("%c", cBuffer[sp+6+i]);
			sp += dlen + 8;
		}
		printf(", ");
	}
	*ddata = 0;
}

/*! \brief 12.5 Write Attribute response
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0D04            Len=var       SrcAddr  SrcEndPoint      Cluster ID       Number
	Status         Attribute ID1                                Status      Attribute IDn
                                                    ...
	SrcAddr – 2 bytes –network address of the device being written attributes.
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –All Cluster ID.
	Number – 1 byte –number of attributes in the list.
	Status – byte – This field shall be set to SUCCESS or UNSUPPORTED_ATTRIBUTE.
		In the case of successful writing of all attributes, only a single write attribute status record shall be included
		in the command, with the status field set to SUCCESS and the attribute identifier field omitted.
	Attribute ID1...n –List –The attribute identifier field is 16-bits in length and shall contain the identifier of the
		attribute that is to be written.
 */
static void WriteAttributeRsp(char * cBuffer, 
			      int * ddata)
{
	int i, n, num, sp, dlen;
	int bch, dtype;
	char numSt[8];
	char nan[64];

	printf("Write Attribute response: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	strncpy(numSt, &cBuffer[18], 2);
	numSt[2] = 0;
	num = strtol(numSt, NULL, 16);
	printf("Number %d, ", num);
	sp = 20;
	for (n=0; n<num; n++)
	{
		printf("AttributeId%d %c%c%c%c, ", n+1, cBuffer[sp], cBuffer[sp+1], cBuffer[sp+2], cBuffer[sp+3]);
		dlen = dataType(&cBuffer[sp+4], nan, &dtype);
		if (dtype == 0x42)
		{
			printf(" %d bytes '", dlen);
			for (i=1; i<=dlen; i++)
			{
				strncpy(nan, &cBuffer[sp+6+i*2], 2);
				nan[2] = 0;
				bch = strtol(nan, NULL, 16);
				printf("%c", bch);
			}
			printf("'");
			sp += 2*dlen + 8;
		}
		else if (dtype == 0x21)
		{
			unsigned long int numVal;

			dlen *= 2;
			for (i=0; i<dlen; i++)
			{
				numSt[i] = cBuffer[sp+6+i];
			}
			numSt[i] = 0;
			sp += dlen + 8;
			numVal = strtoul(numSt, NULL, 16);
			printf("%05ld", numVal);
		}
		else if (dtype == 0x29)
		{
			long int numVal;

			dlen *= 2;
			for (i=0; i<dlen; i++)
			{
				numSt[i] = cBuffer[sp+6+i];
			}
			numSt[i] = 0;
			sp += dlen + 8;
			numVal = strtol(numSt, NULL, 16);
			printf("%05ld", numVal);
		}
		else if (dlen == -8)
		{
			dispIeeeAddr(&cBuffer[sp+8], 1);
			sp += 16;
		}
		else
		{
			printf(" %d. bytes ", dlen);
			dlen *= 2;
			for (i=0; i<dlen; i++)
				printf("%c", cBuffer[sp+6+i]);
			sp += dlen + 8;
		}
		printf(", ");
	}
	*ddata = 0;
}

/*! \brief 12.7 Configure reporting used to configure the reporting mechanism for one or more of the attributes of a cluster.
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0D06               Len=var               AddrMode        DstAddr  DstEndPoint                      Cluster ID
	Number        Attribute reporting configuration record 1                             Attribute reporting configuration record n

Attribute reporting configuration record:
Attribute ID  Direction    dataType    Minimum reporting interval  Maximum reporting interval   Timeout period    Reportable change

	AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits
		GroupAddress (Addrmode = 0x01)
	DstAddr – 2 bytes –network address of the device being read attributes.
	DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –All Cluster ID.
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
static void ConfigReport(char * cBuffer, 
			 int * ddata)
{
	int sp;
	
	printf("Configure reporting: ");
	AddrMode(&cBuffer[8]);
	DstAddr(&cBuffer[10]);
	DstEndpoint(&cBuffer[14]);
	ClusterId(&cBuffer[16]);
	sp = AttReportList(&cBuffer[20]);
	*ddata = 0;
}

/*! \brief 12.11     Report attributes
	\param	buf	input buffer
	\param	ddata	data display flag

Cmd=0x0D0A         Len=var                    SrcAddr    SrcEndPoint     Cluster ID     Number
      Attribute ID1                 Attribute Data                             Attribute IDn             Attribute Data
                         dataType                                                             dataType

	SrcAddr – 2 bytes –network address of the device being written attributes.
	SrcEndpoint – byte – the source EndPoint. represents the application endpoint the data.
	Cluster ID – 2 byte –All Cluster ID.
	Number – 1 byte –number of attributes in the list.
	Attribute ID1...n –List –The attribute identifier field is 16-bits in length and shall contain the identifier of the
		attribute that is to be report.
	Data type – byte –See “Data Type Table”
	Attribute data The attribute data field is variable in length and shall contain the actual value of the attribute that
		is to be written.

*/
static void ReportAttributes(char * cBuffer,
			     int * ddata)
{
	int i , n, num, dlen, sp, bch, dtype;
	char nan[64];
	char numSt[64];
	
	*ddata = 0;

	printf("Report Attributes: ");
	SrcAddr(&cBuffer[8], 2);
	SrcEndpoint(&cBuffer[12]);
	ClusterId(&cBuffer[14]);
	strncpy(nan, &cBuffer[18], 2);
	nan[2] = 0;
	num = (int) strtol(nan, NULL, 16);
	printf("Number %02d, ", num);
	sp = 20;
	for (n=0; n<num; n++)
	{
		printf("AttributeId%d %c%c%c%c, ", n+1, cBuffer[sp], cBuffer[sp+1], cBuffer[sp+2], cBuffer[sp+3]);
		dlen = dataType(&cBuffer[sp+4], nan, &dtype);
		if (dtype == 0x42)
		{
			printf(" %d bytes '", dlen);
			for (i=1; i<=dlen; i++)
			{
				strncpy(nan, &cBuffer[sp+6+i*2], 2);
				nan[2] = 0;
				bch = strtol(nan, NULL, 16);
				printf("%c", bch);
			}
			printf("'");
			sp += 2*dlen + 8;
		}
		else if (dtype == 0x21)
		{
			unsigned long int numVal;

			dlen *= 2;
			for (i=0; i<dlen; i++)
			{
				numSt[i] = cBuffer[sp+6+i];
			}
			numSt[i] = 0;
			sp += dlen + 8;
			numVal = strtoul(numSt, NULL, 16);
			printf("%05ld", numVal);
		}
		else if (dtype == 0x29)
		{
			long int numVal;

			dlen *= 2;
			for (i=0; i<dlen; i++)
			{
				numSt[i] = cBuffer[sp+6+i];
			}
			numSt[i] = 0;
			sp += dlen + 8;
			numVal = strtol(numSt, NULL, 16);
			printf("%05ld", numVal);
		}
		else if (dlen == -8)
		{
			dispIeeeAddr(&cBuffer[sp+8], 1);
			sp += 16;
		}
		else
		{
			printf(" %d. bytes ", dlen);
			dlen *= 2;
			for (i=0; i<dlen; i++)
				printf("%c", cBuffer[sp+6+i]);
			sp += dlen + 8;
		}
		printf(", ");
	}
}

/*! \brief Display buffer
	\param	buf	input buffer
	\param	cLen	data size
	\param	suffix	string to display at end of line

	Acronyms and abbreviations
		ACE 	Ancillary Control Equipment
		CIE 	Control and Indicating Equipment
		IAS 	Intruder Alarm System(s)
		WD 	Warning device(s)
*/
void dispBuffer(char * cBuffer, 
		int cLen, 
		char * suffix)
{
	char CmdStr[8];
	long Cmd;
	int dLen;
	int i, iEnd;
	int dispData = 1;
	int dc;
	
	if (0 == cLen) return;
	
	strncpy(CmdStr, &cBuffer[2], 4);
	CmdStr[4] = 0;
	Cmd = strtol(CmdStr, NULL, 16);
	
	if (verbose)
	{
		printf("* %-50s (%02d) ", cBuffer, cLen);
		if (0 == strncmp(cBuffer, "02", 2)) printf("* ");
		else printf("? ");
	}
	
	if (0 != strncmp(cBuffer, "02", 2)) 
	{
		printf("Invalid packet %c%c [", cBuffer[0], cBuffer[1]);
		for (i=2; i<cLen; i++) printf("%c", cBuffer[i]);
		printf("]\n");
		return;
	}
	
	if (cLen < 9)
	{
		printf("Invalid command %c%c [", cBuffer[0], cBuffer[1]);
		for (i=2; i<cLen; i++) printf("%c", cBuffer[i]);
		printf("]\n");
		return;
	}
	
	CommandId(&cBuffer[2]);
	
	strncpy(CmdStr, &cBuffer[6], 2);
	CmdStr[2] = 0;
	dLen = strtol(CmdStr, NULL, 16);
	printf("Len %2d, ", dLen);
	
	if (cBuffer[cLen-1] == 'Z')
		iEnd = cLen-11;
	else
		iEnd = cLen-10;
	
	switch(Cmd)
	{
		/* System */
		case 0X0005 : printf("%-40s", "System Reset request"); break;
		case 0X1005 : printf("%-40s", "System Reset response"); break;
		case 0X0007 : SystemPing(cBuffer, &dispData); break;
		case 0X1007 : SystemPingRsp(cBuffer, &dispData); break;
		case 0X001C : printf("%-40s", "System Set Channels"); break;
		case 0X101C : printf("%-40s", "System Set Channels response"); break;
		case 0X0010 : 
			printf("%-40s", "System Set Extended Address request: IEEE address "); 
			for (i=0; i<16; i++) printf("%c", cBuffer[i+8]);
			dispData = 0;
			printf(", "); 
			break;
		case 0X1010 : 
			printf("%-40s", "System Set Extended Address response"); 
			break;
		case 0X0011 : 
			printf("%-40s", "System Get Extended Address request, "); 
			dispData = 0;
			break;
		case 0X1011 : 
			printf("%-40s", "System Get Extended Address response: IEEE address "); 
			for (i=0; i<16; i++) printf("%c", cBuffer[i+8]);
			dispData = 0;
			printf(", ");
			break;
		case 0X0014 : 
			printf("%-40s", "System Get Device Information");
			dispData = 0;
			break;
		case 0X1014 : SysDevInfoRsp(cBuffer, &dispData); break;
		case 0X001B : printf("%-40s", "System Set PAN ID"); break;
		case 0X101B : printf("%-40s", "System Set PAN ID Rsp"); break;
		case 0X0023 : printf("%-40s", "System Set Baud Rate"); break;
		/* ZDO interface */
		case 0x0A02 : NetworkAddress(cBuffer, &dispData); break;
		case 0x0A80 : NetworkAddressRsp(cBuffer, &dispData); break;
		case 0x0A03 : IeeeRequest(cBuffer, &dispData); break;
		case 0x0A81 : IeeeResponse(cBuffer, &dispData); break;
		case 0x0A05 : PowerDescriptor(cBuffer, &dispData); break;
		case 0x0A83 : PowerDescriptorRsp(cBuffer, &dispData); break;
		case 0x0A04 : NodeDescriptor(cBuffer, &dispData); break;
		case 0x0A82 : NodeDescriptorRsp(cBuffer, &dispData); break;
		case 0x0A06 : SimpleDescriptor(cBuffer, &dispData); break;
		case 0X0A84 : SimpleDescriptorRsp(cBuffer, &dispData); break;
		case 0x0A07 : ActiveEndPoint(cBuffer, &dispData); break;
		case 0X0A85 : ActiveEndPointRsp(cBuffer, &dispData); break;
		case 0x0A0A : UserDescription(cBuffer, &dispData); break;
		case 0X0A8F : UserDescriptionRsp(cBuffer, &dispData); break;
		case 0x0A13 : UserDescriptorSet(cBuffer, &dispData); break;
		case 0X0A90 : UserDescriptorSetRsp(cBuffer, &dispData); break;
		case 0X0A08 : printf("%-40s", "End device Bind request"); break;
		case 0x0A0B : EndDeviceBind(cBuffer, &dispData); break;
		case 0x0A87 : EndDeviceBindRsp(cBuffer, &dispData); break;
		case 0X0A0C : Bind(cBuffer, &dispData); break;
		case 0X0A88 : BindRsp(cBuffer, &dispData); break;
		case 0x0A0D : Unbind(cBuffer, &dispData); break;
		case 0X0A89 : UnbindRsp(cBuffer, &dispData); break;
		case 0x0A0F : ManageLqiReq(cBuffer, &dispData); break;
		case 0x0A8B : ManageLqiRsp(dLen, cBuffer, &dispData, 1); break;
		case 0x0A10 : printf("%-40s", "Manage RTG request"); break;
		case 0X0A8C : printf("%-40s", "Manage RTG reponse"); break;
		case 0x0A11 : ManageBind(cBuffer, &dispData); break;
		case 0X0A8D : ManageBindRsp(cBuffer, &dispData); break;
		case 0x0A16 : ManagePermitJoin(cBuffer, &dispData); break;
		case 0X0A92 : ManagePermitJoinRsp(cBuffer, &dispData); break;
		case 0x0A0E : ManageNetworkDiscovery(cBuffer, &dispData); break;
		case 0x0A8A : ManageNetworkDiscoveryRsp(cBuffer, &dispData); break;
		case 0x0A15 : printf("%-40s", "Manage Leave request"); break;
		case 0X0A91 : printf("%-40s", "Manage Leave response"); break;
		case 0X0AF0 : EndDeviceAnnce(cBuffer, &dispData); break;
		/* ZCL interface */
		/* Security and Safety */
		case 0X0C00 :
			/* TODO find a better way */
			if (cBuffer[cLen-1] == 'Z') printf("%-40s", "Identify (Identify)");
			else if (dLen == 6) FactoryDefaults(cBuffer, &dispData);
			else if (dLen == 8) ZoneEnrollRsp(cBuffer, &dispData);
			else if (dLen == 7) Arm(cBuffer, &dispData);
			else if (dLen == 9) StartWarning(cBuffer, &dispData);
#if 0
			else if (dLen == 9) SendResetAlarm(cBuffer, &dispData);
			else if (dLen == 7) printf("%-40s", "Arm (IAS ACE)");
			else if (dLen == 9) printf("%-40s", "Send Move to Level (Level Control)");
			/* a device shall move from its current level to the value given in the Level field */
			else if (1) printf("%-40s", "Add Scene (Scene)");
			else if (2) printf("%-40s", "Get Scene Membership (Scene)");
#endif
			break;
		case 0x0C01 :
			/* TODO will not work properly  */
			if (dLen == 6) ResetAllAlarm(cBuffer, &dispData);
			else if (dLen == 8) ViewGroup(cBuffer, &dispData);
			else if (dLen == 7) Squawk(cBuffer, &dispData);
#if 0
			else if (dLen == 6) printf("%-40s", "Identify Query request (Identify)");
			else if (dLen == 8) printf("%-40s", "Send Move (Level Control)");
#endif
			else if (dLen == 9) printf("%-40s", "View Scene (Scene)");
			else Bypass(cBuffer, &dispData);
			break;
		case 0x0C07 : UnBypass(cBuffer, &dispData); break;
		case 0x0C02 :
			/* TODO will not work properly */
			if (dLen == 9) printf("%-40s", "Remove Scene (Scene)");
			else if (dLen == 0x0E) printf("%-40s", "Get Device Configuration (RSSI Location)->CC2431");
			else if (dLen == 7) ZoneUnEnroll(cBuffer, &dispData);
			else if (dLen == 6) GetAlarm(cBuffer, &dispData);
			else if (dLen == 8) printf("%-40s", "Send Step (Level Control)");
#if 0
			else if (dLen == 6) printf("%-40s", "Send Toggle (On/Off)");
#endif
			else GetGroupMembership(cBuffer, &dispData);
			break;
		case 0x0C03 :
			/* TODO duplicate if clauses */
			if (dLen == 0x06) Alarm(0x0C03, cBuffer, &dispData);
			else if (dLen == 8) printf("%-40s", "Remove Group (Group)");
			else if (dLen == 6) printf("%-40s", "Reset Alarm Log (Alarm)");
#if 0
			else if (dLen == 6) printf("%-40s", "Stop (Level Control)");
#endif
			else if (1) printf("%-40s", "Remove All Scenes (Scene)");
			else printf("%-40s", "Get Location Data (RSSI Location)");
			break;
		case 0x0C04 : 
			/* TODO duplicate if clauses */
			if (dLen == 0x06) Alarm(0x0C04, cBuffer, &dispData);
			else if (dLen == 0x09) StoreSceneReq(cBuffer, &dispData);
			else printf("%-40s", "Unknown");
			break;
		case 0x0C05 : 
			if (dLen == 0x06) GetZoneIdMap(cBuffer, &dispData);
			/* TODO this is fubar */
#if 0
			else if (dLen == 0x06) printf("%-40s", "Blind Node Data Request");
#endif
			else printf("%-40s", "Add Group If Identifying (Group)");
			break;
		case 0x0C06 : 
			if (dLen == 6) printf("%-40s", "Get Scene Membership (Scene)");
			else if (dLen == 7) GetZoneInfo(cBuffer, &dispData);
			else printf("%-40s", "Unknown");
			break;
		case 0x0C08 : SetHeartBeatPeriod(cBuffer, &dispData); break;
		case 0x1C07 : SetHeartBeatPeriodRsp(cBuffer, &dispData); break;
		case 0x0C0A : ReadHeartBeatPeriod(cBuffer, &dispData); break;
		case 0x0C09 : GetZoneTroubleMap(cBuffer, &dispData); break;
		case 0x0C0B : GetRecentAlarmInfo(cBuffer, &dispData); break;
		case 0X1C00 : 
			/* TODO if clauses duplicated */
			if (dLen == 9) printf("%-40s", "Add Scene response (Scene)");
			else if (dLen == 8) ZoneStatusChange(cBuffer, &dispData);
			else if (dLen == 7) printf("%-40s", "Zone Status Change Notification (IAS Zone)");
			else if (dLen == 0x0F) printf("%-40s", "Device Configuration response (RSSI Location)->CC2431");
			else if (dLen == 6) ArmRsp(cBuffer, &dispData);
#if 0
			else if (dLen == 8) AddGroupRsp(cBuffer, &dispData);
			else if (dLen == 8) printf("%-40s", "Send Move (Level Control)");
			else if (dLen == 7) printf("%-40s", "Identify Query response (Identify)");
#endif
			break;
		case 0X1C01 :
			/* TODO find a way to differentiate the following messages */
			if (dLen == 13) GetAlarmRsp(cBuffer, &dispData);
			else if (dLen == 9) ZoneEnroll(cBuffer, &dispData);
			else if (dLen == 8) GetZoneIdMapRsp(cBuffer, &dispData);
			else if (dLen == 6) GetAlarmFailRsp(cBuffer, &dispData);
#if 0			/* see 5.8 */
			else if (dLen == 6) ZoneUnEnrollRsp(cBuffer, &dispData);
			else if (2) printf("%-40s", "View Scene response (Scene)");
			else if (1) printf("%-40s", "View Group response (Group)");
			else printf("%-40s", "Location Data response (RSSI Location)->CC2431");
#endif
			else ViewGroupRsp(cBuffer, &dispData);	
			break;
		case 0X1C02 : 
			if (dLen == 0x10) GetZoneInfoRsp(cBuffer, &dispData);
			else if (dLen == 9) RemoveSceneRsp(cBuffer, &dispData);
			else if (dLen == 6) printf("%-40s", "Get Zone Information response(IAS ACE)");
			else GetGroupMembershipRsp(cBuffer, &dispData);
			break;
		case 0X1C03 : 
			if (dLen == 8) RemoveGroupRsp(cBuffer, &dispData);
			else GetZoneTroubleMapRsp(cBuffer, &dispData, dLen);
#if 0
			printf("%-40s", "Remove All Scenes response (Scene)");
#endif
			break;
		case 0X1C04 : 
			if (dLen == 8) ReadHeartBeatPeriodRsp(cBuffer, &dispData);
			else StoreSceneRsp(cBuffer, &dispData);
			break;
		case 0X1C05 :
			if (dLen == 25) AlarmDeviceNotify(cBuffer, &dispData);
			else BlindNodeDataRsp(cBuffer, &dispData);
			break;
		case 0X1C06 : 
			/*?
			if (dLen == 8) GetRecentAlarmInfoRsp(cBuffer, &dispData);
			else printf("%-40s", "Get Scene Membership response (Scene)");
			?*/
			GetRecentAlarmInfoRsp(cBuffer, &dispData);
			break;
		case 0x0C0C : SetDayNightType(cBuffer, &dispData); break;
		case 0x0C0D : GetArmMode(cBuffer, &dispData); break;
		case 0X1C08 : GetArmModeRsp(cBuffer, &dispData); break;
		/* Attribute Command */
		case 0x0D00 : ReadAttribute(cBuffer, &dispData); break;
		case 0x0D01 : ReadAttributeRsp(cBuffer, &dispData); break;
		case 0x0D02 : WriteAttribute(cBuffer, &dispData); break;
		case 0x0D04 : WriteAttributeRsp(cBuffer, &dispData); break;
		case 0x0D0A : ReportAttributes(cBuffer, &dispData); break;
		case 0x0D06 : ConfigReport(cBuffer, &dispData); break;
		case 0x0D07 : printf("%-40s", "Configure Reporting response"); break;
		case 0X0D0D : printf("%-40s", "Discover attributes response"); break;
		default	    : printf("%-40s", "Unknown");
	}

	if (dispData)
	{
		printf(" * %2d[", dLen);
		for (i=0; i<iEnd; i++)
		{
			if (i && ((i%10)==0)) printf(" ");
			printf("%c", cBuffer[i+8]);
		}
		if (cBuffer[cLen-1] == 'Z')
			printf("] %c%c %c", cBuffer[cLen-3], cBuffer[cLen-2], cBuffer[cLen-1]);
		else
			printf("] %c%c", cBuffer[cLen-2], cBuffer[cLen-1]);
		/* Display ASCII values */
		sprintf(CmdStr, "%c%c",cBuffer[8],cBuffer[9]);
		dc = (int) strtol(CmdStr, NULL, 16);
		if (isprint(dc))
		{
			printf(" (%c", dc);
			for (i=2; i<iEnd; i+=2)
			{
				sprintf(CmdStr, "%c%c",cBuffer[i+8],cBuffer[i+9]);
				dc = (int) strtol(CmdStr, NULL, 16);
				if (isprint(dc)) printf("%c", dc);
				else printf(".");
			}
			printf(") %s", suffix);
		}
		else
			printf(" %s", suffix);
	}
	else
	{
		if (cBuffer[cLen-1] == 'Z')
			printf("FCS %c%c %c %s", cBuffer[cLen-3], cBuffer[cLen-2], cBuffer[cLen-1], suffix);
		else
			printf("FCS %c%c %s", cBuffer[cLen-2], cBuffer[cLen-1], suffix);
	}
	fflush(stdout);
}
