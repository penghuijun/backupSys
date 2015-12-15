#include "throttleManager.h"
using namespace com::rj::protos::manager;

zmqSubscribeKey::zmqSubscribeKey(string& bidderIP, unsigned short bidderPort,const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort)
{
	set(bidderIP, bidderPort, bcIP, bcManagerPort, bcDataPort);
}
void zmqSubscribeKey::set(string& bidderIP, unsigned short bidderPort,const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort)
{
	m_bidderIP = bidderIP;
	m_bidderManagerPort = bidderPort;
	m_bcIP = bcIP;
	m_bcManagerPort = bcManagerPort;
	m_bcDataPort = bcDataPort;
	ostringstream os;
	os<<bcIP<<":"<<bcManagerPort<<":"<<bcDataPort<<"-"<<bidderIP<<":"<<bidderPort;
	m_subKey = os.str();
}

bool zmqSubscribeKey::set_register_status(string& bidderIP, unsigned short bidderManagerPort, string& bcIP,
							unsigned short bcManangerPort, unsigned short bcDataPort, bool registed)
{
    if((m_bidderIP == bidderIP)&&(m_bidderManagerPort == bidderManagerPort)&&
			(m_bcIP == bcIP)&&(m_bcManagerPort == bcManangerPort)&&(m_bcDataPort == bcDataPort))
   	{
   		m_bcRegisted = registed;
		return true;
   	}
	return false;
}
bool zmqSubscribeKey::set_register_stataus(bool status){m_bcRegisted = status;}



zmqSubKeyManager::zmqSubKeyManager()
{
}


//add key to key list
string& zmqSubKeyManager::add_subKey(string& bidder_ip, unsigned short bidder_port,const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPOort)
{
	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end(); it++)
	{
		zmqSubscribeKey* subscribe_key =  *it;
		if(subscribe_key == NULL) continue;
		if(((bidder_ip==subscribe_key->get_bidder_ip())&&(bidder_port==subscribe_key->get_bidder_manager_port()))&&
			((bc_ip==subscribe_key->get_bc_ip())&&(bcManagerPort ==subscribe_key->get_bc_manager_port()))
			&&(bcDataPOort == subscribe_key->get_bc_data_port()))
		{
		//key exist
			return subscribe_key->get_subKey();
		}
	}

    //new a key and add it to key list
	zmqSubscribeKey *key = new zmqSubscribeKey(bidder_ip, bidder_port, bc_ip, bcManagerPort, bcDataPOort);
	m_subKey_list.push_back(key);	
	return key->get_subKey();
}


//the  key about bc exist 
bool zmqSubKeyManager::keyExist(const string& bcIp, unsigned short managerPort)
{
	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end(); it++)
	{
		zmqSubscribeKey* subscribe_key =  *it;
		if(subscribe_key == NULL) continue;
		if((bcIp==subscribe_key->get_bc_ip())&&(managerPort ==subscribe_key->get_bc_manager_port()))
		{
			return true;
		}
	}
    return false;
}

// erase key from key list	
void zmqSubKeyManager::erase_subKey(string& subKey)
{
	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end();)
	{
		zmqSubscribeKey* key = *it;
		if(key&&(key->get_subKey() == subKey))
		{
			delete key;
			it = m_subKey_list.erase(it);
		}
		else
		{
			it++;
		}
	}
}

//erase key about bc address
void zmqSubKeyManager::erase_subKey_by_bcAddr(string& bcIP, unsigned short bcManagerPort)
{
	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end();)
	{
		zmqSubscribeKey* key = *it;
		if(key&&(key->get_bc_ip()==bcIP)&&(key->get_bc_manager_port() ==bcManagerPort))
		{
		    //find key about the bc
			delete key;
			it = m_subKey_list.erase(it);
		}
		else
		{
			it++;
		}
	}		
}

//erase key about bidder address
void zmqSubKeyManager::erase_subKey_by_bidderAddr(string& bidderIP, unsigned short bidderManagerPort)
{
	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end();)
	{
		zmqSubscribeKey* key = *it;
		if(key&&(key->get_bidder_ip()==bidderIP)&&(key->get_bidder_manager_port() == bidderManagerPort))
		{
		    //find key about bidder
			delete key;
			it = m_subKey_list.erase(it);
		}
		else
		{
			it++;
		}
	}		
}

