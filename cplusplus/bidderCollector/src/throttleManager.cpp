#include "throttleManager.h"
inline zmqSubscribeKey::zmqSubscribeKey()
{
}

zmqSubscribeKey::zmqSubscribeKey(const string& bidderIP, unsigned short bidderPort, string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort):m_registerToBidder(false)
{
    set(bidderIP, bidderPort, bcIP, bcManagerPort, bcDataPort);
}
inline void zmqSubscribeKey::set(const string& bidderIP, unsigned short bidderPort, string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort)
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

//key exsit
inline bool zmqSubscribeKey::subKey_equal(const string &bidderIP, unsigned short bidderPort
    , const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort)
{
    if( ((bidderIP == m_bidderIP)&&(bidderPort == m_bidderManagerPort))
        &&((bcIP == m_bcIP)&&(bcManagerPort == m_bcManagerPort)&&(bcDataPort == m_bcDataPort))
      )
	{
		return true;
	}
   
    return false;
}

inline string&        zmqSubscribeKey::get_bidder_ip()
{
    return m_bidderIP;
}

inline unsigned short zmqSubscribeKey::get_bidder_manager_port()
{
    return m_bidderManagerPort;
}

inline string&        zmqSubscribeKey::get_bc_ip()
{
    return m_bcIP;
}

inline unsigned short zmqSubscribeKey::get_bc_manager_port()
{
    return m_bcManagerPort;
}

inline unsigned short zmqSubscribeKey::get_bc_data_port()
{
    return m_bcDataPort;
}

inline string&        zmqSubscribeKey::get_subKey()
{
    return m_subKey;
}

inline bool           zmqSubscribeKey::get_regist_status()
{
    return m_registerToBidder;
}

zmqSubscribeKey::~zmqSubscribeKey()
{
    g_manager_logger->warn("erase subscribe key from subKey list:{0}", m_subKey);
}


zmqSubKeyManager::zmqSubKeyManager()
{
}

//add zmq subker to subkeylist
void zmqSubKeyManager::add_subKey(const string& bidder_ip, unsigned short bidder_port, string& bc_ip,
    unsigned short bcManagerPort, unsigned short bcDataPOort , string& subKey)
{
    try
    {
    	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end(); it++)
    	{
    		zmqSubscribeKey* subscribe_key =  *it;
    		if(subscribe_key == NULL) continue; 
            if(subscribe_key->subKey_equal(bidder_ip, bidder_port, bc_ip, bcManagerPort, bcDataPOort))
            {
                subKey = subscribe_key->get_subKey();
        		return;        
            }      
    	}
    	zmqSubscribeKey *key = new zmqSubscribeKey(bidder_ip, bidder_port, bc_ip, bcManagerPort, bcDataPOort);
    	m_subKey_list.push_back(key);	
        subKey = key->get_subKey();
    }
    catch(...)
    {
        cout<<"add sub key exception"<<endl;
    }
}

// erase key
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

