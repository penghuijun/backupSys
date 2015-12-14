#include "throttleManager.h"
#include "spdlog/spdlog.h"
#include "string.h"
#include "zmq.h"

#define PUBLISHKEYLEN_MAX 100

extern shared_ptr<spdlog::logger> g_manager_logger;
extern shared_ptr<spdlog::logger> g_file_logger;

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

zmqSubscribeKey::zmqSubscribeKey(const string& bidderIP, unsigned short bidderPort,const string& bcIP,
		unsigned short bcManagerPort, unsigned short bcDataPort)
{
	set(bidderIP, bidderPort, bcIP, bcManagerPort, bcDataPort);
}

zmqSubscribeKey::zmqSubscribeKey(const string& key)
{
	m_subKey = key;
}

void zmqSubscribeKey::set(const string& bidderIP, unsigned short bidderPort,const string& bcIP
    , unsigned short bcManagerPort, unsigned short bcDataPort)
{
	m_bidder_ip = bidderIP;
	m_bidder_port = bidderPort;
	m_bc_ip = bcIP;
	m_bcManagerport = bcManagerPort;
	m_bcDataPort = bcDataPort;
	ostringstream os;
	os<<bcIP<<":"<<bcManagerPort<<":"<<bcDataPort<<"-"<<bidderIP<<":"<<bidderPort;
	m_subKey = os.str();
}

bool zmqSubscribeKey::parse()
{
    int idx = m_subKey.find("-");
    if(idx == string::npos) return false;
    string bcAddr = m_subKey.substr(0, idx++);
    string bidderAddr = m_subKey.substr(idx, m_subKey.size()-idx);

    idx = bcAddr.find(":");
    if(idx == string::npos) return false;
    m_bc_ip = bcAddr.substr(0, idx++);
    string portStr = bcAddr.substr(idx, bcAddr.size()-idx);
    idx = portStr.find(":");
    if(idx == string::npos) return false;
    string mangerPortStr = portStr.substr(0, idx++);
    string dataPortStr = portStr.substr(idx, portStr.size()-idx);
    m_bcManagerport = atoi(mangerPortStr.c_str());
    m_bcDataPort = atoi(dataPortStr.c_str());
     
    idx = bidderAddr.find(":");
    if(idx == string::npos) return false;
    m_bidder_ip = bidderAddr.substr(0, idx++);
    string bidderPortstr = bidderAddr.substr(idx, bidderAddr.size()-idx);
    m_bidder_port = atoi(bidderPortstr.c_str());
    return true;
}

bcSubKeyManager::bcSubKeyManager(bool fromBidder, const string& bidderIP, unsigned short bidderPort,const string& bcIP,
		unsigned short bcManagerPort, unsigned short bcDataPort)
{
	set(bcIP, bcManagerPort, bcDataPort);
	add(fromBidder, bidderIP, bidderPort, bcIP, bcManagerPort, bcDataPort);
}

void bcSubKeyManager::set(const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort)
{
	m_bc_ip = bcIP;
	m_bcManagerport = bcManagerPort;
	m_bcDataPort = bcDataPort;	
}

bool bcSubKeyManager::add(bool fromBidder, const string& bidderIP, unsigned short bidderPort,const string& bcIP,
		unsigned short bcManagerPort, unsigned short bcDataPort)
{	
	if(fromBidder)
	{
		g_manager_logger->info("###add bidder key###");
		return addKey(m_bidderKeyList,  bidderIP, bidderPort, bcIP, bcManagerPort, bcDataPort);
	}
	else
	{
		g_manager_logger->info("###add connector key###");
		return addKey(m_connectorKeyList,  bidderIP, bidderPort, bcIP, bcManagerPort, bcDataPort);
	}	
}

