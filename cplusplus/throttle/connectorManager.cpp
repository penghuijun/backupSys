#include "connectorManager.h"
#include "login.h"
#include "spdlog/spdlog.h"
extern shared_ptr<spdlog::logger> g_manager_logger;
using namespace com::rj::protos::manager;

inline connectorObject::connectorObject(){}
inline connectorObject::connectorObject(const string& ip, unsigned short port)
{
	set(ip, port);
}
	
inline void connectorObject::set(const string& ip, unsigned short managerPort)
{
	m_devIP = ip;
	m_managerPort = managerPort;
}

bool connectorObject::loginOrHeart(const string &ip, unsigned short managerPort, unsigned short dataPort)
{
	ostringstream os;
	if(m_throttleLogined == false)
	{
	    int size = managerProPackage::send(m_managerHander, managerProtocol_messageTrans_THROTTLE, managerProtocol_messageType_LOGIN_REQ
            , ip, managerPort, dataPort);
        g_manager_logger->info("[login req][throttle -> connector]:{0},{1:d}", m_devIP, m_managerPort);
	}
	else
	{
        m_lostHeartTimes++;
        if(m_lostHeartTimes > m_lostHeartTime_max)
        {
        	m_lostHeartTimes = 0;
            m_throttleLogined = false;
        	return false;
        }	
        else
        {
             managerProPackage::send(m_managerHander, managerProtocol_messageTrans_THROTTLE, managerProtocol_messageType_HEART_REQ
            , ip, managerPort, dataPort);
            g_manager_logger->info("[heart req][throttle->connector]:{0},{1:d},losttime:{2:d}", m_devIP, m_managerPort, m_lostHeartTimes);  
        }
	}
	string bidderstr = os.str();
	zmq_send(m_managerHander, bidderstr.c_str(), bidderstr.size(), ZMQ_NOBLOCK);	
	return true;
}
	
inline bool connectorObject::connect(zeromqConnect& conntor, string& identify, struct event_base* base, event_callback_fn fn, void *arg)
{
	m_managerHander = conntor.establishConnect(true, true, identify, "tcp", ZMQ_DEALER
														, m_devIP.c_str(), m_managerPort, &m_managerFd);//client sub 
	m_managerEvent = event_new(base, m_managerFd, EV_READ|EV_PERSIST, fn, arg); 
	event_add(m_managerEvent, NULL);
	return true;
}

inline bool connectorObject::loginSucess(const string& ip, unsigned short port)
{
	if((ip == m_devIP)&&(port == m_managerPort)) 
	{
		m_lostHeartTimes = 0;
		m_throttleLogined = true;
		return true;
	}
	return false;

}
inline int connectorObject::get_loginFd() const{return m_managerFd;}
	
inline void *connectorObject::get_loginHandler() const{return m_managerHander;}
inline const string& connectorObject::get_devIP() const{return m_devIP;}
inline unsigned short connectorObject::get_managerPort() const{return m_managerPort;}


inline connectorObject::~connectorObject() 
{
    event_del(m_managerEvent);
    zmq_close(m_managerHander);
    g_manager_logger->info("[bidder erase]:{0},{1:d}", m_devIP, m_managerPort);
}


connectorManager::connectorManager()
{
    m_manager_lock.init();
}

void connectorManager::init(connectorInformation& connInfo)
{
	auto devlist = connInfo.get_connectorConfigList();
	for(auto it = devlist.begin(); it != devlist.end(); it++)
	{
		connectorConfig *conn = *it;
		if(conn) 
        {      
    		connectorObject *connector = new connectorObject(conn->get_connectorIP(), conn->get_connectorManagerPort());
            m_manager_lock.lock();
    		m_dev_list.push_back(connector);
            m_manager_lock.unlock();
        }
	}
}

void connectorManager::run(zeromqConnect& conntor, string& identify, struct event_base* base, event_callback_fn fn, void *arg)
{
    m_manager_lock.lock();
	for(auto it = m_dev_list.begin(); it != m_dev_list.end(); it++)
	{
		connectorObject *connector = *it;
		if(connector) connector->connect(conntor, identify,  base, fn, arg);	
	}
    m_manager_lock.unlock();
}
	
