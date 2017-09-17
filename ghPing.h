#ifndef GHPING_H
#define GHPING_H

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: ghPing.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.3  2011/02/10 11:09:42  johan
 * Take out CVS Id tag
 *
 * Revision 1.2  2011/02/03 08:38:15  johan
 * Add CVS stuff
 *
 *
 *
 */

unsigned short checksum(void *b, int len);

void display(void *buf, int bytes);
void ping(struct sockaddr_in *addr);
void listener(void);
int listen1(void);
void ping1(struct sockaddr_in *addr);

int send_listen(char * ipAddr);

#endif