bool bcSubKeyManager::addKey(vector<zmqSubscribeKey*>& keyList, const string& bidderIP, unsigned short bidderPort,const string& bcIP,
		unsigned short bcManagerPort, unsigned short bcDataPort )
{

	for(auto it = keyList.begin(); it != keyList.end(); it++)
	{
		zmqSubscribeKey *key = *it;
		if(key)
		{
			if((key->get_bidder_ip() == bidderIP) &&(key->get_bidder_port() == bidderPort))
			{
				g_manager_logger->warn("publish key exist:{0}", key->get_subKey());
				return false;
			}
		}
	}

	zmqSubscribeKey *zmqkey = new zmqSubscribeKey(bidderIP, bidderPort, bcIP, bcManagerPort, bcDataPort);
	keyList.push_back(zmqkey);	
	g_manager_logger->warn("add new publish key:{0}", zmqkey->get_subKey());
	return true;
}

void bcSubKeyManager::get_keypipe(const char* uuid, string& bidderKey, string& connectorKey)
{
    try
	{
		uint32_t rand_index = hashFnv1a64::get_rand_index(uuid);
		int size = m_bidderKeyList.size();
		if(size) 
		{
			uint32_t pipe_index = (rand_index%size);
			zmqSubscribeKey* zmqkey = m_bidderKeyList.at(pipe_index);
			bidderKey = zmqkey->get_subKey();
		}
		
		size = m_connectorKeyList.size();
		if(size) 
		{
			uint32_t pipe_index = (rand_index%size);
			zmqSubscribeKey* zmqkey = m_connectorKeyList.at(pipe_index);
			if(zmqkey)
			{
				connectorKey = zmqkey->get_subKey();
			}
			else
			{
				g_manager_logger->info("connectorKey is NULL");
			}
		}
	}
	catch(...)
	{
		g_manager_logger->info("bcSubKeyManager get_keypipe exception");
	}
}

bool bcSubKeyManager::erase_publishKey(bidderSymDevType type, string& ip, unsigned short managerport)
{
	switch(type)
	{
		case sys_bidder:
		{
			return erase_publishKey(m_bidderKeyList,  ip, managerport);
		}
		case sys_connector:
		{
			return erase_publishKey(m_connectorKeyList,  ip, managerport);
		}
		default:
		{
			break;
		}
	}
	return false;
}
	
