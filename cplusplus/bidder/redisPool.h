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
	int *get_array_ptr() const {return array_ptr;}
	int get_array_size() {return array_size;}
	void free_memptr(){delete[] array_ptr;}
	~target_result_info(){}
private:
	int *array_ptr;
	int array_size;
};


class target_result_info1
{
public:
	target_result_info1(){}
	target_result_info1(int* array, int len)
	{
		set(array, len);
	}

	target_result_info1(unique_int_array_ptr& array, int len)
	{
		array_ptr = move(array);
		array_size = len;
	}
	void set(int* array, int len)
	{
		array_ptr.reset(array);
		array_size = len;	
	}
	int *get_array_ptr() const {return array_ptr.get();}

	unique_int_array_ptr &get_array_unique_ptr()  {return array_ptr;}
	int get_array_size() {return array_size;}
	~target_result_info1(){}
private:
	unique_int_array_ptr array_ptr;
	int array_size;
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
		if(id.empty()==false)
		{
			target_infomation *info = target_infomation_new(name, id);
			m_target_geo.push_back(info);
		}
	}
	void add_target_os(const char *name, string &id)
	{
		if(id.empty()==false)
		{
			target_infomation *info = target_infomation_new(name,id);
			m_target_os.push_back(info);
		}
	}
	void add_target_dev(const char *name,string &id)
	{
		if(id.empty()==false)
		{
			target_infomation *info = target_infomation_new(name,id);
			m_target_dev.push_back(info);
		}
	}

	void add_target_signal(const char *name, string &id)
	{
		if(id.empty()==false)
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

	void display(vector<int> listC)
	{
		cout<<"---------------------campaignID begin---------------------"<<endl;
		for(auto it = listC.begin(); it != listC.end(); it++)
		{
			
			cout<<*it<<"   "<<endl;
	  
		}
		cout<<"---------------------campaignID	 end---------------------"<<endl;
	}
	bool redis_hget_campaign(int *array, int cnt, vector<string> &camp)
	{
		if(m_context == NULL)
		{
			cout<<"m_context is null"<<endl;
			return false;
		}	

		int get_num = 0;
		for(int idx = 0; idx < cnt; idx++)
		{
			int id = *(array+idx);
			if(redisAppendCommand(m_context, "hget camp %d", id) != REDIS_OK)
			{
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

	int* target_set_union(int *setA, int setA_number, int *setB, int setB_number, int &result_cnt)
	{
		int sum = setA_number+setB_number;
		if(sum==0) return NULL;
		
		int *oper_result = new int[sum];
		auto it = set_union(setA, setA+setA_number, setB, setB+setB_number, oper_result);
		result_cnt = (it-oper_result);
		return oper_result;
	}

	unique_ptr<int[]> target_set_union1(unique_ptr<int[]> &setA, int setA_number, unique_ptr<int[]>& setB, int setB_number, int &result_cnt)
	{
		int sum = setA_number+setB_number;
		if(sum==0) return NULL;
		
		unique_ptr<int[]> oper_result(new int[sum]);
		int *setA_ptr = setA.get();
		int *setB_ptr = setB.get();
		auto it = set_union(setA_ptr, setA_ptr+setA_number, setB_ptr, setB_ptr+setB_number, oper_result.get());
		result_cnt = (it-oper_result.get());
		return oper_result;
	}
	int* target_set_intersection(int *setA, int setA_number, int *setB, int setB_number, int &result_cnt)
	{
		int sum = setA_number+setB_number;
		if(sum==0) return NULL;
		int *oper_result = new int[sum];
		auto it = set_intersection(setA, setA+setA_number, setB, setB+setB_number, oper_result);
		result_cnt = (it-oper_result);
		return oper_result;
	}

	unique_ptr<int[]> target_set_intersection1(unique_ptr<int[]> &setA, int setA_number, unique_ptr<int[]>& setB, int setB_number, int &result_cnt)
	{
		int sum = setA_number+setB_number;
		if(sum==0) return NULL;
		unique_ptr<int[]> oper_result(new int[sum]);
		int *setA_ptr = setA.get();
		int *setB_ptr = setB.get();
		auto it = set_intersection(setA_ptr, setA_ptr+setA_number, setB_ptr, setB_ptr+setB_number, oper_result.get());
		result_cnt = (it-oper_result.get());
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
		}
		result_cnt = campaignID_number;
		return  int_array;
	}

	
	unique_ptr<int[]> get_campaignID_set1(const byte* byte_array, int byte_cnt, int &result_cnt)
	{
	
		result_cnt = 0;
		if((byte_array == NULL) ||(byte_cnt == 0)) return NULL;
		int campaignID_number = byte_cnt/4;

		unique_ptr<int[]> int_array_ptr(new int[campaignID_number]);
		int *int_array = int_array_ptr.get();
		
		for(int i = 0; i < campaignID_number; i++)
		{
			*(int_array+i) = GET_LONG(byte_array+4*i);
		}
		result_cnt = campaignID_number;
		return  int_array_ptr;
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

	void target_item_insert_sorted(list<target_result_info*> &src_list, int *target_item, int target_item_size)
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
		target_result_info *info = new target_result_info(target_item, target_item_size);
		src_list.insert(it, info);
	}
	
	void target_item_insert_sorted1(list<unique_ptr<target_result_info1> > &src_list, unique_ptr<int[]> &target_item, int target_item_size)
	{	
		auto it  = src_list.begin();
		for(it = src_list.begin(); it != src_list.end(); it++)
		{
			unique_ptr<target_result_info1> &item_ptr = *it;
			target_result_info1 *item = item_ptr.get();
			if(item)
			{
				int size = item->get_array_size();
				if(size>target_item_size) break;
			}
		}

		unique_ptr<target_result_info1> info(new target_result_info1(target_item, target_item_size));
		src_list.insert(it, move(info));
	}


	target_result_info* target_set_convergence(list<target_result_info*>& target_result_list, int convergence_num, bool &all_operation)
	{
		int result_item_size = target_result_list.size();
		target_result_info *target_item =NULL;
		all_operation = false;

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

					cout<<"indx--:"<<index<<"-"<<size<<endl;
					if(size <= convergence_num)
					{
						target_result_info *target = new target_result_info(campaignID_set_ptr, size);
						return target;
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
							min_array_ptr = target_set_intersection(first_array, first_array_len, target_item->get_array_ptr(), target_item->get_array_size(), min_array_size);
							target_result_info *front_item = *it;

							front_item->free_memptr();
							delete front_item;
							target_result_list.erase(it);
							it = target_result_list.begin();
							front_item = *it;
							front_item->free_memptr();
							
							if(front_item)
							{
								front_item->set(min_array_ptr,min_array_size);
							}
							else
							{
								return NULL;
							}
							
							if(min_array_size <= convergence_num)
							{
								if(min_array_ptr)
								{
									target_result_info *target = new target_result_info(min_array_ptr, min_array_size);
									return target;
								}
								else
								{
									return NULL;
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
			return NULL;;
		}
		else if(result_item_size==1)
		{
			all_operation = true;
		}
		
		target_result_info *info = target_result_list.front();
		if(info)
		{
			target_result_info *target = new target_result_info(info->get_array_ptr(), info->get_array_size());
			return target;
		}
		else
		{
			return NULL;
		}
	}
	

/*
  *redis get  target by sets operations
  */
	bool redis_get_target(target_set &target_obj, target_result_info &tar_result_info)
	{
		ostringstream os;
		if(m_context == NULL)
		{
			cout<<"m_context is null"<<endl;
			return false;
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
				return false;
			}
			dev_num++;
			total_num++;
		}

		for(auto it = target_signal_vec.begin(); it != target_signal_vec.end(); it++)
		{
			target_infomation *info = *it;
		//	cout<<"signal:"<< info->get_name()<<":"<<info->get_id()<<endl;
			if(redisAppendCommand(m_context, "hget %s %s", info->get_name().c_str(), info->get_id().c_str()) != REDIS_OK)
			{
				cout<<"redisAppendCommand failure"<<endl;
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
		//	cout<<"idx="<<idx<<endl;
			vector<int> tmp_vec;
			redisReply *ply;
			if(redisGetReply(m_context, (void **) &ply)!= REDIS_OK)
			{
				cout<<"redisGetReply errror!!!"<<endl;
				freeReplyObject(ply);	
				return false;
			}

			if(ply)
			{
				if(ply->type == REDIS_REPLY_STRING)
				{
					int size=0;
					int *campaignID_array = get_campaignID_set((const byte*)ply->str, ply->len, size);	
					
					if(idx <= geo_num)//target Geographic, if geo_num is 0, jump to os infomation
					{
						
						campaignID_set[idx-1] = campaignID_array;
						campaignID_set_size[idx-1] = size;
						if(idx == geo_num)
						{
							int tmpSize;
							int* tmp_result = target_set_union(campaignID_set[0], campaignID_set_size[0], campaignID_set[1], campaignID_set_size[1], tmpSize);
							result_set_calculation = target_set_union(campaignID_set[2], campaignID_set_size[2], tmp_result, tmpSize, result_set_calculation_size);
							free_memory(campaignID_set, sizeof(campaignID_set)/sizeof(int *));
							array_zero(campaignID_set_size, sizeof(campaignID_set_size)/sizeof(int));
							delete[] tmp_result;
							target_item_insert_sorted(target_item_list, result_set_calculation, result_set_calculation_size);
						}
					}
					else if(idx <= (geo_num+os_num))
					{
						int array_index = idx-geo_num-1;
						campaignID_set[array_index] = campaignID_array;
						campaignID_set_size[array_index] = size;
						if(idx == (geo_num+os_num))
						{
							result_set_calculation = target_set_union(campaignID_set[0], campaignID_set_size[0], campaignID_set[1], campaignID_set_size[1], result_set_calculation_size);
							free_memory(campaignID_set, sizeof(campaignID_set)/sizeof(int *));
							array_zero(campaignID_set_size, sizeof(campaignID_set_size)/sizeof(int));
							target_item_insert_sorted(target_item_list, result_set_calculation, result_set_calculation_size);
						}
					}
					else if(idx <= geo_os_dev_count)
					{
						int array_index = idx-(geo_num+os_num)-1;
						campaignID_set[array_index] = campaignID_array;
						campaignID_set_size[array_index] = size;
						if(idx == geo_os_dev_count)
						{
							result_set_calculation = target_set_union(campaignID_set[0], campaignID_set_size[0], campaignID_set[1], campaignID_set_size[1], result_set_calculation_size);
							free_memory(campaignID_set, sizeof(campaignID_set)/sizeof(int *));
							array_zero(campaignID_set_size, sizeof(campaignID_set_size)/sizeof(int));
							target_item_insert_sorted(target_item_list, result_set_calculation, result_set_calculation_size);
						}
					}
					else
					{
						target_item_insert_sorted(target_item_list, campaignID_array, size);						
					}
				}
				freeReplyObject(ply);	
			}
		}
		
		int convergence_num = 90;
		bool all_operation = false;
		target_result_info *target_result = target_set_convergence(target_item_list, convergence_num, all_operation);
		if(target_result)
		{
			cout<<"sss:"<<target_result->get_array_size()<<endl;
			tar_result_info.set(target_result->get_array_ptr(), target_result->get_array_size());
			delete target_result;
		}

		for(auto it = target_item_list.begin(); it != target_item_list.end();)
		{
			target_result_info *info = *it;
			if(info)
			{
				info->free_memptr();
				delete info;
			}
			it = target_item_list.erase(it);
		}
		
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
	bool redis_get_camp_pipe(target_set &target_obj, vector<string> &camp)
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

				vector<int> camp_id_vec;
				vector<string> camp_id_string_vec;
				vector<string> string_vec;
				
				struct timeval tm_start, tm_end;
				gettimeofday(&tm_start, NULL);
				
				target_result_info target_result;
				if(client->redis_get_target(target_obj,target_result)==false)
				{
					cout<<"getall false!!!"<<endl;
					redis_disconnenct = true;
					return false;
				}	
				/*int *camp_id_set = target_result.get_array_ptr();
				int camp_id_set_size = target_result.get_array_size();
				if(camp_id_set)
				{
					if(client->redis_hget_campaign(camp_id_set, target_result.get_array_size(),camp)==false)
					{
						redis_disconnenct = true;
						return false;
					}
				}*/

				/*gettimeofday(&tm_end, NULL);
				cout<<"start:"<<tm_start.tv_sec<<"--"<<tm_start.tv_usec<<endl;
				cout<<"end:"<<tm_end.tv_sec<<"--"<<tm_end.tv_usec<<endl;
				unsigned long long t1 = tm_start.tv_sec*1000*1000+tm_start.tv_usec;
				unsigned long long t2 = tm_end.tv_sec*1000*1000+tm_end.tv_usec;
				long long diff = t2-t1; 
				cout<<"the time of operation:"<<diff<<endl; */

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
