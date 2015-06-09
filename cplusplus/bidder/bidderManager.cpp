#include "bidderManager.h"

//init 
void bidderManager::init(zeromqConnect &connector, throttleInformation& throttle_info, bidderInformation& bidder_info, bcInformation& bc_info)
{
    determine_bidder_address(connector, bidder_info, bc_info.get_bcConfigList());
    m_throttle_manager.init(throttle_info.get_throttleConfigList());
    m_bc_manager.init(m_bidderConfig, bc_info.get_bcConfigList());
    ostringstream os;
    os<<m_bidderConfig.get_bidderIP()<<":"<<m_bidderConfig.get_bidderLoginPort();
    m_bidderIdentify = os.str();
}

//determine bidder address
void bidderManager::determine_bidder_address(zeromqConnect &connector, bidderInformation& bidder_info, vector<bcConfig*>& bcList)
{
	int hwm = 30000;
	vector<bidderConfig*>& bidderList = bidder_info.get_bidderConfigList();
		
   	vector<string> ipAddrList;
   	ipAddress::get_local_ipaddr(ipAddrList);
    
   	bool estatblish=false;
   	for(auto it = ipAddrList.begin(); it != ipAddrList.end(); it++)
   	{
       	string &str = *it;
       	for(auto et = bidderList.begin(); et != bidderList.end(); et++)
       	{
           	bidderConfig* bidder = *et;
			if(bidder == NULL) continue;
            string& ip_str = bidder->get_bidderIP();
            if(ip_str.compare(0,ip_str.size(), str)==0)
            {
                g_manager_logger->info("local addr:{0}", ip_str);
        	    m_bidderLoginHandler = connector.establishConnect(false, "tcp", ZMQ_ROUTER, ip_str.c_str(),
                bidder->get_bidderLoginPort(), &m_bidderLoginFd);	
                zmq_setsockopt(m_bidderLoginHandler, ZMQ_RCVHWM, &hwm, sizeof(hwm));
        	    if(m_bidderLoginHandler == nullptr)
        		{
                    continue;
        		}    
				m_bidderConfig.set(bidder);
	            m_redisIP = bidder_info.get_redis_ip();
	            m_redisPort = bidder_info.get_redis_port();
	            estatblish = true;
                break;
            } 
        }
        if(estatblish) break;
    } 
		
    if(!estatblish)
    {
        g_manager_logger->emerg("bidder zmq server bind failure, may you should check confige file");
        exit(1);
    }
    
    g_manager_logger->info("{0:d}, {1}, {2:d}, {3:d}, {4:d} bind to this port success", m_bidderConfig.get_bidderID()
        , m_bidderConfig.get_bidderIP(), m_bidderConfig.get_bidderLoginPort(), m_bidderConfig.get_bidderWorkerNum()
        , m_bidderConfig.get_bidderThreadPoolSize());
}

void bidderManager::login_event_new(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg)
{	
	m_bidderLoginEvent = event_new(base, m_bidderLoginFd, EV_READ|EV_PERSIST, fn, arg); 
	event_add(m_bidderLoginEvent, NULL); 
}

void bidderManager::subscribe_throttle_event_new(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg)
{
	m_throttle_manager.run(connector, base, fn, arg);
}

void bidderManager::registerBCToThrottle()
{
    m_throttle_manager.registerBCToThrottle(m_bidderConfig.get_bidderIP(), m_bidderConfig.get_bidderLoginPort());
}

void bidderManager::registerBCToThrottle(string &throttle_ip, unsigned short throttle_mangerPort)
{
    m_throttle_manager.registerBCToThrottle(m_bidderConfig.get_bidderIP(), m_bidderConfig.get_bidderLoginPort(), throttle_ip, throttle_mangerPort);
}

void bidderManager::dataConnectToBC(zeromqConnect & conntor)
{
	m_bc_manager.startDataConnectToBC(conntor);
}	