bool bcSubKeyManager::erase_publishKey(vector<zmqSubscribeKey*>& keyList, string& ip, unsigned short managerPort)  
{
	for(auto it = keyList.begin(); it != keyList.end();)
	{
		zmqSubscribeKey *key = *it;
		if(key)
		{
			if((key->get_bidder_ip() == ip) &&(key->get_bidder_port() == managerPort))
			{
				delete key;
				it = keyList.erase(it);					
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
	return ((m_bidderKeyList.size()==0)&&(m_connectorKeyList.size()==0));
 }	

const string&  bcSubKeyManager::get_bc_ip() const{return m_bc_ip;}
unsigned short bcSubKeyManager::get_bcManangerPort() const{return m_bcManagerport;}
unsigned short bcSubKeyManager::get_bcDataPort() const{return m_bcDataPort;}


bool bcSubKeyManager::bidder_publishExist(const string& ip, unsigned short port)
{
	for(auto it = m_bidderKeyList.begin(); it != m_bidderKeyList.end(); it++)
	{
        zmqSubscribeKey *subkey = *it;
        if(subkey == NULL) continue;
        if((subkey->get_bidder_ip() == ip)&&(subkey->get_bidder_port() == port))
        {
            return true;
        }
	}    
    return false;
}

bool bcSubKeyManager::connector_publishExist(const string& ip, unsigned short port)
{
	for(auto it = m_connectorKeyList.begin(); it != m_connectorKeyList.end(); it++)
	{
        zmqSubscribeKey *subkey = *it;
        if(subkey == NULL) continue;
        if((subkey->get_bidder_ip() == ip)&&(subkey->get_bidder_port() == port))
        {
            return true;
        }
	}    
    return false;
}

void bcSubKeyManager::publishData(void *pubVastHandler, char *msgData, int msgLen)
{
    int dataLen = msgLen - PUBLISHKEYLEN_MAX;
    for(auto bidder_it = m_bidderKeyList.begin(); bidder_it != m_bidderKeyList.end(); bidder_it++)
    {
        zmqSubscribeKey* keyObj = *bidder_it;
        string bidderKey = keyObj->get_subKey();
        if(bidderKey.empty()==false)
        {
            //publish data to bidder and BC
            zmq_send(pubVastHandler, bidderKey.c_str(), bidderKey.size(), ZMQ_SNDMORE|ZMQ_DONTWAIT);
            zmq_send(pubVastHandler, msgData+PUBLISHKEYLEN_MAX, dataLen, ZMQ_DONTWAIT);  
        }
    }

    for(auto connector_it = m_connectorKeyList.begin(); connector_it != m_connectorKeyList.end(); connector_it++)
    {
        zmqSubscribeKey* keyObj = *connector_it;
        string connectorKey = keyObj->get_subKey();
        if(connectorKey.empty()==false)
        {
            //publish data to connector and BC
            zmq_send(pubVastHandler, connectorKey.c_str(), connectorKey.size(), ZMQ_SNDMORE|ZMQ_DONTWAIT);
            zmq_send(pubVastHandler, msgData+PUBLISHKEYLEN_MAX, dataLen, ZMQ_DONTWAIT);  
        }
    }
}

void bcSubKeyManager::syncShareMemory(MyShmStringVector *shmSubKeyVector)
{
	boost::interprocess::managed_shared_memory segment(boost::interprocess::open_only, "ShareMemory");
	const CharAllocator charalloctor(segment.get_segment_manager());
	MyShmString mystring(charalloctor);	
    for(auto bidder_it = m_bidderKeyList.begin(); bidder_it != m_bidderKeyList.end(); bidder_it++)
    {
		zmqSubscribeKey* keyObj = *bidder_it;
        string bidderKey = keyObj->get_subKey();
        if(bidderKey.empty()==false)
        {
                    cout << "@@@ syncShareMemory" << endl;
			mystring = bidderKey.c_str();
			shmSubKeyVector->push_back(mystring);
			cout << mystring << endl;
		}
	}
	for(auto connector_it = m_connectorKeyList.begin(); connector_it != m_connectorKeyList.end(); connector_it++)
    {
		zmqSubscribeKey* keyObj = *connector_it;
        string connectorKey = keyObj->get_subKey();
        if(connectorKey.empty()==false)
        {
                cout << "@@@ syncShareMemory" << endl;
			mystring = connectorKey.c_str();
			shmSubKeyVector->push_back(mystring);
			cout << mystring << endl;
		}
	}
}



bcSubKeyManager::~bcSubKeyManager()
{
	for(auto it = m_connectorKeyList.begin(); it != m_connectorKeyList.end();)
	{
		delete *it;
		it = m_connectorKeyList.erase(it);
	}
	for(auto it = m_bidderKeyList.begin(); it != m_bidderKeyList.end();)
	{
		delete *it;
		it = m_bidderKeyList.erase(it);
	}	
    g_manager_logger->info("=======subkey manager erase========:{0},{1:d}", m_bc_ip, m_bcManagerport);
}



throttlePubKeyManager::throttlePubKeyManager()
{
    m_publishKey_lock.init();

#if 0
	/*
	 * share memory vector<string> shmSubKeyVector
	 */
	 
	struct shm_remove	{		
		shm_remove() {boost::interprocess::shared_memory_object::remove("ShareMemory");}		
		~shm_remove() {boost::interprocess::shared_memory_object::remove("ShareMemory");}	
	}remover;	
	#endif
#if 0
	boost::interprocess::managed_shared_memory segment(boost::interprocess::open_only, "ShareMemory");
	const StringAllocator stringalloctor(segment.get_segment_manager());
	shmSubKeyVector = segment.construct<MyShmStringVector>("subKeyVector")(stringalloctor);
#endif
}

bool throttlePubKeyManager::add_publishKey(bool frombidder, const string& bidder_ip, unsigned short bidder_port
    ,const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPOort)
{
    m_publishKey_lock.write_lock();

    for(auto it = m_bcSubkeyManagerList.begin(); it != m_bcSubkeyManagerList.end(); it++)
    {
        bcSubKeyManager *keyManager = *it;
        if(keyManager == NULL) continue;
        if((keyManager->get_bc_ip() == bc_ip)&&(keyManager->get_bcManangerPort() == bcManagerPort)&&(keyManager->get_bcDataPort() == bcDataPOort))
        {
        	bool ret = false;
            if(keyManager->add(frombidder, bidder_ip, bidder_port, bc_ip, bcManagerPort, bcDataPOort))
				ret = true;
            m_publishKey_lock.read_write_unlock();
            return ret;
        }
    }

    bcSubKeyManager *manager = new bcSubKeyManager(frombidder, bidder_ip, bidder_port, bc_ip, bcManagerPort, bcDataPOort);
    m_bcSubkeyManagerList.push_back(manager);	
    m_publishKey_lock.read_write_unlock();
    return true;
}

bool throttlePubKeyManager::get_publish_key(const char* uuid, string& bidderKey, string& connectorKey)
{
	try
	{
        m_publishKey_lock.read_lock();
        
        int size = m_bcSubkeyManagerList.size();
        if(size == 0) 
        {
            g_file_logger->warn("publishkeylist is empty");
            m_publishKey_lock.read_write_unlock();
            return false;
        }
        uint32_t rand_index = hashFnv1a64::get_rand_index(uuid);
        uint32_t pipe_index = (rand_index%size);
        bcSubKeyManager* keyManager = m_bcSubkeyManagerList.at(pipe_index);
        keyManager->get_keypipe(uuid, bidderKey, connectorKey);
        m_publishKey_lock.read_write_unlock();
        return true;
	}
	catch(...)
	{
	    g_file_logger->error("get_publish_key exception");
		return false;
	}
}

void throttlePubKeyManager::erase_publishKey(bidderSymDevType type, string& ip, unsigned short managerport)
{
    m_publishKey_lock.write_lock();
    if(type == sys_bc)
    {
    	for(auto it = m_bcSubkeyManagerList.begin(); it != m_bcSubkeyManagerList.end();)
    	{
    		bcSubKeyManager* key = *it;
    		if(key&&(key->get_bc_ip()==ip)&&(key->get_bcManangerPort()==managerport))
    		{
    			delete key;
    			it = m_bcSubkeyManagerList.erase(it);
				syncShmSubKeyVector();
    		}
    		else
    		{
    			it++;
    		}
    	}		
    }
    else
    {
    	for(auto it = m_bcSubkeyManagerList.begin(); it != m_bcSubkeyManagerList.end();)
    	{
    		bcSubKeyManager* key = *it;
            if(key&&(key->erase_publishKey(type, ip, managerport)))
            {
                delete *it;
                it = m_bcSubkeyManagerList.erase(it);
				syncShmSubKeyVector();
            }
            else
            {
                it++;
            }
    	}	    
    }
    m_publishKey_lock.read_write_unlock();
}



void throttlePubKeyManager::erase_publishKey(bidderSymDevType type, vector<ipAddress>& addrList)
{
	for(auto it = addrList.begin(); it != addrList.end(); it++)
	{
		ipAddress &ip_addr=  *it;
        string &lostIP = ip_addr.get_ip();
        unsigned short lostPort = ip_addr.get_port();
        erase_publishKey(type, lostIP, lostPort);
	}
}

bool throttlePubKeyManager::publishExist(bidderSymDevType sysType, const string& ip, unsigned short port)
{
    bool ret = false;
    switch(sysType)
    {
        case sys_bidder:
        {
            ret = bidder_publishExist(ip, port);
            break;
        }
        case sys_connector:
        {
            ret = connector_publishExist(ip, port);
            break;
        }
        case sys_bc:
        {
            ret = bc_publishExist(ip, port);
            break;
        }
        default:
        {
            break;
        }
    }
    return ret;
}

bool throttlePubKeyManager::bc_publishExist(const string& ip, unsigned short port)
{
    bool ret = false;
    m_publishKey_lock.read_lock();
    for(auto it = m_bcSubkeyManagerList.begin(); it != m_bcSubkeyManagerList.end(); it++)
    {
        bcSubKeyManager *bcSub = *it;
        if(bcSub == NULL) continue;
        if((bcSub->get_bc_ip() == ip)&&(bcSub->get_bcManangerPort() == port))
        {
            ret = true;
            break;
        }
    }
    m_publishKey_lock.read_write_unlock();  
    return ret;
}

bool throttlePubKeyManager::bidder_publishExist(const string& ip, unsigned short port)
{
    bool ret = false;
    m_publishKey_lock.read_lock();
    for(auto it = m_bcSubkeyManagerList.begin(); it != m_bcSubkeyManagerList.end(); it++)
    {
        bcSubKeyManager *bcSub = *it;
        if(bcSub == NULL) continue;
        if(bcSub->bidder_publishExist(ip, port))
        {
            ret = true;
            break;
        }
    }
    m_publishKey_lock.read_write_unlock();  
    return ret;
}

bool throttlePubKeyManager::connector_publishExist(const string& ip, unsigned short port)
{
    bool ret = false;
    m_publishKey_lock.read_lock();
    for(auto it = m_bcSubkeyManagerList.begin(); it != m_bcSubkeyManagerList.end(); it++)
    {
        bcSubKeyManager *bcSub = *it;
        if(bcSub == NULL) continue;
        if(bcSub->connector_publishExist(ip, port))
        {
            ret = true;
            break;
        }
    }
    m_publishKey_lock.read_write_unlock();  
    return ret;
}

void throttlePubKeyManager::publishData(void *pubVastHandler, char *msgData, int msgLen)
{
    for(auto it = m_bcSubkeyManagerList.begin(); it != m_bcSubkeyManagerList.end(); it++)
    {
        bcSubKeyManager* obj = *it;
        obj->publishData(pubVastHandler, msgData, msgLen);
    }
}

void throttlePubKeyManager::workerPublishData(void *pubVastHandler, char *msgData, int msgLen)
{
    boost::interprocess::managed_shared_memory segment(boost::interprocess::open_only, "ShareMemory");
	MyShmStringVector *vec = segment.find<MyShmStringVector>("subKeyVector").first;
	string subKey;
	for(MyShmStringVector::iterator it = vec->begin(); it != vec->end(); it++)
	{
		subKey = (*it).data();
		if(subKey.empty()==false)
        {
            //publish data to bidder connector and BC
            
            zmq_send(pubVastHandler, subKey.c_str(), subKey.size(), ZMQ_SNDMORE|ZMQ_DONTWAIT);
            zmq_send(pubVastHandler, msgData+PUBLISHKEYLEN_MAX, msgLen-PUBLISHKEYLEN_MAX, ZMQ_DONTWAIT);  
        }
	}
}


void throttlePubKeyManager::syncShmSubKeyVector()
{
	shmSubKeyVector->clear();
	for(auto it = m_bcSubkeyManagerList.begin(); it != m_bcSubkeyManagerList.end(); it++)
    {
        bcSubKeyManager* obj = *it;
		obj->syncShareMemory(shmSubKeyVector);
    }
}

throttlePubKeyManager::~throttlePubKeyManager()
{
    m_publishKey_lock.write_lock();
    for(auto it = m_bcSubkeyManagerList.begin(); it != m_bcSubkeyManagerList.end();)
    {
        delete *it;
        it = m_bcSubkeyManagerList.erase(it);
    }
    m_publishKey_lock.read_write_unlock();
    m_publishKey_lock.destroy();
    g_manager_logger->info("=========throttlePubKeyManager erase======");
}

