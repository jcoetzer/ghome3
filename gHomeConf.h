#ifndef GHOMECONF_H
#define GHOMECONF_H

struct gHomeConfStruct 
{
	int readTemp;
	int unEnroll;
	int discover;
	int sleepTime;
	int connectionTimer;
	int retry;
};
typedef struct gHomeConfStruct gHomeConfRec;

void gHomeConfInit(gHomeConfRec * cfg);

#endif

