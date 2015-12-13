#include "throttleManager.h"
#include "spdlog/spdlog.h"
#include "string.h"

extern shared_ptr<spdlog::logger> g_manager_logger;


throttleManager::throttleManager(){}



/*
  *name:init
  *argument:
  *func:throttle manager init
  *return:
  */
void throttleManager::init(zeromqConnect &connector, throttleInformation& thro_info
    , connectorInformation& conn_info,  bidderInformation& bidder_info, bcInformation& bc_info)
{
	address_init(connector, thro_info);
	m_bidder_manager.init(bidder_info);
    m_connector_manager.init(conn_info);
	m_bc_manager.init(bc_info);
	ostringstream os;
	os<<m_throttle_config.get_throttleIP()<<":"<<m_throttle_config.get_throttleManagerPort()<<":"<<m_throttle_config.get_throttlePubPort();
	m_throttle_identify = os.str();
}


bool throttleManager::update(throttleInformation &new_throttle_info)
{
    auto old_throttleip = m_throttle_config.get_throttleIP();//
    auto old_throttleAdPort = m_throttle_config.get_throttleAdPort();
    auto old_throttlePubPort = m_throttle_config.get_throttlePubPort();
    auto old_throttleManagerPort = m_throttle_config.get_throttleManagerPort();
    auto old_throttleWorkerNum = m_throttle_config.get_throttleWorkerNum();
    auto new_throttleList = new_throttle_info.get_throttleConfigList();

    for(auto it = new_throttleList.begin(); it != new_throttleList.end(); it++)
    {
        throttleConfig *throttle = *it;
        if(throttle == NULL) continue;  
        if((throttle->get_throttleIP() == old_throttleip)&&(throttle->get_throttleAdPort() == old_throttleAdPort)
            &&(throttle->get_throttlePubPort() == old_throttlePubPort) &&(throttle->get_throttleManagerPort() == old_throttleManagerPort)
          )
       {
            if(throttle->get_throttleWorkerNum() != old_throttleWorkerNum)
            {
                m_throttle_config.set_throttleWorkerNum(throttle->get_throttleWorkerNum());
                return true;
            }
       }
       else
       {
            g_manager_logger->error("Not implemented throttle address update");
       }
    }
    return false;
}


/*
  *name:address_init
  *argument:
  *func:determine throttle address and bind the address
  *return:
  */
