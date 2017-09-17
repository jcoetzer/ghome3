/*
 ** tcpServer.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <libgen.h>
#include <popt.h>
#include <netinet/ip.h>
#include <ctype.h>
#include <time.h>

#include "rs232.h"
#include "unitRec.h"
#include "dbRs232.h"

#define NMA_ALARM_TYPE 4

int MessageOffset = 1;

int DefaultBaudRate = 9600;

int trace = 0;
int verbose = 0;
int SendReply = 0;
int bdrate=0;       		/*!< 115200 baud */
int cport_nr=0;        			/*!< /dev/ttyS0 (COM1 on windows) */
int ListPorts=0;

int Timeout = 0;
int Poll = 0;
time_t HeartBeat = 0;
time_t HeartTime = 0;
time_t DefaultHeartTime = 3;

char * InputData = NULL;
char * InputFile = NULL;

int EnablBurglr = 0;
int HeartUnit = 0;
int DefaultHeartUnit = 1111;

int medic = 0;
int panic = 0;
int arm = 0;
int disarm = 0;
int burglar = 0;
int fire = 0;
int ping = 0;

char Arm[] 	= "14 1111 18 R408 01 U00";
char Disarm[] 	= "14 1111 18 E401 01 U00";
char Burglar[] 	= "14 1111 18 E134 01 C08";
char Fire[] 	= "14 1111 18 E115 01 C00";
char Medical[] 	= "14 1111 18 E100 01 C00";
char Panic[]	= "14 1111 18 E120 01 C00";
char Ping[] 	= "00 OKAY @";

char * HeartbeatMsg = NULL;
char DefaultHeartbeatMsg[] = "Have a nice day";

