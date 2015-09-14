#include "bcManager.h"
extern shared_ptr<spdlog::logger> g_worker_logger;

bcObject::bcObject(const string& bc_ip, unsigned short date_Port, unsigned short manager_port,unsigned short thread_PoolSize)
{
    m_bc_ip = bc_ip;
    m_bc_datePort = date_Port;
    m_bc_managerPort = manager_port;
    m_bc_threadPoolSize = thread_PoolSize;
}
void bcObject::connectToBC(zeromqConnect &connector, string& Identify, struct event_base * base, event_callback_fn fn, void * arg)
{
    m_sendLoginHeartToBCHandler = connector.establishConnect(true, true, Identify, "tcp", ZMQ_DEALER, 
									m_bc_ip.c_str(), m_bc_managerPort, &m_sendLoginHeartToBCFd);//client sub	
    m_sendLoginHeartToBCEvent= event_new(base, m_sendLoginHeartToBCFd, EV_READ|EV_PERSIST, fn, arg); 
    event_add(m_sendLoginHeartToBCEvent, NULL);
}

void bcObject::connectToBCDataPort(zeromqConnect &connector)
{
    m_bcDataHandler = connector.establishConnect(true,"tcp",ZMQ_DEALER,m_bc_ip.c_str(),m_bc_datePort,NULL);
    if(m_bcDataHandler)
    {
        g_manager_logger->info("[thread #{0}] connector to BC dataPort success:{1},{2:d}",getpid(),m_bc_ip,m_bc_datePort);
    }
    else
    {
        g_manager_logger->info("[thread #{0}] connector to BC dataPort fail:{1},{2:d}",getpid(),m_bc_ip,m_bc_datePort);
    }
}

void *bcObject::get_sendLoginHeartToBCHandler(int fd)
{
    if(fd == m_sendLoginHeartToBCFd)
        return m_sendLoginHeartToBCHandler;
    else
        return NULL;
}
void bcObject::sendLoginHeartToBC(const string& ip,unsigned short manager_port)
{
    if(!m_connectorLoginToBC)
    {        
        managerProPackage::send(m_sendLoginHeartToBCHandler, managerProtocol_messageTrans_CONNECTOR
            , managerProtocol_messageType_LOGIN_REQ, ip, manager_port);
        g_manager_logger->info("[login req][connector -> BC]:{0},{1:d}", m_bc_ip, m_bc_managerPort);		
        return;
    }
    else
    {        
        managerProPackage::send(m_sendLoginHeartToBCHandler, managerProtocol_messageTrans_CONNECTOR
            , managerProtocol_messageType_HEART_REQ, ip, manager_port);
        g_manager_logger->info("[heart req][connector -> BC]:{0},{1:d}, lostTimes:{2:d}", m_bc_ip, m_bc_managerPort, m_lostBCHeartTimes);	
        
    }    
}
bool bcObject::drop(const string& connectorIP,unsigned short conManagerPort,vector<string> &unsubKeyList)
{
    if(m_connectorLoginToBC == true)
    {
        m_lostBCHeartTimes++;        
        if(m_lostBCHeartTimes > m_lostBCHeartTime_max)
        {
            ostringstream os;
	        os<<m_bc_ip<<":"<<m_bc_managerPort<<":"<<m_bc_datePort<<"-"<<connectorIP<<":"<<conManagerPort;
	        string key = os.str();
            unsubKeyList.push_back(key);
            
            m_lostBCHeartTimes = 0;
            m_connectorLoginToBC = false;            
            g_manager_logger->warn("lost heart with bc");
            return true;
        }
    }
    return false;
}



bcManager::bcManager()
{
    m_bcList_lock.init();
}
void bcManager::init(bcInformation& bcInfo)
{
    try
    {
        for(auto bc_it = bcInfo.get_bcConfigList().begin();bc_it != bcInfo.get_bcConfigList().end();bc_it++)
        {
            bcConfig* bc_config = *bc_it;
            if(bc_config == NULL) continue;
            bcObject *bc_Obj = new bcObject(bc_config->get_bcIP(),bc_config->get_bcDataPort(),bc_config->get_bcMangerPort(),bc_config->get_bcThreadPoolSize());

            m_bcList_lock.lock();
            m_bc_list.push_back(bc_Obj);
            m_bcList_lock.unlock();
        }         
    }
    catch(...)
    {
        g_manager_logger->emerg("bcManager init error");
        exit(1);
    }   
}
bool bcManager::add_bc(zeromqConnect &connector, string& bc_ip, unsigned short bc_ManagerPort, unsigned short bc_dataPort,unsigned short bc_threadPoolSize)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bcObject *bc = *it;
		if(bc == NULL) continue;
		if((bc_ip == bc->get_bcIP())&&(bc_ManagerPort == bc->get_bcManagerPort())&&(bc_dataPort == bc->get_bcDataPort()))
		{
		    m_bcList_lock.unlock();
			return false;
		}
	}

	bcObject *bc = new bcObject(bc_ip, bc_ManagerPort, bc_dataPort,bc_threadPoolSize);
    //bc->startDataConnectToBC(connector);
    m_bc_list.push_back(bc);
    m_bcList_lock.unlock();
	return true;
}