void throttleManager::address_init(zeromqConnect &connector, throttleInformation& thro_info)
{
	try
	{
		int hwm = 30000;
		const vector<throttleConfig*>& throttleList = thro_info.get_throttleConfigList();
			
	   	vector<string> ipAddrList;
	    ipAddress::get_local_ipaddr(ipAddrList);//get local ip address
	    bool estatblish=false;
	   	for(auto it = ipAddrList.begin(); it != ipAddrList.end(); it++)
	   	{
	        string &str = *it;
	       	for(auto et = throttleList.begin(); et != throttleList.end(); et++)
	       	{
	           	throttleConfig* throttle = *et;
				if(throttle == NULL) continue;
                const string& ip_str = throttle->get_throttleIP();
	            if(ip_str.compare(0,ip_str.size(), str)==0)//find equal local address ip
	            {
	                g_manager_logger->info("local ip address:{0}", ip_str);
	        		m_throttleAdHandler = connector.establishConnect(false, "tcp", ZMQ_ROUTER, ip_str.c_str(),
						throttle->get_throttleAdPort(), &m_throttleAdFd);	
	        		if(m_throttleAdHandler == nullptr)
	                {
	                   	continue;
	        		}    
					zmq_setsockopt(m_throttleAdHandler, ZMQ_RCVHWM, &hwm, sizeof(hwm));

	        		m_throttlePubHandler = connector.establishConnect(false, "tcp", ZMQ_PUB, "*",
							throttle->get_throttlePubPort(), NULL);	
	        		if(m_throttlePubHandler == nullptr)
	        		{
	        		  	zmq_close(m_throttlePubHandler);
	                   	continue;
	        		}    
					zmq_setsockopt(m_throttlePubHandler, ZMQ_RCVHWM, &hwm, sizeof(hwm));

	        		m_throttleManagerHandler = connector.establishConnect(false, "tcp", ZMQ_ROUTER, ip_str.c_str(),
						throttle->get_throttleManagerPort(), &m_throttleManagerFd);	
	        		if(m_throttleManagerHandler == nullptr)
	        	    {
	        		    zmq_close(m_throttlePubHandler);
						zmq_close(m_throttlePubHandler);
	                   	continue;
	        		}    
					zmq_setsockopt(m_throttleManagerHandler, ZMQ_RCVHWM, &hwm, sizeof(hwm));
		
					m_throttle_config.set(throttle);
		            estatblish = true;
	                break;
	            } 
	        }
	        if(estatblish) break;
	    } 
				
	    if(!estatblish) throw 1;
        g_manager_logger->info("throttleID:{0:d}, throttleIP:{1}, adPort:{2:d}, pubPort:{3:d}, managerPort{4:d}, workerNum{5:d} bind serv port sucess!!!"
        , m_throttle_config.get_throttleID(), m_throttle_config.get_throttleIP()
        , m_throttle_config.get_throttleAdPort(), m_throttle_config.get_throttlePubPort() 
        , m_throttle_config.get_throttleManagerPort(), m_throttle_config.get_throttleWorkerNum());
	}
	catch(...)
	{
	    g_manager_logger->emerg("throttle zmq server bind failure, may you should check confige file");
		exit(1);
	}
}	


/*
  *name:loginOrHeartReq
  *argument:
  *func:send login or heart request to bidder ,connector, bc£¬if lost heart 3 times erase publish key from key list
  *return:if lost heart 3 times return false£¬ else true;
  */
bool throttleManager::loginOrHeartReq()
{
    try
    {
         bool ret = true;
    	 vector<ipAddress> lostHeartAddrList;
         const string&        throttleIP = m_throttle_config.get_throttleIP();
         unsigned short throttleManagerPort = m_throttle_config.get_throttleManagerPort();
         unsigned short throttleDataPort = m_throttle_config.get_throttlePubPort();
    	 if(m_bidder_manager.loginOrHeart(throttleIP, throttleManagerPort, throttleDataPort, lostHeartAddrList) ==false)//lost heart 
    	 {
    	 	 m_throttlePublish.erase_publishKey(sys_bidder, lostHeartAddrList);//delete bidder and erase publishKey
    	 	 ret = false;
    	 }
         
    	 lostHeartAddrList.clear();
    	 if(m_bc_manager.loginOrHeart(throttleIP, throttleManagerPort, throttleDataPort, lostHeartAddrList) ==false)
    	 {
    		  m_throttlePublish.erase_publishKey(sys_bc, lostHeartAddrList);//delete bidder and erase publishKey	  
    		  ret = false;
    	 }

    	 lostHeartAddrList.clear();
    	 if(m_connector_manager.loginOrHeart(throttleIP, throttleManagerPort, throttleDataPort, lostHeartAddrList)==false)
    	 {
    	      m_throttlePublish.erase_publishKey(sys_connector, lostHeartAddrList);//delete bidder and erase publishKey
    		  ret = false;
    	 }     
         return ret;
    }
    catch(...)
    {
    	 g_manager_logger->info("throttleManager loginOrHeartReq exception");
    }
    return false;
}

void throttleManager::updateDev( bidderInformation& bidder_info,  bcInformation& bc_info
    ,  connectorInformation& conn_info)
{
    m_bidder_manager.updateBidder(bidder_info.get_bidderConfigList());
    m_bc_manager.updateBC(bc_info.get_bcConfigList());
    m_connector_manager.update(conn_info.get_connectorConfigList());
}


