#include "bcManager.h"
using namespace com::rj::protos::manager;


bcManager::bcManager()
{
}
void bcManager::init(zeromqConnect &connector, throttleInformation& throttle_info, bidderInformation& bidder_info, bcInformation& bc_info)
{
	determine_bc_address(connector, bc_info);
	ostringstream os;
    auto bidderConfigList = bidder_info.get_bidderConfigList();
    m_throttle_manager.init(throttle_info.get_throttleConfigList());
    m_bidder_manager.init(bidder_info.get_bidderConfigList());

    os.str("");
	os<<m_bcConfig.get_bcIP()<<":"<<m_bcConfig.get_bcMangerPort()<<":"<<m_bcConfig.get_bcDataPort();
	m_bcIdentify = os.str();
    
    os.str("");
    os<<m_bcConfig.get_bcIP()<<":"<<m_bcConfig.get_bcMangerPort();
    m_bcIPPortID = os.str();

    //init subKeyList
    for(auto it = bidderConfigList.begin(); it != bidderConfigList.end(); it++)
    {
        string subkey;
        bidderConfig *bidder_conf = *it;
        if(bidder_conf == NULL) continue;
        add_subKey(bidder_conf->get_bidderIP(), bidder_conf->get_bidderLoginPort(), subkey);
    }
}

