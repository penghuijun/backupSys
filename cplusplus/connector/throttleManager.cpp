#include "throttleManager.h"
using namespace com::rj::protos::manager;

zmqSubscribeKey::zmqSubscribeKey(const string& bidderIP, unsigned short bidderPort, const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort)
{
	set(bidderIP, bidderPort, bcIP, bcManagerPort, bcDataPort);
}
void zmqSubscribeKey::set(const string& bidderIP, unsigned short bidderPort,const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort)
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

string& zmqSubKeyManager::add_subKey(const string& bidder_ip, unsigned short bidder_port,const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPOort)
{
	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end(); it++)
	{
		zmqSubscribeKey* subscribe_key =  *it;
		if(subscribe_key == NULL) continue;
		if(((bidder_ip==subscribe_key->get_bidder_ip())&&(bidder_port==subscribe_key->get_bidder_manager_port()))&&
			((bc_ip==subscribe_key->get_bc_ip())&&(bcManagerPort ==subscribe_key->get_bc_manager_port()))
			&&(bcDataPOort == subscribe_key->get_bc_data_port()))
		{
			return subscribe_key->get_subKey();
		}
	}
	zmqSubscribeKey *key = new zmqSubscribeKey(bidder_ip, bidder_port, bc_ip, bcManagerPort, bcDataPOort);
	m_subKey_list.push_back(key);	
	return key->get_subKey();
}
	
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


throttleObject::throttleObject(const string& throttle_ip, unsigned short throttle_port, unsigned short manager_port)
{
try
{
	m_throttle_ip = throttle_ip;
	m_throttle_pubPort = throttle_port;
	m_throttle_managerPort = manager_port;
    g_manager_logger->info("[add new throttle]:{0}, {1:d}", throttle_ip, manager_port);
}
catch(...)
{
    g_manager_logger->emerg("throttleObject structrue exception");
    exit(1);
}
}

bool throttleObject::startSubThrottle(zeromqConnect& conntor, struct event_base* base,  event_callback_fn fn, void *arg)
{
	m_throtlePubHandler = conntor.establishConnect(true, "tcp", ZMQ_SUB, 
										m_throttle_ip.c_str(), m_throttle_pubPort, &m_throttle_pubFd);//client sub 
    m_throttleEvent = event_new(base, m_throttle_pubFd, EV_READ|EV_PERSIST, fn, arg); 
	event_add(m_throttleEvent, NULL);
	return true;
}

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
	
void throttleObject::registerBCToThrottle(const string &devIP, unsigned short devPort)
{
    if(m_bidderLoginToThro==false)//first login to throttle
    {
		managerProPackage::send(m_throtleManagerHandler, managerProtocol_messageTrans_CONNECTOR, managerProtocol_messageType_LOGIN_REQ
                ,devIP, devPort);
        g_manager_logger->info("[login req][connector -> throttle]:{0},{1:d}", m_throttle_ip, m_throttle_managerPort);	
        return;
    }

    for(auto it = m_subThroKey_list.begin(); it != m_subThroKey_list.end(); it++)
    {
       subThroKey* zmqSubKey = *it;
       if(zmqSubKey == NULL) continue;
       const string &subKey =  zmqSubKey->get_subKey();
       if(subKey.empty()) continue;
       if(zmqSubKey->get_subscribed()==false)
       {
           int rc =  zmq_setsockopt(m_throtlePubHandler, ZMQ_SUBSCRIBE, subKey.c_str(), subKey.size());
           if(rc==0)
           {
               zmqSubKey->set_subscribed();
               g_manager_logger->info("[add sub thorttle]:{0}", subKey);    
           }                     
       }

       if(zmqSubKey->get_regist_status() == false)
       {
           managerProPackage::send(m_throtleManagerHandler, managerProtocol_messageTrans_CONNECTOR,
            managerProtocol_messageType_REGISTER_REQ, subKey, devIP, devPort);
           g_manager_logger->info("[register req][connector -> throttle>:{0},{1:d},{2}", m_throttle_ip, m_throttle_managerPort, subKey); 
        }
    }
}


