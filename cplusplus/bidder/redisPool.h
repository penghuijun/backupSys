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

#define unique_int_array_ptr unique_ptr<int[]>

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
	void set(string &name, string &id)
	{
		m_name = name;
		m_id = id;
	}
	void set(const char *name, string &id)
	{
		m_name = name;
		m_id = id;
	}
	string get_name(){return m_name;}
	string get_id(){return m_id;}
	~target_infomation(){}
private:
	string m_name;
	string m_id;
	
};

class target_set
{
public:
	target_set(){}

	target_infomation *target_infomation_new(const char *name, string &id)
	{
		target_infomation *geo_info = new target_infomation;
		geo_info->set(name,id);
		return geo_info;
	}
	void add_target_geo(const char *name, string &id)
	{
		if((id.empty()==false)&&is_number(id))
		{
			target_infomation *info = target_infomation_new(name, id);
			m_target_geo.push_back(info);
		}
	}
	void add_target_os(const char *name, string &id)
	{
		if((id.empty()==false)&&is_number(id))
		{
			target_infomation *info = target_infomation_new(name,id);
			m_target_os.push_back(info);
		}
	}
	void add_target_dev(const char *name,string &id)
	{
		if((id.empty()==false)&&is_number(id))
		{
			target_infomation *info = target_infomation_new(name,id);
			m_target_dev.push_back(info);
		}
	}

	void add_target_signal(const char *name, string &id)
	{
		if((id.empty()==false)&&is_number(id))
		{
			target_infomation *info = target_infomation_new(name, id);
			m_target_signal.push_back(info);
		}
	}

	vector<target_infomation*>& get_target_geo(){return m_target_geo;}
	vector<target_infomation*>& get_target_os(){return m_target_os;}
	vector<target_infomation*>& get_target_dev(){return m_target_dev;}
	vector<target_infomation*>& get_target_signal(){return m_target_signal;}
	void erase_target()
	{
		erase_target_vector(m_target_geo);
		erase_target_vector(m_target_os);
		erase_target_vector(m_target_dev);
		erase_target_vector(m_target_signal);
	}

	void display(vector<target_infomation*> & listC)
	{
		for(auto it = listC.begin(); it != listC.end(); it++)
		{
			target_infomation* info = *it;
			if(info)
			{
				cout<<info->get_name()<<":"<<info->get_id()<<endl;
			}
		}
	}

