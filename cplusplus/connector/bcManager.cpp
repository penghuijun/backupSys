#include "bcManager.h"
//key  reqestSym@bcIP:bcPort-bidderIP:bidderPort  eg:  1@1.1.1.1:1111-2.2.2.2:2222
using namespace com::rj::protos::manager;

bidderCollecter::bidderCollecter(const string &bidder_ip, unsigned short bidder_port,const string& bc_ip, unsigned short bc_managerPort, unsigned short bc_dataPort)
{
    m_bidderLoginToBC = false;
	set( bidder_ip, bidder_port, bc_ip, bc_managerPort, bc_dataPort);
    g_manager_logger->info("[add a bc]:{0},{1:d}", bc_ip, bc_managerPort);
}
void bidderCollecter::set(const string &bidder_ip, unsigned short bidder_port,const string& bc_ip, unsigned short bc_managerPort, unsigned short bc_dataPort)
{
	m_bcIP = bc_ip;
	m_bcManagerPort = bc_managerPort;
	m_bcDataPort = bc_dataPort;
	ostringstream os;
	os<<bc_ip<<":"<<bc_managerPort<<":"<<bc_dataPort<<"-"<<bidder_ip<<":"<<bidder_port;
	m_subKey = os.str();

	os.str("");
	os<<bidder_ip<<":"<<bidder_port;
	m_bidderIdentify = os.str();

	os.str("");
	os<<bc_ip<<":"<<bc_managerPort<<":"<<bc_dataPort;
	m_bcIdentify = os.str();
}

bool bidderCollecter::startManagerConnectToBC(zeromqConnect& conntor, string& identify,  struct event_base* base,  event_callback_fn fn, void *arg)
{
	m_bidderStateHander = conntor.establishConnect(true, true, identify, "tcp", ZMQ_DEALER, m_bcIP.c_str(),
											m_bcManagerPort, &m_bidderStateFd);//client sub 
	m_bidderStateEvent = event_new(base, m_bidderStateFd, EV_READ|EV_PERSIST, fn, arg); 
	event_add(m_bidderStateEvent, NULL);
	m_bidderLoginToBC = false;
	m_lostHeartTimes = 0;
	return true;
}

bool bidderCollecter::startDataConnectToBC(zeromqConnect& conntor)
{
	m_bidderDataHandler = conntor.establishConnect(true, "tcp", ZMQ_DEALER, m_bcIP.c_str(),
												m_bcDataPort, NULL);//client sub 
	if(m_bidderDataHandler)
	{
	    g_manager_logger->info("response success:{0},{1:d}", m_bcIP, m_bcDataPort);
	}
	return true;
}

void bidderCollecter::erase_throttle_identify(string &id)
{
	for(auto it = m_throttleIdentifyList.begin(); it != m_throttleIdentifyList.end();)
	{
		string &identify = *it;
		if(identify == id)
		{
			it = m_throttleIdentifyList.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void bidderCollecter::add_throttle_id(string& id)
{
	if(find(m_throttleIdentifyList.begin(), m_throttleIdentifyList.end(), id)==m_throttleIdentifyList.end())
	{
        m_throttleIdentifyList.push_back(id);
	}
}

bool bidderCollecter::loginOrHeartReqToBc(const string &bidderIP, unsigned short bidderPort, vector<string> &unsubKeyList)
{
	ostringstream os;
	if(m_bidderLoginToBC)
	{
		m_lostHeartTimes++;
		if(m_lostHeartTimes > m_lostHeartTime_max)
		{
			os.str("");
			os<<m_bcIP<<":"<<m_bcManagerPort<<":"<<m_bcDataPort<<"-"<<bidderIP<<":"<<bidderPort;
			
			string key = os.str();
			unsubKeyList.push_back(key);
			m_lostHeartTimes = 0;
            m_bidderLoginToBC = false;
			return false;
		}
		else
		{
		    managerProPackage::send(m_bidderStateHander, managerProtocol_messageTrans_CONNECTOR, managerProtocol_messageType_HEART_REQ
                ,bidderIP, bidderPort);
            g_manager_logger->info("[heart req][connector -> BC>:{0},{1:d}, lostTimes:{2:d}", m_bcIP, m_bcManagerPort, m_lostHeartTimes);
		}
	}
	else
	{
		managerProPackage::send(m_bidderStateHander, managerProtocol_messageTrans_CONNECTOR, managerProtocol_messageType_LOGIN_REQ
                ,bidderIP, bidderPort);
        g_manager_logger->info("[login req][connector -> BC>:{0},{1:d}", m_bcIP, m_bcManagerPort);	
	}
	return true;
}

void *bidderCollecter::get_handler(int fd)
{
	if(fd == m_bidderStateFd) return m_bidderStateHander;
	return NULL;
}

void bcManager::init(connectorConfig& conn_conf,const  vector<bcConfig*>& bc_conf_list)
{
	for(auto it = bc_conf_list.begin(); it != bc_conf_list.end(); it++)
	{
		bcConfig* bcCon = *it;
		if(bcCon == NULL) continue;
		bidderCollecter *bc = new bidderCollecter(conn_conf.get_connectorIP(), conn_conf.get_connectorManagerPort()
											,bcCon->get_bcIP(), bcCon->get_bcMangerPort(), bcCon->get_bcDataPort());
        m_bcList_lock.lock();
		m_bc_list.push_back(bc);
        m_bcList_lock.unlock();
	}		
}

void bcManager::add_throttle_identify(string& throttleID)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc) bc->add_throttle_id(throttleID);
	}
    m_bcList_lock.unlock();
}

