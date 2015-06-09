#include "bcManager.h"
#include "spdlog/spdlog.h"
#include "managerProto.pb.h"
using namespace com::rj::protos::manager;

extern shared_ptr<spdlog::logger> g_manager_logger;

inline bidderCollecter::bidderCollecter(){}
inline bidderCollecter::bidderCollecter(const string& bc_ip, unsigned short managerPort)
{
	set(bc_ip, managerPort);
}

inline void bidderCollecter::set(const string& bc_ip, unsigned short managerPort)
{
	m_bcIP = bc_ip;
	m_bcManagerPort = managerPort;	
}

/*
  *name:connect
  *argument:
  *func:connect to bc and establish async event
  *return: return true
  */
inline bool bidderCollecter::connect(zeromqConnect& conntor, string& throttleID, struct event_base* base,  event_callback_fn fn, void *arg)
{
	m_bcManagerHander = conntor.establishConnect(true, true, throttleID, "tcp", ZMQ_DEALER, m_bcIP.c_str(),
												m_bcManagerPort, &m_bcManagerFd);//client sub 
	m_bcManagerEvent = event_new(base, m_bcManagerFd, EV_READ|EV_PERSIST, fn, arg); 
    event_add(m_bcManagerEvent, NULL);
	return true;
}

/*
  *name:loginOrHeart
  *argument:
  *func:send login or heart req to bc, if lost times more than max times relogin bc
  *return:if lost times more than max times return false, else true
  */
bool bidderCollecter::loginOrHeart(const string& tip, unsigned short tmanagerPort, unsigned short tdataport)
{
    ostringstream os;
	if(m_throttleLoginedBC == false)
	{
	    managerProPackage::send(m_bcManagerHander, managerProtocol_messageTrans_THROTTLE, managerProtocol_messageType_LOGIN_REQ
            , tip, tmanagerPort, tdataport);
        g_manager_logger->info("[login req][throttle -> bc]:{0},{1:d}", m_bcIP, m_bcManagerPort);
	}
	else
	{
		m_lostHeartTimes++;
       	if(m_lostHeartTimes > m_lostHeartTime_max)
       	{
       		m_lostHeartTimes = 0;
            m_throttleLoginedBC = false;
       		return false;
       	}
        else
        {
            managerProPackage::send(m_bcManagerHander, managerProtocol_messageTrans_THROTTLE, managerProtocol_messageType_HEART_REQ
                , tip, tmanagerPort, tdataport);   
            g_manager_logger->info("[heart req][throttle -> bc]:{0},{1:d},losttime:{2:d}", m_bcIP, m_bcManagerPort, m_lostHeartTimes);            
		}
    }
	return true;
}	

/*
  *name:loginedSuccess
  *argument:
  *func:login to the bc success
  *return:if it is the bc, return true ,else false
  */
inline bool bidderCollecter::loginedSuccess(const string& bcIP, unsigned short bcPort)
{
	if((bcIP == m_bcIP)&&(bcPort == m_bcManagerPort)) 
	{
		m_lostHeartTimes = 0;
		m_throttleLoginedBC = true;
		return true;
	}
	return false;
}

inline void *bidderCollecter::get_handler(int fd) const
{
	if(fd == m_bcManagerFd) return m_bcManagerHander;
	return NULL;
}

inline const string& bidderCollecter::get_bcIP() const{return m_bcIP;}
inline unsigned short bidderCollecter::get_bcManagerPort()const{return m_bcManagerPort;}
inline bool bidderCollecter::bcManagerAddr_compare(string& bcIP, unsigned short bcMangerPort)
{
	return ((bcIP == m_bcIP)&&(bcMangerPort == m_bcManagerPort));
}

inline bidderCollecter::~bidderCollecter()
{
    if(m_bcManagerEvent)event_del(m_bcManagerEvent);
    if(m_bcManagerHander)zmq_close(m_bcManagerHander);
    g_manager_logger->info("[bc erase]:{0},{1:d}", m_bcIP, m_bcManagerPort);
}

bcManager::bcManager()
{
    m_bcList_lock.init();
}

/*
  *name:init
  *argument:
  *func:init bc list
  *return:
  */

void bcManager::init( bcInformation& bcInfo)
{
	const vector<bcConfig*>& bcList = bcInfo.get_bcConfigList();
	for(auto it = bcList.begin(); it != bcList.end(); it++)
	{
		bcConfig *bc_conf = *it;
		if(bc_conf)
        {      
    		bidderCollecter *bc = new bidderCollecter(bc_conf->get_bcIP(), bc_conf->get_bcMangerPort());
            m_bcList_lock.lock();
            m_bidderCollecter_list.push_back(bc);
            m_bcList_lock.unlock();
        }
    }
}

/*
  *name:run
  *argument:
  *func:connect to bc device in bc list
  *return:
  */	
void bcManager::run(zeromqConnect& conntor, string& throttleID, struct event_base* base, event_callback_fn fn, void *arg)
{
    m_bcList_lock.lock();
	for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end(); it++)
	{
		bidderCollecter* bc = *it;
		if(bc) bc->connect(conntor, throttleID, base, fn, arg);	
	}
    m_bcList_lock.unlock();
}	



