#include "redisPoolManager.h"
redisPoolManager::redisPoolManager()
{
    m_redis_pool_lock.init();
}

redisPoolManager::redisPoolManager(const string &ip, unsigned short port, unsigned short connNum)
{
    m_redis_pool_lock.init();
    connectorPool_set(ip, port, connNum);
}

void redisPoolManager::connectorPool_set(const string &ip, unsigned short port, unsigned short connNum)
{
    m_ip = ip;
    m_port = port;
    m_conncoter_number = connNum;   
}

void redisPoolManager::connectorPool_init(void)
{
    for(int i = 0; i < m_conncoter_number; i++)
    {
        redisClient * client = new redisClient(m_ip, m_port);
        client->redis_connect();
        m_redis_pool_lock.lock();
        m_redis_client.push_back(client);
        m_redis_pool_lock.unlock();
    }
}

void redisPoolManager::connectorPool_init(const string &ip, unsigned short port, unsigned short connNum)
{
    connectorPool_set(ip, port, connNum);   
    connectorPool_init();
}
void redisPoolManager::redisPool_update(const string &ip, unsigned short port, unsigned short connNum)
	{
		if((ip==m_ip)&&(port==m_port)&&(m_conncoter_number==connNum)) return;
		
		if((ip==m_ip)&&(port==m_port))
		{
			if(connNum>=m_conncoter_number)
			{
				int diff = connNum-m_conncoter_number;
				for(int i = 0; i < diff; i++)
				{
					redisClient * client = new redisClient(m_ip, m_port);
					client->redis_connect();
					
					m_redis_pool_lock.lock();
					m_redis_client.push_back(client);
					m_redis_pool_lock.unlock();
				}
			}
			else
			{
				int diff = m_conncoter_number-connNum;
				m_redis_pool_lock.lock();
				int index = 0;
				for(auto it = m_redis_client.begin(); it != m_redis_client.end(); index++)
				{
					if(index>=diff) break;
					redisClient *client =  *it;
					if(client) client->set_stop();
					if(client->erase_redis_client())
					{
						delete client;					
					}
					it = m_redis_client.erase(it);
				}
				m_redis_pool_lock.unlock();			
			} 
            m_conncoter_number = connNum;
		}
		else
		{
			connectorPool_earse();
			connectorPool_init(ip, port, connNum);
		}
	}
	void redisPoolManager::connectorPool_earse()
	{
		cerr<<"connectorPool_earse"<<endl;
		m_redis_pool_lock.lock();
		for(auto it = m_redis_client.begin(); it != m_redis_client.end();)
		{
			redisClient *client =  *it;
			if(client) client->set_stop();
			if(client->erase_redis_client())
			{
				delete client;					
			}
			it = m_redis_client.erase(it);
		}
		m_redis_pool_lock.unlock();
	}
	bool redisPoolManager::redis_get_camp_pipe(operationTarget &target_obj, bufferManager &buf_manager, unsigned short  conver_num)
	{
		redisClient *client = get_idle_redis();
		if(client==NULL)
		{
			cerr<<"can not find idle redis from redis pool"<<endl;
			return false;
		}
	
		target_result_info target_result;
		if(client->redis_get_target(target_obj,target_result, conver_num)==false)
		{
			cerr<<"redis_get_target false!!!"<<endl;
			client->set_redis_status(false);
			return false;
		}	

		int *camp_id_set = target_result.get_array_ptr();
		int camp_id_set_size = target_result.get_array_size();
        g_worker_logger->debug("camp_id_set_size:{0:d}", camp_id_set_size);
		if(camp_id_set)
		{
			if(client->redis_hget_campaign(camp_id_set, camp_id_set_size, buf_manager)==false)
			{
				cerr<<"redis_hget_campaign false!!!"<<endl;
				client->set_redis_status(false);
				return false;
			}
		}
		client->set_redis_status(false);

		return true;	
	}

	bool redisPoolManager::redis_get_creative_pipe(vector<string>& creatvieIDList, vector<string> &creatvieList)
	{
		redisClient *client = get_idle_redis();
		if(client==NULL)
		{
			cerr<<"can not find idle redis from redis pool"<<endl;
			return false;
		}

		if(client->redis_hget_creative(creatvieIDList, creatvieList) == false)
		{
			cerr<<"redis_hget_creative false!!!"<<endl;
			client->set_redis_status(false);
			return false;
		}
		client->set_redis_status(false);
		return true;	
	}


	redisPoolManager::~redisPoolManager()
	{
	    cout<<"redisPoolManager erase"<<endl;
		m_redis_pool_lock.destroy();
		connectorPool_earse();
	}
	redisClient *redisPoolManager::get_idle_redis()
	{
		m_redis_pool_lock.lock();
		for(auto it = m_redis_client.begin(); it != m_redis_client.end();it++)
		{
			redisClient *client =  *it;
			if((client)&&(client->get_redis()))
			{
				m_redis_pool_lock.unlock();
				return client;
			}
		}
		m_redis_pool_lock.unlock();
		return NULL;
	}


