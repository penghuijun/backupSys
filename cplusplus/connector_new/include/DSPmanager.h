#ifndef __DSPMANAGER_H__
#define __DSPMANAGER_H__

#include "DSPconfig.h"

class dspManager
{
public:
	dspManager(){}
	void init(bool enTele, bool enGYin, bool enSmaato, bool enInmobi, bool enBaidu);		
	list<listenObject *> *getListenObjectList();
	bool isChinaTelecomObjectCeritifyCodeEmpty();
	bool getCeritifyCodeFromChinaTelecomDSP();
	bool sendAdRequestToChinaTelecomDSP(const char *data, int dataLen, bool enLogRsq, string& ua);
	bool sendAdRequestToGuangYinDSP(const char *data, int dataLen, string& ua);
	int sendAdRequestToSmaatoDSP(const char *data, int dataLen, string& uuid, string& ua);
	int sendAdRequestToInMobiDSP(const char *data, int dataLen, bool enLogRsq, string& ua);
	int sendAdRequestToBaiduDSP(const char *data, int dataLen, string& uuid, string& ua);
	bool recvBidResponseFromSmaatoDsp(int sock, struct spliceData_t *fullData_t);
	bool recvBidResponseFromInmobiDsp(int sock, struct spliceData_t *fullData_t);
	bool recvBidResponseFromBaiduDsp(int sock, struct spliceData_t *fullData_t);
	
	chinaTelecomObject * getChinaTelecomObject(){return m_chinaTelecomObject;}
	guangYinObject * getGuangYinObject(){return m_guangYinObject;}
	smaatoObject * getSmaatoObject(){return m_smaatoObject;}
	inmobiObject * getInMobiObject(){return m_inmobiObject;}
	baiduObject  * getBaiduObject(){return m_baiduObject;}
	~dspManager(){}
private:
	chinaTelecomObject  	*m_chinaTelecomObject;	
	guangYinObject		*m_guangYinObject;
	smaatoObject			*m_smaatoObject;
	inmobiObject			*m_inmobiObject;
	baiduObject			*m_baiduObject;
	
};
#endif
