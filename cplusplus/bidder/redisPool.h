#ifndef __REDIS_POOL_H__
#define __REDIS_POOL_H__
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string.h>
#include <set>
#include <list>
#include <memory>
#include <algorithm>
#include <pthread.h>
#include <sys/time.h>
#include "hiredis.h"
#include "lock.h"
#include "spdlog/spdlog.h"
using namespace std;
extern shared_ptr<spdlog::logger> g_worker_logger;


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

#define unique_int_array_ptr unique_ptr<int[]>
class stringBuffer
{
public:
	stringBuffer(){}
	stringBuffer(char* data, int dataLen)
	{
		string_new(data, dataLen);
	}

	void string_set(char* data, int dataLen)
	{
		m_data = data;
		m_dataLen = dataLen;
	}
	void string_new(char* data, int dataLen)
	{
		if(dataLen > 0)
		{
			m_data = new char[dataLen];
			memcpy(m_data, data, dataLen);
			m_dataLen = dataLen;
		}
	}
	char *get_data(){return m_data;}
	int   get_dataLen(){return m_dataLen;}
	~stringBuffer()
	{
		delete[] m_data;
		m_data = NULL;
		m_dataLen = 0;
	}
private:
	char *m_data=NULL;
	int   m_dataLen=0;
};

class bufferManager
{
public:
	bufferManager(){}
	void add_campaign(char *data, int dataLen)
	{
		stringBuffer *buf = new stringBuffer(data, dataLen);
		m_bufferList.push_back(buf);
	}
    vector<stringBuffer*>& get_bufferList(){return m_bufferList;}	
	int bufferListSize(){return m_bufferList.size();}
	~bufferManager()
	{
		for(auto it = m_bufferList.begin(); it != m_bufferList.end();)
		{
			delete *it;
			it = m_bufferList.erase(it);
		}
	}
	
private:
	vector<stringBuffer*> m_bufferList;
};

class target_result_info
{
public:
	target_result_info(){}
	target_result_info(int* array, int len)
	{
		set(array, len);
	}
	void set(int* array, int len)
	{
		array_ptr = array;
		array_size = len;	
	}

	target_result_info(int* array, int len, const char* name)
	{
		set(array, len, name);
	}
	void set(int* array, int len, const char* name)
	{
		array_ptr = array;
		array_size = len;	
		m_name = name;
	}


	void target_result_new(int *array, int len)
	{
		array_size = len;
		array_ptr = new int[len];
		memcpy(array_ptr, array, len*sizeof(int));
	}
	int *get_array_ptr() {return array_ptr;}
	int get_array_size() {return array_size;}
	string& get_name(){return m_name;}
	void free_memptr()
	{
		delete[] array_ptr;
		m_name.clear();
	}
	~target_result_info()
	{
		free_memptr();
	}
private:
	int *array_ptr=NULL;
	int array_size=0;
	string m_name;
};


class target_infomation
{
public:
	target_infomation(){}
	target_infomation(string &name, string &id)
	{
		set(name, id);
	}
	target_infomation(const char*name, string &id)
	{
		set(name, id);
	}
	void set(const string &name,const string &id)
	{
		m_name = name;
		m_id = id;
	}
	void set(const char *name,const string &id)
	{
		m_name = name;
		m_id = id;
	}
	string &get_name(){return m_name;}
	string &get_id(){return m_id;}
	~target_infomation(){}
private:
	string m_name;
	string m_id;
	
};

class unionTarget
{
public:
	unionTarget(){}
	void display(shared_ptr<spdlog::logger>& file_logger)
	{
		for(auto it = m_target_union.begin(); it != m_target_union.end(); it++)
		{
			target_infomation* info = *it;
			if(info)
			{
				file_logger->debug("{0}:{1}", info->get_name(), info->get_id());
			}
		}
	}

	bool is_number(const string& str)
	{
		const char *id_str = str.c_str();
		size_t id_size = str.size();
		if(id_size==0) return false;
		for(int i=0; i < id_size; i++)
		{
			char ch = *(id_str+i);
			if(ch<'0'||ch>'9') return false;
		}
		return true;
	}
	vector<target_infomation*>& get(){return m_target_union;}
	void erase()
	{
		for(auto it = m_target_union.begin(); it != m_target_union.end();)
		{
			delete *it;
			it = m_target_union.erase(it);
		}
	}

	void add(const char *name,const string &id)
	{
		if((id.empty()==false)&&is_number(id))
		{
			target_infomation *target = new target_infomation;
			target->set(name,id);
			m_target_union.push_back(target);
		}
	}

	int redis_append_command(redisContext* context)
	{
		int cnt = 0;
		for(auto it = m_target_union.begin(); it != m_target_union.end(); it++)
		{
		   target_infomation *info = *it;
		   if(!info) continue;
		   if(redisAppendCommand(context, "hget %s %s", info->get_name().c_str(), info->get_id().c_str()) != REDIS_OK)
		   {
			   return -1;
		   }
		   cnt++;
		}
		return cnt;
	}
	
	~unionTarget()
	{
		for(auto it = m_target_union.begin(); it != m_target_union.end();)
		{
			delete *it;
			it = m_target_union.erase(it);
		}
	}
private:
	vector<target_infomation*> m_target_union;
};


class operationTarget
{
public:
	operationTarget(){}