//erase one key
void zmqSubKeyManager:: erase_subKey(string& bidderIP, unsigned short bidderManagerPort, string& bcIP, unsigned short bcManagerPort)
{
    for(auto it = m_subKey_list.begin(); it != m_subKey_list.end();)
    {
       zmqSubscribeKey* key = *it;
       if(key&&(key->get_bidder_ip()==bidderIP)&&(key->get_bidder_manager_port() == bidderManagerPort)
        &&(key->get_bc_ip() == bcIP)&&(key->get_bc_manager_port() == bcManagerPort))
       {
          delete key;
          it = m_subKey_list.erase(it);
       }
       else
       {
          it++;
       }
    }       
}

zmqSubKeyManager::~zmqSubKeyManager()
{
    for(auto it = m_subKey_list.begin(); it != m_subKey_list.end();)
    {
        delete *it;
        it = m_subKey_list.erase(it);
    }
}


throttleObject::throttleObject(const string& throttle_ip, unsigned short throttle_port, unsigned short manager_port, unsigned short throttle_workerNum)
{
try
{
	m_throttle_ip = throttle_ip;
	m_throttle_pubPort = throttle_port;
	m_throttle_managerPort = manager_port;
	m_throttle_workerNum = throttle_workerNum;
    g_manager_logger->info("[add new throttle]:{0}, {1:d}", throttle_ip, manager_port);
}
catch(...)
{
    g_manager_logger->emerg("throttleObject structrue exception");
    exit(1);
}
}

//connect to throttle publish port, and establish async event
bool throttleObject::startSubThrottle(zeromqConnect& conntor, struct event_base* base,  event_callback_fn fn, void *arg)
{
    for(int i=0; i<m_throttle_workerNum; i++)
    {
        recvAdReq_t *recvAdReq = new recvAdReq_t();
        recvAdReq->m_throtlePubHandler = conntor.establishConnect(true, "tcp", ZMQ_SUB, 
										m_throttle_ip.c_str(), m_throttle_pubPort+i, &recvAdReq->m_throttle_pubFd);//client sub 
        recvAdReq->m_throttleEvent = event_new(base, recvAdReq->m_throttle_pubFd, EV_READ|EV_PERSIST, fn, arg); 
        event_add(recvAdReq->m_throttleEvent, NULL);
        m_recvAdReqVector.push_back(recvAdReq);
    }
    
    return true;
}

//connect to throttle manager port and establish async event
void throttleObject::connectToThrottle(zeromqConnect &connector, string& bidderID, struct event_base * base, event_callback_fn fn, void * arg)
{

	m_throtleManagerHandler = connector.establishConnect(true, true, bidderID, "tcp", ZMQ_DEALER, 
									m_throttle_ip.c_str(), m_throttle_managerPort, &m_throttleManagerFd);//client sub
    m_throttleManagerEvent = event_new(base, m_throttleManagerFd, EV_READ|EV_PERSIST, fn, arg); 
    event_add(m_throttleManagerEvent, NULL);	
}

void throttleObject::add_throttle_identify(string& throttleIP, unsigned short throttlePort, string& identify)	
{
}

void* throttleObject::get_manager_handler(int fd)
{
	if(fd == m_throttleManagerFd) return m_throtleManagerHandler;
	return NULL;
}

//regist key to throttle
void throttleObject::registerBCToThrottle(string& ip, unsigned short port)
{
    if(m_bidderLoginToThro==false)//have not login to throttle
    {
		managerProPackage::send(m_throtleManagerHandler, managerProtocol_messageTrans_BIDDER
            , managerProtocol_messageType_LOGIN_REQ, ip, port);
        g_manager_logger->info("[login req][bidder -> throttle]:{0},{1:d}", ip, port);		
        return;
    }

    for(auto it = m_subThroKey_list.begin(); it != m_subThroKey_list.end(); it++)
    {
       subThroKey* zmqSubKey = *it;
       if(zmqSubKey == NULL) continue;
       string &subKey =  zmqSubKey->get_subKey();
       if(subKey.empty()) continue;
       if(zmqSubKey->get_subscribed()==false)//have not subscribe
       {
            for(auto it=m_recvAdReqVector.begin(); it != m_recvAdReqVector.end(); it++)
            {
                recvAdReq_t *obj = *it;
                int rc =  zmq_setsockopt(obj->m_throtlePubHandler, ZMQ_SUBSCRIBE, subKey.c_str(), subKey.size());
               if(rc==0)
               {
                   zmqSubKey->set_subscribed();
                   g_manager_logger->info("[add sub thorttle]:{0}", subKey);    
               }    
            }
                            
       }

       if(zmqSubKey->get_regist_status() == false)// have not register to throttle
       {
           managerProPackage::send(m_throtleManagerHandler, managerProtocol_messageTrans_BIDDER
               , managerProtocol_messageType_REGISTER_REQ, subKey, ip, port);
           g_manager_logger->info("[register req][bidder -> throttle]:{0},{1:d}", ip, port);      
        }
    }
}


