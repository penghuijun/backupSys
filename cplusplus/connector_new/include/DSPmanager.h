#ifndef __DSPMANAGER_H__
#define __DSPMANAGER_H__

#include "DSPconfig.h"

class dspManager
{
public:
	dspManager(){}
	void init(bool enTele, bool enGYin, bool enSmaato, bool enInmobi);		
	list<listenObject *> *getListenObjectList();
	bool isChinaTelecomObjectCeritifyCodeEmpty();
	bool getCeritifyCodeFromChinaTelecomDSP();
	bool sendAdRequestToChinaTelecomDSP(const char *data, int dataLen, bool enLogRsq);
	bool sendAdRequestToGuangYinDSP(const char *data, int dataLen);
	int sendAdRequestToSmaatoDSP(const char *data, int dataLen, string& uuid);
	bool sendAdRequestToInMobiDSP(const char *data, int dataLen, bool enLogRsq);
	bool recvBidResponseFromSmaatoDsp(int sock, struct spliceData_t *fullData_t);
	
	chinaTelecomObject * getChinaTelecomObject(){return m_chinaTelecomObject;}
	guangYinObject * getGuangYinObject(){return m_guangYinObject;}
	smaatoObject * getSmaatoObject(){return m_smaatoObject;}
	inmobiObject * getInMobiObject(){return m_inmobiObject;}
	~dspManager(){}
private:
	chinaTelecomObject  	*m_chinaTelecomObject;	
	guangYinObject		*m_guangYinObject;
	smaatoObject			*m_smaatoObject;
	inmobiObject			*m_inmobiObject;
	
};
#endif