//erase key about bc
void zmqSubKeyManager::erase_subKey_by_bcAddr(string& bcIP, unsigned short bcManagerPort)
{
	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end();)
	{
		zmqSubscribeKey* key = *it;
		if(key&&(key->get_bc_ip()==bcIP)&&(key->get_bc_manager_port() ==bcManagerPort))
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

//erase key about bidder or connector
void zmqSubKeyManager::erase_subKey_by_bidderAddr(string& bidderIP, unsigned short bidderManagerPort)
{
	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end();)
	{
		zmqSubscribeKey* key = *it;
		if(key&&(key->get_bidder_ip()==bidderIP)&&(key->get_bidder_manager_port() == bidderManagerPort))
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

//erase key
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
//////////////





inline throttleObject::throttleObject()
{
}

throttleObject::throttleObject(const string& throttleIP, unsigned short throttleManagerPort, unsigned short throttlePubPort):m_heart_times(0),m_loginThrottled(false)
{
try
{
    m_throttleIP = throttleIP;
    m_throttlePubPort = throttlePubPort;
    m_throttleManagerPort = throttleManagerPort;
    g_manager_logger->info("[add throttle]:{0}, {1:d}", throttleIP, throttleManagerPort);
}
catch(...)
{
    g_manager_logger->error("add throttle exception");
}
}


//subscribe key
bool throttleObject::set_subscribe_opt(string &key)
{
    int rc =  zmq_setsockopt(m_throtllePubHandler, ZMQ_SUBSCRIBE, key.c_str(), key.size());
    if(rc!=0)
    {
        g_manager_logger->error("sucsribe {0} failure {1}", key, zmq_strerror(zmq_errno()));
        return false;
    }
    else
    {
        g_manager_logger->info("subscribe {0} success", key);
    }   
    return true;
}

//unsubscribe key
bool throttleObject::set_unsubscribe_opt(string &key)
{
    int rc =  zmq_setsockopt(m_throtllePubHandler, ZMQ_UNSUBSCRIBE, key.c_str(), key.size());
    if(rc!=0)
    {
        g_manager_logger->error("unsucsribe {0} failure:{1}", key, zmq_strerror(zmq_errno()));
        return false;
    }
    else
    {
        g_manager_logger->info("unsubscribe {0} success", key);
    }   
    return true;
}


//connect to throttle and establish async event
void throttleObject::startConnectToThrottle(zeromqConnect& conntor,  string& bc_id, struct event_base* base,  event_callback_fn fn, void *arg)
{
    m_throttleManagerHandler = conntor.establishConnect(true, true, bc_id, "tcp", ZMQ_DEALER, m_throttleIP.c_str(),
                                            m_throttleManagerPort, &m_throttleManagerFd);//client sub 
    m_throttleManagerEvent = event_new(base, m_throttleManagerFd, EV_READ|EV_PERSIST, fn, arg); 
    event_add(m_throttleManagerEvent, NULL);
}


//connect to throttle and subscribe all key and establish recv throttle publish data event
inline bool throttleObject::startSubThrottle(vector<zmqSubscribeKey*>& subKeyList, zeromqConnect& conntor, 
    struct event_base* base,  event_callback_fn fn, void *arg)
{
    m_throtllePubHandler = conntor.establishConnect(true, "tcp", ZMQ_SUB, m_throttleIP.c_str(), m_throttlePubPort, &m_throttlepubFd);//client sub 
    g_manager_logger->info("subscribe throttle:{0},{1:d}", m_throttleIP, m_throttlePubPort);   

    for(auto it = subKeyList.begin(); it != subKeyList.end(); it++)
    {
        zmqSubscribeKey *subscribeKey = *it;
        if(subscribeKey==NULL) continue;
        string &key = subscribeKey->get_subKey();
        int rc =  zmq_setsockopt(m_throtllePubHandler, ZMQ_SUBSCRIBE, key.c_str(), key.size());
        if(rc!=0)
        {
            g_manager_logger->error("subscribe key {0} fialure:{1}", m_throttleIP, zmq_strerror(zmq_errno())); 
            return false;
        }
        else
        {
            g_manager_logger->info("subscribe key {0} success", key);   
        } 
    }

    m_throttlePubEvent = event_new(base, m_throttlepubFd, EV_READ|EV_PERSIST, fn, arg); 
    event_add(m_throttlePubEvent, NULL);
    return true;
}


//login to throttle
void throttleObject::loginThrottle(const string &ip, unsigned short managerPort, unsigned short dataPort)
{
    if(m_loginThrottled==false)//first login to throttle
    {
        managerProPackage::send(m_throttleManagerHandler, managerProtocol_messageTrans_BC
            ,managerProtocol_messageType_LOGIN_REQ, ip, managerPort, dataPort);
        g_manager_logger->info("[login req][bc -> throttle]:{0},{1:d},{2:d}", ip, managerPort, dataPort);
        return;
    }
}


void *throttleObject::get_throttleManagerHandler(int fd)
{
    if(fd == m_throttleManagerFd) return m_throttleManagerHandler;
    return NULL;
}

inline int throttleObject::get_fd()
{
    return m_throttlepubFd;
}

inline void *throttleObject::get_throttlePubHandler()
{
    return m_throtllePubHandler;
}

string& throttleObject::get_throttleIP()
{
    return m_throttleIP;
}

unsigned short throttleObject::get_throttlePubPort()
{
    return m_throttlePubPort;
}

throttleObject::~throttleObject()
{   
    g_manager_logger->info("[erase throttle]:{0},{1:d}", m_throttleIP, m_throttleManagerPort);
    if(m_throttlePubEvent) event_del(m_throttlePubEvent);
    if(m_throtllePubHandler) zmq_close(m_throtllePubHandler);
    if(m_throttleManagerEvent) event_del(m_throttleManagerEvent);
    if(m_throttleManagerHandler) zmq_close(m_throttleManagerHandler);
}

throttleManager::throttleManager():c_lost_times_max(3)
{
    m_throttleList_lock.init();
}

void throttleManager::init(vector<throttleConfig*>& throttleList)
{
    for(auto thro_it = throttleList.begin(); thro_it != throttleList.end(); thro_it++)
    {
        throttleConfig * thro_conf = *thro_it;
        if(thro_conf == NULL) continue;
        throttleObject *throttle = new throttleObject(thro_conf->get_throttleIP(), 
            thro_conf->get_throttleManagerPort(), thro_conf->get_throttlePubPort());
        m_throttleList_lock.lock();
        m_throttle_list.push_back(throttle);
        m_throttleList_lock.unlock();
    }
}

/*
  *start start subscribe throttle all keys
  */
void throttleManager::run(vector<zmqSubscribeKey*> subkeylist, zeromqConnect & conntor
                            , struct event_base * base, event_callback_fn fn, void * arg)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
    {
        throttleObject* throttle =  *it;
        if(throttle)    throttle->startSubThrottle(subkeylist, conntor, base, fn, arg);
    }
    m_throttleList_lock.unlock();
}

void* throttleManager::get_throttle_handler(int fd)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
    {
        throttleObject *throttle = *it;
        if(throttle&&(fd == throttle->get_fd()))
        {
            void *handler = throttle->get_throttlePubHandler();
            m_throttleList_lock.unlock();
            return handler;
        }
    }
    m_throttleList_lock.unlock();
    return NULL;
}


