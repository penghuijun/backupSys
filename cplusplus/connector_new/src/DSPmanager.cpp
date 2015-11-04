#include "DSPmanager.h"

void dspManager::init(bool enTele, bool enGYin, bool enSmaato)
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
}
bool dspManager::isChinaTelecomObjectCeritifyCodeEmpty()
{
    return m_chinaTelecomObject->isCeritifyCodeEmpty();
}
list<listenObject *>& dspManager::getListenObjectList()
{
    return m_chinaTelecomObject->getListenObjectList();
}
bool dspManager::getCeritifyCodeFromChinaTelecomDSP()
{
    return m_chinaTelecomObject->getCeritifyCodeFromChinaTelecomDSP();
}
bool dspManager::sendAdRequestToChinaTelecomDSP(struct event_base * base, const char *data, int dataLen, bool enLogRsq,event_callback_fn fn, void *arg)
{
    return m_chinaTelecomObject->sendAdRequestToChinaTelecomDSP(base, data, dataLen, enLogRsq, fn, arg);
}
bool dspManager::sendAdRequestToGuangYinDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg)
{
    return m_guangYinObject->sendAdRequestToGuangYinDSP(base, data, dataLen, fn, arg);
}
void dspManager::creatConnectDSP(bool enTele, bool enGYin, bool enSmaato,struct event_base * base, event_callback_fn tele_fn, event_callback_fn gyin_fn, event_callback_fn smaato_fn, void *arg)
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
        m_smaatoObject->creatConnectDSP(base, smaato_fn, arg);
    }
}