void bcManager::determine_bc_address(zeromqConnect &connector, bcInformation& bc_info)
{
	int hwm = 30000;
	vector<bcConfig*>& bcConfigList = bc_info.get_bcConfigList();
		
   	vector<string> ipAddrList;
    ipAddress::get_local_ipaddr(ipAddrList);
    bool estatblish=false;
    for(auto it = ipAddrList.begin(); it != ipAddrList.end(); it++)
    {
        string &str = *it;
        g_manager_logger->info("local ip:{0}", str);
       	for(auto et = bcConfigList.begin(); et != bcConfigList.end(); et++)
        {
           	bcConfig* bc_con = *et;
			if(bc_con == NULL) continue;
            string& ip_str = bc_con->get_bcIP();
            if(ip_str.compare(0,ip_str.size(), str)==0)
            {
        	    m_bcAdRspHandler = connector.establishConnect(false, "tcp", ZMQ_ROUTER, ip_str.c_str(),
                bc_con->get_bcDataPort(), &m_bcAdRspFd);
                zmq_setsockopt(m_bcAdRspHandler, ZMQ_RCVHWM, &hwm, sizeof(hwm));
            	if(m_bcAdRspHandler == nullptr)
        		{
                   	continue;
        		}
                m_bcManagerHandler = connector.establishConnect(false, "tcp", ZMQ_ROUTER, ip_str.c_str(),
                                                          bc_con->get_bcMangerPort(), &m_bcManagerFd);
                zmq_setsockopt(m_bcManagerHandler, ZMQ_RCVHWM, &hwm, sizeof(hwm));
            	if(m_bcManagerHandler == nullptr)
        	    {
       		        zmq_close(m_bcAdRspHandler);
                    continue;
       			}                  
	            m_bcConfig.set(bc_con);
	            m_redisIP = bc_info.get_redis_ip();
	            m_redisPort = bc_info.get_redis_port();
	            m_redisTimeout = bc_info.get_redis_timeout();
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
	else
	{
	    g_manager_logger->info("{0:d},{1},{2:d},{3:d},{4:d} bind success", m_bcConfig.get_bcID(), m_bcConfig.get_bcIP()
            , m_bcConfig.get_bcDataPort(), m_bcConfig.get_bcMangerPort(), m_bcConfig.get_bcThreadPoolSize());
	}
}
void bcManager::AdRsp_event_new(struct event_base * base, event_callback_fn fn, void * arg)
{
    m_bcAdRspEvent = event_new(base, m_bcAdRspFd, EV_READ|EV_PERSIST, fn, arg); 
	event_add(m_bcAdRspEvent, NULL); 	
}

void bcManager::manager_event_new( struct event_base * base, event_callback_fn fn, void * arg)
{	
	 m_bcManagerEvent = event_new(base, m_bcManagerFd, EV_READ|EV_PERSIST, fn, arg); 
     event_add(m_bcManagerEvent, NULL); 
}


void bcManager::connectAll(zeromqConnect& conntor, struct event_base* base,  event_callback_fn fn, void *arg)
{
	m_throttle_manager.startConnectToThrottle(conntor, m_bcIdentify, base, fn, arg);
	m_bidder_manager.startConnectToBidder(conntor, m_bcIdentify, base, fn, arg);
    m_connector_manager.connect(conntor, m_bcIdentify, base, fn, arg);
}

void bcManager::login()
{
    
	m_bidder_manager.loginBidder(m_bcConfig.get_bcIP(), m_bcConfig.get_bcMangerPort(), m_bcConfig.get_bcDataPort());
    m_connector_manager.login(m_bcConfig.get_bcIP(), m_bcConfig.get_bcMangerPort(), m_bcConfig.get_bcDataPort());
   // m_throttle_manager.loginThrottle(m_bcConfig.get_bcIP(), m_bcConfig.get_bcMangerPort(), m_bcConfig.get_bcDataPort()); //bc对throttle的登录暂时未实现
}

void bcManager::loginThrottleSuccess(const string &ip, unsigned short managerPort)
{
    m_throttle_manager.loginThrottleSuccess(ip, managerPort);
}

void *bcManager::get_bidderManagerHandler(int fd)
{
	void *handler = m_bidder_manager.get_bidderManagerHandler(fd);
    if(handler)
    {
        return handler;
    }
    handler = m_throttle_manager.get_throttleManagerHandler(fd); 
    if(handler)
    {
        return handler;
    }
    handler = m_connector_manager.get_managerHandler(fd);
    return handler;
}

void *bcManager::get_throttle_handler(int fd)
{
	m_throttle_manager.get_throttle_handler(fd);
}

void bcManager::register_sucess(bool bidder, const string &ip, unsigned short managerPort)
{
    if(bidder)
    {
	    m_bidder_manager.register_sucess(ip, managerPort);
    }
    else
    {
        m_connector_manager.register_sucess(ip, managerPort);    
    }
}

void bcManager::startSubThrottle(zeromqConnect & conntor,struct event_base * base,event_callback_fn fn,void * arg)
{
	m_throttle_manager.run(m_subKey_manager.get_subKey_list(), conntor,  base, fn, arg);
}
void *bcManager::get_bcManagerHandler()
{
    return m_bcManagerHandler;
}

void *bcManager::get_adRspHandler()
{
    return m_bcAdRspHandler;
}

string& bcManager::get_redis_ip()
{
    return m_redisIP;
}

string& bcManager::get_bcIdentify()
{
    return m_bcIdentify;
}

string& bcManager::get_bcIPPortID()
{
    return m_bcIPPortID;
}

unsigned short bcManager::get_redis_port()
{
    return m_redisPort;
}

unsigned short bcManager::get_redis_timeout()
{
    return m_redisTimeout;
}

bcConfig& bcManager::get_bc_config()
{
    return m_bcConfig;
}

//bidder with bc
bool bcManager::devDrop()
{
    bool bidderRet = true;
    vector<string> unsubKeyList;
    
    m_throttle_manager.recv_throttle_heart_time_increase();
	bidderRet = m_bidder_manager.devDrop(m_bcIdentify, unsubKeyList);
    for(auto it = unsubKeyList.begin(); it != unsubKeyList.end(); it++)
    {
        string& key = *it;
        m_subKey_manager.erase_subKey(key);
    }
    m_throttle_manager.unSubscribe(unsubKeyList);
    return (bidderRet == true);
}

void bcManager::update_dev(throttleInformation& thro_info, bidderInformation& bidder_info)
{
   m_throttle_manager.update_throttle(thro_info.get_throttleConfigList());
   m_bidder_manager.update_bidder(bidder_info.get_bidderConfigList());
}

bool bcManager::recv_heartBeat(managerProtocol_messageTrans trans,const string& ip, unsigned short managerPort)
{
    bool ret = false;
    switch(trans)
    {
        case managerProtocol_messageTrans_THROTTLE:
        {
            ret = m_throttle_manager.recv_heart_from_throttle(ip,managerPort);
            break;
        }
        case managerProtocol_messageTrans_BIDDER:
        {
            ret = m_bidder_manager.recv_heart_from_bidder(ip, managerPort);
            break;
        }
        case managerProtocol_messageTrans_CONNECTOR:
        {
            ret = m_connector_manager.recv_heart(ip, managerPort);
            break;
        }
        default:
        {
            g_manager_logger->error("recv_heartBeat trans exception:{0:d}", (int)trans);
            break;
        }
    }
    return ret;
}

void bcManager::add_subKey(const string& bidderIP, unsigned short bidderPort, string &subKey)
{
    string &bcIP = m_bcConfig.get_bcIP();
    unsigned short bcManagerPort = m_bcConfig.get_bcMangerPort();
    unsigned short bcDataPort = m_bcConfig.get_bcDataPort();
    m_subKey_manager.add_subKey(bidderIP, bidderPort, bcIP, bcManagerPort, bcDataPort, subKey);
}

bool bcManager::add_bidder(const string& bidderIP, unsigned short bidderPort
    ,zeromqConnect & conntor, struct event_base * base,event_callback_fn fn,void * arg)
{
    try
    {
        string subKey;
    	m_bidder_manager.add_bidder(bidderIP, bidderPort, conntor, m_bcIdentify, base, fn, arg);
     
        add_subKey(bidderIP, bidderPort, subKey); 
        if(subKey.empty()==false) m_throttle_manager.set_subscribe_opt(subKey);//throttle manager subscribe key     
        return true;
    }
    catch(...)
    {
        g_manager_logger->error("add bidder exception:{0},{1:d}", bidderIP, bidderPort);
    }
    return false;
}

void bcManager::add_connector(const string& ip, unsigned short port
    ,zeromqConnect & zmqConn, struct event_base * base,event_callback_fn fn,void * arg)
{
	m_connector_manager.add_connector(ip, port, zmqConn, m_bcIdentify, base, fn, arg); 
    string subKey;
    add_subKey(ip, port, subKey);
    m_throttle_manager.set_subscribe_opt(subKey);//throttle manager subscribe key            
}

void bcManager::add_throttle(const string& throtteleIP, unsigned short throttleManagerPort, unsigned short throttlePubPort
    ,zeromqConnect & conntor, struct event_base * base,event_callback_fn fn,void * arg)
{
	m_throttle_manager.add_throttle(throtteleIP, throttleManagerPort, throttlePubPort, m_subKey_manager.get_subKey_list(), 
        conntor, base, fn, arg);
}
	
bcManager::~bcManager()
{
	event_del(m_bcManagerEvent);
	zmq_close(m_bcManagerHandler);
	event_del(m_bcAdRspEvent);
	zmq_close(m_bcAdRspHandler);
}