/*! \struct Command line parsing data structure */
struct poptOption optionsTable[] =
{
	{"baud", 	'B', POPT_ARG_INT, 	&bdrate, 	0, "Baud rate", "<integer>"},
	{"port", 	'P', POPT_ARG_INT, 	&cport_nr,	0, "/dev/ttyS<N> port number", "<integer>"},
	{"list",	'1', POPT_ARG_NONE,     &ListPorts, 	0, "List ports", NULL},
	{"poll", 	'Q', POPT_ARG_INT, 	&Poll,		0, "polling message interval in minutes", "<integer>"},
	{"data", 	'D', POPT_ARG_STRING, 	&InputData, 	0, "Input data",  "<string>"},
	{"file", 	'I', POPT_ARG_STRING, 	&InputFile, 	0, "Input file",  "<string>"},
	{"burglar",	'b', POPT_ARG_NONE,     &EnablBurglr, 	0, "Enable burglar alarm", NULL},
	{"arm",		'1', POPT_ARG_NONE,     &arm, 		0, "Send arm", NULL},
	{"disarm",	'2', POPT_ARG_NONE,     &disarm,	0, "Send disarm", NULL},
	{"burglar",	'3', POPT_ARG_NONE,     &burglar, 	0, "Send burglar alert", NULL},
	{"fire",	'4', POPT_ARG_NONE,     &fire, 		0, "Send fire alert", NULL},
	{"medic",	'5', POPT_ARG_NONE,     &medic, 	0, "Send medic alert", NULL},
	{"panic",	'6', POPT_ARG_NONE,     &panic, 	0, "Send panic alert", NULL},
	{"heart", 	'H', POPT_ARG_STRING, 	&HeartbeatMsg, 	0, "Heartbeat message",  "<string>"},
	{"time", 	'T', POPT_ARG_INT, 	&HeartTime,	0, "Heartbeat timeout in minutes", "<integer>"},
	{"unit", 	'T', POPT_ARG_INT, 	&HeartUnit,	0, "Unit number for heartbeat timeout", "<integer>"},
	{"ping",	'7', POPT_ARG_NONE,     &ping, 		0, "Send ping", NULL},
	{"trace", 	't', POPT_ARG_NONE, 	&trace, 	0, "Traced (debug) output", NULL},
	{"verbose", 	'v', POPT_ARG_NONE, 	&verbose, 	0, "Verbose output", NULL},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

/* Local functions */
void sighandle(int signum);
int read_msg(char * sendst, size_t txbytes);

int check_heartbeat();

void alarm_nma(PGconn * conn, int unitnr, char * msg, int mstart, char * action);
void alarm_fire(PGconn * conn, int unitnr, char * action);
void alarm_medic(PGconn * conn, int unitnr, char * action);
void alarm_panic(PGconn * conn, int unitnr, char * action);
void alarm_burglar(PGconn * conn, int unitnr, char * msg, int mstart, char * action);
void alarm_disarm(PGconn * conn, int unitnr, char * action);
void alarm_arm(PGconn * conn, int unitnr, char * action);
void alarm_tech(int errn, PGconn * conn, int unitnr, char * action);
void alarm_other(int errn, PGconn * conn, int unitnr, char * action);
int set_unit_name(PGconn * conn, int unitnr, char * unitNam);

void HexDumpData(char *pfix, char * buf, int n);
void SplitMessage(char * buf, int * n, char * msg, int * m);
int ReadComData(char * portname);
int ReadDataFile(FILE * ifile, char * buf, int blen);

/*! \brief Main function
 * 	\param argc argument count
 * 	\param argv argument values
 *	\return	Zero when successful, otherwise non-zero
 */
int main(int argc, char *argv[])
{
	poptContext optcon;        		/*!< Context for parsing command line options */
	char * portname;
	int rc;

	signal(SIGINT, sighandle);
	signal(SIGQUIT, sighandle);
	signal(SIGPIPE, sighandle);
	
	/* Read command line */
	optcon = poptGetContext(argv[0],argc,(const char **)argv,optionsTable,0);
	rc = poptGetNextOpt(optcon);
	if (rc < -1)
	{
		printf("%s: %s - %s\n", basename(argv[0]), poptBadOption(optcon,0),
		poptStrerror(rc));
		poptPrintUsage(optcon, stderr, 0);
		return -1;
	}
	poptFreeContext(optcon);
	
	if (ListPorts)
	{
		list_ports();
		return 0;
	}
	
	if (! HeartbeatMsg) 
		HeartbeatMsg = DefaultHeartbeatMsg;
	
	if (! HeartTime)
		HeartTime = DefaultHeartTime;
	/* Convert to seconds */
	HeartTime *= 60;
	
	if (! HeartUnit)
		HeartUnit = DefaultHeartUnit;
	
	portname = get_port(cport_nr);
	if (NULL == portname)
	{
		printf("Illegal comport number %d\n", cport_nr);
		return(1);
	}
	
	if (medic)
		InputData = Medical;
	else if (panic)
		InputData = Panic;
	else if (arm)
		InputData = Arm;
	else if (disarm)
		InputData = Disarm;
	else if (burglar)
		InputData = Burglar;
	else if (fire)
		InputData = Fire;
	else if (ping)
		InputData = Ping;

	if (InputData)
	{
		if (verbose) printf("\tSimulate data '%s'\n", InputData);
		read_msg(InputData, strlen(InputData));
		return 0;
	}
	
	if (! EnablBurglr)
		printf("%s: Burglar alarms disabled!\n", DateTimeStampStr(-1));

	if (Poll) Poll *= 600;
	else Poll = 3000;		/* 5 minutes */

	HeartBeat = time(NULL);
	rc = ReadComData(portname);
	
	return rc;
}

/** \brief Read communication data from RS-232 port or input file 
 * 	\param	portname	Communication port name, e.g. /dev/ttyS0
 *	\return	Zero when successful, otherwise non-zero
 */
int ReadComData(char * portname)
{ 
	int m, n, count;
	unsigned char buf[1024], msg[1024];
	FILE * ifile = NULL;
	
	if (InputFile)
	{
		/* Open file (for debugging) */
		if (strcmp(InputFile, "-"))
		{
			ifile = fopen(InputFile, "r");
			if (! ifile)
			{
				printf("%s ERROR: Could not read file %s\n", DateTimeStampStr(-1), InputFile);
				return 0;
			}
		}
		else
			ifile = stdin;
	}
	else
	{
		/* Open RS-232 port */
		if (! bdrate) bdrate = DefaultBaudRate;

		/* Use 8N1 (8 databits, no parity, 1 stopbit) */
		printf("%s Open COM port %s, baud rate %d\n", DateTimeStampStr(-1), portname, bdrate);
		if(OpenComport(cport_nr, bdrate))
		{
			printf("%s ERROR: Could not open comport %s\n", DateTimeStampStr(-1), portname);
			return 0;
		}
	}

	count = m = msg[0] = n = 0;
	while (1)
	{
		/* Check for unprocessed data */
		if (! n)
		{
			if (ifile)
			{
				/* Read from file (for debugging) */
				n = ReadDataFile(ifile, buf, 1023);
				if (n < 0) break;
			}
			else
			{
				/* Read from RS-232 port */
				n = PollComport(cport_nr, buf, 1023);
				/* put "null" at end of string */
				buf[n] = 0;   
			}
			HexDumpData("DATA", buf, n);
		}
		else
		{
			if (trace) HexDumpData("\tPrevious ", buf, n);
		}
		/* Check for presence of data */
		if(n > 0)
		{
			/* LF (0xa) at start and CR (0xd) at end of message */
			if ((buf[0] == 0xa) && (buf[n-1] == 0xd))
			{
				/* Complete message */
				strncpy(msg, buf, n);
				m = n;
				msg[m] = 0;
#ifdef DEBUG
				if (trace) printf("\tProcess %d-byte message \n{\n%s\n}\n", m, msg);
#endif
				/* Check for two messages in one line */
				SplitMessage(buf, &n, msg, &m);
				read_msg(msg, m);
				/* Flush standard output so that log file gets updated */
				fflush(stdout);
				m = msg[0] = 0;
			}
			/* LF (0xa) at start of message */
			else if (buf[0] == 0xa)
			{
				/* Start of message */
				if (m)
				{
					if (trace) printf("\tProcess previous incomplete message\n");
					read_msg(msg, m);
					fflush(stdout);
					m = msg[0] = 0;
				}
				if (trace) printf("\tStart of new message\n");
				strncpy(msg, buf, n);
				msg[n] = 0;
				m = n;
				n = 0;
				/* Check for two messages in one line */
				SplitMessage(buf, &n, msg, &m);
			}
			/* CR (0xd) at end of message */
			else if (buf[n-1] == 0xd)
			{
				/* End of message */
				if (trace) printf("\tEnd of message\n");
				m += n;
				if (m >= sizeof(msg))	
				{
					printf("%s ERROR: buffer overflow!\n", DateTimeStampStr(-1));
					msg[0] = m = n = 0;
				}
				else
				{
					strcat(msg, buf);
					/* Check for two messages in one line */
					SplitMessage(buf, &n, msg, &m);
					read_msg(msg, m);
					fflush(stdout);
					m = msg[0] = 0;
				}
			}
			else
			{
				if (trace) printf("\tContinue...\n");
				m += n;
				if (m >= sizeof(msg))
				{
					printf("%s ERROR: buffer overflow!\n", DateTimeStampStr(-1));
					msg[0] = m = 0;
				}
				else
					strcat(msg, buf);
				n = 0;
			}
		}
		/* Sleep for 100 milliSeconds */
		usleep(100000);
		if (++count > Poll)
		{
			count = 0;
			if (verbose) printf("%s Polling...\n", DateTimeStampStr(-1));
			/* Flush standard output so that log file gets updated */
			fflush(stdout);
		}
		if (count % 300 == 0)
			check_heartbeat();
	}

	/* Only gets here after EOF of input file */
	return 0;
}

/** \brief Print hexadecimal values 
 * 	\param	pfix		Prefix string
 * 	\param	buf		Buffer
 * 	\param	n		Number of bytes 
 */
void HexDumpData(char *pfix, 
		 char * buf, 
		 int n)
{
	int i;
	
	if (! n) return;

	printf("%s%04d> %02x", pfix, (int) n, buf[0]);
	
	for (i=1; i<n; i++)
	{
		printf("-%02x", buf[i]);
	}
	printf("\n");
}

/** \brief Check for two messages in one line 
 */
void SplitMessage(char * buf, int * n, char * msg, int * m)
{
	int i;

#ifdef DEBUG
	if (trace) 
	{
		printf("\tCheck for extra message in\n");
		HexDumpData("\t", msg, *m);
		printf("{\n%s\n}\n", msg);
	}
#endif

	*n = 0;
	for (i=0; i<*m; i++)
	{
		if (msg[i] == 0xd && msg[i+1] == 0xa)
		{
			strcpy(buf, &msg[i+1]);
			*n = strlen(buf);
			if (trace) 
			{
				printf("\tFound second %d-byte message\n", *n);
				HexDumpData("\t", buf, *n);
#ifdef DEBUG
				printf("{\n%s\n}\n", buf);
#endif
			}
			++i;
			msg[i] = 0;
			*m = i;
			if (trace) 
			{
				printf("\tProcess revised %d-byte message\n", *m);
				HexDumpData("\t", msg, *m);
#ifdef DEBUG
				printf("{\n%s\n}\n", msg);
#endif
			}
			break;
		}
	}
}

/** \brief Read data file
 * 	\param	ifile		Input file pointer
 * 	\param	buf		Buffer
 * 	\param	blen		Buffer size
 *	\return	Zero when successful, otherwise non-zero
 * 
 * Entry format:
 * 	DATA, bytes, '>', hexadecimal values of bytes seperated by '-'
 * e.g.
 * 	DATA0017> 0a-48-61-76-65-20-61-20-6e-69-63-65-20-64-61-79-0d
 */
int ReadDataFile(FILE * ifile,
		 char * buf,
		 int blen)
{
	int i, n;
	unsigned long cval;
	char numstr[8];
	char * ftok;
	static char fbuf[1024];
	
	if (feof(ifile)) 
	{
		if (trace) printf("\nReached end of file\n");
		if (strcmp(InputFile, "-")) fclose(ifile);
		return -1;
	}
	
	fgets(fbuf, sizeof(fbuf), ifile);
	
	if (strncmp(fbuf, "DATA", 4)) return 0;
	
	i = strlen(fbuf) - 1;
	while (i >= 0)
	{
		if (isspace(fbuf[i])) fbuf[i] = 0;
		else break;
	}
	
	strncpy(numstr, &fbuf[4], 4);
	numstr[4] = 0;
	n = atoi(numstr);
	
	i = 0;
	ftok = strtok(&fbuf[10], "-");
	while (ftok)
	{
		cval = strtoul(ftok, NULL, 16);
		buf[i] = (char) cval;
		++i;
		ftok = strtok(NULL, "-");
	}
	buf[i] = 0;

	if (i != n) printf("WARNING: data count %d does not match characters read %d\n", n, i);

	return i;
}

/** \brief Check heartbeat and send alarm
 *	\return	Zero when successful, otherwise non-zero
 */
int check_heartbeat()
{
	PGconn * conn;
	time_t now, intvl;
	char unitNam[256];
	
	now = time(NULL);
	
	intvl = now - HeartBeat;
	if (verbose) printf("%s: Last heartbeat was %d seconds ago\n", DateTimeStampStr(-1), (int) intvl);
	
	if (intvl >= HeartTime)
	{
		printf("%s: ERROR: Last heartbeat was %d seconds ago\n", DateTimeStampStr(-1), (int) intvl);

		/* Connect to database */
		conn = Rs232DbConnect();
		if (conn == NULL)
		{
			printf("%s: ERROR : could not open database connection\n", DateTimeStampStr(-1));
			return -1;
		}

		set_unit_name(conn, HeartUnit, unitNam);
	
		Rs232LogEvent(conn, HeartUnit, unitNam, "Technical problem", 6, 0);
		Rs232LogStatus(conn, HeartUnit, ZBSTATUS_LOST_COMMS, 0);
		Rs232Status(conn, ZBSTATUS_LOST_COMMS, HeartUnit);
		Rs232AlarmMsg(conn, HeartUnit, "Server error", "Technical problem", 6, 0);
		
		PQfinish(conn);
		
		/* Reset */
		HeartBeat = time(NULL);
	}
	
	return 0;
}

/*! \brief Check that all characters in a string are numeric
 *	\return	Zero when successful, otherwise non-zero
 */
int check_num(char * numst)
{
	int i, n;

	n = strlen(numst);
	for (i=0; i<n; i++)
		if (! isdigit(numst[i]))
			return 1;

	return 0;
}

/*! \brief Convert message to readable format
 * 	\param sst Data buffer
 * 	\param txbytes Buffer length
	\return	Zero when successful, otherwise non-zero
 * 
 * Medical
 *	14 1111 18 E100 01 C00
 *	14 1111 18 R100 01 C00
 * Ping
 *	00 OKAY @
 * Fire
 *	14 1111 18 E115 01 C00
 *	14 1111 18 R115 01 C00
 * Panic
 *	14 1111 18 E120 01 C00
 *	14 1111 18 R120 01 C00
 * Burglar
 *	14 1111 18 E134 01 C08
 *	14 1111 18 R134 01 C08
 * Arm
 *	14 1111 18 R408 01 U00
 * Disarm
 *	14 1111 18 E401 01 U00
 */
int read_msg(char * sst, 
	     size_t txbytes)
{
	int i, n, unitnr;
	char unitnrSt[8];
	char alrmCodeSt[8];
	char action[16];
	char alrmType;
	int alrmCode;
	PGconn * conn;
	int UNIT_NUMBER_START = 3;
	int UNIT_NUMBER_LNGTH = 4;
	int ALARM_TYPE_START = 11;
	int ALARM_CODE_START = 12;
	int ALARM_CODE_LNGTH = 3;
	char * sendst;
	
	strcpy(action, "?");

	sendst = sst;
	i = 0;
	/* Skip unreadable control-codes at start of string */
	while (sst[i] < 32) 
	{
		++sendst;
		--txbytes;
		i++;
	}

	/* Erase unreadable control-codes at start of string */
	n = txbytes - 1;
	for (i=n; i>=0; i--)
	{
		if(sendst[i] < 32) sendst[i] = 0;
		else break;
	}

	/* Check if anything is left */
	txbytes = strlen(sendst);
	if (txbytes <= 0) 
	{
		if (trace) printf("\tIgnore empty message\n");
		return -1;
	}

	/* Replace other unreadable control-codes */
	for(i=0; i < txbytes; i++)
	{
		if(sendst[i] < 32) sendst[i] = '?';
	}
	if (verbose) printf("%s Received %i bytes: '%s'\n", DateTimeStampStr(-1), (int)txbytes, sendst);

	if (! strncmp(sendst, HeartbeatMsg, strlen(HeartbeatMsg)))
	{
		HeartBeat = time(NULL);
		if (verbose) printf("%s Heartbeat\n", DateTimeStampStr(-1));
		return 0;
	}

	/* Connect to database */
	conn = Rs232DbConnect();
	if (conn == NULL)
	{
		printf("%s: ERROR : could not open database connection\n", DateTimeStampStr(-1));
		return -1;
	}

	/* Check message length */
	if (txbytes < 22)
	{
		printf("%s: ERROR : truncated message of %d bytes\n", DateTimeStampStr(-1), (int) txbytes);
		/* Write message to log table */
		dbRs232_AlarmLog(conn, 0, sendst, '?', 0, action);
		PQfinish(conn);
		return -1;
	}

	if (sendst[2] == ' ' && sendst[7] == ' ')
	{
		if (trace) printf("\tUse 4-digit unit number\n");
	}
	else if (sendst[2] == ' ' && sendst[9] == ' ')
	{
		if (trace) printf("\tUse 6-digit unit number\n");
		UNIT_NUMBER_LNGTH = 6;
		ALARM_TYPE_START = 13;
		ALARM_CODE_START = 14;
	}
	else
	{
		printf("%s: ERROR : invalid unit number in '%s'\n", DateTimeStampStr(-1), sendst);
		/* Write message to log table */
		dbRs232_AlarmLog(conn, 0, sendst, '?', 0, action);
		PQfinish(conn);
		return -1;
	}

	strncpy(unitnrSt, &sendst[UNIT_NUMBER_START], UNIT_NUMBER_LNGTH);
	unitnrSt[UNIT_NUMBER_LNGTH] = 0;
	if (check_num(unitnrSt))
	{
		printf("%s: ERROR : invalid unit number %s\n", DateTimeStampStr(-1), unitnrSt);
		/* Write message to log table */
		dbRs232_AlarmLog(conn, 0, sendst, '?', 0, action);
		PQfinish(conn);
		return -1;
	}
	if (trace) printf("\tValid unit number '%s'\n", unitnrSt);
	unitnr = atoi(unitnrSt);
	
	alrmType = sendst[ALARM_TYPE_START];
	
	strncpy(alrmCodeSt, &sendst[ALARM_CODE_START], ALARM_CODE_LNGTH);
	alrmCodeSt[ALARM_CODE_LNGTH] = 0;
	if (check_num(alrmCodeSt))
	{
		printf("%s: ERROR : invalid alarm code number %s\n", DateTimeStampStr(-1), alrmCodeSt);
		/* Write message to log table */
		dbRs232_AlarmLog(conn, unitnr, sendst, alrmType, 0, action);
		PQfinish(conn);
		return -1;
	}
	if (trace) printf("\tValid alarm code number '%s'\n", alrmCodeSt);
	alrmCode = atoi(alrmCodeSt);

	printf("%s : Unit %d alarm code %c %d\n", DateTimeStampStr(-1), unitnr, alrmType, alrmCode);
	
	action[0] = 0;
	switch(alrmType)
	{
	case 'E' :
		switch (alrmCode)
		{
		case 0 :
			printf("%s : Ignore test\n", DateTimeStampStr(-1));
			break;
		case 100 :
			alarm_medic(conn, unitnr, action);
			break;
		case 110 :
		case 111 :
		case 115 :
			alarm_fire(conn, unitnr, action);
			break;
		case 120 :
		case 121 :
		case 122 :
		case 123 :
			alarm_panic(conn, unitnr, action);
			break;
		case 134 :
			alarm_nma(conn, unitnr, sendst, ALARM_CODE_START, action );
			return 0;
		case 130 :
		case 131 :
		case 132 :
		case 133 :
		case 134 :
		case 135 :
		case 136 :
		case 137 :
		case 140 :
		case 143 :
		case 144 :
		case 145 :
		case 150 :
		case 156 :
			alarm_burglar(conn, unitnr, sendst, ALARM_CODE_START, action );
			break;
		case 300 :
		case 301 :
		case 302 :
		case 305 :
		case 307 :
		case 321 :
		case 330 :
		case 333 :
		case 334 :
		case 336 :
		case 351 :
		case 355 :
		case 373 :
		case 380 :
		case 381 :
		case 384 :
			alarm_tech(alrmCode, conn, unitnr, action);
			break;
		case 401 :
		case 402 :
		case 403 :
		case 404 :
		case 405 :
		case 406 :
		case 407 :
		case 408 :
		case 409 :
			alarm_disarm(conn, unitnr, action);
			break;
		case 411 :
		case 421 :
		case 570 :
		case 574 :
		case 602 :
		case 627 :
		case 628 :
		case 997 :
		case 998 :
		case 999 :
			alarm_other(alrmCode, conn, unitnr, action);
			break;
		default :
			printf("%s: WARNING : unknown alarm code %d\n", DateTimeStampStr(-1), alrmCode);
			/* Write message to log table */
			alrmCode *= -1;
			dbRs232_AlarmLog(conn, unitnr, sendst, alrmType, alrmCode, action);
			PQfinish(conn);
			return -1;
		}
		break;
	case 'R' :
		switch (alrmCode)
		{
		case 0 :
			printf("%s : Ignore test\n", DateTimeStampStr(-1));
			break;
		case 100 :
		case 110 :
		case 111 :
		case 115 :
		case 120 : 
		case 121 :
		case 122 :
		case 123 :
		case 130 :
		case 131 :
		case 132 :
		case 133 :
		case 134 :
		case 135 :
		case 136 :
		case 137 :
		case 140 :
		case 143 :
		case 144 :
		case 145 :
		case 150 :
		case 156 :
		case 300 :
		case 301 :
		case 302 :
		case 305 :
		case 307 :
		case 321 :
		case 330 :
		case 333 :
		case 334 :
		case 336 :
		case 351 :
		case 355 :
		case 373 :
		case 380 :
		case 381 :
		case 384 :
		case 411 :
		case 421 :
		case 570 :
		case 574 :
		case 602 :
		case 627 :
		case 628 :
		case 997 :
		case 998 :
		case 999 :
			printf("%s : Ignore response code %d\n", DateTimeStampStr(-1), alrmCode);
			break;
		case 401 :
		case 402 :
		case 403 :
		case 404 :
		case 405 :
		case 406 :
		case 407 :
		case 408 :
		case 409 :
			alarm_arm(conn, unitnr, action);
			break;
		default :
			printf("%s: WARNING : unknown response code %d\n", DateTimeStampStr(-1), alrmCode);
			/* Write message to log table */
			alrmCode *= -1;
			dbRs232_AlarmLog(conn, unitnr, sendst, alrmType, alrmCode, action);
			PQfinish(conn);
			return -1;
		}
		break;
	default :
		printf("%s: WARNING : unknown alarm type %c\n", DateTimeStampStr(-1), alrmType);
		/* Write message to log table */
		alrmCode *= -1;
		dbRs232_AlarmLog(conn, unitnr, sendst, alrmType, alrmCode, action);
		PQfinish(conn);
		return -1;
	}
	
	/* Write message to log table */
	dbRs232_AlarmLog(conn, unitnr, sendst, alrmType, alrmCode, action);
	
	PQfinish(conn);
	
	return 0;
}

/*! \brief Set unit number from message string 
 * 	\param	conn		Database connection info
 *	\param	unitnr		Unit number
 *	\return	Zero when successful, otherwise non-zero
 */
int set_unit_name(PGconn * conn, 
		  int unitnr, 
		  char * unitNam)
{
	int rc;
	char oname[132], appt[132];
	
	rc = Rs232GetInfo(unitnr, conn, oname, appt);
	if (rc)
	{
		sprintf(unitNam, "Unit %d", unitnr);
	}
	else
	{
		if (0 == strcmp(oname, appt))
			sprintf(unitNam, "%s", oname);
		else
			sprintf(unitNam, "%s %s", oname, appt);
	}
	return 0;
}

/** \brief Handle burglar alarm
 */
void alarm_nma(PGconn * conn, 
	       int unitnr, 
	       char * msg, 
	       int mstart, 
	       char * action)
{
	char unitNam[256];
	char zoneSt[8];
	int rc, atyp, zone;
	
	if (trace) printf("\tNMA message: '%s'\n", msg+mstart+5);
	
	/* Get zone number */
	if (msg[mstart+7] == 'C')
	{
		strncpy(zoneSt, msg+mstart+8, 2);
		zoneSt[2] = 0;
		if (check_num(zoneSt))
		{
			printf("%s : zone %s is not a valid number\n", DateTimeStampStr(-1), zoneSt);
			zone = 0;
			sprintf(action, "Burglar <%s>", zoneSt);
		}
		else
		{
			zone = atoi(zoneSt);
			sprintf(action, "Burglar %d", zone);
		}
	}
	else
	{
		printf("%s : no zone number in message '%s'\n", DateTimeStampStr(-1), msg+mstart+7);
		zone = 0;
		strcpy(action, "Burglar");
	}
	
	/* Get alarm type */
	strncpy(zoneSt, msg+mstart+5, 2);
	zoneSt[2] = 0;
	atyp = atoi(zoneSt);
	if (atyp == NMA_ALARM_TYPE)
	{
		sprintf(action, "NMA %d", atyp);
		printf("%s : non-movement alarm message %d in zone %d\n", DateTimeStampStr(-1), atyp, zone);
		Rs232PirEvent(unitnr, conn);
		return;
	}

	if (EnablBurglr)
	{
		sprintf(action, "Burglar %d", atyp);

		printf("%s : Burglar alarm message %d in zone %d\n", DateTimeStampStr(-1), atyp, zone);

		set_unit_name(conn, unitnr, unitNam);

		rc = 1;		/* Location number not available yet */

		Rs232LogEvent(conn, unitnr, unitNam, "Burglar alarm", 6, zone);
		Rs232LogStatus(conn, unitnr, ZBSTATUS_ALARM, rc);
		Rs232Status(conn, ZBSTATUS_ALARM, unitnr);
		Rs232AlarmMsg(conn, unitnr, unitNam, "Burglar alarm", 6, rc);
	}
	else
	{
		sprintf(action, "Silent %d", atyp);
		printf("%s : Silent alarm message %d in zone %d\n", DateTimeStampStr(-1), atyp, zone);
	}
	return;
}

/** \brief Handle burglar alarm
 */
void alarm_burglar(PGconn * conn, 
		   int unitnr, 
		   char * msg, 
		   int mstart, 
		   char * action)
{
	char unitNam[256];
	char zoneSt[8];
	int rc, atyp, zone;
	
	if (trace) printf("\tBurglar alarm message: '%s'\n", msg+mstart+5);
	
	/* Get zone number */
	if (msg[mstart+7] == 'C')
	{
		strncpy(zoneSt, msg+mstart+8, 2);
		zoneSt[2] = 0;
		if (check_num(zoneSt))
		{
			printf("%s : zone %s is not a valid number\n", DateTimeStampStr(-1), zoneSt);
			zone = 0;
			sprintf(action, "Burglar <%s>", zoneSt);
		}
		else
		{
			zone = atoi(zoneSt);
			sprintf(action, "Burglar %d", zone);
		}
	}
	else
	{
		printf("%s : no zone number in message '%s'\n", DateTimeStampStr(-1), msg+mstart+7);
		zone = 0;
		strcpy(action, "Burglar");
	}
	
	/* Get alarm type */
	strncpy(zoneSt, msg+mstart+5, 2);
	zoneSt[2] = 0;
	atyp = atoi(zoneSt);
	if (atyp == NMA_ALARM_TYPE)
	{
		sprintf(action, "NMA %d", atyp);
		printf("%s : non-movement alarm message %d in zone %d\n", DateTimeStampStr(-1), atyp, zone);
		Rs232PirEvent(unitnr, conn);
		return;
	}

	if (EnablBurglr)
	{
		sprintf(action, "Burglar %d", atyp);

		printf("%s : Burglar alarm message %d in zone %d\n", DateTimeStampStr(-1), atyp, zone);

		set_unit_name(conn, unitnr, unitNam);

		rc = 1;		/* Location number not available yet */

		Rs232LogEvent(conn, unitnr, unitNam, "Burglar alarm", 6, zone);
		Rs232LogStatus(conn, unitnr, ZBSTATUS_ALARM, rc);
		Rs232Status(conn, ZBSTATUS_ALARM, unitnr);
		Rs232AlarmMsg(conn, unitnr, unitNam, "Burglar alarm", 6, rc);
	}
	else
	{
		sprintf(action, "Silent %d", atyp);
		printf("%s : Silent alarm message %d in zone %d\n", DateTimeStampStr(-1), atyp, zone);
	}
}

/** \brief Handle fire alarm
 * 	\param	conn		Database connection info
 *	\param	unitnr		Unit number
 */
void alarm_fire(PGconn * conn, 
		int unitnr,
		char * action)
{
	char unitNam[256];
	
	printf("%s : Fire alarm\n", DateTimeStampStr(-1));
	strcpy(action, "Fire");

	set_unit_name(conn, unitnr, unitNam);
	
	Rs232LogEvent(conn, unitnr, unitNam, "Fire alarm", 6, 0);
	Rs232LogStatus(conn, unitnr, ZBSTATUS_FIRE, 0);
	Rs232Status(conn, ZBSTATUS_FIRE, unitnr);
	Rs232AlarmMsg(conn, unitnr, unitNam, "Fire alarm", 6, 0);
} /* alarm_fire() */

/** \brief Handle panic alarm
 * 	\param	conn		Database connection info
 *	\param	unitnr		Unit number
 */
void alarm_panic(PGconn * conn, 
		 int unitnr, 
		 char * action)
{
	char unitNam[256];
	
	printf("%s : Panic alarm\n", DateTimeStampStr(-1));
	strcpy(action, "Panic");
			
	set_unit_name(conn, unitnr, unitNam);
	
	Rs232LogEvent(conn, unitnr, unitNam,  "Panic alert", 6, 0);
	Rs232LogStatus(conn, unitnr, ZBSTATUS_PANIC, 0);
	Rs232Status(conn, ZBSTATUS_PANIC, unitnr);
	Rs232AlarmMsg(conn, unitnr, unitNam, "Panic alert", 6, 0);
} /* alarm_panic() */

/** \brief Handle medic alarm
 * 	\param	conn		Database connection info
 *	\param	unitnr		Unit number
 */
void alarm_medic(PGconn * conn, 
		 int unitnr,
		 char * action)
{
	char unitNam[256];
	
	printf("%s : Medical emergency\n", DateTimeStampStr(-1));
	strcpy(action, "Medic");
	
	set_unit_name(conn, unitnr, unitNam);
	
	Rs232LogEvent(conn, unitnr, unitNam,  "Medical alert", 6, 0);
	Rs232LogStatus(conn, unitnr, ZBSTATUS_PANIC, 0);
	Rs232Status(conn, ZBSTATUS_PANIC, unitnr);
	Rs232AlarmMsg(conn, unitnr, unitNam, "Medical alert", 6, 0);
}

/** \brief Alarm was armed
 * 	\param	conn		Database connection info
 *	\param	unitnr		Unit number
 */
void alarm_arm(PGconn * conn, 
	       int unitnr,
	       char * action)
{
	printf("%s : Alarm armed\n", DateTimeStampStr(-1));
	strcpy(action, "arm");

	Rs232LogStatus(conn, unitnr, ZBSTATUS_ARMED, 0);
	Rs232Status(conn, ZBSTATUS_ARMED, unitnr);
}

/** \brief Alarm was disarmed
 * 	\param	conn		Database connection info
 *	\param	unitnr		Unit number
 */
void alarm_disarm(PGconn * conn, 
		  int unitnr,
		  char * action)
{
	printf("%s : Alarm disarmed\n", DateTimeStampStr(-1));
	strcpy(action, "disarm");

	Rs232LogStatus(conn, unitnr, ZBSTATUS_DISARMED, 0);
	Rs232Status(conn, ZBSTATUS_DISARMED, unitnr);
}

/** \brief Technical error
 * 	\param	conn		Database connection info
 *	\param	unitnr		Unit number
 */
void alarm_tech(int errn,
		PGconn * conn, 
		int unitnr,
		char * action)
{
	printf("%s : Technical error %d\n", DateTimeStampStr(-1), errn);
	sprintf(action, "Tech %d", errn);
}

/** \brief Technical error
 * 	\param	conn		Database connection info
 *	\param	unitnr		Unit number
 */
void alarm_other(int errn,
		PGconn * conn, 
		int unitnr,
		char * action)
{
	printf("%s : Other alarm %d\n", DateTimeStampStr(-1), errn);
	sprintf(action, "Other %d", errn);
}

/*! \brief Signal handler
 *	\param	signum	signal number
 */
void sighandle(int signum)
{
	printf("\n%s Shutting down after signal %d\n", DateTimeStampStr(-1), signum);
	
	exit(1);
}