void bcManager::update_bcList(zeromqConnect &connector, bcInformation& bcInfo)
{
    auto bcList = bcInfo.get_bcConfigList();
	for(auto it = bcList.begin(); it != bcList.end(); it++)
	{
		bcConfig* bc_conf = *it;
		if(bc_conf)
		{
			add_bc(connector, bc_conf->get_bcIP(), bc_conf->get_bcMangerPort(), bc_conf->get_bcDataPort(),bc_conf->get_bcThreadPoolSize());
		}
	}
}
void bcManager::update_bcList(vector<bcConfig*>& bcConfList)
{
    m_bcList_lock.lock();
    for(auto it = m_bc_list.begin(); it != m_bc_list.end();)
    {
        bcObject *bc = *it;
        if(bc)
        {            
            auto itor  = bcConfList.begin();
            for(; itor != bcConfList.end(); itor++)
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

void bcManager::connectToBCList(zeromqConnect &connector, string& Identify, struct event_base * base, 
												event_callback_fn fn, void * arg)
{
    m_bcList_lock.lock();
    for(auto it = m_bc_list.begin();it != m_bc_list.end();it++)
    {
        bcObject* bc =  *it;
		if(bc) bc->connectToBC(connector, Identify, base, fn, arg);
    }
    m_bcList_lock.unlock();
}

void bcManager::connectToBCListDataPort(zeromqConnect &connector)
{
    m_bcList_lock.lock();
    for(auto it = m_bc_list.begin();it != m_bc_list.end();it++)
    {
        bcObject* bc =  *it;
		if(bc) bc->connectToBCDataPort(connector);
    }
    m_bcList_lock.unlock();
}

void bcManager::sendLoginHeartToBCList(const string& ip,unsigned short manager_port)
{
    m_bcList_lock.lock();
    for(auto it = m_bc_list.begin();it != m_bc_list.end();it++)
    {
        bcObject* bc =  *it;
		if(bc) bc->sendLoginHeartToBC(ip,manager_port);
    }
    m_bcList_lock.unlock();
}

void *bcManager::get_sendLoginHeartToBCHandler(int fd)
{
     m_bcList_lock.lock();
    for(auto it = m_bc_list.begin();it != m_bc_list.end();it++)
    {
        bcObject* bc =  *it;        
		if(bc == NULL) continue;
        void *handler = bc->get_sendLoginHeartToBCHandler(fd);
        m_bcList_lock.unlock();
        if(handler) return handler;
    }
    m_bcList_lock.unlock();
    return NULL;
}
bool bcManager::add_bc(const string& ip,unsigned short dataPort,unsigned short managerPort,
                                      zeromqConnect & connector,string & Identify,struct event_base * base,event_callback_fn fn,void * arg)
{
    m_bcList_lock.lock();
    for(auto it = m_bc_list.begin();it != m_bc_list.end();it++)
    {
        bcObject* bc =  *it;
        if(bc == NULL) continue;
        if((ip == bc->get_bcIP())&&(managerPort == bc->get_bcManagerPort())
			&&(dataPort == bc->get_bcDataPort()))
        {
            m_bcList_lock.unlock();
			return false;
        }
    }
    bcObject *bc = new bcObject(ip, dataPort, managerPort);
    bc->connectToBC(connector,Identify,base,fn,arg);
    m_bc_list.push_back(bc);
    m_bcList_lock.unlock();
    return true;
}
void bcManager::logined(const string& bcIP, unsigned short bcManagerPort)	
{
    m_bcList_lock.lock();
    for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
    {
        bcObject *bc = *it;
        if(bc&&(bc->get_bcIP() == bcIP)&&(bc->get_bcManagerPort() == bcManagerPort))
        {
            //bc logined success
            bc->set_connectorLoginToBC(true);
            break;
        }
    }
    m_bcList_lock.unlock();
}
bool bcManager::recvHeartRsp(const string& bcIP, unsigned short bcPort)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bcObject *bc =  *it;
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
bool bcManager::dropDev(const string& connectorIP,unsigned short conManagerPort,vector<string> &unsubKeyList)
{
    bool ret = false;
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bcObject *bc = *it;
		if(bc == NULL) continue;
        if(bc->drop(connectorIP,conManagerPort,unsubKeyList)) ret = true;
	}
    m_bcList_lock.unlock();
    return ret;
}

uint32_t hashFnv1a64::hash_fnv1a_64(const char *key, size_t key_length)
{
	uint32_t hash = (uint32_t) FNV_64_INIT;
	size_t x;
		
	for (x = 0; x < key_length; x++)
	{
		uint32_t val = (uint32_t)key[x];
		hash ^= val;
		hash *= (uint32_t) FNV_64_PRIME;
	}
		
	return hash;
}
uint32_t hashFnv1a64::get_rand_index(const char* uuid)
{
	const int keyLen=32;
	char key[keyLen];
    int strLen = strlen(uuid);
	for(int i= 0; i < strLen; i++)
	{
		char ch = *(uuid+i);
    	if(ch != '-') key[i]=ch;
	}
	uint32_t secret = hash_fnv1a_64(key, keyLen);
	return secret;
}

void bcManager::hashGetBCinfo(string& uuid,string& bcIP,unsigned short& bcDataPort)
{
    m_bcList_lock.lock();
    int size = m_bc_list.size();
    if(size == 0)
    {
        g_worker_logger->error("hashGetBCinfo failed, BC list is empty !");
        m_bcList_lock.unlock();
        return;
    }
    uint32_t rand_index = hashFnv1a64::get_rand_index(uuid.c_str());
    uint32_t pipe_index = (rand_index%size);
    bcObject* bc = m_bc_list.at(pipe_index);
    bcIP = bc->get_bcIP();
    bcDataPort = bc->get_bcDataPort();    
    m_bcList_lock.unlock();
}

//send ad response to bc
int bcManager::sendAdRspToBC(string& bcIP, unsigned short bcDataPort, void* data, size_t dataSize, int flags)
{
    m_bcList_lock.lock();
	for(auto it = m_bc_list.begin(); it != m_bc_list.end(); it++)
	{
		bcObject *bc =  *it;
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