void throttleObject::subThrottle(string& bidderIP, unsigned short bidderPort, string &bcIP,
											unsigned short bcManagerPort, unsigned short bcDataPort)
{
	ostringstream os;
	os<<bcIP<<":"<<bcManagerPort<<":"<<bcDataPort<<"-"<<bidderIP<<":"<<bidderPort;
	string key = os.str();
	for(auto it = m_recvAdReqVector.begin(); it != m_recvAdReqVector.end(); it++)
	{
	    recvAdReq_t * obj = *it;
            int rc =  zmq_setsockopt(obj->m_throtlePubHandler, ZMQ_SUBSCRIBE, key.c_str(), key.size());
            if(rc==0)
            {
                g_manager_logger->info("[add sub thorttle]:{0}", key); 
            }
            else
            {
                g_manager_logger->emerg("add sub thorttle failure");
                exit(1);
            }
	}
	
}

bool throttleObject::set_publishKey_registed(const string& ip, unsigned short managerPort,const string& publishKey)
{
	if((m_throttle_ip != ip)||(m_throttle_managerPort != managerPort)) return false;
	for(auto it = m_subThroKey_list.begin(); it != m_subThroKey_list.end(); it++)
	{
		subThroKey *key = *it;
		if(key == NULL) continue;
		if(key->get_subKey()==publishKey) 
		{
			key->set_registed();
			return true;
		}
	}
	return false;
}

void throttleObject::set_publishKey_status(bool registed)
{
	for(auto it = m_subThroKey_list.begin(); it != m_subThroKey_list.end(); it++)
	{
		subThroKey *key = *it;
		if(key == NULL) continue;
        key->set_regist_status(registed);
	}    
}

void *throttleObject::get_handler(int fd)
{
    for(auto it = m_recvAdReqVector.begin(); it != m_recvAdReqVector.end(); it++)
    {
        recvAdReq_t *obj = *it;
        if(obj->m_throttle_pubFd == fd)
            return obj->m_throtlePubHandler;
    }
    return NULL;
	
}

bool throttleObject::set_subscribe_opt(string &key)
{
    for(auto it=m_recvAdReqVector.begin(); it != m_recvAdReqVector.end(); it++)
    {
        recvAdReq_t *obj = *it;

        int rc =  zmq_setsockopt(obj->m_throtlePubHandler, ZMQ_SUBSCRIBE, key.c_str(), key.size());
	if(rc!=0)
	{
		g_manager_logger->info("sucsribe {0} failure:: {1}", key, zmq_strerror(zmq_errno()));
		return false;
	}
	else
	{
		g_manager_logger->info("sucsribe {0} success", key);
	}	
    }
	
	return true;
}

//unsubscribe the key	
bool throttleObject::set_unsubscribe_opt(string &key)
{
     for(auto it=m_recvAdReqVector.begin(); it != m_recvAdReqVector.end(); it++)
     {
        recvAdReq_t *obj = *it;

        int rc =  zmq_setsockopt(obj->m_throtlePubHandler, ZMQ_UNSUBSCRIBE, key.c_str(), key.size());
	if(rc!=0)
	{
		g_manager_logger->info("unsucsribe {0} failure:: {1}", key, zmq_strerror(zmq_errno()));	
		return false;
	}
	else
	{
		g_manager_logger->info("unsucsribe {0} success", key);
	}	
     }
	
	return true;
}


bool throttleObject::drop()
{
    if(m_bidderLoginToThro)
    {
        m_lost_heart_times++;
        if(m_lost_heart_times > c_lost_times_max)
        {
            m_lost_heart_times = 0;
            m_bidderLoginToThro = false;
            set_publishKey_status(false);  
            g_manager_logger->warn("lost heart with throttle");
            return true;
        }
    }
    return false;
}

