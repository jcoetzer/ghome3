/*! \file zbCmdHelp.c
	\brief	Display help for Zigbee commands
 
*/

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbCmdHelp.c,v $
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

#include <stdio.h>
#include <string.h>

/*! \brief	Get Recent Alarm Information
 */
void help_RecentAlarmInfo()
{
	printf("Get Recent Alarm Information(ACE ClusterID)\n\n"
			"Cmd=0x0C0B       Len=0x06  AddrMode     DstAddr    DstEndPoint     Cluster ID\n\n"
			"AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits "
			"GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 2 bytes –network address of the destination address.\n"
			"DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.\n"
			"Cluster ID – 2 byte –IAS ACE cluster ID.\n");
}

/*! \brief	Network Address
 */
static void help_NetworkAddress()
{
	printf("Network Address Request\n\n"
			"CMD = 0x0A02         Len = 0x0C      AddrMode         IEEEAddr       ReqType       StartIndex     SecuritySuite\n\n"
			"AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits "
			"GroupAddress (Addrmode = 0x01),IEEE Address Addrmode = 0x03).\n"
			"IEEEAddr(reverse)– 64 bits – IEEE address of the destination device.\n"
			"ReqType – byte – following options:\n"
			"	0	→      Single device response\n"
			"	1	→      Extended – include associated devices\n");
	printf("StartIndex – 1 byte – Starting index into the list of children. This is used to get more of the list if the list is too "
			"large for one message\n"
			"SecuritySuite - 1 byte – Security options");
}

/*! \brief	Read Heart Beat
 */
void help_HeartBeatPeriod()
{
	printf("Read Heart Beat period(ACE ClusterID)\n\n"
			"Cmd=0x0C0A       Len=0x06  AddrMode     DstAddr    DstEndPoint     Cluster ID\n\n"
			"AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits "
			"GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 2 bytes –network address of the destination address.\n"
			"DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.\n"
			"Cluster ID – 2 byte –IAS ACE cluster ID.\n");
}

/*! \brief	Set Day Night Type
 */
void help_DayNightType()
{
	printf("Set Day Night Type\n\nCmd=0x0C0C       Len=0x08   AddrMode      DstAddr     DstEndPoint    Cluster ID ZoneID   Type\n\n");
	printf("AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 2 bytes –network address of the destination address.\n"
			"DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.\n"
			"Cluster ID – 2 byte –IAS ACE cluster ID.\n"
			"ZoneID-1byte- The Zone ID field is the index into the zone table of the CIE.\n"
			"Type-1byte- 0 night zone;1 day zone\n");
}

/*! \brief	User Description
 */
static void help_UserDescription()
{
	printf("User Description request\n\n"
			"Cmd=0x0A0A       Len=0x06        AddrMode    DstAddr NWKAddrOfIneterest   SecuritySuite\n\n"
			"AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits "
			"GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 16 bit – NWK address of the device generating the inquiry..\n"
			"NWKAddrOfInterest – 16 bit - NWK address of the destination device being queried.\n"
			"SecuritySuite - 1 byte – Security options.\n");
}

/*! \brief	Get Zone Information
 */
static void help_ZoneInfo()
{
	printf("Get Zone Information (IAS ACE)\n\n"
			"Cmd=0x0C06 Len=0x07 AddrMode DstAddr DstEndPoint Cluster ID Zone ID\n\n"
			"AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits "
			"GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 2 bytes –network address of the destination address.\n"
			"DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.\n"
			"Cluster ID – 2 byte –IAS ACE cluster ID.\n"
			"Zone ID– byte –the IAS Zone ID.\n");
}

/*! \brief	Bypass
 */
