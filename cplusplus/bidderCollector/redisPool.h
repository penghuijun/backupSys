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
#include "lock.h"
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
	redisClient()
	{
		m_redisClient_lock.init();
	}
	redisClient(const string &ip, unsigned short port)
	{
		set_redis_ipAddr(ip, port);
		m_redisClient_lock.init();
	}
	
	void set_redis_ipAddr(const string &ip, unsigned short port)
	{
		m_redis_ip = ip;
		m_redis_port = port;
	}

	bool redis_connect()
	{
		m_context = redisConnect(m_redis_ip.c_str(), m_redis_port);
		if(m_context == NULL) 
		{
			cerr <<"connectaRedis failure:: context is null" << endl;
			return false;
		}
		if(m_context->err)
		{
			redis_free();
			cerr <<"connectaRedis failure" << endl;
			return false;
		}

		cerr << "connectaRedis success:" <<m_redis_ip<<":"<< m_redis_port<< endl;
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
			bool ret = redis_connect();
			if(ret==false)
			{
				return false;
			}
		}
		redisReply *ply= (redisReply *)redisCommand(m_context, "hset %s %s %b", key, field, value, len);
		if(ply == NULL)
		{
			redis_free();
			bool ret = redis_connect();
			if(ret)
			{
				ply= (redisReply *)redisCommand(m_context, "hset %s %s %b", key, field, value, len);
				if(ply == NULL)
				{
					cerr<<"redis_hset_request redisCommand error"<<endl;
					return false;
				}
				freeReplyObject(ply);	
				return true;
			}
			cerr<<"redis_hset_request redisCommand error"<<endl;
			return false;
		}
		freeReplyObject(ply);	
		return true;		
	}

	bool get_redis()
	{
		m_redisClient_lock.lock();
		if(m_context_using==true)
		{
			m_redisClient_lock.unlock();
			return false;
		}
		m_context_using = true;
		m_redisClient_lock.unlock();
		return true;
	}
	
	bool set_redis_status(bool use)
	{
		m_redisClient_lock.lock();
		m_context_using=use;
		if((m_context_using==false)&&(m_stop))
		{
			redis_free();
			delete this;
		}
		m_redisClient_lock.unlock();
	}
	bool erase_redis_client()
	{
		m_redisClient_lock.lock();
		if(m_context_using==false)
		{
			redis_free();
			m_stop = false;
			m_redisClient_lock.unlock();
			return true;
		}
		m_redisClient_lock.unlock();		
		return false;
	}

	void redis_free()
	{
         if(m_context)
		 {
		 	redisFree(m_context);
	     	m_context = NULL;
         }
	}

	void set_stop()
	{
		m_redisClient_lock.lock();
		m_stop = true;
		m_redisClient_lock.unlock();

	}

	~redisClient()
	{
		redis_free();
		m_redisClient_lock.destroy();
	}
private:
	bool           m_stop = false;
	bool           m_context_using = false;
	string         m_redis_ip;
	unsigned short m_redis_port;
	redisContext*  m_context=NULL;
	mutex_lock     m_redisClient_lock;
};

//
#endif
