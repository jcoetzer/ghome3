/*! \file gHomePing.c
	\brief	Use the ICMP protocol to request "echo" from destination
 */

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: gHomePing.c,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.1.1.1  2011/05/28 09:00:03  johan
 * gHome logging server
 *
 * Revision 1.3  2011/02/10 11:09:41  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:14  johan
 * Add CVS stuff
 *
 *
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <libgen.h>
#include <popt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#include "ghPing.h"
#include "gversion.h"

char * IpAddr = NULL;

extern int pid;
extern struct protoent *proto;

int trace = 0;
int verbose = 0;
int ShowVersion = 0;

/*! \struct Command line parsing data structure */
struct poptOption optionsTable[] =
{
	{"ip", 		'I', POPT_ARG_STRING, 	&IpAddr, 	0, "IP address", NULL},
 	{"trace", 	't', POPT_ARG_NONE, 	&trace, 	0, "Trace/debug output", NULL},
	{"verbose", 	'v', POPT_ARG_NONE, 	&verbose, 	0, "Verbose output", NULL},
	{"version", 	'V', POPT_ARG_NONE, 	&ShowVersion,  0, "Display version information", NULL},
	POPT_AUTOHELP
	{NULL,0,0,NULL,0}
};

/*--------------------------------------------------------------------*/
/*--- main - look up host and start ping processes.                ---*/
/*--------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	int rc;
	poptContext optcon;        /* Context for parsing command line options */

	optcon = poptGetContext(argv[0],argc,(const char **)argv,optionsTable,0);
	rc = poptGetNextOpt(optcon);
	if (rc < -1)
	{
		printf("%s: %s - %s\n", basename(argv[0]), poptBadOption(optcon,0), poptStrerror(rc));
		poptPrintUsage(optcon, stderr, 0);
		return -1;
	}
	if (ShowVersion) displayVersion(argv[0]);
	if (NULL == IpAddr)
	{
		fprintf(stderr, "IP address not specified\n");
		poptPrintUsage(optcon, stderr, 0);
		return -1;
	}

	rc = send_listen(IpAddr);

	poptFreeContext(optcon);
	return rc;
}