void bidderManager::connectAll(zeromqConnect &connector, struct event_base * base, 
												event_callback_fn fn, void * arg)
{
	m_bc_manager.BCManagerConnectToBC(connector, m_bidderIdentify, base, fn, arg);
	m_throttle_manager.connectToThrottle(connector, m_bidderIdentify, base, fn, arg);
}

void *bidderManager::get_throttle_handler(int fd)
{
	m_throttle_manager.get_throttle_handler(fd);
}

void *bidderManager::get_throttle_manager_handler(int fd)
{
	void *handler = m_throttle_manager.get_throttle_manager_handler(fd);
    if(handler) return handler;
    handler = m_bc_manager.get_bcManager_handler(fd);
    return handler;
}
	
void bidderManager::add_throttle_identify(string& throttleIP, unsigned short throttlePort, string& identify)
{
	m_throttle_manager.add_throttle_identify(throttleIP, throttlePort, identify);
	m_bc_manager.add_throttle_identify(identify);
}

//add bc to bc list and add key to key list
bool bidderManager::add_bc(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg,
				const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPort)
{
	bool ret = m_bc_manager.add_bc(connector, m_bidderIdentify, base, fn, arg, m_bidderConfig.get_bidderIP(), m_bidderConfig.get_bidderLoginPort()
							, bc_ip, bcManagerPort, bcDataPort);
    add_subKey(bc_ip, bcManagerPort, bcDataPort);
	return ret;
}

//add throttle to throttle list , add key to key list which in throttle object
bool bidderManager::add_throttle(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, event_callback_fn fn1,void * arg,
				const string& ip, unsigned short managerPort, unsigned short dataPort)
{
	bool ret = m_throttle_manager.add_throttle(connector, base, fn, fn1, arg, m_bidderIdentify,
		ip, managerPort, dataPort);
    m_subKey_lock.read_lock();
    auto subList = m_subKey_manager.get_subKey_list();
    for(auto it  = subList.begin(); it != subList.end(); it++)
    {
        zmqSubscribeKey *subKey = *it;
        if(subKey) m_throttle_manager.add_subKey(subKey->get_subKey());
    }
    m_subKey_lock.read_write_unlock();
    m_throttle_manager.registerBCToThrottle(m_bidderConfig.get_bidderIP(), m_bidderConfig.get_bidderLoginPort(), ip, managerPort);
	return ret;
}

//update bclist
void bidderManager::update_bcList(zeromqConnect &connector, bcInformation& bcInfo)
{
	auto bcList = bcInfo.get_bcConfigList();
	for(auto it = bcList.begin(); it != bcList.end(); it++)
	{
		bcConfig* bc_conf = *it;
		if(bc_conf)
		{
			m_bc_manager.add_bc(connector, m_bidderConfig.get_bidderIP(), m_bidderConfig.get_bidderLoginPort(),
					bc_conf->get_bcIP(), bc_conf->get_bcMangerPort(), bc_conf->get_bcDataPort());
		}
	}
}

//send login or heart to bc,
void bidderManager::loginOrHeartReqToBc(configureObject& config)
{
	 vector<string> unsubKeyList;
     const string bIP = m_bidderConfig.get_bidderIP();
	 if(m_bc_manager.loginOrHeartReqToBc(bIP, m_bidderConfig.get_bidderLoginPort(), unsubKeyList) ==false)
	 {
	     for(auto it = unsubKeyList.begin(); it != unsubKeyList.end(); it++)
         {
            string& key =  *it;
            m_subKey_manager.erase_subKey(key);
         }   
	 	 m_throttle_manager.erase_bidder_publishKey(unsubKeyList);
         config.readConfig();
         m_bc_manager.update_bc(config.get_bc_info().get_bcConfigList());
     }
}
bool bidderManager::dropDev()
{
	m_throttle_manager.dropDev();
}
	
