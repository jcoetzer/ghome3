#include "gHomeConf.h"

extern int ReadTemp;
extern int UnEnroll;
extern int SleepTime;
extern int ConnectionTimer;

void gHomeConfInit(gHomeConfRec * cfg)
{
	cfg->readTemp = ReadTemp;
	cfg->unEnroll = UnEnroll;
	cfg->discover = 0;
	cfg->sleepTime = SleepTime;
	cfg->connectionTimer = ConnectionTimer;
	cfg->retry = 7;
}
