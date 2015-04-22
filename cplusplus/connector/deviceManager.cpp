#include "deviceManager.h"
void deviceManager::init(zeromqConnect &connector,const throttleInformation& throttle_info
                        ,const connectorInformation& connector_info,const bcInformation& bc_info)
{
    determine_local_address(connector, connector_info, bc_info.get_bcConfigList());
    m_throttle_manager.init(throttle_info.get_throttleConfigList());
    m_bc_manager.init(m_connectorConfig, bc_info.get_bcConfigList());
    ostringstream os;
    os<<m_connectorConfig.get_connectorIP()<<":"<<m_connectorConfig.get_connectorManagerPort();  
    m_bidderIdentify = os.str();
}

void deviceManager::determine_local_address(zeromqConnect &zmq_handler,const connectorInformation& connector_info, const vector<bcConfig*>& bcList)
{
	int hwm = 30000;
	const vector<connectorConfig*>& connectorList = connector_info.get_connectorConfigList();
		
   	vector<string> ipAddrList;
   	ipAddress::get_local_ipaddr(ipAddrList);
   	bool estatblish=false;
   	for(auto it = ipAddrList.begin(); it != ipAddrList.end(); it++)
   	{
       	string &str = *it;
       	for(auto et = connectorList.begin(); et != connectorList.end(); et++)
       	{
           	connectorConfig* cc = *et;
			if(cc == NULL) continue;
            const string& ip_str = cc->get_connectorIP();
            if(ip_str.compare(0,ip_str.size(), str)==0)
            {
                g_manager_logger->info("local addr:{0}", ip_str);
        	    m_bidderLoginHandler = zmq_handler.establishConnect(false, "tcp", ZMQ_ROUTER, ip_str.c_str(),
                cc->get_connectorManagerPort(), &m_bidderLoginFd);	
                zmq_setsockopt(m_bidderLoginHandler, ZMQ_RCVHWM, &hwm, sizeof(hwm));
        	    if(m_bidderLoginHandler == nullptr)
        		{
                    continue;
        		}    
                
                m_connectorConfig.set(cc);
	            m_redisIP = connector_info.get_redis_ip();
	            m_redisPort = connector_info.get_redis_port();
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
    
    g_manager_logger->info("{0:d}, {1}, {2:d}, {3:d}, {4:d} bind to this port success", m_connectorConfig.get_connectorID()
        , m_connectorConfig.get_connectorIP(), m_connectorConfig.get_connectorManagerPort(), m_connectorConfig.get_connectorWorkerNum()
        , m_connectorConfig.get_connectorThreadPoolSize());
}

void deviceManager::login_event_new(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg)
{	
	m_bidderLoginEvent = event_new(base, m_bidderLoginFd, EV_READ|EV_PERSIST, fn, arg); 
	event_add(m_bidderLoginEvent, NULL); 
}

void deviceManager::subscribe_throttle_event_new(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg)
{
	m_throttle_manager.run(connector, base, fn, arg);
}

void deviceManager::registerBCToThrottle()
{
    m_throttle_manager.registerBCToThrottle(m_connectorConfig.get_connectorIP(), m_connectorConfig.get_connectorManagerPort());
}

void deviceManager::registerBCToThrottle(string &throttle_ip, unsigned short throttle_mangerPort)
{
    m_throttle_manager.registerBCToThrottle(m_connectorConfig.get_connectorIP(), m_connectorConfig.get_connectorManagerPort()
        , throttle_ip, throttle_mangerPort);
}
void deviceManager::throttle_init(const string &throttle_ip, unsigned short throttle_mangerPort)
{
  //  m_throttle_manager.throttle_init(throttle_ip, throttle_mangerPort);
    m_throttle_manager.registerBCToThrottle(m_connectorConfig.get_connectorIP(), m_connectorConfig.get_connectorManagerPort()
    , throttle_ip, throttle_mangerPort);
}

void deviceManager::dataConnectToBC(zeromqConnect & conntor)
{
	m_bc_manager.startDataConnectToBC(conntor);
}	

void deviceManager::connectAll(zeromqConnect & conntor, struct event_base * base,event_callback_fn fn,void * arg)
{
	m_bc_manager.BCManagerConnectToBC(conntor, m_bidderIdentify, base, fn, arg);
    m_throttle_manager.connectToThrottle(conntor, m_bidderIdentify, base, fn, arg);
}	


void *deviceManager::get_throttle_handler(int fd)
{
	m_throttle_manager.get_throttle_handler(fd);
}

void *deviceManager::get_throttle_manager_handler(int fd)
{
	m_throttle_manager.get_throttle_manager_handler(fd);		
}
	
void deviceManager::add_throttle_identify(string& throttleIP, unsigned short throttlePort, string& identify)
{
	m_throttle_manager.add_throttle_identify(throttleIP, throttlePort, identify);
	m_bc_manager.add_throttle_identify(identify);
}


bool deviceManager::add_bc(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg,
				 const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPort)
{
	bool ret = m_bc_manager.add_bc(connector, m_bidderIdentify, base, fn, arg, m_connectorConfig.get_connectorIP(), m_connectorConfig.get_connectorManagerPort()
							, bc_ip, bcManagerPort, bcDataPort);
    add_subKey(bc_ip, bcManagerPort, bcDataPort);
	return ret;
}
bool deviceManager::add_throttle(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, event_callback_fn fn1,void * arg,
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
	return ret;
}

void deviceManager::update_bcList(zeromqConnect &connector,const bcInformation& bcInfo)
{
	auto bcList = bcInfo.get_bcConfigList();
	for(auto it = bcList.begin(); it != bcList.end(); it++)
	{
		bcConfig* bc_conf = *it;
		if(bc_conf)
		{
			m_bc_manager.add_bc(connector, m_connectorConfig.get_connectorIP(), m_connectorConfig.get_connectorManagerPort(),
					bc_conf->get_bcIP(), bc_conf->get_bcMangerPort(), bc_conf->get_bcDataPort());
		}
	}
}


void deviceManager::loginOrHeartReqToBc(configureObject& config)
{
	 vector<string> unsubKeyList;
     const string &bidderIP = m_connectorConfig.get_connectorIP();
     unsigned short bidderPort = m_connectorConfig.get_connectorManagerPort();
	 if(m_bc_manager.loginOrHeartReqToBc(bidderIP, bidderPort, unsubKeyList) ==false)
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
bool deviceManager::devDrop()
{
	m_throttle_manager.devDrop();
}
	
void deviceManager::set_publishKey_registed(const string& ip, unsigned short port,const string& publishKey)
{
	m_throttle_manager.set_publishKey_registed(ip, port, publishKey);
}

void deviceManager::loginToThroSucess(const string& throttleIP, unsigned short throttleManagerPort)
{
	m_throttle_manager.loginToThroSucess(throttleIP, throttleManagerPort);
}

bool deviceManager::update_throttle(const throttleInformation& throttle_info)
{
    m_throttle_manager.update_throttle(throttle_info.get_throttleConfigList());
    return true;
}


void *deviceManager::get_bidderLoginHandler(){return m_bidderLoginHandler;}

int deviceManager::sendAdRspToBC(string& bcIP, unsigned short bcDataPort, void* data, size_t dataSize, int flags)
{
	return m_bc_manager.sendAdRspToBC(bcIP, bcDataPort, data, dataSize, flags);
}
void *deviceManager::get_bcManager_handler(int fd)
{
	void* handler = m_bc_manager.get_bcManager_handler(fd);
    if(handler) return handler;
    handler = m_throttle_manager.get_handler(fd);
    return handler;
}
string& deviceManager::get_redis_ip(){return m_redisIP;}
unsigned short deviceManager::get_redis_port(){return m_redisPort;}
const connectorConfig& deviceManager::get_connector_config(){return m_connectorConfig;}
string& deviceManager::get_bidder_identify(){return m_bidderIdentify;}

void deviceManager::lostheartTimesWithBC_clear(const string& bcIP, unsigned short bcPort)
{
	m_bc_manager.lostheartTimesWithBC_clear(bcIP, bcPort);
}

void deviceManager::bidderLoginedBcSucess(const string& bcIP, unsigned short bcPort)
{
	m_bc_manager.bidderLoginedBcSucess(bcIP, bcPort);
}
bool deviceManager::recv_heart_from_throttle(const string &ip, unsigned short port)
{
	return m_throttle_manager.recv_heart_from_throttle(ip, port);
}

bool deviceManager::add_subKey(const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPOort)
{
    m_subKey_lock.write_lock();
    string &key = m_subKey_manager.add_subKey(m_connectorConfig.get_connectorIP(), m_connectorConfig.get_connectorManagerPort(),
                                                bc_ip, bcManagerPort, bcDataPOort);
    m_subKey_lock.read_write_unlock();
    if(key.empty()==false)
    {
        m_throttle_manager.add_subKey(key);
    }
}

bool deviceManager::parse_protocal(const char* ori_str, int recvLen, int &protoType, string& protoValue)
{
    string origin = ori_str;

    int idx = origin.find("@");
    if(idx == string::npos) return false;
    string typeValue = origin.substr(0, idx++);
    protoType = atoi(typeValue.c_str());
    protoValue = origin.substr(idx, recvLen-idx);
    return true;
}
bool deviceManager::parse_bidderIdentify(string &origin, string& ip, unsigned short &port)
{
    int idx = origin.find(":");
    if(idx == string::npos) return false;
    ip = origin.substr(0, idx++);
    string port_str = origin.substr(idx, origin.size()-idx);
    port = atoi(port_str.c_str());
    return true;
}

bool deviceManager::parse_bcOrThrottelIdentify(string& origin, string& ip, unsigned short &managerPort,
                                                                    unsigned short &dataPort)
{
    int idx = origin.find(":");
    if(idx == string::npos) return false;
    ip = origin.substr(0, idx++);
    string portStr = origin.substr(idx, origin.size()-idx);
    idx = portStr.find(":");
    if(idx == string::npos) return false;
    string mangerPortStr = portStr.substr(0, idx++);
    string dataPortStr = portStr.substr(idx, portStr.size()-idx);
    managerPort = atoi(mangerPortStr.c_str());
    dataPort = atoi(dataPortStr.c_str());
 
    return true;
}



deviceManager::~deviceManager()
{
	event_del(m_bidderLoginEvent);
	zmq_close(m_bidderLoginEvent);
    m_subKey_lock.destroy();
}

