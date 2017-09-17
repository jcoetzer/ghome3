#include <stdio.h>
#include <stdlib.h>

/*
 * $Author: johan $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: gversion.c,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.22  2011/03/09 08:35:45  johan
 * Fix statuscurr update bug
 *
 * Revision 1.21  2011/02/24 16:20:55  johan
 * Store user descriptor in database
 *
 * Revision 1.20  2011/02/23 16:27:15  johan
 * Store time of last heartbeat request
 *
 * Revision 1.19  2011/02/23 15:48:49  johan
 * Reset alarm log
 *
 * Revision 1.18  2011/02/18 06:43:47  johan
 * Streamline polling
 *
 * Revision 1.17  2011/02/16 20:42:05  johan
 * Add user description field
 *
 * Revision 1.16  2011/02/15 15:03:36  johan
 * Remove command parameter in message read
 *
 * Revision 1.15  2011/02/15 14:42:38  johan
 * Add initialize state description
 *
 * Revision 1.14  2011/02/15 14:15:27  johan
 * Reduce number of system ping retries
 *
 * Revision 1.13  2011/02/15 13:58:01  johan
 * Check for error after sending
 *
 * Revision 1.12  2011/02/15 13:21:48  johan
 * New version rolled out
 *
 * Revision 1.11  2011/02/13 19:49:09  johan
 * Turn temperature poll
 *
 * Revision 1.10  2011/02/10 15:43:00  johan
 * Log wrong zone number in PIR message
 *
 * Revision 1.9  2011/02/10 11:09:42  johan
 * Take out CVS Id tag
 *
 * Revision 1.8  2011/02/09 19:31:49  johan
 * Stop core dump during polling
 *
 * Revision 1.7  2011/02/03 13:41:17  johan
 * New version number
 *
 * Revision 1.6  2011/02/03 08:38:15  johan
 * Add CVS stuff
 *
 * Revision 1.5  2011/02/03 08:07:49  johan
 * Add CVS stuff
 *
 *
 */

const int Version = 1;
const int Major = 12;
const int Minor = 1;

void displayVersion(char * pname)
{
	fprintf(stderr, "%s version %d.%d.%d (%s %s)\n", pname, Version, Major, Minor, __DATE__, __TIME__);
	exit(0);
}
