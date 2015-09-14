#include "DSPmanager.h"

void dspManager::init()
{
    m_chinaTelecomObject = new chinaTelecomObject();
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
bool dspManager::sendAdRequestToChinaTelecomDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg)
{
    return m_chinaTelecomObject->sendAdRequestToChinaTelecomDSP(base, data, dataLen, fn, arg);
}