bool bcManager::add_bc(zeromqConnect &connector, string& identify, struct event_base * base, event_callback_fn fn, void * arg, 
	const	string &bidder_ip, unsigned short bidder_port,const string& bc_ip, unsigned short bc_ManagerPort, unsigned short bc_dataPort)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc == NULL) continue;
		if((bc_ip == bc->get_bcIP())&&(bc_ManagerPort == bc->get_bcManagerPort())&&(bc_dataPort == bc->get_bcDataPort()))
        {      
			bc->lost_time_clear();
            m_bcList_lock.unlock();
			return false;
		}
	}

	bidderCollecter *bc = new bidderCollecter(bidder_ip, bidder_port, bc_ip, bc_ManagerPort, bc_dataPort);
	bc->startManagerConnectToBC(connector, identify, base, fn, arg);
    bc->startDataConnectToBC(connector);
	m_bc_list.push_back(bc);
    m_bcList_lock.unlock();
	return true;
}


bool bcManager::add_bc(zeromqConnect &connector,const string &bidder_ip, unsigned short bidder_port,
		const string& bc_ip, unsigned short bc_ManagerPort, unsigned short bc_dataPort)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc == NULL) continue;
		if((bc_ip == bc->get_bcIP())&&(bc_ManagerPort == bc->get_bcManagerPort())&&(bc_dataPort == bc->get_bcDataPort()))
		{
		    m_bcList_lock.unlock();
			return false;
		}
	}


	bidderCollecter *bc = new bidderCollecter(bidder_ip, bidder_port, bc_ip, bc_ManagerPort, bc_dataPort);
    bc->startDataConnectToBC(connector);
    m_bc_list.push_back(bc);
    m_bcList_lock.unlock();
	return true;
}


bool bcManager::loginOrHeartReqToBc(const string &bidderIP, unsigned short bidderPort, vector<string> &unsubKeyList)
{
	string bcIP;
	unsigned short bcManagerPort;
	unsigned short bcDataPort;	
    bool ret = true;
	ostringstream os;
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end();)
	{
		bidderCollecter *bc = *it;
		if(bc&&(bc->loginOrHeartReqToBc(bidderIP, bidderPort, unsubKeyList)==false))
		{
			ret = false;
		}
		else
		{
			it++;
		}
	}
    m_bcList_lock.unlock();
	return ret;
}

void bcManager::update_bc(const vector<bcConfig*>& bcConfList)
{
    m_bcList_lock.lock();
    for(auto it = m_bc_list.begin(); it != m_bc_list.end();)
    {
        bidderCollecter *bc = *it;
        if(bc)
        {
            auto itor = bcConfList.begin();
            for(itor  = bcConfList.begin(); itor != bcConfList.end(); itor++)
            {
                bcConfig* bConf = *itor;
                if(bConf == NULL) continue;
                if((bConf->get_bcIP()==bc->get_bcIP())
                    &&(bConf->get_bcMangerPort()== bc->get_bcManagerPort()))
                    break;
            }       

            if(itor == bcConfList.end())
            {
                delete bc;
                it = m_bc_list.erase(it);
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


bool bcManager::startDataConnectToBC(zeromqConnect& conntor)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc) bc->startDataConnectToBC(conntor);
	}		
    m_bcList_lock.unlock();
}

bool bcManager::BCManagerConnectToBC(zeromqConnect & conntor, string& identify, struct event_base * base,event_callback_fn fn,void * arg)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc) bc->startManagerConnectToBC(conntor, identify, base, fn, arg);
	}		
    m_bcList_lock.unlock();
}

void bcManager::registerToThrottle(string &identify)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
	    bidderCollecter* bc = *it;
		if(bc) bc->add_throttle_id(identify);
	}
    m_bcList_lock.unlock();
}

int bcManager::sendAdRspToBC(string& bcIP, unsigned short bcDataPort, void* data, size_t dataSize, int flags)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc =  *it;
		if(bc == NULL) continue;
		if((bc->get_bcIP() == bcIP)&&(bc->get_bcDataPort() == bcDataPort))
		{
		    void *handler = bc->get_bcDataHandler();
            int ssize = zmq_send(handler, data, dataSize, flags);
            m_bcList_lock.unlock();
			return ssize;
		}
	}
    m_bcList_lock.unlock();
	return -1;
}

void *bcManager::get_bcManager_handler(int fd)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
    	bidderCollecter *bc =  *it;
		if(bc == NULL) continue;
		if(bc->get_fd()==fd)
		{
		    void *handler = bc->get_handler();
            m_bcList_lock.unlock();
			return handler;
		}
	}
    m_bcList_lock.unlock();
	return NULL;
}		

void bcManager::lostheartTimesWithBC_clear(const string& bcIP, unsigned short bcPort)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc =  *it;
		if(bc == NULL) continue;
		if((bc->get_bcIP() == bcIP)&&(bc->get_bcManagerPort() == bcPort))
		{
			bc->lost_time_clear();
			break;
		}
	}
    m_bcList_lock.unlock();
}

void bcManager::bidderLoginedBcSucess(const string& bcIP, unsigned short bcPort)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc =  *it;
		if(bc == NULL) continue;
		if((bc->get_bcIP() == bcIP)&&(bc->get_bcManagerPort() == bcPort))
		{
			bc->set_login_status(true);
			break;
        }
	}
    m_bcList_lock.unlock();
}

void *bcManager::get_handler(int fd)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc == NULL) continue;
		void *handler = bc->get_handler(fd);
        m_bcList_lock.unlock();
		if(handler) return handler;
	}
    m_bcList_lock.unlock();
	return NULL;
}

