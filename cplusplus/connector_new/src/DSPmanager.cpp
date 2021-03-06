#include "DSPmanager.h"

void dspManager::init(bool enTele, bool enGYin, bool enSmaato, bool enInmobi, bool enBaidu)
{
    if(enTele)
    {        
        m_chinaTelecomObject = new chinaTelecomObject();
    }
    if(enGYin)
    {        
        m_guangYinObject = new guangYinObject();
    }
    if(enSmaato)
    {        
        m_smaatoObject = new smaatoObject();
    }
    if(enInmobi)
    {
        m_inmobiObject = new inmobiObject();
    }
    if(enBaidu)
    {
        m_baiduObject = new baiduObject();
    }
}
bool dspManager::isChinaTelecomObjectCeritifyCodeEmpty()
{
    return m_chinaTelecomObject->isCeritifyCodeEmpty();
}
list<listenObject *> *dspManager::getListenObjectList()
{
    return m_chinaTelecomObject->getListenObjectList();
}
bool dspManager::getCeritifyCodeFromChinaTelecomDSP()
{
    return m_chinaTelecomObject->getCeritifyCodeFromChinaTelecomDSP();
}
bool dspManager::sendAdRequestToChinaTelecomDSP(const char *data, int dataLen, bool enLogRsq, string& ua)
{
    return m_chinaTelecomObject->sendAdRequestToChinaTelecomDSP(data, dataLen, enLogRsq, ua);
}
bool dspManager::sendAdRequestToGuangYinDSP(const char *data, int dataLen, string& ua)
{
    return m_guangYinObject->sendAdRequestToGuangYinDSP( data, dataLen, ua);
}
int dspManager::sendAdRequestToSmaatoDSP(const char *data, int dataLen, string& uuid, string& ua)
{
    return m_smaatoObject->sendAdRequestToSmaatoDSP(data, dataLen, uuid, ua);
}
int dspManager::sendAdRequestToInMobiDSP(const char *data, int dataLen, bool enLogRsq, string& ua)
{
    return m_inmobiObject->sendAdRequestToInMobiDSP(data, dataLen, enLogRsq, ua);
}
int dspManager::sendAdRequestToBaiduDSP(const char *data, int dataLen, string& uuid, string& ua)
{
    return m_baiduObject->sendAdRequestToBaiduDSP(data, dataLen, uuid, ua);
}

bool dspManager::recvBidResponseFromSmaatoDsp(int sock, struct spliceData_t *fullData_t)
{
    return m_smaatoObject->recvBidResponseFromSmaatoDsp(sock, fullData_t);
}

bool dspManager::recvBidResponseFromInmobiDsp(int sock, struct spliceData_t *fullData_t)
{
    return m_inmobiObject->recvBidResponseFromInmobiDsp(sock, fullData_t);
}

bool dspManager::recvBidResponseFromBaiduDsp(int sock, struct spliceData_t *fullData_t)
{
    return m_baiduObject->recvBidResponseFromBaiduDsp(sock, fullData_t);
}