void *bcManager::get_manger_handler(int fd)
{
    m_bcList_lock.lock();
	for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc == NULL) continue;
		void *handler = bc->get_handler(fd);
		if(handler) 
        {
            m_bcList_lock.unlock();
            return handler;
        }
	}		
    m_bcList_lock.unlock();
	return NULL;
}

	
void bcManager::loginedSuccess(const string& ip, unsigned short port)
{
    m_bcList_lock.lock();
	for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc == NULL) continue;
		if(bc->loginedSuccess(ip, port))
		{
		    m_bcList_lock.unlock();
			return;
		}
	}
    m_bcList_lock.unlock();
}

/*
  *name:erase_bc
  *argument:bc ip, bc manager port
  *func:erase bc
  *return:
  */	
void bcManager::erase_bc(string &bcIP, unsigned short bcManagerPort)
{
    m_bcList_lock.lock();
	for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end();)
	{
		bidderCollecter *bc = *it;
		if(bc&&bc->bcManagerAddr_compare(bcIP, bcManagerPort))
		{
			delete bc;
			it = m_bidderCollecter_list.erase(it);
		}
		else
		{
			it++;
		}
	}		
    m_bcList_lock.unlock();
}
/*
  *name:loginOrHeart
  *argument:throttle ip, throttle manager port, throttle data port, bc list which lost heart to max times
  *func:send login or heart req to bc which in bc list
  *return:
  */	
bool bcManager::loginOrHeart(const string& tip, unsigned short tmanagerPort, unsigned short tdataport 
, vector<ipAddress> &LostAddrList)
{
	bool ret = true;
    m_bcList_lock.lock();
	for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end();)
	{
		bidderCollecter *bc = *it;
		if(bc)
		{
			if(bc->loginOrHeart(tip, tmanagerPort, tdataport)==false)
			{
				ipAddress addr(bc->get_bcIP(), bc->get_bcManagerPort());
				LostAddrList.push_back(addr);
				ret = false;
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
    m_bcList_lock.unlock();
	return ret;
}

/*
  *name:updateBC
  *argument:bc list
  *func:update bclist
  *return:
  */	

void bcManager::updateBC(const vector<bcConfig*>& bcConfigList)
{
    m_bcList_lock.lock();
    for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end();)
    {
        bidderCollecter *bc = *it;
        if(bc)
        {
            auto itor = bcConfigList.begin();
            for(itor  = bcConfigList.begin(); itor != bcConfigList.end(); itor++)
            {
                bcConfig* bConf = *itor;
                if(bConf == NULL) continue;
                if((bConf->get_bcIP()==bc->get_bcIP())
                    &&(bConf->get_bcMangerPort()== bc->get_bcManagerPort()))
                    break;
            }       

            if(itor == bcConfigList.end())
            {
                delete bc;
                it = m_bidderCollecter_list.erase(it);
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
    m_bcList_lock.unlock();
}

/*
  *name:add_bc
  *argument:
  *func:add bc
  *return:
  */	
void bcManager::add_bc(const string& bcIP, unsigned short bcManagerPort, zeromqConnect& conntor, string& throttleID, 
						struct event_base* base,  event_callback_fn fn, void *arg)
{
    m_bcList_lock.lock();
	for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc&&(bc->get_bcIP()==bcIP)&&(bc->get_bcManagerPort()==bcManagerPort))
        {
            m_bcList_lock.unlock();
            g_manager_logger->info("###bc exsit:{0},{1:d}", bcIP, bcManagerPort);
            return;
        }
	}
	bidderCollecter *bc = new bidderCollecter(bcIP, bcManagerPort);
	m_bidderCollecter_list.push_back(bc);
	bc->connect(conntor, throttleID, base, fn, arg);
    m_bcList_lock.unlock();
    g_manager_logger->info("###add new bc:{0},{1:d}", bcIP, bcManagerPort);
}

void bcManager::set_login_status(bool status)
{
    m_bcList_lock.lock();
	for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc) bc->set_login_success(status);
	}
    m_bcList_lock.unlock();
}

/*
  *name:recvHeartFromBC
  *argument:bc ip, bc manager port;
  *func:recv heart response from bc
  *return:if find the bc return turn , else false
  */	
bool bcManager::recvHeartFromBC(const string& bcIP, unsigned short bcPort)
{
    m_bcList_lock.lock();
	for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc&&(bc->get_bcIP()==bcIP)&&(bc->get_bcManagerPort()==bcPort))
        {
            bc->lostHeartTimeClear();
            m_bcList_lock.unlock();
            return true;
        }
	}
    m_bcList_lock.unlock();
    return false;
}

/*
  *name:reloginDevice
  *argument:bc ip, bc manager port;
  *func:set device to need relogin bc
  *return:
  */	
void bcManager::reloginDevice(const string& ip, unsigned short port)
{
    m_bcList_lock.lock();
	for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc == NULL)continue; 
        if( (bc->get_bcIP()== ip)&&(bc->get_bcManagerPort()== port) ) 
        {
            bc->set_login_success(false);
            break;
        }
	}
    m_bcList_lock.unlock();
}

