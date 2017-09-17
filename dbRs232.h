#ifndef DBRS232HARDWARE_H
#define DBRS232HARDWARE_H

PGconn * Rs232DbConnect();

char * DateTimeStampStr(int ats);

int getUnitIp(PGconn * conn, char * ipadr);

int dbRs232_AlarmLog(PGconn * conn, int unitnr, char * rxbuf, char atype, int acode, char * action);

int Rs232AlarmMsg(PGconn * conn, int unitNum, char* name, char * descr, int eventtype, int loc);

int Rs232LogEvent(PGconn * conn, int unitNum, char * name, char * descr, int eventtype, int loc);

int Rs232LogStatus(PGconn * conn, int unitNum, int eventtype, int loc);

int Rs232Status(PGconn * conn, int eventtype, int unitNum); 

int Rs232GetInfo(int unitNo, PGconn * conn, char * oname, char *appt);

int Rs232PirEvent(int unitNo, PGconn * conn);

int Rs232Result(PGresult * result);

#endif
