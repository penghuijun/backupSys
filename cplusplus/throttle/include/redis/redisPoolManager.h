#ifndef __REDISPOOL_MANAGER_H__
#define __REDISPOOL_MANAGER_H__

#include "redisPool.h"
class redisPoolManager
{
public:
	redisPoolManager()
	{
		m_redis_pool_lock.init();
	}

	redisPoolManager(const string &ip, unsigned short port, unsigned short connNum)
	{
	    m_redis_pool_lock.init();
		connectorPool_set(ip, port, connNum);
	}
	
	void connectorPool_set(const string &ip, unsigned short port, unsigned short connNum)
	{
		m_ip = ip;
		m_port = port;
		m_conncoter_number = connNum;	
	}

	void connectorPool_init(void)
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
	
	void connectorPool_init(const string &ip, unsigned short port, unsigned short connNum)
	{
		connectorPool_set(ip, port, connNum);	
		connectorPool_init();
	}

	void redisPool_update(const string &ip, unsigned short port, unsigned short connNum)
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
	void connectorPool_earse()
	{
		g_manager_logger->info("connectorPool_earse");
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

	bool redis_hset_request(const char *key, const char *field,const char* value, int len, 	unsigned short expireTime)
	{
		redisClient *client = NULL;
		try
		{
			if((key == NULL)||(field == NULL)||(value==NULL)||(len <= 0)) 
			{
				g_manager_logger->error("redis_hset_request param error");
				return false;
			}

			client = get_idle_redis();
			if(client==NULL)
			{
				g_manager_logger->error("can not find idle redis from redis pool");
				return false;
			}
			int ret = client->redis_hset_request_expire(key, field, value, len, expireTime);			

			client->set_redis_status(false);	
			if(ret ==false)
			{
				g_manager_logger->error("redis_hset_request false!!!");
				return false;
			}
		}
		catch(...)
		{
			if(client)
			{
				client->set_redis_status(false);	
			}
			g_manager_logger->error("redis_hset_request exception!!");			
		}
		return true;		
	}

	bool redis_lpush(const char *key, const char* value, int len)
	{
		redisClient *client = NULL;
		try
		{
			if((key == NULL)||(value==NULL)||(len <= 0)) 
			{
				g_manager_logger->error("redis_lpush param error");
				return false;
			}

			client = get_idle_redis();
			if(client==NULL)
			{
				g_manager_logger->error("can not find idle redis from log redis pool");
				return false;
			}
			int ret = client->redis_lpush(key, value, len);			
	
			client->set_redis_status(false);	
			if(ret ==false)
			{
				g_manager_logger->error("redis_lpush false!!!");
				return false;
			}
		}
		catch(...)
		{
			if(client)
			{
				client->set_redis_status(false);	
			}
			g_manager_logger->error("redis_lpush exception!!");			
		}
		return true;		
	}

	
	~redisPoolManager()
	{
		m_redis_pool_lock.destroy();
		connectorPool_earse();
	}
private:
	redisClient *get_idle_redis()
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
	
	mutex_lock     m_redis_pool_lock;
	string         m_ip;
	unsigned short m_port;
	unsigned short m_conncoter_number=0;
	vector<redisClient*> m_redis_client;
};

#endif
