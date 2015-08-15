#include "redisPool.h"

redisClient::redisClient()
{
    m_redisClient_lock.init();
}

redisClient::redisClient(const string &ip, unsigned short port)
{
    set_redis_ipAddr(ip, port);
    m_redisClient_lock.init();
}

void redisClient::set_redis_ipAddr(const string &ip, unsigned short port)
{
    m_redis_ip = ip;
    m_redis_port = port;
}

bool redisClient::redis_connect()
{
    m_context = redisConnect(m_redis_ip.c_str(), m_redis_port);
    if(m_context == NULL) 
    {
        g_worker_logger->error("connectaRedis failure:: context is null");
        return false;
    }
    if(m_context->err)
    {
        redis_free();
        g_worker_logger->error("connectaRedis failure,{0},{1:d}", m_redis_ip, m_redis_port);
        return false;
    }

    g_worker_logger->info("connectaRedis success:{0},{1:d}", m_redis_ip, m_redis_port);
    return true;    
}

bool redisClient::redis_connect(const string &ip, unsigned short port)
{
    set_redis_ipAddr(ip, port);
    return redis_connect();
}


int* redisClient::target_set_union(int *setA, int setA_number, int *setB, int setB_number, int &result_cnt)
{
    int sum = setA_number+setB_number;
    result_cnt=0;
    if(sum==0) return NULL;
        
    int *oper_result = new int[sum];
    auto it = set_union(setA, setA+setA_number, setB, setB+setB_number, oper_result);
    result_cnt = (it-oper_result);
    return oper_result;
}

int* redisClient::target_set_intersection(int *setA, int setA_number, int *setB, int setB_number, int &result_cnt)
{
    int sum = setA_number+setB_number;
    result_cnt = 0;
    if(sum==0) return NULL;
    int *oper_result = new int[sum];
    auto it = set_intersection(setA, setA+setA_number, setB, setB+setB_number, oper_result);
    result_cnt = (it-oper_result);
    return oper_result;
}

void redisClient::target_item_insert_sorted(list<target_result_info*> &src_list, int *target_item, int target_item_size, const char* name)
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

int *redisClient::get_campaignID_set(const byte* byte_array, int byte_cnt, int &result_cnt)
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
    return  int_array;
}


void redisClient::free_memory(int *array[], int array_cnt)
{
    for(int i = 0; i < array_cnt; i++)
    {
        delete array[i];
        array[i]=NULL;
    }
}

void redisClient::array_zero(int array[], int array_cnt)
{
    for(int i = 0; i < array_cnt; i++)
    {
        array[i] = 0;
    }   
}



