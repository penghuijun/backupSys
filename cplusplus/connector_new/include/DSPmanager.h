#ifndef __DSPMANAGER_H__
#define __DSPMANAGER_H__

#include "DSPconfig.h"

class dspManager
{
public:
	dspManager(){}
	void init();		
	list<listenObject *>& getListenObjectList();
	bool isChinaTelecomObjectCeritifyCodeEmpty();
	bool getCeritifyCodeFromChinaTelecomDSP();
	bool sendAdRequestToChinaTelecomDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg);
	chinaTelecomObject * getChinaTelecomObject(){return m_chinaTelecomObject;}
	~dspManager(){}
private:
	chinaTelecomObject *m_chinaTelecomObject;	
	
};
#endif