//erase key from key list
void throttleObject::erase_subKey(string &key)
{
    for(auto it = m_subThroKey_list.begin(); it != m_subThroKey_list.end();)
    {
        subThroKey* throKey =  *it;
        if(throKey)
        {
            if(throKey->get_subKey()==key)
            {
                it = m_subThroKey_list.erase(it);
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
}


throttleManager::throttleManager(vector<throttleConfig*>& thro_conf)
{
    m_throttleList_lock.init();
	init(thro_conf);
}
	
void throttleManager::init(vector<throttleConfig*>& thro_conf)
{
try
{
   	for(auto thro_it = thro_conf.begin(); thro_it != thro_conf.end(); thro_it++)
    {
       	throttleConfig *thro_config = *thro_it;
       	if(thro_config==NULL) continue;
       	throttleObject *throttle = new throttleObject(thro_config->get_throttleIP(), thro_config->get_throttlePubPort(),
											thro_config->get_throttleManagerPort(), thro_config->get_throttleWorkerNum());
        m_throttleList_lock.lock();
        m_throttle_list.push_back(throttle);
        m_throttleList_lock.unlock();
    }
}
catch(...)
{
    g_manager_logger->emerg("throttleManager init");
    exit(1);
}
}

void throttleManager::run(zeromqConnect& conntor, struct event_base * base, event_callback_fn fn, void * arg)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject* throttle =  *it;
		if(throttle) throttle->startSubThrottle(conntor, base, fn, arg);
	}
    m_throttleList_lock.unlock();
}

//connect to throttle
void throttleManager::connectToThrottle(zeromqConnect &connector, string& bidderID, struct event_base * base, 
												event_callback_fn fn, void * arg)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject* throttle =  *it;
		if(throttle) throttle->connectToThrottle(connector, bidderID, base, fn, arg);
	}
    m_throttleList_lock.unlock();
}

//add throttle to throttle list
void throttleManager::add_throttle(string &throttleIP, unsigned short throttlePort, unsigned short managerPort)
{
	bool throttle_exsit = false;
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject* throttle = *it;
		if(throttle==NULL) continue;
		if((throttleIP == throttle->get_throttle_ip())&&(throttlePort == throttle->get_throttle_port()))
		{
		    //find the throttle
			throttle_exsit = true;
			break;
		}
	}
	if(!throttle_exsit)
	{
		throttleObject *thro = new throttleObject(throttleIP, throttlePort, managerPort, 3);
		m_throttle_list.push_back(thro);
	}
    m_throttleList_lock.unlock();
}

//get throttle data handler with fd   
void* throttleManager::get_throttle_handler(int fd)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle = *it;
		if(throttle == NULL) continue;

              void *handler =  throttle->get_handler(fd);
              m_throttleList_lock.unlock();
              if(handler != NULL)
                return handler;
		#if 0
		if(fd == throttle->get_fd())
        {
            void *handler =  throttle->get_handler();
            m_throttleList_lock.unlock();
            return handler;
        }
        #endif
	}
    m_throttleList_lock.unlock();
	return NULL;
}

//get throttle manager handler with fd    
void* throttleManager::get_throttle_manager_handler(int fd)
{
  
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
	    throttleObject *throttle = *it;
		if(throttle == NULL) continue;
		void *handler = throttle->get_manager_handler(fd);
        m_throttleList_lock.unlock();
		if(handler) return handler;
	}
    m_throttleList_lock.unlock();
	return NULL;
}

void throttleManager::add_throttle_identify(string& throttleIP, unsigned short throttlePort, string& identify)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle = *it;
		if(throttle) throttle->add_throttle_identify(throttleIP, throttlePort, identify);
	}
    m_throttleList_lock.unlock();
}

//register key to throttle
void throttleManager::registerBCToThrottle(string& ip, unsigned short port)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle = *it;
		if(throttle) throttle->registerBCToThrottle(ip, port);
	}
    m_throttleList_lock.unlock();
}

