#ifndef __REDIS_POOL_H__
#define __REDIS_POOL_H__
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <set>
#include <list>
#include <memory>
#include <pthread.h>
#include <sys/time.h>
#include "hiredis.h"
using namespace std;


typedef unsigned char byte;

#define PUT_LONG(buffer, value) { \
    (buffer) [0] =  (((value) >> 24) & 0xFF); \
    (buffer) [1] =  (((value) >> 16) & 0xFF); \
    (buffer) [2] =  (((value) >> 8)  & 0xFF); \
    (buffer) [3] =  (((value))       & 0xFF); \
    }

#define GET_LONG(buffer) \
      ((byte)(buffer) [0] << 24) \
    + ((byte)(buffer) [1] << 16) \
    + ((byte)(buffer) [2] << 8)  \
    +  (byte)(buffer) [3]


class redisClient
{
public:
	redisClient(){}
	redisClient(string &ip, unsigned short port)
	{
		set_redis_ipAddr(ip, port);
	}
	bool redis_connect()
	{
		m_context = redisConnect(m_redis_ip.c_str(), m_redis_port);
		if(m_context == NULL) 
		{
			cout <<"connectaRedis failure:: context is null" << endl;
			return NULL;
		}
		if(m_context->err)
		{
			redis_free();
			cout <<"connectaRedis failure" << endl;
			return false;
		}

		cout << "connectaRedis success:" <<m_redis_ip<<":"<< m_redis_port<< endl;
		return true;	
	}

	bool redis_connect(const string &ip, unsigned short port)
	{
		set_redis_ipAddr(ip, port);
		return redis_connect();
	}

	bool redis_hset_request(char *key, char *field, char* value, int len )
	{
		if(m_context == NULL)
		{
			cout<<"m_context is null"<<endl;
			return false;
		}	

		redisReply *ply= (redisReply *)redisCommand(m_context, "hset %s %s %b", key, field, value, len);
		if(ply == NULL)
		{
				return false;
		}
		freeReplyObject(ply);	
		return true;		
	}

	bool get_redis_context(redisContext* &content)// get redis content, if content is using ,then return false
	{
		if(m_context_using == true) return false;
		m_context_using = true;
		content = m_context;
		return true;
	}
	redisContext* get_redis(bool &content_is_null)// get redis content, if content is using ,then return false
	{
		content_is_null = false;
		if(m_context==NULL)
		{
			content_is_null = true;
			return NULL;
		}
		if(m_context_using == true) return NULL;
		m_context_using = true;
		return m_context;
	}

	redisContext *get_redis_content() const
	{
		return m_context;
	}
	bool set_redis_context_status(redisContext* context, bool use_con)
	{
		if(context == m_context)
		{
			m_context_using = use_con;
			return true;
		}
		return false;
	}

	void set_redis_ipAddr(const string &ip, unsigned short port)
	{
		m_redis_ip = ip;
		m_redis_port = port;
	}
	void redis_free()
	{
         if(m_context)
		 {
		 	redisFree(m_context);
	     	m_context = NULL;
         }
	}

	~redisClient(){redis_free();}
private:
	bool   m_context_using = false;
	string m_redis_ip;
	unsigned short m_redis_port;
	redisContext* m_context=NULL;
};

//
class redisPool
{
public:
	redisPool()
	{

	}

	
	redisPool(string &ip, unsigned short port, unsigned short connNum)
	{
	    pthread_rwlock_init(&redis_lock, NULL);
		connectorPool_set(ip, port, connNum);
	}

	void connectorPool_init(void)
	{
		pthread_rwlock_init(&redis_lock, NULL);
		for(int i = 0; i < m_conncoter_number; i++)
		{
			redisClient * client = new redisClient(m_ip, m_port);
			client->redis_connect();
			m_redis_client.push_back(client);
		}
	}
	
	void connectorPool_init(const string &ip, unsigned short port, unsigned short connNum)
	{
		connectorPool_set(ip, port, connNum);	
		connectorPool_init();
	}


	void connectorPool_set(const string &ip, unsigned short port, unsigned short connNum)
	{
		m_ip = ip;
		m_port = port;
		m_conncoter_number = connNum;	
	}

	void connectorPool_earse()
	{
		redis_wr_lock();
		cout<<"connectorPool_earse"<<endl;
		for(auto it = m_redis_client.begin(); it != m_redis_client.end();)
		{
			redisClient *client =  *it;
			if(client)
			{
				client->redis_free();
			}
			it = m_redis_client.erase(it);
		}
		redis_rw_unlock();
	}

	void check_redis_pool(redisContext *context)
	{
		redis_wr_lock();
		for(auto it = m_redis_client.begin(); it != m_redis_client.end();it++)
		{
			redisClient *client =  *it;
			if(client==NULL) continue;
			redisContext *redisC = client->get_redis_content();
			if(redisC==NULL|| redisC == context)
			{
				client->redis_free();
				client->redis_connect();
			}
		}
		redis_rw_unlock();
	}

	bool redis_hset_request(char *key, char *field, char* value, int len)
	{
		redisContext *r_context=NULL;
		bool redis_disconnenct = false;
		bool content_is_null=false;
		
		redis_wr_lock();
		for(auto it = m_redis_client.begin(); it != m_redis_client.end();it++)
		{
			redisClient *client =  *it;
			if(client==NULL) continue;
			r_context = client->get_redis(content_is_null);
			if(content_is_null)
			{
				redis_disconnenct = true;
			}
			if(r_context)
			{
				redis_rw_unlock();
				if(client->redis_hset_request(key, field, value, len)==false)
				{
					cout<<"getall false!!!"<<endl;
					redis_disconnenct = true;
					return false;
				}	
				break;
			}	
		}
		
		//redis_client set to can use
		if(r_context)
		{
			redis_wr_lock();
			for(auto it = m_redis_client.begin(); it != m_redis_client.end();it++)
			{
				redisClient *client =  *it; 		
				if(client&&(client->set_redis_context_status(r_context, false)==true))
				{
					break;
				}
			}		
			redis_rw_unlock();
		}
		else
		{
			redis_rw_unlock();
		}

		//if connenct error, then reconnect
		if(redis_disconnenct)
		{
			cout<<"redis_disconnenct"<<endl;
			check_redis_pool(r_context);
		}
		return true;		
	}


	void redis_rd_lock(){ pthread_rwlock_rdlock(&redis_lock);}
	void redis_wr_lock(){ pthread_rwlock_wrlock(&redis_lock);}
	void redis_rw_unlock(){ pthread_rwlock_unlock(&redis_lock);}
	
	~redisPool()
	{
		pthread_rwlock_destroy(&redis_lock);
		connectorPool_earse();
	}
private:
	pthread_rwlock_t redis_lock;
	string m_ip;
	unsigned short m_port;
	unsigned short m_conncoter_number;
	vector<redisClient*> m_redis_client;
};

#endif
