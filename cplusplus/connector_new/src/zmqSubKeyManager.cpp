#include "zmqSubKeyManager.h"
zmqSubKeyObject::zmqSubKeyObject(const string& clientIP, unsigned short clientManagerPort,
                                         const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort)
{    
	set(clientIP, clientManagerPort, bcIP, bcManagerPort, bcDataPort);
}
void zmqSubKeyObject::set(const string& clientIP, unsigned short clientManagerPort,
                           const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort)
{
	m_clientIP = clientIP;
	m_clientManagerPort = clientManagerPort;
	m_bcIP = bcIP;
	m_bcManagerPort = bcManagerPort;
	m_bcDataPort = bcDataPort;
	ostringstream os;
	os<<bcIP<<":"<<bcManagerPort<<":"<<bcDataPort<<"-"<<clientIP<<":"<<clientManagerPort;
	m_subKey = os.str();
}
zmqSubKeyManager::zmqSubKeyManager()
{
    m_subKeyList_lock.init();
}

string& zmqSubKeyManager::add_subKey(const string& clientIP, unsigned short clientManagerPort,
                                           const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPort)
{    
    m_subKeyList_lock.lock();    
	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end(); it++)
	{
		zmqSubKeyObject* subscribe_key =  *it;
		if(subscribe_key == NULL) continue;
		if(((clientIP==subscribe_key->get_clientIP())&&(clientManagerPort==subscribe_key->get_clientManagerPort()))&&
			((bc_ip==subscribe_key->get_bcIP())&&(bcManagerPort ==subscribe_key->get_bcManagerPort()))
			&&(bcDataPort == subscribe_key->get_bcDataPort()))
		{		
		    m_subKeyList_lock.unlock();
			return subscribe_key->get_subKey();
		}
	}
   
	zmqSubKeyObject *key = new zmqSubKeyObject(clientIP, clientManagerPort, bc_ip, bcManagerPort, bcDataPort);
	m_subKey_list.push_back(key);	
    m_subKeyList_lock.unlock();
	return key->get_subKey();
}
void zmqSubKeyManager::erase_subKey(string& key)
{
    m_subKeyList_lock.lock();    
	for(auto it = m_subKey_list.begin(); it != m_subKey_list.end();)
	{
		zmqSubKeyObject* subscribe_key =  *it;		
		if(subscribe_key && (key == subscribe_key->get_subKey()))
        {
            it = m_subKey_list.erase(it);
        }      
        else
            it++;
	}
    m_subKeyList_lock.unlock();    
}


