#ifndef UNITREC_H
#define UNITREC_H

/*
 * $Author: johan $
 * $Date: 2011/11/19 16:31:44 $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 * $Log: unitRec.h,v $
 * Revision 1.2  2011/11/19 16:31:44  johan
 * Ping panel
 *
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.9  2011/02/23 16:27:15  johan
 * Store time of last heartbeat request
 *
 * Revision 1.8  2011/02/23 15:48:49  johan
 * Reset alarm log
 *
 * Revision 1.7  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.6  2011/02/16 20:42:05  johan
 * Add user description field
 *
 * Revision 1.5  2011/02/15 14:42:38  johan
 * Add initialize state description
 *
 * Revision 1.4  2011/02/10 15:20:40  johan
 * Log errors in PIR messages
 *
 * Revision 1.3  2011/02/10 11:09:42  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:16  johan
 * Add CVS stuff
 *
 *
 *
 */
#ifdef WIN32
#define pid_t int
#endif


#include <sys/types.h>
#include <unistd.h>
#include <libpq-fe.h>
#include <time.h>

#include "unitLqi.h"

#define ZBSTATUS_UNKNOWN    0
#define ZBSTATUS_ALARM      1
#define ZBSTATUS_FIRE       2
#define ZBSTATUS_PANIC      3
#define ZBSTATUS_STAY       0xFF
#define ZBSTATUS_ARMED      0x50
#define ZBSTATUS_INIT       99
#define ZBSTATUS_ARM_DAY    8
#define ZBSTATUS_ARM_NITE   9
#define ZBSTATUS_DISARMED   0x51
#define ZBSTATUS_LOST_COMMS 10111

#define MAX_PIR 10

#define MAX_DEVICES 32

#define MAX_LOC 16

/*! \struct unitInfoStruct Unit information
 */
struct unitInfoStruct
{
#ifdef WIN32
   void * dbConnctn;
#else
	PGconn * dbConnctn;
#endif

	int unitNo;		/*!< Unit number */
	pid_t pid;		/*!< Process ID */
	int ustatus;		/*!< Current status (armed, disarmed, etc.) */
	int dlay;		/*!< Entry delay in seconds */
	int live;		/*!< Live update flag */
	char pan[32];
	char appt[128];		/*!< Description */
	char oname[128];	/*!< Occupant name */
	char gwip[32];		/*!< IP address of Zigbee gateway */
	char dinip[32];		/*!< IP address of Android panel */
	unsigned short gwport;
	int edelay;		/*!< Entry delay in seconds */
	time_t hbeatreq;	/*!< Last heartbeat request */
	time_t readmsg;
	int hbeatmins;		/*!< Heartbeat period in minutes */
	int hbeatloss;		/*!< Heartbeat loss */
	time_t cfedit;		/*!< Last update to adomis_box table */
	time_t hwedit;		/*!< Last update to hardware table */
	
	int currnt_hour;
	int pirevnt[24];
	
	struct unitLqiStruct lqi[MAX_DEVICES];	/*!< Link quality information */

	int pircount;
	int piraddr[MAX_PIR];		/*!< Short address of PIR */
	char pirieee[MAX_PIR][32];	/*!< IEEE address of PIR */
	time_t pirtime[MAX_PIR];	/*!< Last activity time */
	time_t pirbeat[MAX_PIR];	/*!< Last heartbeat time */	
	int pirzone[MAX_PIR];		/*!< Day (1) or night (0) zone */
	char pirdesc[MAX_PIR][128];	/*!< Location string */
	char piruser[MAX_PIR][128];	/*!< User description string */
	int pirloc[MAX_PIR];		/*!< Location value */
	float pirtemp[MAX_PIR];		/*!< Temperature */
	time_t tmptime[MAX_PIR];	/*!< Last temperature time */

	time_t sysping;		/*!< Time of last system ping */
	time_t getalarm;	/*!< Time when last alarm status was received */	
	int alrmaddr;		/*!< Short address of alarm */
	char alrmieee[32];	/*!< IEEE address of alarm */
	time_t alrmreset;
	int gwaddr;		/*!< Short address of gateway */
	char gwieee[32];	/*!< IEEE address of gateway */
	char cordieee[32];	/*!< IEEE address of coordinator */
	time_t momentarm;	/*!< Time to arm again */
	int loglevel;
	int pir0err;		/*!< Erroneous PIR messages */
	time_t pir0moment;	/*!< Last PIR error */
};
typedef struct unitInfoStruct unitInfoRec;

void unitDbInit(unitInfoRec * unit, int unitNo, int update);

int unitLqiGateway(unitInfoRec * unit, char * rxBuff);

#endif
