#include "bidderManager.h"
#include "spdlog/spdlog.h"

extern shared_ptr<spdlog::logger> g_manager_logger;

inline bidderObject::bidderObject(){}
inline bidderObject::bidderObject(const string& ip, unsigned short port)
{
	set(ip, port);
}
	
inline void bidderObject::set(const string& ip, unsigned short managerPort)
{
	m_bidderIP = ip;
	m_bidderLoginPort = managerPort;
}

/*
  *name:loginOrHeart
  *argument:throttle ip, throttle manager port, throttle data port
  *func:send login or heart request to bidder dev
  *return:if lost heart with bidder more than 3 times return false, else return true 
  */   
bool bidderObject::loginOrHeart(const string& tip, unsigned short tmanagerPort, unsigned short tdataport)
{
    try
    {
    	if(m_throttleLoginedBidder == false)//if not login in bidder , send login requset 
    	{
    	    managerProPackage::send(m_bidderLoginHander, managerProtocol_messageTrans_THROTTLE, managerProtocol_messageType_LOGIN_REQ
                , tip, tmanagerPort, tdataport);
            g_manager_logger->info("[login req][throttle -> bidder]:{0},{1:d}", m_bidderIP, m_bidderLoginPort);
    	}
    	else//logined bidder
    	{
            m_lostHeartTimes++;
            if(m_lostHeartTimes > m_lostHeartTime_max)//lost time to max times, need relogin dev
            {
            	m_lostHeartTimes = 0;
                m_throttleLoginedBidder = false;
            	return false;
            }	
            else//send heart request to bidder
            {
                managerProPackage::send(m_bidderLoginHander, managerProtocol_messageTrans_THROTTLE, managerProtocol_messageType_HEART_REQ
                    , tip, tmanagerPort, tdataport);
                g_manager_logger->info("[heart req][throttle -> bidder]:{0},{1:d},losttime:{2:d}", m_bidderIP, m_bidderLoginPort, m_lostHeartTimes);  
            }
    	}
        return true;
    }
    catch(...)
    {
        g_manager_logger->error("bidderObject loginOrHeart exception");
    }
	return true;
}


/*
  *name:connect
  *argument:
  *func:connecto to bidder and establish recv response event
  *return:
  */   	
inline bool bidderObject::connect(zeromqConnect& conntor, string& identify, struct event_base* base, event_callback_fn fn, void *arg)
{
	m_bidderLoginHander = conntor.establishConnect(true, true, identify, "tcp", ZMQ_DEALER
														, m_bidderIP.c_str(), m_bidderLoginPort, &m_bidderLoginFd);//client sub 
	m_bidderLoginEvent = event_new(base, m_bidderLoginFd, EV_READ|EV_PERSIST, fn, arg); 
	event_add(m_bidderLoginEvent, NULL);
	return true;
}

/*
  *name:loginedSucess
  *argument:
  *func:if it is the device logined success, then reset lost times and logined bidder success 
  *return:if it is the deveice reponse return true, else false
  */  
inline bool bidderObject::loginedSucess(const string& bidderIP, unsigned short bidderPort)
{
	if((bidderIP == m_bidderIP)&&(bidderPort == m_bidderLoginPort)) 
	{
		m_lostHeartTimes = 0;
		m_throttleLoginedBidder = true;
		return true;
	}
	return false;

}

inline int bidderObject::get_loginFd() const {return m_bidderLoginFd;}
	
inline void *bidderObject::get_loginHandler() const{return m_bidderLoginHander;}
inline const string& bidderObject::get_bidderIP() const{return m_bidderIP;}
inline unsigned short bidderObject::get_bidderPort() const{return m_bidderLoginPort;}

bidderObject::~bidderObject() 
{
    event_del(m_bidderLoginEvent);
    zmq_close(m_bidderLoginHander);
    g_manager_logger->info("[bidder erase]:{0},{1:d}", m_bidderIP, m_bidderLoginPort);
}


bidderManager::bidderManager()
{
    m_bidderList_lock.init();
}


/*
  *name:init
  *argument:
  *func:bidder manager init , init the bidder list
  *return:
  */  
void bidderManager::init(bidderInformation& bidderInfo)
{
	auto bidderList = bidderInfo.get_bidderConfigList();
	for(auto it = bidderList.begin(); it != bidderList.end(); it++)
	{
		bidderConfig *bidder_conf = *it;
		if(bidder_conf) 
        {      
    		bidderObject *bidder_obj = new bidderObject(bidder_conf->get_bidderIP(), bidder_conf->get_bidderLoginPort());
            m_bidderList_lock.lock();
    		m_bidder_list.push_back(bidder_obj);
            m_bidderList_lock.unlock();
        }
	}
}

/*
  *name:run
  *argument:
  *func:connect to all bidder device
  *return:
  */

void bidderManager::run(zeromqConnect& conntor, string& identify, struct event_base* base, event_callback_fn fn, void *arg)
{
    m_bidderList_lock.lock();
	for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
	{
		bidderObject *bidder = *it;
		if(bidder) bidder->connect(conntor, identify,  base, fn, arg);	
	}
    m_bidderList_lock.unlock();
}

/*
  *name:loginOrHeart
  *argument:
  *func:send login or heart request to bidder int the bidder list
  *return:
  */	