static void help_Bypass(char * desc)
{
	printf("%s Bypass (IAS ACE)\n"
			"\n"
			"Cmd=0x0C01  Len=var AddrMode DstAddr DstEndPoint Cluster ID Number of Zone Zone ID1 ... Zone IDn\n"
			"\n", desc);
	printf("AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits "
			"GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 2 bytes –network address of the destination address.\n"
			"DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.\n"
			"Cluster ID – 2 byte –IAS ACE cluster ID.\n"
			"Number of Zone – byte –This is the number of Zone IDs included in the payload.\n"
			"Zone ID – byte –Zone ID is the index of the Zone in the CIE's zone table.\n");
	printf("\n"
			"Format of the Zone Table\n"
			"\tField                    Valid range                Description\n"
			"\tZone ID                  0x00 - 0xfe                The unique identifier of the zone\n"
			"\tZone Type                0x0000 - 0xfffe            See “Zone Type attribute of IAS Zone”\n"
			"\tZone Address             64bit IEEE                 Device address\n");
}

/*! \brief	Get Zone ID Map
 */
static void help_ZoneIdMap()
{
	printf("Get Zone ID Map(IAS ACE)\n"
			"\n"
			"Cmd=0x0C05        Len=0x06      AddrMode    DstAddr     DstEndPoint     Cluster ID\n"
			"\n"
			"AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits "
			"GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 2 bytes –network address of the destination address.\n"
			"DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.\n"
			"Cluster ID – 2 byte –IAS ACE cluster ID.\n");
}

/*! \brief	Squawk
 */
static void help_Squawk()
{
	printf("Squawk (IAS WD)\n"
			"\n"
			"Cmd=0x0C01      Len=0x07      AddrMode    DstAddr        DstEndPoint\n"
			"Cluster ID      Squawk level(bit6~7)      Strobe(bit4)   Squawk mode(bit0~3)\n"
			"\n");
	printf("AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits\n"
			"GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 2 bytes –network address of the destination address.\n"
			"DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.\n"
			"Cluster ID – 2 byte –IAS WD cluster ID.\n");
	printf("Squawk mode(bit0~3) The Squawk Mode field is used as a 4-bit enumeration. Following\n"
			"		Squawk Mode	Meaning\n"
			"		0		Notification sound for \"System is armed\"\n"
			"		1		Notification sound for \"System is disarmed\"\n");
	printf("Strobe(bit4) The strobe field is used as a boolean, and determines if the visual indication is also required in "
			"addition to the audible squawk.\n"
					"		Value          Meaning\n"
					"		0              No strobe\n"
			"		1              Use strobe in parallel to warning\n");
	printf("Squawk level(bit6~7) The squawk level field is used as a 2-bit enumeration, and determines the intensity of "
					"audible squawk sound\n"
							"		Value          Meaning\n"
							"		0              Low level sound\n"
							"		1              Medium level sound\n"
							"		2              High level sound\n"
							"		3              Very High level sound\n");
}

/*! \brief 9.1 Zone Un-Enroll
 */
static void help_ZoneUnEnroll()
{
	printf("Zone Un-Eroll Request(IAS Zone ClusterID)\n"
			"\n");
	printf("Cmd=0x0C02     Len=0x07    AddrMode     DstAddr    DstEndPoint     Cluster ID ZoneID\n"
			"\n");
	printf("AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits "
			"GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 2 bytes –network address of the destination address.\n"
			"DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.\n"
			"Cluster ID – 2 byte –IAS Zone cluster ID.\n"
			"ZoneID -1byte- The Zone ID field is the index into the zone table of the CIE.\n");
}

/*! \brief 9.6 Zone Trouble Map
 */
static void help_ZoneTroubleMap()
{
	printf("Get Zone Trouble Map(ACE ClusterID)\n"
			"\n"
			"Cmd=0x0C09     Len=0x06    AddrMode     DstAddr    DstEndPoint     Cluster ID\n"
			"AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits "
			"GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 2 bytes –network address of the destination address.\n"
			"DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.\n"
			"Cluster ID – 2 byte –IAS ACE cluster ID.\n");
}

/*! \briefZone Enroll	
 */