	void erase_target_origin(string &name)
	{
		if(name.empty()) return;
#ifdef DEBUG
//		cout<<"erase  name:"<<name<<endl;
#endif
		if(name=="target_geo")
		{
			erase_target_vector(m_target_geo);			
		}
		else if(name == "target_os")
		{
			erase_target_vector(m_target_os);
		}
		else if(name == "target_dev")
		{
			erase_target_vector(m_target_dev);
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

	void display()
	{
		cout<<"display start:"<<endl;
		display(m_target_geo);
		display(m_target_os);
		display(m_target_dev);
		display(m_target_signal);
		cout<<"display end:"<<endl;
	}

	bool is_number(string& str)
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
	
	~target_set()
	{
		erase_target();
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
	
	vector<target_infomation*> m_target_geo;
	vector<target_infomation*> m_target_os;
	vector<target_infomation*> m_target_dev;
	vector<target_infomation*> m_target_signal;
};

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
	void display(vector<int> listC)
	{
		cout<<"---------------------campaignID begin---------------------"<<endl;
		for(auto it = listC.begin(); it != listC.end(); it++)
		{
			cout<<*it<<"   "<<endl;
		}
		cout<<"---------------------campaignID	 end---------------------"<<endl;
	}
	
	int* target_set_union(int *setA, int setA_number, int *setB, int setB_number, int &result_cnt)
	{
		int sum = setA_number+setB_number;
		result_cnt=0;
		if(sum==0) return NULL;
			
		int *oper_result = new int[sum];
		auto it = set_union(setA, setA+setA_number, setB, setB+setB_number, oper_result);
		result_cnt = (it-oper_result);
		return oper_result;
	}
	
	int* target_set_intersection(int *setA, int setA_number, int *setB, int setB_number, int &result_cnt)
	{
		int sum = setA_number+setB_number;
		result_cnt = 0;
		if(sum==0) return NULL;
		int *oper_result = new int[sum];
		auto it = set_intersection(setA, setA+setA_number, setB, setB+setB_number, oper_result);
		result_cnt = (it-oper_result);
		return oper_result;
	}
	
	int *get_campaignID_set(const byte* byte_array, int byte_cnt, int &result_cnt)
	{
		result_cnt = 0;
		if((byte_array == NULL) ||(byte_cnt == 0)) return NULL;
		int campaignID_number = byte_cnt/4;
		int *int_array = new int[campaignID_number];
			
		for(int i = 0; i < campaignID_number; i++)
		{
			*(int_array+i) = GET_LONG(byte_array+4*i);
			//cout<<*(int_array+i)<<endl;
		}
		result_cnt = campaignID_number;
		return	int_array;
	}
		
	void free_memory(int *array[], int array_cnt)
	{
		for(int i = 0; i < array_cnt; i++)
		{
			delete array[i];
			array[i]=NULL;
		}
	}
	
	void array_zero(int array[], int array_cnt)
	{
		for(int i = 0; i < array_cnt; i++)
		{
			array[i] = 0;
		}	
	}
	
	void display(int *array, int cnt)
	{
		for(int i = 0; i < cnt;  i++)
		{
			cout<< *(array + i) <<" ";
			if(i!=0&&(i%50==0))
				cout <<endl;
		}
		cout<<endl;
	}

	
	void target_item_insert_sorted(list<target_result_info*> &src_list, int *target_item, int target_item_size, const char* name)
	{	
		auto it  = src_list.begin();
		for(it = src_list.begin(); it != src_list.end(); it++)
		{
			target_result_info *item = *it;
			if(item)
			{
				int size = item->get_array_size();
				if(size>target_item_size) break;
			}
		}
		target_result_info *info = new target_result_info(target_item, target_item_size, name);
		src_list.insert(it, info);
	}
		
	
	bool target_set_convergence(list<target_result_info*>& target_result_list,target_set &target_obj, int convergence_num, target_result_info &target_result)
	{
		int result_item_size = target_result_list.size();
		target_result_info *target_item =NULL;
	
		if(result_item_size > 1)
		{
			int index = 0;
			int* min_array_ptr = NULL;
			int min_array_size = 0;
			int* first_array = NULL;
			int first_array_len = 0;
				
			for(auto it = target_result_list.begin(); it != target_result_list.end();it++,index++)
			{
				target_item = *it;
				if(target_item)
				{
					int *campaignID_set_ptr = target_item->get_array_ptr();
					int  size = target_item->get_array_size();
	
						//cout<<"indx--:"<<index<<"-"<<size<<endl;
					if(size <= convergence_num)
					{
						target_obj.erase_target_origin(target_item->get_name());
						target_result.target_result_new(campaignID_set_ptr, size);
						return true;
					}
					else
					{
						if(index == 0)
						{
							first_array = target_item->get_array_ptr();
							first_array_len = target_item->get_array_size();							
						}
						else
						{
							min_array_ptr = target_set_intersection(first_array, first_array_len, target_item->get_array_ptr(), target_item->get_array_size(), min_array_size);//new
	
								//free node1 and node2, erase node1, add intersection result to node2, so as a word, erase the origin node, insert the result
							target_result_info *front_item = *it;
							target_obj.erase_target_origin(front_item->get_name());
							delete front_item;
							target_result_list.erase(it);
						
							it = target_result_list.begin();
							if(it == target_result_list.end()) 
							{
								delete[] min_array_ptr;
								return false;
							}
							front_item = *it;
							target_obj.erase_target_origin(front_item->get_name());
							front_item->free_memptr();
								
							if(front_item)
							{
								front_item->set(min_array_ptr,min_array_size);
							}
							else
							{
								delete[] min_array_ptr;
								return false;
							}
								
							if(min_array_size <= convergence_num)
							{
								if(min_array_ptr)
								{
									target_result.target_result_new(min_array_ptr, min_array_size);
									return true;
								}
								else
								{
									delete[] min_array_ptr;
									return false;
								}
	
							}
							else
							{
								first_array = min_array_ptr;
								first_array_len = min_array_size;
							}
						}
					}
				}
			}
		}
	
		result_item_size = target_result_list.size();
		if(result_item_size==0)
		{
			return false;;
		}
	
			
		target_result_info *info = target_result_list.front();
		if(info)
		{
			target_result.target_result_new(info->get_array_ptr(), info->get_array_size());
			return true;
		}
		else
		{
			return false;
		}
	}
		
	
	/*
	  *redis get  target by sets operations
	  */
	bool redis_get_target(target_set &target_obj, target_result_info &tar_result_info, unsigned short  conver_num)
	{
		ostringstream os;
		if(m_context == NULL)
		{
			bool ret = redis_connect();
			if(ret==false)
			{
				return false;
			}
		}	
			
		int total_num = 0;
		int geo_num = 0;
		int os_num = 0;
		int dev_num = 0;
		int frequency_num = 0;
			
		vector<target_infomation*>& target_geo = target_obj.get_target_geo();
		vector<target_infomation*>& target_os = target_obj.get_target_os();
		vector<target_infomation*>& target_dev = target_obj.get_target_dev();
		vector<target_infomation*> target_signal_vec = target_obj.get_target_signal();
	
		for(auto it = target_geo.begin(); it != target_geo.end(); it++)
		{
			target_infomation *info = *it;
			if(!info) continue;
			if(redisAppendCommand(m_context, "hget %s %s", info->get_name().c_str(), info->get_id().c_str()) != REDIS_OK)
			{
				redis_free();
				redis_connect();
				return false;
			}
			geo_num++;
			total_num++;
		}
		for(auto it = target_os.begin(); it != target_os.end(); it++)
		{
			target_infomation *info = *it;
			if(!info) continue;
			if(redisAppendCommand(m_context, "hget %s %s", info->get_name().c_str(), info->get_id().c_str()) != REDIS_OK)
			{
				redis_free();
				redis_connect();
				return false;
			}
			os_num++;
			total_num++;
		}
		for(auto it = target_dev.begin(); it != target_dev.end(); it++)
		{
			target_infomation *info = *it;
			if(!info) continue;
			if(redisAppendCommand(m_context, "hget %s %s", info->get_name().c_str(), info->get_id().c_str()) != REDIS_OK)
			{
				redis_free();
				redis_connect();
				return false;
			}
			dev_num++;
			total_num++;
		}
	
		for(auto it = target_signal_vec.begin(); it != target_signal_vec.end(); it++)
		{
			target_infomation *info = *it;
			if(redisAppendCommand(m_context, "hget %s %s", info->get_name().c_str(), info->get_id().c_str()) != REDIS_OK)
			{
				redis_free();
				redis_connect();
				return false;
			}
			total_num++;
		}
			
		const int geo_os_dev_count = geo_num+os_num+dev_num;
		int *campaignID_set[3] = {NULL, NULL, NULL};
		int  campaignID_set_size[3] = {0,0,0};
		int *result_set_calculation = NULL;
		int  result_set_calculation_size = 0;
		list<target_result_info*> target_item_list;
			
		for(int idx = 1; idx <= total_num; idx++)
		{
		//		cerr<<"idx="<<idx<<endl;
			vector<int> tmp_vec;
			redisReply *ply;
			if(redisGetReply(m_context, (void **) &ply)!= REDIS_OK)
			{
				cerr<<"redisGetReply errror!!!"<<endl;
				freeReplyObject(ply);	
				redis_free();
				redis_connect();
				return false;
			}
	
			if(ply)
			{
				int size = 0;
				int *campaignID_array = NULL;
				if(ply->type == REDIS_REPLY_STRING)
				{
					campaignID_array = get_campaignID_set((const byte*)ply->str, ply->len, size);//new	
				//		cerr<<"size:"<< size<<endl;
				}
					
				if(idx <= geo_num)//target Geographic, if geo_num is 0, jump to os infomation
				{
					campaignID_set[idx-1] = campaignID_array;
					campaignID_set_size[idx-1] = size;
					if(idx == geo_num)
					{
						int tmpSize;
						int* tmp_result = target_set_union(campaignID_set[0], campaignID_set_size[0], campaignID_set[1], campaignID_set_size[1], tmpSize);//new
						result_set_calculation = target_set_union(campaignID_set[2], campaignID_set_size[2], tmp_result, tmpSize, result_set_calculation_size);//new
						free_memory(campaignID_set, sizeof(campaignID_set)/sizeof(int *));//free
						array_zero(campaignID_set_size, sizeof(campaignID_set_size)/sizeof(int));
						delete[] tmp_result;//free
						target_item_insert_sorted(target_item_list, result_set_calculation, result_set_calculation_size, "target_geo");
					}
				}
				else if(idx <= (geo_num+os_num))
				{
					int array_index = idx-geo_num-1;
					campaignID_set[array_index] = campaignID_array;
					campaignID_set_size[array_index] = size;
					if(idx == (geo_num+os_num))
					{
						result_set_calculation = target_set_union(campaignID_set[0], campaignID_set_size[0], campaignID_set[1], campaignID_set_size[1], result_set_calculation_size);//new
						free_memory(campaignID_set, sizeof(campaignID_set)/sizeof(int *));//free
						array_zero(campaignID_set_size, sizeof(campaignID_set_size)/sizeof(int));
						target_item_insert_sorted(target_item_list, result_set_calculation, result_set_calculation_size, "target_os");
					}
				}
				else if(idx <= geo_os_dev_count)
				{
					int array_index = idx-(geo_num+os_num)-1;
					campaignID_set[array_index] = campaignID_array;
					campaignID_set_size[array_index] = size;
					if(idx == geo_os_dev_count)
					{
						result_set_calculation = target_set_union(campaignID_set[0], campaignID_set_size[0], campaignID_set[1], campaignID_set_size[1], result_set_calculation_size);//new
						free_memory(campaignID_set, sizeof(campaignID_set)/sizeof(int *));//free
						array_zero(campaignID_set_size, sizeof(campaignID_set_size)/sizeof(int));
						target_item_insert_sorted(target_item_list, result_set_calculation, result_set_calculation_size, "target_dev");
					}
				}
				else//signal
				{
					int signal_idx = (idx -geo_os_dev_count-1);
					int i=0;
					for(auto it = target_signal_vec.begin(); it != target_signal_vec.end(); it++, i++)
					{
						if(i==signal_idx)
						{
							target_infomation *info=*it;
							if(info)
							{
								target_item_insert_sorted(target_item_list, campaignID_array, size, info->get_name().c_str());						
							}
						}
					}
				}
				
				freeReplyObject(ply);	
			}
		}
		int convergence_num = conver_num;
		target_set_convergence(target_item_list, target_obj,convergence_num, tar_result_info);
		for(auto it = target_item_list.begin(); it != target_item_list.end();)
		{
			target_result_info *info = *it;
			delete info;
			it = target_item_list.erase(it);
		}	
		return true;		
	}


	bool redis_hget_campaign(int *array, int cnt, vector<string> &camp)
	{
		if(m_context == NULL)
		{
			bool ret = redis_connect();
			if(ret==false)
			{
				return false;
			}
		}	
	
		
		int get_num = 0;
		for(int idx = 0; idx < cnt; idx++)
		{
			int id = *(array+idx);
			if(redisAppendCommand(m_context, "hget camp %d", id) != REDIS_OK)
			{
				redis_free();
				redis_connect();
				return false;
			}
			get_num++;
		}
	
		for(int idx = 0; idx < get_num; idx++)
		{
			redisReply *ply;
			if(redisGetReply(m_context, (void **) &ply)!= REDIS_OK)
			{
				freeReplyObject(ply);
				redis_free();
				redis_connect();
				return false;
			}
				
			if(ply&&(ply->type == REDIS_REPLY_STRING))
			{
				camp.push_back(ply->str);
			}
			freeReplyObject(ply);			
		}
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



#endif