bool redisClient::target_set_convergence(list<target_result_info*>& target_result_list,operationTarget &target_obj, int convergence_num, target_result_info &target_result)
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
						g_worker_logger->info("target_set_intersection time[{0:d}] result size : {1:d}",index,min_array_size);
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
		
	
bool redisClient::redis_get_target(operationTarget &target_obj, target_result_info &tar_result_info, unsigned short  conver_num)
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
    int app_num = 0;
    int frequency_num = 0;
                
    vector<target_infomation*>& target_geo = target_obj.get_target_geo().get();
    vector<target_infomation*>& target_os = target_obj.get_target_os().get();
    vector<target_infomation*>& target_dev = target_obj.get_target_dev().get();
    vector<target_infomation*>& target_app = target_obj.get_app().get();
    vector<target_infomation*>& target_signal_vec = target_obj.get_target_signal();
    
    
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

     for(auto it = target_app.begin(); it != target_app.end(); it++)
     {
        target_infomation *info = *it;
        if(!info) continue;
        if(redisAppendCommand(m_context, "hget %s %s", info->get_name().c_str(), info->get_id().c_str()) != REDIS_OK)
        {
            redis_free();
            redis_connect();
            return false;
        }
        app_num++;
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
      const int allUnion_cnt = geo_num+os_num+dev_num+app_num;
      int *campaignID_set[3] = {NULL, NULL, NULL};
      int  campaignID_set_size[3] = {0,0,0};
      int *result_set_calculation = NULL;
      int  result_set_calculation_size = 0;
      list<target_result_info*> target_item_list;
                
      for(int idx = 1; idx <= total_num; idx++)
      {
         vector<int> tmp_vec;
         redisReply *ply;
         if(redisGetReply(m_context, (void **) &ply)!= REDIS_OK)
         {
             g_worker_logger->error("redisGetReply errror!!!");
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
                     g_worker_logger->info("target_geo union result size : {0:d}",result_set_calculation_size);
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
                      g_worker_logger->info("target_os union result size : {0:d}",result_set_calculation_size);
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
                       g_worker_logger->info("target_dev union result size : {0:d}",result_set_calculation_size);
                       target_item_insert_sorted(target_item_list, result_set_calculation, result_set_calculation_size, "target_dev");
                    }
              }
              else if(idx <= allUnion_cnt)
              {
                   int array_index = idx-(geo_os_dev_count)-1;
                   campaignID_set[array_index] = campaignID_array;
                   campaignID_set_size[array_index] = size;
                   if(idx == allUnion_cnt)
                   {
                       result_set_calculation = target_set_union(campaignID_set[0], campaignID_set_size[0], campaignID_set[1], campaignID_set_size[1], result_set_calculation_size);//new
                       free_memory(campaignID_set, sizeof(campaignID_set)/sizeof(int *));//free
                       array_zero(campaignID_set_size, sizeof(campaignID_set_size)/sizeof(int));
                       g_worker_logger->info("target_app union result size : {0:d}",result_set_calculation_size);
                       target_item_insert_sorted(target_item_list, result_set_calculation, result_set_calculation_size, "target_app");
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
    target_set_convergence(target_item_list, target_obj, convergence_num, tar_result_info);
    for(auto it = target_item_list.begin(); it != target_item_list.end();)
    {
        target_result_info *info = *it;
        delete info;
        it = target_item_list.erase(it);
    }   
    return true;        
}

        bool redisClient::redis_lpush(const char *key, const char* value, int len)
            {
                if(m_context == NULL)
                {
                    bool ret = redis_connect();
                    if(ret == false)
                    {
                        return false;
                    }
                }
                redisReply *ply= (redisReply *)redisCommand(m_context, "lpush  %s %b", key, value, len);
                if(ply == NULL)
                {
                    redis_free();
                    bool ret = redis_connect();
                    if(ret)
                    {
                        ply= (redisReply *)redisCommand(m_context, "lpush  %s %b", key, value, len);
                        if(ply == NULL)
                        {
                            g_worker_logger->error("redis_lpush redisCommand error");
                            return false;
                        }
                        freeReplyObject(ply);   
                        return true;
                    }
                    g_worker_logger->error("redis_lpush redisCommand error");
                    return false;
                }
                freeReplyObject(ply);   
                return true;        
            }

        bool redisClient::redis_hget_campaign(int *array, int cnt, bufferManager &buf_manager)
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
                        buf_manager.add_campaign(ply->str, ply->len);
                    }
                    freeReplyObject(ply);           
                }
                return true;        
            }

            bool redisClient::redis_hget_creative(vector<string>& creativeIDList, vector<string> &creativeList)
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
                    for(auto it = creativeIDList.begin(); it != creativeIDList.end(); it++)
                    {
                        string& id = *it;
                        if(redisAppendCommand(m_context, "hget tar.index.cre %s", id.c_str()) != REDIS_OK)
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
                            creativeList.push_back(ply->str);
                        }
                        freeReplyObject(ply);           
                    }
                    return true;        
                }

                bool redisClient::get_redis()
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
                
                bool redisClient::set_redis_status(bool use)
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
                bool redisClient::erase_redis_client()
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
                
                void redisClient::redis_free()
                {
                     if(m_context)
                     {
                        redisFree(m_context);
                        m_context = NULL;
                     }
                }
                
                void redisClient::set_stop()
                {
                    m_redisClient_lock.lock();
                    m_stop = true;
                    m_redisClient_lock.unlock();
                
                }
                
                redisClient::~redisClient()
                {
                    redis_free();
                    m_redisClient_lock.destroy();
                }

