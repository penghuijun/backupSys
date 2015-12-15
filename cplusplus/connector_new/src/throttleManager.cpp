#include "throttleManager.h"

throttleObject::throttleObject(const string& throttle_ip, unsigned short pub_port, unsigned short manager_port, unsigned short worker_num)
{
    try
    {
    	m_throttle_ip = throttle_ip;
    	m_throttle_pubPort = pub_port;
    	m_throttle_managerPort = manager_port;
    	m_throttle_workerNum = worker_num;
        g_manager_logger->info("[add new throttle]:{0}, {1:d}", throttle_ip, manager_port);
    }
    catch(...)
    {
        g_manager_logger->emerg("throttleObject structrue exception");
        exit(1);
    }
}
void throttleObject::connectToThrottleManagerPort(zeromqConnect &connector, string& Identify, struct event_base * base, event_callback_fn fn, void * arg)
{    
    m_sendLHRToThrottleHandler = connector.establishConnect(true, true, Identify, "tcp", ZMQ_DEALER, 
									m_throttle_ip.c_str(), m_throttle_managerPort, &m_sendLHRToThrottleFd);//client sub    
    m_sendLHRToThrottleEvent = event_new(base, m_sendLHRToThrottleFd, EV_READ|EV_PERSIST, fn, arg); 
    event_add(m_sendLHRToThrottleEvent, NULL);
}
void throttleObject::connectToThrottlePubPort(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg)
{    
    for(int i=0; i<m_throttle_workerNum; i++)
    {
        recvAdReq_t *recvAdReq = new recvAdReq_t();
        recvAdReq->m_recvAdReqFromThrottleHandler = connector.establishConnect(true,"tcp", ZMQ_SUB, 
									m_throttle_ip.c_str(), m_throttle_pubPort+i, &recvAdReq->m_recvAdReqFromThrottleFd);    
        recvAdReq->m_recvAdReqFromThrottleEvent = event_new(base, recvAdReq->m_recvAdReqFromThrottleFd, EV_READ|EV_PERSIST, fn, arg); 
        event_add(recvAdReq->m_recvAdReqFromThrottleEvent, NULL);   
        m_recvAdReqVector.push_back(recvAdReq);
    }
    
}

void throttleObject::sendLoginRegisterToThrottle(const string& ip,unsigned short manager_port)
{
    if(!m_connectorLoginToThro)
    {        
        managerProPackage::send(m_sendLHRToThrottleHandler, managerProtocol_messageTrans_CONNECTOR
            , managerProtocol_messageType_LOGIN_REQ, ip, manager_port);
        g_manager_logger->info("[login req][connector -> throttle]:{0},{1:d}", m_throttle_ip, m_throttle_managerPort);		
        return;
    }
    for(auto it = m_throSubKey_list.begin(); it != m_throSubKey_list.end(); it++)
    {
        throSubKeyObject* subKeyObject = *it;
        if(subKeyObject == NULL) continue;
        string& key = subKeyObject->get_throSubKey();
        if(key.empty()) continue;
        if(subKeyObject->get_throSubscribed() == false)
        {
            //if don't setsockopt with ZMQ_SUBSCRIBE option,this zmq_socket will recv nothing !!!
            for(auto it=m_recvAdReqVector.begin(); it != m_recvAdReqVector.end(); it++)
            {
                recvAdReq_t *obj = *it;
                int rc = zmq_setsockopt(obj->m_recvAdReqFromThrottleHandler,ZMQ_SUBSCRIBE,key.c_str(),key.size());
                if(rc == 0)
                {
                    subKeyObject->set_throSubscribed(true);
                    g_manager_logger->info("[add ZMQ_SUBSCRIBE]: {0}",key);
                    cout << "[add ZMQ_SUBSCRIBE]: " << key << endl;
                }    
            }
                           
        }
        if(subKeyObject->get_throRegisted() == false)
        {
            managerProPackage::send(m_sendLHRToThrottleHandler, managerProtocol_messageTrans_CONNECTOR
               , managerProtocol_messageType_REGISTER_REQ, key, ip, manager_port);
            g_manager_logger->info("[register req][connector -> throttle]:{0},{1:d}", m_throttle_ip, m_throttle_managerPort);   
            cout << "[register req][connector -> throttle]!" << endl;
        }
    }
}
void *throttleObject::get_sendLoginRegisterToThrottleHandler(int fd)
{   
    if(fd == m_sendLHRToThrottleFd)
    {        
        return m_sendLHRToThrottleHandler;
    }
    else
        return NULL;
}
void *throttleObject::get_recvAdReqFromThrottleHandler(int fd)
{   
    for(auto it=m_recvAdReqVector.begin(); it != m_recvAdReqVector.end(); it++)
    {
        recvAdReq_t *obj = *it;
        if(fd == obj->m_recvAdReqFromThrottleFd)
        {        
            return obj->m_recvAdReqFromThrottleHandler;
        }
    }

    return NULL;
}