void bidderManager::set_publishKey_registed(const string& ip, unsigned short managerPort,const string& publishKey)
{
	m_throttle_manager.set_publishKey_registed(ip, managerPort, publishKey);
}

void bidderManager::loginedThrottle(const string& throttleIP, unsigned short throttleManagerPort)
{
	m_throttle_manager.logined(throttleIP, throttleManagerPort);
}

bool bidderManager::update_throttle(throttleInformation& throttle_info)
{
    m_throttle_manager.update_throttle(throttle_info.get_throttleConfigList());
    return true;
}


void *bidderManager::get_bidderLoginHandler(){return m_bidderLoginHandler;}

int bidderManager::sendAdRspToBC(string& bcIP, unsigned short bcDataPort, void* data, size_t dataSize, int flags)
{
	return m_bc_manager.sendAdRspToBC(bcIP, bcDataPort, data, dataSize, flags);
}
void *bidderManager::get_bcManager_handler(int fd)
{
	return m_bc_manager.get_bcManager_handler(fd);
}
string& bidderManager::get_redis_ip() {return m_redisIP;}
unsigned short bidderManager::get_redis_port() {return m_redisPort;}
bidderConfig& bidderManager::get_bidder_config(){return m_bidderConfig;}
string& bidderManager::get_bidder_identify(){return m_bidderIdentify;}


//recv heart response from bc, 
void bidderManager::recv_BCHeartBeatRsp(const string& bcIP, unsigned short bcPort)
{
    bool ret = m_bc_manager.recv_heartBeatRsp(bcIP, bcPort);

    if(ret)//the bc exsit
    {
        bool exist = false;
        m_subKey_lock.read_lock();
        exist = m_subKey_manager.keyExist(bcIP, bcPort);
        m_subKey_lock.read_write_unlock();
        if(!exist)//publish key not exist, so relogin the device
        {
            m_bc_manager.relogin(bcIP, bcPort);
        }        
    }	
}

void bidderManager::loginedBC(const string& bcIP, unsigned short bcPort)
{
	m_bc_manager.loginSucess(bcIP, bcPort);
}
bool bidderManager::recv_throttleHeartReq(const string &ip, unsigned short port)
{
	return m_throttle_manager.recvHeartReq(ip, port);
}

//add key to key list , and add key to key list which in throttle object
bool bidderManager::add_subKey(const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPOort)
{
    m_subKey_lock.write_lock();
    string &key = m_subKey_manager.add_subKey(m_bidderConfig.get_bidderIP(), m_bidderConfig.get_bidderLoginPort(),
                                                bc_ip, bcManagerPort, bcDataPOort);
    m_subKey_lock.read_write_unlock();
    if(key.empty()==false)
    {
        m_throttle_manager.add_subKey(key);
    }
}

//update 
bool bidderManager::update(bidderInformation &new_bidder_info)
{
    auto old_bidderip = m_bidderConfig.get_bidderIP();//
    auto old_bidderPort = m_bidderConfig.get_bidderLoginPort();
    auto old_bidderWorkerNum = m_bidderConfig.get_bidderWorkerNum();
    auto new_bidderList = new_bidder_info.get_bidderConfigList();

    for(auto it = new_bidderList.begin(); it != new_bidderList.end(); it++)
    {
        bidderConfig *bidder = *it;
        if(bidder == NULL) continue;  
        if((bidder->get_bidderIP() == old_bidderip)&&(bidder->get_bidderLoginPort() == old_bidderPort))
        {
             if(bidder->get_bidderWorkerNum()!= old_bidderWorkerNum)
             {
                 m_bidderConfig.set_bidderWorkerNum(bidder->get_bidderWorkerNum());
                 return true;
             }
        }
        else
        {
             g_manager_logger->error("Not implemented bidder address update");
        }
    }
    return false;
}



bidderManager::~bidderManager()
{
	event_del(m_bidderLoginEvent);
	zmq_close(m_bidderLoginEvent);
    m_subKey_lock.destroy();
}