bool bidderManager::loginOrHeart(const string& tip, unsigned short tmanagerPort, unsigned short tdataport, vector<ipAddress> &LostAddrList)
{
    try
    {
    	bool ret = true;
        m_bidderList_lock.lock();
    	for(auto it = m_bidder_list.begin(); it != m_bidder_list.end();)
    	{
    		bidderObject *bidder = *it;
    	    if(bidder&&(bidder->loginOrHeart(tip, tmanagerPort, tdataport)==false))
    		{			   
    			ipAddress addr(bidder->get_bidderIP(), bidder->get_bidderPort());
    			LostAddrList.push_back(addr);
    			ret = false;
    		}
    		else
    		{
    			it++;
    		}
    	}
        m_bidderList_lock.unlock();
    	return ret;
    }
    catch(...)
    {
        g_manager_logger->error("bidderManager loginOrHeart exception");
    }
    return false;
}

/*
  *name:updateBidder
  *argument:new bidderList
  *func:update bidder 
  *return:
  */	
void bidderManager::updateBidder(const vector<bidderConfig*>& bidderConfigList)
{
    m_bidderList_lock.lock();
    for(auto it = m_bidder_list.begin(); it != m_bidder_list.end();)
    {
        bidderObject *bidder = *it;
        if(bidder)
        {
            auto itor = bidderConfigList.begin();
            for(itor  = bidderConfigList.begin(); itor != bidderConfigList.end(); itor++)
            {
                bidderConfig * bConf = *itor;
                if(bConf == NULL) continue;
                if((bConf->get_bidderIP()==bidder->get_bidderIP())
                    &&(bConf->get_bidderLoginPort() == bidder->get_bidderPort()))
                    break;
            }       

            if(itor == bidderConfigList.end())//need delete the device
            {
                delete bidder;
                it = m_bidder_list.erase(it);
                g_manager_logger->info("erase bidder object");
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
    m_bidderList_lock.unlock();
}


void *bidderManager::get_login_handler(int fd)
{

    int size = m_bidder_list.size();
    m_bidderList_lock.lock();
	for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
	{
		bidderObject *bidder = *it;
		if(bidder == NULL) continue;
        if(bidder->get_loginFd() == fd)
        {
		    void *handler = bidder->get_loginHandler();
            m_bidderList_lock.unlock();
            return handler;            
        }
	}		
    m_bidderList_lock.unlock();
	return NULL;
}


/*
  *name:recvHeartFromBidder
  *argument:bidder ip, biddermanagerport
  *func:recv heart response from bidder handler
  *return:the bidder list have not the bidder which param point,return false, else true
  */	
bool bidderManager::recvHeartFromBidder(const string& bidderIP, unsigned short bidderPort)
{
    m_bidderList_lock.lock();
	for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
	{
		bidderObject *bidder = *it;
		if(bidder == NULL) continue;
		if(bidder&&(bidderIP == bidder->get_bidderIP())&&(bidderPort == bidder->get_bidderPort()))
		{
		    bidder->lostTimesClear();
		    m_bidderList_lock.unlock();
			return true;
		}
	}
    m_bidderList_lock.unlock();
    return false;
}
    
void bidderManager::loginedSucess(const string& bidderIP, unsigned short bidderPort)
{
    m_bidderList_lock.lock();
	for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
	{
		bidderObject *bidder = *it;
		if(bidder&&bidder->loginedSucess(bidderIP, bidderPort))
		{
		    m_bidderList_lock.unlock();
			return;
		}
	}
    m_bidderList_lock.unlock();
}		

/*
  *name:add_bidder
  *argument:
  *func:add bidder device, if had exsit, return ,not ,new bidder
  *return:
  */	
void bidderManager::add_bidder(const string& bidderIP, unsigned short bidderPort, zeromqConnect& conntor, string& throttleID, 
						struct event_base* base,  event_callback_fn fn, void *arg)
{
    m_bidderList_lock.lock();
	for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
	{
		bidderObject *bidder = *it;
		if(bidder == NULL) continue;
		if((bidder->get_bidderIP()==bidderIP)&&(bidder->get_bidderPort()==bidderPort))
        {
            m_bidderList_lock.unlock();
            g_manager_logger->info("bidder exist:{0},{1:d}", bidderIP, bidderPort);
            return;
        }
	}
	bidderObject *bidder = new bidderObject(bidderIP, bidderPort);
	bidder->connect(conntor, throttleID, base, fn, arg);
	m_bidder_list.push_back(bidder);

    m_bidderList_lock.unlock();
    g_manager_logger->info("new bidder:{0},{1:d}:{2:d}:{3:d}", bidderIP, bidderPort, m_bidder_list.size(), getpid());
}

void bidderManager::set_login_status(bool status)
{
    m_bidderList_lock.lock();
	for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
	{
		bidderObject *bidder = *it;
		if(bidder) bidder->set_login_status(status);
	}
    m_bidderList_lock.unlock();

}


/*
  *name:reloginDevice
  *argument:bidder ip, bidder managerport
  *func:relogin bidder, find the bidder and set login symbol to false
  *return:
  */	
void bidderManager::reloginDevice(const string& ip, unsigned short port)
{
    m_bidderList_lock.lock();
	for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); it++)
	{
		bidderObject *bidder = *it;
		if(bidder == NULL)continue; 
        if( (bidder->get_bidderIP() == ip)&&(bidder->get_bidderPort() == port) ) 
        {
            bidder->set_login_status(false);
            break;
        }
	}
    m_bidderList_lock.unlock();
}