static void help_ZoneEnroll()
{
	printf("Zone Eroll Req(IAS Zone Client) CIE received.\n"
			"\n"
			"Cmd=0x1C01     Len=0x09      SrcAddr  SrcEndPoint    Cluster ID   Zone Type   Manufacturer Code\n"
			"SrcAddr – 2 byte –network address of the source address (device’s Zone status change and sent).\n"
			"SrcEndPoint– 1 byte –the source EndPoint. represents the application endpoint the data.\n"
			"Cluster ID– 2 byte –IAS Zone Cluster ID.\n"
			"Zone Type(reverse)-2byte- The Zone Type field shall be the current value of the ZoneType attribute.\n");
	printf("ManuFacturer Code(reverse) – 2bytes- The Manufacturer Code field shall be the manufacturer code as held in\n"
			"	the node descriptor for the device. Manufacturer Codes are allocated by the ZigBee Alliance.\n");
}

/*! \brief	Unbind request
 */
static void help_Unbind()
{
	printf("Unbind request\n"
			"\n"
			"Cmd=0x0A0D       Len=0x19      AddrMode        DstAddr     SrcAddress       SrcEndPoint\n"
			"Cluster ID          DstAddrMode           DstAddress    DstEndpoint      SecuritySuite\n"
			"\n");
	printf(
			"AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 2 byte –destination address of the device generating the bind request\n"
			"SrcAddress(Active) – 8 bytes – 64 bit Binding source IEEE address\n"
			"SrcEndpoint(Active) – 1 byte – Binding source endpoint.\n"
			"ClusterID – 2 byte – Cluster ID to match in messages.\n");
	printf(
			"DstAddrMode(Passive) – 1 byte – Destination address mode:\n"
			"	Mode                                     Value         Description\n"
			"	ADDRESS_NOT_PRESENT                      0x00          Address Not Present\n"
			"	GROUP_ADDRESS                            0x01          Group address\n"
			"	ADDRESS_16_BIT                           0x02          Address 16 bit\n"
			"	ADDRESS_64_BIT                           0x03          Address 64 bit\n"
			"	BROADCAST                                0xFF          Broadcast\n");
	printf(
			"DstAddress(Passive) – 8/2 bytes – Binding destination IEEE address. Not to be confused with DstAddr.\n"
			"DstEndpoint(Passive) – 1 byte – Binding destination endpoint. It is used only when DstAddrMode is 64 bits extended address\n"
			"SecuritySuite - 1 byte – Security options.\n");
}

/*! \brief	Bind request
 */
static void help_Bind()
{
	printf("Bind request     request a Bind\n"
			"\n"
			"Cmd=0x0A0C   Len=var AddrMode DstAddr    SrcAddress SrcEndPoint\n"
			"Cluster ID       DstAddrMode          DstAddress  DstEndpoint    SecuritySuite\n"
			"\n");
	printf(	"AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits GroupAddress (Addrmode = 0x01)\n"
			"DstAddr – 16 bits –destination address of the device generating the bind request\n"
			"SrcAddress(Active) – 8 bytes – 64 bit Binding source IEEE address\n"
			"SrcEndpoint (Active)– 8 bits – Binding source endpoint.\n"
			"ClusterID – 2 byte – Cluster ID to match in messages.\n");
	printf("DstAddrMode(Passive) – 1 byte – Destination address mode:\n"
			"	Mode                                 Value         Description\n"
			"	ADDRESS_NOT_PRESENT                  0x00          Address Not Present\n"
			"	GROUP_ADDRESS                        0x01          Group address\n"
			"	ADDRESS_16_BIT                       0x02          Address 16 bit\n"
			"	ADDRESS_64_BIT                       0x03          Address 64 bit\n"
			"	BROADCAST                            0xFF          Broadcast\n");
	printf("DstAddress(Passive) – 8 bytes / 2bytes – Binding destination IEEE address. Not to be confused with DstAddr.\n"
			"DstEndpoint(Passive)– 8 bits / 0 byte – Binding destination endpoint. It is used only when DstAddrMode is 64 bits extended address\n"
			"SecuritySuite - 1 byte – Security options.\n");
}

/*! \brief Set Heart Beat Period(ACE ClusterID)
 */