bool throttleObject::drop()
{    
    if(m_connectorLoginToThro)
    {
        m_throLostHeartTimes++;        
        if(m_throLostHeartTimes > m_throLostHeartTimes_max)
        {
            m_throLostHeartTimes = 0;
            m_connectorLoginToThro = false;    
            set_throSubKeyListRegistedStatus(false);
            g_manager_logger->warn("lost heart with throttle");
            return true;
        }
    }
    return false;
}
void throttleObject::set_zmqUnsubscribeKey(string& key)
{
    for(auto it=m_recvAdReqVector.begin(); it != m_recvAdReqVector.end(); it++)
    {
        recvAdReq_t *obj = *it;
        int rc =  zmq_setsockopt(obj->m_recvAdReqFromThrottleHandler, ZMQ_UNSUBSCRIBE, key.c_str(), key.size());
        if(rc!=0)
        {
        	g_manager_logger->info("unsucsribe {0} failure:: {1}", key, zmq_strerror(zmq_errno()));	    	
        }
        else
        {
        	g_manager_logger->info("unsucsribe {0} success", key);
        }	
    }    
}
bool throttleObject::add_throSubKey(string &key)
{
    m_throSubKeyList_lock.lock();
	for(auto it = m_throSubKey_list.begin(); it != m_throSubKey_list.end(); it++)
	{
		throSubKeyObject* throSubKey =  *it;
		if(throSubKey==NULL) continue;
		if(key == throSubKey->get_throSubKey())
		{
		    m_throSubKeyList_lock.unlock();
			return false;
		}
	}
	throSubKeyObject* subKey = new throSubKeyObject(key);
	m_throSubKey_list.push_back(subKey);
    m_throSubKeyList_lock.unlock();
	return true;
}
void throttleObject::erase_throSubKey(string &key)
{
    m_throSubKeyList_lock.lock();
	for(auto it = m_throSubKey_list.begin(); it != m_throSubKey_list.end();)
	{
		throSubKeyObject* throSubKey =  *it;		
		if(throSubKey && (key == throSubKey->get_throSubKey()))
		{
		    set_zmqUnsubscribeKey(key);
            it = m_throSubKey_list.erase(it);        	
		}
        else
            it++;
	}	
    m_throSubKeyList_lock.unlock();
}
bool throttleObject::set_throSubKeyRegisted(const string& throttleIP,unsigned short throManagerPort,const string& key)
{
    if((m_throttle_ip != throttleIP)||(m_throttle_managerPort != throManagerPort)) return false;
    m_throSubKeyList_lock.lock();
	for(auto it = m_throSubKey_list.begin(); it != m_throSubKey_list.end(); it++)
	{
		throSubKeyObject *SubKey = *it;
		if(SubKey == NULL) continue;
		if(SubKey->get_throSubKey() == key) 
		{
			SubKey->set_throRegisted(true);
            m_throSubKeyList_lock.unlock();
			return true;
		}
	}
    m_throSubKeyList_lock.unlock();
	return false;
}
void throttleObject::set_throSubKeyListRegistedStatus(bool value)
{
    m_throSubKeyList_lock.lock();
	for(auto it = m_throSubKey_list.begin(); it != m_throSubKey_list.end(); it++)
	{
		throSubKeyObject *SubKey = *it;
		if(SubKey == NULL) continue;
		SubKey->set_throRegisted(value);
	}
    m_throSubKeyList_lock.unlock();
}




throttleManager::throttleManager()
{
    m_throttleList_lock.init();
}

void throttleManager::init(throttleInformation& throttleInfo)
{
    try
    {
        for(auto thro_it = throttleInfo.get_throttleConfigList().begin();thro_it != throttleInfo.get_throttleConfigList().end();thro_it++)
        {
            throttleConfig* thro_config = *thro_it;
            if(thro_config == NULL) continue;
            throttleObject *thro_Obj = new throttleObject(thro_config->get_throttleIP(),thro_config->get_throttlePubPort(),thro_config->get_throttleManagerPort(), thro_config->get_throttleWorkerNum());

            m_throttleList_lock.lock();
            m_throttle_list.push_back(thro_Obj);
            m_throttleList_lock.unlock();
        }         
    }
    catch(...)
    {
        g_manager_logger->emerg("throttleManager init error");
        exit(1);
    }

}
void throttleManager::connectToThrottleManagerPortList(zeromqConnect &connector, string& Identify, struct event_base * base, 
												event_callback_fn fn, void * arg)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin();it != m_throttle_list.end();it++)
    {
        throttleObject* throttle =  *it;
		if(throttle) throttle->connectToThrottleManagerPort(connector, Identify, base, fn, arg);
    }
    m_throttleList_lock.unlock();
}
void throttleManager::connectToThrottlePubPortList(zeromqConnect &connector, struct event_base * base, 
												event_callback_fn fn, void * arg)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin();it != m_throttle_list.end();it++)
    {
        throttleObject* throttle =  *it;
		if(throttle) throttle->connectToThrottlePubPort(connector, base, fn, arg);
    }
    m_throttleList_lock.unlock();
}

