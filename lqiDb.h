#ifndef LQIDB_H
#define LQIDB_H

#include <libpq-fe.h>

int lqiDbProc(int unitNo,
	      struct unitLqiStruct * lqiRec, 
              int unitnr);

int lqiDbInsertInfo(PGconn * DbConn,
		    int unitNo, 
		    int nwaddr,
		    int rxqual);
#endif