static void help_SetHeartBeatPeriod()
{
	printf("Set Heart Beat Period(ACE ClusterID)\n");
	printf("\n");
	printf("Cmd=0x0C08     Len=0x09  AddrMode     DstAddr  DstEndPoint    Cluster ID HeartBeatPeriod  MaxLossHeartBeat\n");
	printf("\n");
	printf("AddrMode – byte –indicates that the DstAddr is either 16 bits ShortAddress (Addrmode = 0x02) or 16 bits GroupAddress (Addrmode = 0x01)\n");
	printf("DstAddr – 2 bytes –network address of the destination address.\n");
	printf("DstEndpoint – byte – the destination EndPoint. represents the application endpoint the data.\n");
	printf("Cluster ID – 2 byte –IAS ACE cluster ID.\n");
	printf("HeartBeatPeriod(reverse) -2bytes- Set the device heart beat period.\n");
	printf("MaxLossHeartBeat-1byte- The maximum count of loss heart beat\n");
}

/*! \brief	
 */
static void help_()
{
	printf("\n");
}

/*! \brief	Help is available for
 */
void helpDisplayList()
{
	printf("Help is available for:\n");
	printf("\t Bind\n");
	printf("\t Bypass\n");
	printf("\t DayNightType\n");
	printf("\t HeartBeatPeriod\n");
	printf("\t NetworkAddress\n");
	printf("\t RecentAlarmInfo\n");
	printf("\t Squawk\n");
	printf("\t Unbind\n");
	printf("\t UnBypass\n");
	printf("\t UserDescription\n");
	printf("\t ZoneIdMap\n");
	printf("\t ZoneInfo\n");
	printf("\t ZoneTroubleMap\n");
	printf("\t ZoneUnEnroll\n");
	printf("\t SetHeartBeatPeriod\n");
	/*
	printf("\t \n");
	printf("\t \n");
	*/
}

/*! \brief	Display help for command
	\param	cmd	command name	
 */
void helpHelp(char * cmd)
{
	printf("Unknown command %s\n", cmd);
	helpDisplayList();
}

/*! \brief	
 */
void help()
{
	helpDisplayList();
}

/*! \brief	Check command and display appropriate help
	\param	cmd	command name
 */
void helpDisplay(char * cmd)
{
	printf("\n");
	if (! strncasecmp(cmd, "Bi", 2)) help_Bind();
	else if (! strncasecmp(cmd, "By", 2)) help_Bypass("Bypass");
	else if (! strncasecmp(cmd, "Da", 2)) help_DayNightType();
	else if (! strncasecmp(cmd, "He", 2)) help_HeartBeatPeriod();
	else if (! strncasecmp(cmd, "Ne", 2)) help_NetworkAddress();
	else if (! strncasecmp(cmd, "Re", 2)) help_RecentAlarmInfo();
	else if (! strncasecmp(cmd, "Sq", 2)) help_Squawk();
	else if (! strncasecmp(cmd, "Unbi", 4)) help_Unbind();
	else if (! strncasecmp(cmd, "SetH", 4)) help_SetHeartBeatPeriod();
	else if (! strncasecmp(cmd, "Unby", 4)) help_Bypass("Un-Bypass");
	else if (! strncasecmp(cmd, "Us", 2)) help_UserDescription();
	else if (! strncasecmp(cmd, "ZoneE", 6)) help_ZoneEnroll();
	else if (! strncasecmp(cmd, "ZoneId", 6)) help_ZoneIdMap();
	else if (! strncasecmp(cmd, "ZoneIn", 6)) help_ZoneInfo();
	else if (! strncasecmp(cmd, "ZoneT", 5)) help_ZoneTroubleMap();
	else if (! strncasecmp(cmd, "ZoneU", 5)) help_ZoneUnEnroll();
	/*
	else if (! strncasecmp(cmd, "", 4)) help_();
	else if (! strncasecmp(cmd, "", 4)) help_();
	else if (! strncasecmp(cmd, "", 4)) help_();
	*/
	else help(cmd);
	printf("\n");
}