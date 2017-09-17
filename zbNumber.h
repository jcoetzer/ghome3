#ifndef ZBNUMBER_H
#define ZBNUMBER_H

/*
 * $Author: johan $
 * $Date: 2011/06/13 19:39:21 $
 * $Revision: 1.1.1.1 $
 * $State: Exp $
 *
 * $Log: zbNumber.h,v $
 * Revision 1.1.1.1  2011/06/13 19:39:21  johan
 * Initialising sources in CVS
 *
 * Revision 1.4  2011/02/18 06:43:47  johan
 * Streamline polling
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

int zbNumberHex2dec(char hex);
int readBuffer(int skt, void * unit, char * cBuffer, int cLen, int usec);
int checkBuffer(int skt, void * unit, char * cBuffer, int cLen, int usec);

int zbNumberComputeFCS(char * sData);

#endif
