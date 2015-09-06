#include "bcManager.h"
using namespace com::rj::protos::manager;

bidderCollecter::bidderCollecter(string &bidder_ip, unsigned short bidder_port,const string& bc_ip, unsigned short bc_managerPort, unsigned short bc_dataPort)
{
    m_bidderLoginToBC = false;
	set( bidder_ip, bidder_port, bc_ip, bc_managerPort, bc_dataPort);
    g_manager_logger->info("[add a bc]:{0},{1:d}", bc_ip, bc_managerPort);
}
//bcip:bcmanagerport:bcdataport-bidderip:bidderport
void bidderCollecter::set(string &bidder_ip, unsigned short bidder_port,const string& bc_ip, unsigned short bc_managerPort, unsigned short bc_dataPort)
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
//connect to bc and establish async event
bool bidderCollecter::startManagerConnectToBC(zeromqConnect& conntor, string& identify,struct event_base* base,  event_callback_fn fn, void *arg)
{
	m_bidderStateHander = conntor.establishConnect(true, true, identify, "tcp", ZMQ_DEALER, m_bcIP.c_str(),
											m_bcManagerPort, &m_bidderStateFd);//client sub 
	m_bidderStateEvent = event_new(base, m_bidderStateFd, EV_READ|EV_PERSIST, fn, arg); 
	event_add(m_bidderStateEvent, NULL);
	m_bidderLoginToBC = false;
	m_lostHeartTimes = 0;
	return true;
}

//connect to bc data port
bool bidderCollecter::startDataConnectToBC(zeromqConnect& conntor,string& identify)
{
	m_bidderDataHandler = conntor.establishConnect(true, true, identify, "tcp", ZMQ_DEALER, m_bcIP.c_str(),
												m_bcDataPort, NULL);//client sub 
	if(m_bidderDataHandler)
	{
	    g_manager_logger->info("response success:{0},{1:d}", m_bcIP, m_bcDataPort);
	}
    else
    {
    }
	return true;
}



void bidderCollecter::add_throttle_id(string& id)
{
	if(find(m_throttleIdentifyList.begin(), m_throttleIdentifyList.end(), id)==m_throttleIdentifyList.end())
	{
        m_throttleIdentifyList.push_back(id);
	}
}

bool bidderCollecter::loginOrHeartReqToBc(const string &ip, unsigned short port, vector<string> &unsubKeyList)
{
	ostringstream os;
	if(m_bidderLoginToBC)
	{
		m_lostHeartTimes++;
		if(m_lostHeartTimes > m_lostHeartTime_max)
		{
			os.str("");
			os<<m_bcIP<<":"<<m_bcManagerPort<<":"<<m_bcDataPort<<"-"<<ip<<":"<<port;
			string key = os.str();
			unsubKeyList.push_back(key);
			m_lostHeartTimes = 0;
            m_bidderLoginToBC = false;
			return false;
		}
		else
		{
		    managerProPackage::send(m_bidderStateHander, managerProtocol_messageTrans_BIDDER
                , managerProtocol_messageType_HEART_REQ, ip, port);
            g_manager_logger->info("[heart req][bidder -> BC>:{0},{1:d}, lostTimes:{2:d}", m_bcIP, m_bcManagerPort, m_lostHeartTimes);
		}
	}
	else
	{
		managerProPackage::send(m_bidderStateHander, managerProtocol_messageTrans_BIDDER
            , managerProtocol_messageType_LOGIN_REQ, ip, port);
        g_manager_logger->info("[login req][bidder -> BC>:{0},{1:d}", m_bcIP, m_bcManagerPort);		
	}
	return true;
}

void *bidderCollecter::get_handler(int fd)
{
	if(fd == m_bidderStateFd) return m_bidderStateHander;
	return NULL;
}


//bc manager init
void bcManager::init(bidderConfig& bidder_conf, vector<bcConfig*>& bc_conf_list)
{
	for(auto it = bc_conf_list.begin(); it != bc_conf_list.end(); it++)
	{
		bcConfig* bcCon = *it;
		if(bcCon == NULL) continue;
		bidderCollecter *bc = new bidderCollecter(bidder_conf.get_bidderIP(), bidder_conf.get_bidderLoginPort()
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


//add bc to bc list
bool bcManager::add_bc(zeromqConnect &connector, string& identify, struct event_base * base, event_callback_fn fn, void * arg, 
		 string &bidder_ip, unsigned short bidder_port,const string& bc_ip, unsigned short bc_ManagerPort, unsigned short bc_dataPort)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc == NULL) continue;
		if((bc_ip == bc->get_bcIP())&&(bc_ManagerPort == bc->get_bcManagerPort())&&(bc_dataPort == bc->get_bcDataPort()))
        {      
            //bc exist
			bc->lost_time_clear();
            m_bcList_lock.unlock();
			return false;
		}
	}

    //new a bc and add it to bc list
	bidderCollecter *bc = new bidderCollecter(bidder_ip, bidder_port, bc_ip, bc_ManagerPort, bc_dataPort);
	bc->startManagerConnectToBC(connector, identify, base, fn, arg);
    bc->startDataConnectToBC(connector, identify);
	m_bc_list.push_back(bc);
    m_bcList_lock.unlock();
	return true;
}


//add bc to bc list
bool bcManager::add_bc(zeromqConnect &connector, string& identify, string &bidder_ip, unsigned short bidder_port,
		string& bc_ip, unsigned short bc_ManagerPort, unsigned short bc_dataPort)
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
    bc->startDataConnectToBC(connector, identify);
    m_bc_list.push_back(bc);
    m_bcList_lock.unlock();
	return true;
}

//send login request or heart req to bc
bool bcManager::loginOrHeartReqToBc(const string &ip, unsigned short port, vector<string> &unsubKeyList)
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
		if(bc&&(bc->loginOrHeartReqToBc(ip, port, unsubKeyList)==false))// lost heart to max times
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

//update bc
void bcManager::update_bc(vector<bcConfig*>& bcConfList)
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

//connect to bc data port
bool bcManager::startDataConnectToBC(zeromqConnect& conntor,string& identify)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc = *it;
		if(bc) bc->startDataConnectToBC(conntor, identify);
	}		
    m_bcList_lock.unlock();
}

//connect to bc manager port
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

//register key to throttle
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

//send ad response to bc
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

//recv heart response form bc
bool bcManager::recv_heartBeatRsp(const string& bcIP, unsigned short bcPort)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc =  *it;
		if(bc == NULL) continue;
		if((bc->get_bcIP() == bcIP)&&(bc->get_bcManagerPort() == bcPort))
		{
			bc->lost_time_clear();
            m_bcList_lock.unlock();
			return true;
		}
	}
    m_bcList_lock.unlock();
    return false;
}

void bcManager::loginSucess(const string& bcIP, unsigned short bcPort)
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

void bcManager::relogin(const string& bcIP, unsigned short bcPort)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bidderCollecter *bc =  *it;
		if(bc == NULL) continue;
		if((bc->get_bcIP() == bcIP)&&(bc->get_bcManagerPort() == bcPort))
		{
			bc->set_login_status(false);
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