/*
  *name:recvHeartBeatRsp
  *param:systype, the devece type of msg from£¬ip, the ip of msg from , port, the port of msg from
  *function:handler heart response msg, if success, check is the address exist in publishkey list, if it is, relogin the device 
  */
void throttleManager::recvHeartBeatRsp(bidderSymDevType sysType, const string& ip, unsigned short port)
{
    bool ret = false;
    static int  checktimes = 0;
    switch(sysType)
    {
        case sys_bidder:
        {
           	ret = m_bidder_manager.recvHeartFromBidder(ip, port);
            break;
        }
        case sys_connector:
        {
            ret = m_connector_manager.recvHeartRsp(ip, port);
            break;
        }
        case sys_bc:
        {
            ret = m_bc_manager.recvHeartFromBC(ip, port);
            break;
        }
        default:
        {
            break;
        }
    }

    if(ret&&(checktimes++>3))
    {
        checktimes = 0;
        if(m_throttlePublish.publishExist(sysType, ip, port) == false)//publish key not exist, so relogin the device
        {
            reloginDevice(sysType, ip, port);         
        }        
    }
}

/*
  *name:loginSuccess
  *argument:
  *func:if login success, handler sucess
  *return:
  */

void throttleManager::loginSuccess(bidderSymDevType sysType, const string& ip, unsigned short port)
{
    switch(sysType)
    {
        case sys_bidder:
        {
            m_bidder_manager.loginedSucess(ip, port);
            break;
        }
        case sys_connector:
        {
            m_connector_manager.loginSucess(ip, port);
            break;
        }
        case sys_bc:
        {
            m_bc_manager.loginedSuccess(ip, port);
            break;
        }
        default:
        {
            break;
        }
    }

}

void throttleManager::throttleRequest_event_new(struct event_base* base,  event_callback_fn fn, void *arg)
{
    m_throttleAdEvent = event_new(base, m_throttleAdFd, EV_READ|EV_PERSIST, fn, arg); 
    event_add(m_throttleAdEvent, NULL);
}

void throttleManager::throttleManager_event_new(struct event_base* base,  event_callback_fn fn, void *arg)
{
    m_throttleManagerEvent = event_new(base, m_throttleManagerFd, EV_READ|EV_PERSIST, fn, arg); 
   	event_add(m_throttleManagerEvent, NULL);
}


void throttleManager::connectAllDev(zeromqConnect& connector, struct event_base* base,  event_callback_fn fn, void *arg)
{
    m_bidder_manager.run(connector, m_throttle_identify, base, fn, arg);
    m_connector_manager.run(connector, m_throttle_identify, base, fn, arg);
    m_bc_manager.run(connector, m_throttle_identify, base, fn, arg);
}

void *throttleManager::get_throttle_request_handler(){return m_throttleAdHandler;}
void *throttleManager::get_throttle_publish_handler(){return m_throttlePubHandler;}
void *throttleManager::get_throttle_manager_handler(){return m_throttleManagerHandler;}
void throttleManager::add_throttle_publish_key(bool fromBidder,const string& bidderIP, unsigned short bidderPort,const string& bcIP,
    unsigned short bcManagerPort, unsigned short bcDataPort)
{
	if(m_throttlePublish.add_publishKey(fromBidder, bidderIP, bidderPort, bcIP, bcManagerPort, bcDataPort))
		m_throttlePublish.syncShmSubKeyVector();
}

void *throttleManager::get_login_handler(int fd)
{
    void *handler = m_bidder_manager.get_login_handler(fd);
    if(handler) return handler;
    
    handler = m_connector_manager.get_login_handler(fd);
    if(handler) return handler;
    
	handler = m_bc_manager.get_manger_handler(fd);
	return handler;
}

bool throttleManager::throttleManager::get_throttle_publishKey(const char* uuid, string& bidderKey, string& connectorKey)
{
	return m_throttlePublish.get_publish_key(uuid, bidderKey, connectorKey);
}

