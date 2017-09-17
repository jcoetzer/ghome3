# File: Makefile
#
# Make of ZigBee utilities
#
# $Author: johan $
# $Date: 2011/11/19 16:31:44 $
# $Revision: 1.2 $
# $State: Exp $
#
# $Log: Makefile,v $
# Revision 1.2  2011/11/19 16:31:44  johan
# Ping panel
#
# Revision 1.1.1.1  2011/06/13 19:39:21  johan
# Initialising sources in CVS
#
# Revision 1.1.1.1  2011/05/28 09:00:03  johan
# gHome logging server
#
# Revision 1.4  2011/02/18 06:43:46  johan
# Streamline polling
#
# Revision 1.3  2011/02/10 11:09:40  johan
# Take out CVS Id tag
#
# Revision 1.2  2011/02/03 08:38:13  johan
# Add CVS stuff
#
#
#


WARN := -Wall -pedantic -Wno-pointer-sign

LIBS :=
BIND := bin
LIBD := lib
OBJD := tmp
SCRD := bin
SRCD := src
COPT := -fPIC -L/usr/local/lib

# All programs built here
GH_BINARY := ${BIND}/gHomeSend ${BIND}/gHomeLogger ${BIND}/gHomeNotify ${BIND}/gHomeTraffic \
	${BIND}/gHomeListDev ${BIND}/gHomeControl ${BIND}/gHomeAlarm ${BIND}/gHomeUnit \
	${BIND}/gHomePing ${BIND}/gHomeReceive ${BIND}/gHomePoll ${BIND}/gHomeReport \
	${BIND}/tcpServer ${BIND}/rs232Server

GH_SCRIPT := ${BIND}/gHomeTraffic.sh ${BIND}/gHomeLogger.sql ${BIND}/gHome.sh

# All binaries built here
EXES := ${GH_BINARY} ${GH_SCRIPT}

all: ${GH_BINARY}

# gHomeListDev
HDOBJS := ${OBJD}/gHomeListDev.o ${OBJD}/zbSocket.o ${OBJD}/zbDisplay.o ${OBJD}/zbData.o \
	${OBJD}/zbSend.o ${OBJD}/zbNumber.o ${OBJD}/zbPacket.o ${OBJD}/dbConnect.o ${OBJD}/unitRec.o \
	${OBJD}/unitLqi.o ${OBJD}/lqiDb.o ${OBJD}/ldUpdate.o ${OBJD}/deviceCheck.o ${OBJD}/updateDb.o \
	${OBJD}/unitDb.o

# gHomePoll
HPOBJS := ${OBJD}/gHomePoll.o ${OBJD}/zbSocket.o ${OBJD}/zbDisplay.o ${OBJD}/zbData.o ${OBJD}/lqiDb.o \
	${OBJD}/zbSend.o ${OBJD}/zbNumber.o ${OBJD}/zbPacket.o ${OBJD}/dbConnect.o ${OBJD}/unitRec.o \
	${OBJD}/unitLqi.o ${OBJD}/ldUpdate.o ${OBJD}/deviceCheck.o ${OBJD}/updateDb.o ${OBJD}/unitDb.o \
	${OBJD}/gversion.o

# gHomeSend
HSOBJS := ${OBJD}/gHomeSend.o ${OBJD}/zbSocket.o ${OBJD}/zbDisplay.o ${OBJD}/zbData.o \
	${OBJD}/zbSend.o ${OBJD}/zbNumber.o ${OBJD}/zbCmdHelp.o ${OBJD}/zbPacket.o ${OBJD}/gversion.o

# gHomeNotify
HNOBJS := ${OBJD}/gHomeNotify.o ${OBJD}/zbSend.o ${OBJD}/zbSocket.o ${OBJD}/zbDisplay.o ${OBJD}/zbData.o ${OBJD}/zbNumber.o \
	${OBJD}/zbPacket.o ${OBJD}/dbConnect.o ${OBJD}/gversion.o

# gHomeLogger
HLOBJS := ${OBJD}/gHomeLogger.o ${OBJD}/zbPoll.o ${OBJD}/updateDb.o ${OBJD}/zbSocket.o ${OBJD}/zbDisplay.o ${OBJD}/zbData.o \
	${OBJD}/zbNumber.o ${OBJD}/zbSend.o ${OBJD}/zbPacket.o ${OBJD}/zbState.o ${OBJD}/unitDb.o ${OBJD}/dbConnect.o \
	${OBJD}/lqiDb.o ${OBJD}/unitLqi.o ${OBJD}/unitRec.o ${OBJD}/gHomeConf.o ${OBJD}/gversion.o

# gHomeReport
GROBJS := ${OBJD}/gHomeReport.o ${OBJD}/ipStatus.o ${OBJD}/gversion.o ${OBJD}/dbConnect.o

# gHomeTraffic
HTOBJS := ${OBJD}/gHomeTraffic.o ${OBJD}/zbNumber.o ${OBJD}/zbDisplay.o ${OBJD}/zbData.o ${OBJD}/gversion.o

# gHomeControl
HCOBJS := ${OBJD}/gHomeControl.o ${OBJD}/zbSend.o ${OBJD}/zbSocket.o ${OBJD}/zbNumber.o ${OBJD}/zbDisplay.o \
	${OBJD}/zbData.o ${OBJD}/zbPacket.o

# gHomeAlarm
HAOBJS := ${OBJD}/gHomeAlarm.o ${OBJD}/zbSend.o ${OBJD}/zbSocket.o ${OBJD}/zbNumber.o ${OBJD}/zbDisplay.o \
	${OBJD}/zbData.o ${OBJD}/zbPacket.o