void throttleManager::sendLoginRegisterToThrottleList(const string& ip,unsigned short manager_port)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin();it != m_throttle_list.end();it++)
    {
        throttleObject* throttle =  *it;
		if(throttle) throttle->sendLoginRegisterToThrottle(ip,manager_port);
    }
    m_throttleList_lock.unlock();
}
void *throttleManager::get_sendLoginHeartToThrottleHandler(int fd)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin();it != m_throttle_list.end();it++)
    {
        throttleObject* throttle =  *it;
		if(throttle == NULL) continue;
        void *handler = throttle->get_sendLoginRegisterToThrottleHandler(fd);
        m_throttleList_lock.unlock();
        if(handler) return handler;
    }
    m_throttleList_lock.unlock();
    return NULL;
}
void *throttleManager::get_recvAdReqFromThrottleHandler(int fd)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin();it != m_throttle_list.end();it++)
    {
        throttleObject* throttle =  *it;
		if(throttle == NULL) continue;
        void *handler = throttle->get_recvAdReqFromThrottleHandler(fd);
        m_throttleList_lock.unlock();
        if(handler) return handler;
    }
    m_throttleList_lock.unlock();
    return NULL;
}
bool throttleManager::add_throttle(const string& ip,unsigned short dataPort,unsigned short managerPort,
                                      zeromqConnect & connector,string & Identify,struct event_base * base,event_callback_fn fn,void * arg)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin();it != m_throttle_list.end();it++)
    {
        throttleObject* throttle =  *it;
        if(throttle == NULL) continue;
        if((ip == throttle->get_throttleIP())&&(managerPort == throttle->get_throttleManagerPort())
			&&(dataPort == throttle->get_throttlePubPort()))
        {
            m_throttleList_lock.unlock();
			return false;
        }
    }
    throttleObject *throttle = new throttleObject(ip, dataPort, managerPort, 3);
    throttle->connectToThrottleManagerPort(connector,Identify,base,fn,arg);
    m_throttle_list.push_back(throttle);
    m_throttleList_lock.unlock();
    return true;
}
void throttleManager::logined(const string& throttleIP, unsigned short throttleManagerPort)	
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
    {
        throttleObject *thorttle = *it;
        if(thorttle&&(thorttle->get_throttleIP() == throttleIP)&&(thorttle->get_throttleManagerPort() == throttleManagerPort))
        {
            //throttle logined success
            thorttle->set_connectorLoginToThro(true);
            break;
        }
    }
    m_throttleList_lock.unlock();
}
bool throttleManager::recvHeartReq(const string &throttleIP, unsigned short throttleManagerPort)
{    
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end();it++)
	{
		throttleObject *throttle = *it;
		if(throttle)
		{
		    if((throttle->get_throttleIP()==throttleIP)&&(throttle->get_throttleManagerPort()==throttleManagerPort))
			{
			    //find the throttle
				throttle->lost_times_clear();
                m_throttleList_lock.unlock();                    
                return true;
			}
		}
	}		
    m_throttleList_lock.unlock();    
    return false;
}
bool throttleManager::dropDev()
{
    bool ret = false;
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle = *it;
		if(throttle == NULL) continue;
        if(throttle->drop()) ret = true;
	}
    m_throttleList_lock.unlock();
    return ret;
}
bool throttleManager::update_throttleList(vector<throttleConfig*>& throConfList)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end();)
    {
        throttleObject *throttle = *it;
        if(throttle)
        {
            auto itor = throConfList.begin();
            for(itor  = throConfList.begin(); itor != throConfList.end(); itor++)
            {
                throttleConfig * bConf = *itor;
                if(bConf == NULL) continue;
                if((bConf->get_throttleIP() == throttle->get_throttleIP())
                    &&(bConf->get_throttleManagerPort()== throttle->get_throttleManagerPort()))
                    break;
            }       

            if(itor == throConfList.end())//delete throttle
            {
                delete throttle;
                it = m_throttle_list.erase(it);
            }
            else
            {
                it++;
            }
        }
        else
        {
            it++;
        }
    }
    m_throttleList_lock.unlock();
    return true;
}
void throttleManager::add_throSubKey(string& key)
{
	m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle =  *it;
		if(throttle)
		{
			throttle->add_throSubKey(key);
		}
	}
	m_throttleList_lock.unlock();
}
void throttleManager::erase_throSubKey(string& key)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle =  *it;
		if(throttle)
		{
			throttle->erase_throSubKey(key);
		}
	}
	m_throttleList_lock.unlock();
}

void throttleManager::set_throSubKeyRegisted(const string& throttleIP,unsigned short throManagerPort,const string& key)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *thorttle = *it;
		if(thorttle == NULL) continue;
		if(thorttle->set_throSubKeyRegisted(throttleIP, throManagerPort, key)) break;
	}
    m_throttleList_lock.unlock();
}