bool connectorManager::loginOrHeart(const string &ip, unsigned short managerPort, unsigned short dataPort, vector<ipAddress> &LostAddrList)
{
	string lostHeartIP;
	unsigned short lostHeartPort;
	bool ret = true;

    m_manager_lock.lock();
	for(auto it = m_dev_list.begin(); it != m_dev_list.end();)
	{
		connectorObject *connector = *it;
		if(connector)
		{
			if(connector->loginOrHeart(ip, managerPort, dataPort)==false)
			{			   
				ipAddress addr(connector->get_devIP(), connector->get_managerPort());
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
    m_manager_lock.unlock();
	return ret;
}

void connectorManager::update(const vector<connectorConfig*>& configList)
{
    m_manager_lock.lock();
    for(auto it = m_dev_list.begin(); it != m_dev_list.end();)
    {
        connectorObject *connector = *it;
        if(connector)
        {
            auto itor = configList.begin();
            for(itor  = configList.begin(); itor != configList.end(); itor++)
            {
                connectorConfig * ccc = *itor;
                if(ccc == NULL) continue;
                if((ccc->get_connectorIP()==connector->get_devIP())
                    &&(ccc->get_connectorManagerPort() == connector->get_managerPort()))
                    break;
            }       

            if(itor == configList.end())
            {
                delete connector;
                it = m_dev_list.erase(it);
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
    m_manager_lock.unlock();
}


void *connectorManager::get_login_handler(int fd)
{
    m_manager_lock.lock();
	for(auto it = m_dev_list.begin(); it != m_dev_list.end(); it++)
	{
		connectorObject *connector = *it;
		if(connector == NULL) continue;
        if(connector->get_loginFd() == fd)
        {
		    void *handler = connector->get_loginHandler();
            m_manager_lock.unlock();
            return handler;
        }
	}		
    m_manager_lock.unlock();
	return NULL;
}

bool connectorManager::recvHeartRsp(const string& ip, unsigned short port)
{
    m_manager_lock.lock();
	for(auto it = m_dev_list.begin(); it != m_dev_list.end(); it++)
	{
		connectorObject *connector = *it;
		if(connector == NULL) continue;
		if(connector&&(ip == connector->get_devIP())&&(port == connector->get_managerPort()))
		{
		    connector->lostTimesClear();
		    m_manager_lock.unlock();
			return true;
		}
	}
    m_manager_lock.unlock();
    return false;
}
    
void connectorManager::loginSucess(const string& ip, unsigned short port)
{
    m_manager_lock.lock();
	for(auto it = m_dev_list.begin(); it != m_dev_list.end(); it++)
	{
		connectorObject *connector = *it;
		if(connector&&connector->loginSucess(ip, port))
		{
		    m_manager_lock.unlock();
			return;
		}
	}
    m_manager_lock.unlock();
}		

void connectorManager::add(const string& ip, unsigned short managerPort, zeromqConnect& conntor, string& throttleID, 
						struct event_base* base,  event_callback_fn fn, void *arg)
{
    m_manager_lock.lock();
	for(auto it = m_dev_list.begin(); it != m_dev_list.end(); it++)
	{
		connectorObject *connector = *it;
		if(connector == NULL) continue;
		if((connector->get_devIP()==ip)&&(connector->get_managerPort()==managerPort))
        {
            m_manager_lock.unlock();
            g_manager_logger->info("bidder exist:{0},{1:d}", ip, managerPort);
            return;
        }
	}
	connectorObject *connector = new connectorObject(ip, managerPort);
	m_dev_list.push_back(connector);
	connector->connect(conntor, throttleID, base, fn, arg);
    m_manager_lock.unlock();
    g_manager_logger->info("new bidder:{0},{1:d}", ip, managerPort);
}

void connectorManager::set_login_status(bool status)
{
    m_manager_lock.lock();
	for(auto it = m_dev_list.begin(); it != m_dev_list.end(); it++)
	{
		connectorObject *connector = *it;
		if(connector) connector->set_login_status(false);
	}
    m_manager_lock.unlock();

}

void connectorManager::get_logined_address(vector<ipAddress*> &addr)
{
    m_manager_lock.lock();
	for(auto it = m_dev_list.begin(); it != m_dev_list.end(); it++)
	{
        connectorObject *connector = *it;
		if(connector == NULL)continue; 
        if(connector->get_login_status() == false) continue;
        ipAddress *ipaddr = new ipAddress(connector->get_devIP(), connector->get_managerPort());
        addr.push_back(ipaddr);
	}
    m_manager_lock.unlock();
}

void connectorManager::reloginDevice(const string& ip, unsigned short port)
{
    m_manager_lock.lock();
	for(auto it = m_dev_list.begin(); it != m_dev_list.end(); it++)
	{
		connectorObject *connector = *it;
		if(connector == NULL)continue; 
        if( (connector->get_devIP() == ip)&&(connector->get_managerPort() == port) ) 
        {
            connector->set_login_status(false);
            break;
        }
	}
    m_manager_lock.unlock();
}