# gHomeUnit
HUOBJS := ${OBJD}/unitDb.o ${OBJD}/updateDb.o ${OBJD}/gHomeUnit.o ${OBJD}/zbData.o ${OBJD}/dbConnect.o \
	${OBJD}/lqiDb.o ${OBJD}/unitLqi.o ${OBJD}/gversion.o ${OBJD}/unitRec.o

# gHomePing
GPOBJS := ${OBJD}/gHomePing.o ${OBJD}/ghPing.o ${OBJD}/gversion.o

# gHomeReceive
RXOBJS := ${OBJD}/gHomeReceive.o ${OBJD}/zbSocket.o ${OBJD}/zbDisplay.o ${OBJD}/zbData.o \
	${OBJD}/zbSend.o ${OBJD}/zbNumber.o ${OBJD}/zbCmdHelp.o ${OBJD}/zbPacket.o ${OBJD}/gversion.o

# mbTestClient
MCOBJS := ${OBJD}/mbTestClient.o

# mbTestServer
MSOBJS := ${OBJD}/mbTestServer.o

# mbTest
MTOBJS := ${OBJD}/mbTest.o

# rs232Server
RSOBJS := ${OBJD}/rs232.o ${OBJD}/rs232Server.o ${OBJD}/dbRs232.o ${OBJD}/dbConnect.o

# tcpServer
TSOBJS := ${OBJD}/tcpServer.o ${OBJD}/dbHardware.o ${OBJD}/dbConnect.o

CC := ${CC} -g

${BIND}/gHomeListDev: ${HDOBJS} Makefile
	${CC} ${WARN} ${COPT} ${HDOBJS} -lpq -lpopt -o ${BIND}/gHomeListDev

${BIND}/gHomePoll: ${HPOBJS} Makefile
	${CC} ${WARN} ${COPT} ${HPOBJS} -lpq -lpopt -o ${BIND}/gHomePoll

${BIND}/gHomeSend: ${HSOBJS} Makefile
	${CC} ${WARN} ${COPT} ${HSOBJS} -lpopt -o ${BIND}/gHomeSend

${BIND}/gHomeNotify: ${HNOBJS} Makefile
	${CC} ${WARN} ${COPT} ${HNOBJS} -lpq -lpopt -o ${BIND}/gHomeNotify

${BIND}/gHomeLogger: ${HLOBJS} Makefile
	${CC} ${WARN} ${COPT} ${HLOBJS} -lpopt -lpq -o ${BIND}/gHomeLogger

${BIND}/gHomeReport: ${GROBJS} Makefile
	${CC} ${WARN} ${COPT} ${GROBJS} -lpq -lpopt -o ${BIND}/gHomeReport

${BIND}/gHomeTraffic: ${HTOBJS} Makefile
	${CC} ${WARN} ${COPT} ${HTOBJS} -lpopt -o ${BIND}/gHomeTraffic

${BIND}/gHomeControl: ${HCOBJS} Makefile
	${CC} ${WARN} ${COPT} ${HCOBJS} -lpopt -o ${BIND}/gHomeControl

${BIND}/gHomeAlarm: ${HAOBJS} Makefile
	${CC} ${WARN} ${COPT} ${HAOBJS} -lpopt -o ${BIND}/gHomeAlarm

${BIND}/gHomeUnit: ${HUOBJS} Makefile
	${CC} ${WARN} ${COPT} ${HUOBJS} -lpq -lpopt -o ${BIND}/gHomeUnit

${BIND}/gHomePing: ${GPOBJS} Makefile
	${CC} ${WARN} ${COPT} ${GPOBJS} -lpopt -o ${BIND}/gHomePing

${BIND}/gHomeReceive: ${RXOBJS} Makefile
	${CC} ${WARN} ${COPT} ${RXOBJS} -lpopt -o ${BIND}/gHomeReceive

${BIND}/mbTestClient : ${MCOBJS}
	${CC} ${WARN} ${COPT} ${MCOBJS} -lpopt -lmodbus -o ${BIND}/mbTestClient

${BIND}/mbTestServer : ${MSOBJS}
	${CC} ${WARN} ${COPT} ${MSOBJS} -lpopt -lmodbus -o ${BIND}/mbTestServer

${BIND}/mbTest : ${MTOBJS}
	${CC} ${WARN} ${COPT} ${MTOBJS} -lpopt -lmodbus -o ${BIND}/mbTest

${BIND}/rs232Server : ${RSOBJS}
	${CC} ${WARN} ${COPT} ${RSOBJS} -lpq -lpopt -o ${BIND}/rs232Server

${BIND}/tcpServer : ${TSOBJS}
	${CC} ${WARN} ${COPT} ${TSOBJS} -lpq -lpopt -o ${BIND}/tcpServer

# General rule to build objects

${OBJD}/testDb.o : testDb.c Makefile
	${CC} ${COPT} -c $< -o $@

${OBJD}/%.o : %.c %.h Makefile
	${CC} ${WARN} ${COPT} -c $< -o $@

clean:
	${RM} ${GH_BINARY}
	@find ${OBJD} -name "*.o" -print -exec rm {} \;
	@find . -name "*~" -print -exec rm {} \;
	@find . -name "core*" -print -exec rm {} \;
	@find . -name "vgcore*" -print -exec rm {} \;

install: ${EXES}
	cp ${EXES} /usr/local/bin