//register key to throttle
void throttleManager::registerBCToThrottle(string &ip, unsigned short port,const string &throttle_ip, unsigned short throttle_mangerPort)
{
     m_throttleList_lock.lock();
     for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
     {
        throttleObject *throttle = *it;
        if(throttle)
        {
            if((throttle->get_throttle_ip() == throttle_ip)&&(throttle->get_throttle_managerPort() == throttle_mangerPort))
            {
                throttle->registerBCToThrottle(ip, port);
            }
        }
     }
     m_throttleList_lock.unlock();
}

//throttle init
void throttleManager::throttle_init(string &throttle_ip, unsigned short throttle_mangerPort)
{
     m_throttleList_lock.lock();
     for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
     {
        throttleObject *throttle = *it;
        if(throttle)
        {
            if((throttle->get_throttle_ip() == throttle_ip)&&(throttle->get_throttle_managerPort() == throttle_mangerPort))
            {
                throttle->throttle_init();
            }
        }
     }
     m_throttleList_lock.unlock();
}

//key register success
void throttleManager::set_publishKey_registed(const string& ip, unsigned short managerPort,const string& publishKey)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *thorttle = *it;
		if(thorttle == NULL) continue;
		if(thorttle->set_publishKey_registed(ip, managerPort,  publishKey)) break;
	}
    m_throttleList_lock.unlock();
}

//logined throttle success , so set the sym to logined
void throttleManager::logined(const string& throttleIP, unsigned short throttleManagerPort)	
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
    {
        throttleObject *thorttle = *it;
        if(thorttle&&(thorttle->get_throttle_ip() == throttleIP)&&(thorttle->get_throttle_managerPort() == throttleManagerPort))
        {
            //throttle logined success
            thorttle->set_bidderLoginToThro(true);
            break;
        }
    }
    m_throttleList_lock.unlock();
}

void *throttleManager::get_handler(int fd)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle = *it;
		if(throttle == NULL) continue;
	    void *handler = throttle->get_handler(fd);
        m_throttleList_lock.unlock();
		if(handler) return handler;
	}
    m_throttleList_lock.unlock();
	return NULL;
}

//erase publish key
 void throttleManager::erase_bidder_publishKey(vector<string>& unsubKeyList)
 {
     m_throttleList_lock.lock();
	 for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	 {
	 	throttleObject *throttle = *it;
		if(throttle)
		{	
			for(auto it = unsubKeyList.begin(); it != unsubKeyList.end(); it++)
			{
				string &key = *it;
				throttle->set_unsubscribe_opt(key);//unsubscribe key
                throttle->erase_subKey(key);//erase key
			}
		}
	 }
     m_throttleList_lock.unlock();
 }

//add throttle to throttle list and connect to throttle , and subscribe all key
bool throttleManager::add_throttle(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, event_callback_fn fn1,void * arg, string& bidderIdentify,
	const	string& throttleIP, unsigned short managerPort, unsigned short dataPort)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle = *it;
		if(throttle == NULL) continue;
		if((throttleIP == throttle->get_throttle_ip())&&(throttle->get_throttle_managerPort() == managerPort)
			&&(dataPort == throttle->get_throttle_port()))
		{
		    //throttle exist, so set all key status to unregister
		    throttle->set_publishKey_status(false);
		    m_throttleList_lock.unlock();
			return false;
		}
	}

    //new a throttle 
	throttleObject *throttle = new throttleObject(throttleIP, dataPort, managerPort, 3);
	throttle->connectToThrottle(connector, bidderIdentify, base, fn, arg);//connect to throttle
	throttle->startSubThrottle(connector,  base, fn1, arg);//subscribe to the throttle
	m_throttle_list.push_back(throttle);
    m_throttleList_lock.unlock();
	return true;
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

// update throttle list
bool throttleManager::update_throttle(vector<throttleConfig*>& throConfList)
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
                if((bConf->get_throttleIP() == throttle->get_throttle_ip())
                    &&(bConf->get_throttleManagerPort()== throttle->get_throttle_managerPort()))
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

//recv heart req from throttle
bool throttleManager::recvHeartReq(const string &ip, unsigned short port)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end();it++)
	{
		throttleObject *throttle = *it;
		if(throttle)
		{
		    if((throttle->get_throttle_ip()==ip)&&(throttle->get_throttle_managerPort()==port))
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
throttleManager::~throttleManager()
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end();)
	{
		delete *it;
		it = m_throttle_list.erase(it);
    }
    m_throttleList_lock.unlock();
    m_throttleList_lock.destroy();
}


