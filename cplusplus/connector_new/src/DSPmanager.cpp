#include "DSPmanager.h"

void dspManager::init()
{
    m_chinaTelecomObject = new chinaTelecomObject();
    m_guangYinObject     = new guangYinObject();
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




