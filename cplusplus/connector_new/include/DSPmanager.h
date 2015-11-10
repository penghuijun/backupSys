#ifndef __DSPMANAGER_H__
#define __DSPMANAGER_H__

#include "DSPconfig.h"

class dspManager
{
public:
	dspManager(){}
	void init(bool enTele, bool enGYin, bool enSmaato);		
	list<listenObject *> *getListenObjectList();
	bool isChinaTelecomObjectCeritifyCodeEmpty();
	bool getCeritifyCodeFromChinaTelecomDSP();
	bool sendAdRequestToChinaTelecomDSP(struct event_base * base, const char *data, int dataLen, bool enLogRsq,event_callback_fn fn, void *arg);
	bool sendAdRequestToGuangYinDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg);
	int sendAdRequestToSmaatoDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg);
	bool recvBidResponseFromSmaatoDsp(int sock, struct spliceData_t *fullData_t);
	void creatConnectDSP(bool enTele, bool enGYin, bool enSmaato, struct event_base * base, event_callback_fn tele_fn, event_callback_fn gyin_fn, event_callback_fn smaato_fn, void *arg);
	chinaTelecomObject * getChinaTelecomObject(){return m_chinaTelecomObject;}
	guangYinObject * getGuangYinObject(){return m_guangYinObject;}
	smaatoObject * getSmaatoObject(){return m_smaatoObject;}
	~dspManager(){}
private:
	chinaTelecomObject  	*m_chinaTelecomObject;	
	guangYinObject		*m_guangYinObject;
	smaatoObject			*m_smaatoObject;
	
};
#endif
