#ifndef GHOMELOGGER_H
#define GHOMELOGGER_H

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: gHomeLogger.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.1.1.1  2011/05/28 09:00:03  johan
 * gHome logging server
 *
 * Revision 1.4  2011/02/13 19:49:09  johan
 * Turn temperature poll
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

int LoggerStart(int n, int readTemp);

int LoggerCheckPid(pid_t cpid);

#endif
