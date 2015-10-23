#include "redis/redisPoolManager.h"
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
	    g_worker_logger->warn("new redisPool:{0},{1:d},{2:d}", ip, port, connNum);
		if((ip==m_ip)&&(port==m_port)&&(m_conncoter_number==connNum))
        {
            g_worker_logger->warn("redisPool not update");
            return;
        }
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
                g_worker_logger->warn("diff:{0:d},m_redis_client{1:d}", diff, m_redis_client.size());
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
		    g_worker_logger->warn("redisPool_update erase:{0},{1:d},{2:d}", ip, port, connNum);
			connectorPool_earse();
			connectorPool_init(ip, port, connNum);
		}
}
	void redisPoolManager::connectorPool_earse()
	{
	    g_worker_logger->warn("connectorPool_earse");
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
		    g_worker_logger ->info("can not find idle redis from redis pool");
            display();
			return false;
		}
	
		target_result_info target_result;
		if(client->redis_get_target(target_obj,target_result, conver_num)==false)
		{
			g_worker_logger ->info("redis_get_target false");
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
				g_worker_logger ->info("redis_hget_campaign false");
				client->set_redis_status(false);
				return false;
			}
		}
		client->set_redis_status(false);

		return true;	
	}

	bool redisPoolManager::redis_lpush(const char *key, const char* value, int len)
	{
		redisClient *client = NULL;
		try
		{
			if((key == NULL)||(value==NULL)||(len <= 0)) 
			{
				g_worker_logger->error("redis_lpush param error");
				return false;
			}

			client = get_idle_redis();
			if(client==NULL)
			{
				g_worker_logger->error("can not find idle redis from log redis pool");
                display();
				return false;
			}
			int ret = client->redis_lpush(key, value, len);			
	
			client->set_redis_status(false);	
			if(ret ==false)
			{
				g_worker_logger->error("redis_lpush false!!!");
				return false;
			}
		}
		catch(...)
		{
			if(client)
			{
				client->set_redis_status(false);	
			}
			g_worker_logger->error("redis_lpush exception!!");			
		}
		return true;		
	}


	bool redisPoolManager::redis_get_creative_pipe(vector<string>& creatvieIDList, vector<string> &creatvieList)
	{
		redisClient *client = get_idle_redis();
		if(client==NULL)
		{
			g_worker_logger->error("can not find idle redis from redis pool");	
			return false;
		}

		if(client->redis_hget_creative(creatvieIDList, creatvieList) == false)
		{
		    g_worker_logger->error("redis_hget_creative false");
			client->set_redis_status(false);
			return false;
		}
		client->set_redis_status(false);
		return true;	
	}


	redisPoolManager::~redisPoolManager()
	{
		m_redis_pool_lock.destroy();
		connectorPool_earse();
	}

    int redisPoolManager::redisSize()
    {
        int size = -1;
		m_redis_pool_lock.lock();
		size = m_redis_client.size();
		m_redis_pool_lock.unlock();
        return size;
    }

    void redisPoolManager::display()
    {
		m_redis_pool_lock.lock();
        g_worker_logger->info("m_redis_client size:{0:d}", m_redis_client.size());
		for(auto it = m_redis_client.begin(); it != m_redis_client.end();it++)
		{
			redisClient *client =  *it;
            if(client) client->display();
		}
		m_redis_pool_lock.unlock();

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