/*
  *name:reloginDevice
  *argument:
  *func:relogin bidder ,connector ,bc
  *return:
  */

void throttleManager::reloginDevice(bidderSymDevType sysType, const string& ip, unsigned short port)
{

    switch(sysType)
    {
        case sys_bidder:
        {
            g_manager_logger->info("###===relogin bidder:{0},{1:d}", ip, port);
           	m_bidder_manager.reloginDevice(ip, port);
            break;
        }
        case sys_connector:
        {
            g_manager_logger->info("###===relogin connector:{0},{1:d}", ip, port);
            m_connector_manager.reloginDevice(ip, port);
            break;
        }
        case sys_bc:
        {
            g_manager_logger->info("###===relogin bc:{0},{1:d}", ip, port);
            m_bc_manager.reloginDevice(ip, port);
            break;
        }
        default:
        {
            g_manager_logger->info("###===relogin invalid node:{0},{1:d}", ip, port);
            break;
        }
    }
}


/*
  *name:add_dev
  *argument:
  *func:add bidder ,connector ,bc with dev type
  *return:
  */

void throttleManager::add_dev(bidderSymDevType sysType,const string& ip, unsigned short port, zeromqConnect& connector
, struct event_base* base,  event_callback_fn fn, void *arg)
{
    switch(sysType)
    {
        case sys_bidder:
        {
            m_bidder_manager.add_bidder(ip, port, connector, m_throttle_identify, base, fn, arg);
            break;
        }
        case sys_connector:
        {
            m_connector_manager.add_connector(ip, port, connector, m_throttle_identify, base, fn, arg);
            break;
        }
        case sys_bc:
        {
            m_bc_manager.add_bc(ip, port, connector, m_throttle_identify, base, fn, arg);
            break;
        }
        default:
        {
            break;
        }
    }

}


throttleConfig& throttleManager::get_throttleConfig() {return m_throttle_config;}
string &throttleManager::get_throttle_identify(){return m_throttle_identify;}	

/*
  *name:add_dev
  *argument:
  *func:recv register request, parse the key, add device and key
  *return:
  */
bool throttleManager::registerKey_handler(bool fromBidder, const string &key, zeromqConnect& connector
    , struct event_base* base,  event_callback_fn fn, void *arg)
{
    zmqSubscribeKey subKey(key);

    if(subKey.parse())//parse key
    {
        //add publish key
        add_throttle_publish_key(fromBidder,subKey.get_bidder_ip(), subKey.get_bidder_port()
            , subKey.get_bc_ip(), subKey.get_bcManangerPort(), subKey.get_bcDataPort());

        if(fromBidder)
        {
            //add bidder 
            add_dev(sys_bidder, subKey.get_bidder_ip(), subKey.get_bidder_port(), connector, base, fn , arg);
        }
        else
        {
            //add connector
            add_dev(sys_connector, subKey.get_bidder_ip(), subKey.get_bidder_port(), connector, base, fn , arg);        
        }
        //add bc
        add_dev(sys_bc, subKey.get_bc_ip(), subKey.get_bcManangerPort(), connector, base, fn ,arg);
        return true;
    }
    else
    {
        g_manager_logger->info("[register key famat error from bidder]:{0}", key); 
    }
    return false;
}

/*
  *name:delete_bc
  *argument:
  *func:delete bc and erase the publish key about the bc
  *return:
  */
void throttleManager::delete_bc(string bcIP, unsigned short bcPort)
{
	m_bc_manager.erase_bc(bcIP, bcPort);
	m_throttlePublish.erase_publishKey(sys_bc, bcIP, bcPort);
}

void throttleManager::publishData(void *pubVastHandler, char *msgData, int msgLen)
{
    m_throttlePublish.publishData(pubVastHandler, msgData, msgLen);
}

void throttleManager::workerPublishData(void *pubVastHandler, char *msgData, int msgLen)
{
    m_throttlePublish.workerPublishData(pubVastHandler, msgData, msgLen);
}