//connect to throttle
void throttleManager::startConnectToThrottle(zeromqConnect& conntor,  string& bc_id, struct event_base* base,  event_callback_fn fn, void *arg)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
    {
        throttleObject *throttle = *it;
        if(throttle == NULL) continue;
        throttle->startConnectToThrottle(conntor, bc_id, base, fn, arg);
    }
    m_throttleList_lock.unlock();
}

//add throttle device
void throttleManager::add_throttle(const string& throttleIP, unsigned short throttleManagerPort, unsigned short throttlePubPort,
    vector<zmqSubscribeKey*>& subKeyList ,zeromqConnect & conntor, struct event_base * base,event_callback_fn fn,void * arg)
{
    bool find = false;
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)   
    {
        throttleObject *throttle = *it;
        if(throttle == NULL ) continue;
        if((throttle->get_throttleIP()== throttleIP)&&(throttle->get_throttlePubPort() == throttlePubPort)
            &&(throttle->get_throttleManagerPort() == throttleManagerPort))
        {
            throttle->set_loginThrottle(false);
            throttle->heart_times_clear();
            m_throttleList_lock.unlock();
            return;
        }
    }   
    
    if(!find)
    {
        throttleObject *throttle = new throttleObject(throttleIP, throttleManagerPort, throttlePubPort);
        m_throttle_list.push_back(throttle);
        throttle->startSubThrottle(subKeyList, conntor, base, fn, arg);
    }
    m_throttleList_lock.unlock();
}

void throttleManager::set_subscribe_opt(string &key)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
    {
        throttleObject *throttle = *it;
        if(throttle) throttle->set_subscribe_opt(key);
    }
    m_throttleList_lock.unlock();
}

void throttleManager::loginThrottle(const string &ip, unsigned short managerPort, unsigned short dataPort)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
    {
        throttleObject *throttle = *it;
        if(throttle)
        throttle->loginThrottle(ip, managerPort, dataPort);
    }
    m_throttleList_lock.unlock();
}

void throttleManager::loginThrottleSuccess(const string &ip, unsigned short managerPort)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
    {
        throttleObject *throttle = *it;
        if(throttle == NULL) continue;
        if((throttle->get_throttleIP() == ip)&&(throttle->get_throttleManagerPort() == managerPort))
        {
            throttle->set_loginThrottle(true);
        }
    }
    m_throttleList_lock.unlock();

}

void *throttleManager::get_throttleManagerHandler(int fd)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
    {
        throttleObject *throttle = *it;
        if(throttle == NULL) continue;
        void *handler = throttle->get_throttleManagerHandler(fd);
        m_throttleList_lock.unlock();
        if(handler) return handler;
    }
    m_throttleList_lock.unlock();
    return NULL;
}
bool throttleManager::recv_heart_from_throttle(const string& ip, unsigned short port)
{
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)   
    {
        throttleObject *throttle = *it;
        if(throttle == NULL ) continue;
        if((throttle->get_throttleIP() == ip)&&(throttle->get_throttleManagerPort()== port))
        {
            throttle->heart_times_clear();
            m_throttleList_lock.unlock();  
            return true;
        }
    }       
    m_throttleList_lock.unlock();
    return false;
}
bool throttleManager::recv_throttle_heart_time_increase()
{      
    bool ret = true;
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)   
    {
        throttleObject *throttle = *it;
        if(throttle&&throttle->get_loginThrottle())
        {
            throttle->heart_times_increase();
            if(throttle->get_heart_times()>c_lost_times_max)
            {
                throttle->set_loginThrottle(false);
                ret = false;
            }
        }
    }
    m_throttleList_lock.unlock();
    return ret;
}

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
                if((bConf->get_throttleIP() == throttle->get_throttleIP())
                    &&(bConf->get_throttleManagerPort()== throttle->get_throttleManagerPort()))
                    break;
            }       

            if(itor == throConfList.end())
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


void throttleManager::unSubscribe(vector<string> unsubList)
{       
    m_throttleList_lock.lock();
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)   
    {
        throttleObject *throttle = *it;
        if(throttle)
        {
            for(auto etor = unsubList.begin(); etor != unsubList.end(); etor++)
            {
                string &key = *etor;
                throttle->set_unsubscribe_opt(key);             
            }
        }
    }
    m_throttleList_lock.unlock();
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