	target_infomation *target_infomation_new(const char *name,const string &id)
	{
		target_infomation *geo_info = new target_infomation;
		geo_info->set(name,id);
		return geo_info;
	}
	void add_target_geo(const char *name,const string &id)
	{
		m_target_geo.add(name, id);
	}
	void add_target_os(const char *name,const string &id)
	{
		m_target_os.add(name, id);
	}
	void add_target_dev(const char *name,const string &id)
	{
		m_target_dev.add( name, id);
	}

	void add_target_app(const char *name,const string &id)
	{
		m_target_app.add(name, id);
	}
	
	void add_target_signal(const char *name,const string &id)
	{
		if((id.empty()==false)&&is_number(id))
		{
			target_infomation *info = target_infomation_new(name, id);
			m_target_signal.push_back(info);
		}
	}

	unionTarget&                get_target_geo(){return m_target_geo;}
    unionTarget&                get_target_os(){return m_target_os;}
	unionTarget&                get_target_dev(){return m_target_dev;}
	unionTarget&                get_app(){return m_target_app;}
	vector<target_infomation*>& get_target_signal(){return m_target_signal;}


	void display(vector<target_infomation*> & listC, shared_ptr<spdlog::logger>& file_logger)
	{
		for(auto it = listC.begin(); it != listC.end(); it++)
		{
			target_infomation* info = *it;
			if(info)
			{
				file_logger->debug("{0}:{1}", info->get_name(), info->get_id());
			}
		}
	}

	void erase_target_origin(string &name)
	{
		if(name.empty()) return;
		if(name=="target_geo")
		{
			m_target_geo.erase();			
		}
		else if(name == "target_os")
		{
			m_target_os.erase();
		}
		else if(name == "target_dev")
		{
			m_target_dev.erase();
		}
		else if(name == "target_app")
		{
			m_target_app.erase();
		}
		else
		{
			for(auto it=m_target_signal.begin(); it != m_target_signal.end(); it++)
			{
				target_infomation* info=  *it;
				if(info)
				{
					string g_name = info->get_name();
					if(g_name==name)
					{
						delete info;
						m_target_signal.erase(it);
						return;
					}
				}
			}
		}
	}

	void display(shared_ptr<spdlog::logger>& file_logger)
	{
		m_target_geo.display(file_logger);
		m_target_os.display(file_logger);
		m_target_dev.display(file_logger);
		m_target_app.display(file_logger);
		display(m_target_signal, file_logger);
	}

	bool is_number(const string& str)
	{
		const char *id_str = str.c_str();
		size_t id_size = str.size();
		if(id_size==0) return false;
		for(int i=0; i < id_size; i++)
		{
			char ch = *(id_str+i);
			if(ch<'0'||ch>'9') return false;
		}
		return true;
	}
	
	~operationTarget()
	{
		erase_target_vector(m_target_signal);
	}
	
	
private:
	void erase_target_vector(vector<target_infomation*> &src_vec)
	{
		for(auto it = src_vec.begin(); it != src_vec.end();)
		{
			delete (*it);
			it=src_vec.erase(it);
		}	
	}
	
	unionTarget                m_target_geo;
	unionTarget                m_target_os;
	unionTarget                m_target_dev;
	unionTarget				   m_target_app;
	vector<target_infomation*> m_target_signal;
};

class redisClient
{
public:
	redisClient();
	redisClient(const string &ip, unsigned short port);
	void set_redis_ipAddr(const string &ip, unsigned short port);
	bool redis_connect();
	bool redis_connect(const string &ip, unsigned short port);
	int* target_set_union(int *setA, int setA_number, int *setB, int setB_number, int &result_cnt);
	int* target_set_intersection(int *setA, int setA_number, int *setB, int setB_number, int &result_cnt);
	int *get_campaignID_set(const byte* byte_array, int byte_cnt, int &result_cnt);
		
	void free_memory(int *array[], int array_cnt);
	void array_zero(int array[], int array_cnt);
	void target_item_insert_sorted(list<target_result_info*> &src_list, int *target_item, int target_item_size, const char* name);
	bool target_set_convergence(list<target_result_info*>& target_result_list,operationTarget &target_obj, int convergence_num, target_result_info &target_result);
	/*
	  *redis get  target by sets operations
	  */
	bool redis_get_target(operationTarget &target_obj, target_result_info &tar_result_info, unsigned short  conver_num);
	bool redis_hget_campaign(int *array, int cnt, bufferManager &buf_manager);
	bool redis_hget_creative(vector<string>& creativeIDList, vector<string> &creativeList);
	bool redis_lpush(const char *key, const char* value, int len);
	bool get_redis();
	bool set_redis_status(bool use);
	bool erase_redis_client();
	void redis_free();
	void set_stop();
	void display()
	{
		g_worker_logger->info("ip:port:using:stop:contenxt--{0},{1:d},{2:d},{3:d},{4}"
			, m_redis_ip, m_redis_port, (int) m_context_using, (int) m_stop, (m_context!=NULL)?"true":"false");
	}
	~redisClient();
private:
	bool           m_stop = false;
	bool           m_context_using = false;
	string         m_redis_ip;
	unsigned short m_redis_port;
	redisContext*  m_context=NULL;
	mutex_lock     m_redisClient_lock;
};



#endif
