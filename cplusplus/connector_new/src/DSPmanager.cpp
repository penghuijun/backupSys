#include "DSPmanager.h"

void dspManager::init(bool enTele, bool enGYin, bool enSmaato, bool enInmobi)
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
bool dspManager::sendAdRequestToChinaTelecomDSP(const char *data, int dataLen, bool enLogRsq)
{
    return m_chinaTelecomObject->sendAdRequestToChinaTelecomDSP(data, dataLen, enLogRsq);
}
bool dspManager::sendAdRequestToGuangYinDSP(const char *data, int dataLen)
{
    return m_guangYinObject->sendAdRequestToGuangYinDSP( data, dataLen);
}
int dspManager::sendAdRequestToSmaatoDSP(const char *data, int dataLen, string& uuid)
{
    return m_smaatoObject->sendAdRequestToSmaatoDSP(data, dataLen, uuid);
}
bool dspManager::sendAdRequestToInMobiDSP(const char *data, int dataLen, bool enLogRsq)
{
    return m_inmobiObject->sendAdRequestToInMobiDSP(data, dataLen, enLogRsq);
}

bool dspManager::recvBidResponseFromSmaatoDsp(int sock, struct spliceData_t *fullData_t)
{
    return m_smaatoObject->recvBidResponseFromSmaatoDsp(sock, fullData_t);
}
void dspManager::creatConnectDSP(bool enTele, bool enGYin, bool enSmaato, bool enInmobi,
                                                                    struct event_base * base, 
                                                                    event_callback_fn tele_fn, event_callback_fn gyin_fn, event_callback_fn smaato_fn, event_callback_fn inmobi_fn,
                                                                    void *arg)
{
    if(enTele)
    {        
        m_chinaTelecomObject->creatConnectDSP(base, tele_fn, arg);
    }
    if(enGYin)
    {        
        m_guangYinObject->creatConnectDSP(base, gyin_fn, arg);
    }
    if(enSmaato)
    {        
        //m_smaatoObject->creatConnectDSP(base, smaato_fn, arg);
        //m_smaatoObject->smaatoConnectDSP();
    }
    if(enInmobi)
    {
        m_inmobiObject->creatConnectDSP(base, inmobi_fn, arg);
    }
}