void throttleObject::subThrottle(string& bidderIP, unsigned short bidderPort, string &bcIP,
											unsigned short bcManagerPort, unsigned short bcDataPort)
{
	ostringstream os;
	os<<bcIP<<":"<<bcManagerPort<<":"<<bcDataPort<<"-"<<bidderIP<<":"<<bidderPort;
	string key = os.str();
	int rc =  zmq_setsockopt(m_throtlePubHandler, ZMQ_SUBSCRIBE, key.c_str(), key.size());
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

bool throttleObject::set_publishKey_registed(const string& ip, unsigned short port,const string& publishKey)
{
	if((m_throttle_ip != ip)||(m_throttle_managerPort != port)) return false;
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
	if(fd == m_throttleManagerFd) return m_throtleManagerHandler;
	return NULL;
}

bool throttleObject::set_subscribe_opt(string &key)
{
	int rc =  zmq_setsockopt(m_throtlePubHandler, ZMQ_SUBSCRIBE, key.c_str(), key.size());
	if(rc!=0)
	{
		g_manager_logger->info("sucsribe {0} failure:: {1}", key, zmq_strerror(zmq_errno()));
		return false;
	}
	else
	{
		g_manager_logger->info("sucsribe {0} success", key);
	}	
	return true;
}
	
bool throttleObject::set_unsubscribe_opt(string &key)
{
	int rc =  zmq_setsockopt(m_throtlePubHandler, ZMQ_UNSUBSCRIBE, key.c_str(), key.size());
	if(rc!=0)
	{
		g_manager_logger->info("unsucsribe {0} failure:: {1}", key, zmq_strerror(zmq_errno()));	
		return false;
	}
	else
	{
		g_manager_logger->info("unsucsribe {0} success", key);
	}	
	return true;
}

bool throttleObject::devDrop()
{
    m_lost_heart_times++;;
    if(m_lost_heart_times > c_lost_times_max)
    {
        m_lost_heart_times = 0;
        m_bidderLoginToThro = false;
        set_publishKey_status(false); 
        g_manager_logger->warn("lost heart with throttle");
        return true;
	}    
    return false;
}
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
	
void throttleManager::init(const vector<throttleConfig*>& thro_conf)
{
try
{
   	for(auto thro_it = thro_conf.begin(); thro_it != thro_conf.end(); thro_it++)
    {
       	throttleConfig *thro_config = *thro_it;
       	if(thro_config==NULL) continue;
       	throttleObject *throttle = new throttleObject(thro_config->get_throttleIP(), thro_config->get_throttlePubPort(),
											thro_config->get_throttleManagerPort());
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
			throttle_exsit = true;
			break;
		}
	}
	if(!throttle_exsit)
	{
		throttleObject *thro = new throttleObject(throttleIP, throttlePort, managerPort);
		m_throttle_list.push_back(thro);
	}
    m_throttleList_lock.unlock();
}


void* throttleManager::get_throttle_handler(int fd)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle = *it;
		if(throttle == NULL) continue;
		if(fd == throttle->get_fd())
        {
            void *handler =  throttle->get_handler();
            m_throttleList_lock.unlock();
            return handler;
        }
	}
    m_throttleList_lock.unlock();
	return NULL;
}
	
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

void throttleManager::registerBCToThrottle(const string &devIP, unsigned short devPort)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle = *it;
		if(throttle) throttle->registerBCToThrottle(devIP, devPort);
	}
    m_throttleList_lock.unlock();
}

void throttleManager::registerBCToThrottle(const string &devIP, unsigned short devPort,const string &throttle_ip, unsigned short throttle_mangerPort)
{
     m_throttleList_lock.lock();
     for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
     {
        throttleObject *throttle = *it;
        if(throttle)
        {
            if((throttle->get_throttle_ip() == throttle_ip)&&(throttle->get_throttle_managerPort() == throttle_mangerPort))
            {
                throttle->registerBCToThrottle(devIP, devPort);
            }
        }
     }
     m_throttleList_lock.unlock();
}


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


void throttleManager::set_publishKey_registed(const string& ip, unsigned short port,const string& publishKey)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *thorttle = *it;
		if(thorttle == NULL) continue;
		if(thorttle->set_publishKey_registed(ip, port,  publishKey)) break;
	}
    m_throttleList_lock.unlock();
}

void throttleManager::loginToThroSucess(const string& throttleIP, unsigned short throttleManagerPort)	
{
    m_throttleList_lock.lock();
            cout<<throttleIP<<"=-="<<throttleManagerPort<<endl;
    for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
    {
        throttleObject *thorttle = *it;
        cout<<thorttle->get_throttle_ip()<<"=="<<thorttle->get_throttle_managerPort()<<endl;
        if(thorttle&&(thorttle->get_throttle_ip() == throttleIP)&&(thorttle->get_throttle_managerPort() == throttleManagerPort))
        {
            cout<<"logined" <<endl;
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
				throttle->set_unsubscribe_opt(key);
                throttle->erase_subKey(key);
			}
		}
	 }
     m_throttleList_lock.unlock();
 }

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
		    throttle->set_publishKey_status(false);
		    m_throttleList_lock.unlock();
			return false;
		}
	}

	throttleObject *throttle = new throttleObject(throttleIP, dataPort, managerPort);
	throttle->connectToThrottle(connector, bidderIdentify, base, fn, arg);
	throttle->startSubThrottle(connector,  base, fn1, arg);
	m_throttle_list.push_back(throttle);
    m_throttleList_lock.unlock();
	return true;
}	


bool throttleManager::devDrop()
{
    bool ret = false;
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
	{
		throttleObject *throttle = *it;
		if(throttle == NULL) continue;
		if(throttle->get_bidderLoginToThro() == false) continue;
        if(throttle->devDrop() == true) ret = true;
	}
    m_throttleList_lock.unlock();
    return ret;
}

bool throttleManager::update_throttle(const vector<throttleConfig*>& throConfList)
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


bool throttleManager::recv_heart_from_throttle(const string &ip, unsigned short port)
{
    m_throttleList_lock.lock();
	for(auto it = m_throttle_list.begin(); it != m_throttle_list.end();it++)
	{
		throttleObject *throttle = *it;
		if(throttle)
		{
		    if((throttle->get_throttle_ip()==ip)&&(throttle->get_throttle_managerPort()==port))
			{
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


