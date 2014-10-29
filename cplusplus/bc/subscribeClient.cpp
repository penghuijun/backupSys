/*
*/
#include <iostream>
#include <pthread.h>
#include <string>
#include <map>
#include <set>
#include <utility>
#include <unistd.h>
#include <time.h>
#include "hiredis.h"

#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"

#include "zeroMqServer.h"
#include"subscribeClient.h"


using namespace std;
using namespace com::rj::protos::msg;

extern pthread_mutex_t redisCmdMutex;
extern pthread_mutex_t bcDataMutex;
map<string, bcDataRec*> bcDataRecList;

subscribeClient::subscribeClient(const char *ip, unsigned short port, const char *str, const char *redisIP, unsigned short redisPort):redisClient(ip, port), m_subStr(str)
{
    m_saveDataRedisIP = redisIP;
    m_saveDataRedisPort = redisPort;
}


subscribeClient::subscribeClient(string ip, unsigned short port, string str, string redisIP, unsigned short redisPort):redisClient(ip, port), m_subStr(str)
{
    m_saveDataRedisIP = redisIP;
    m_saveDataRedisPort = redisPort;
}


int subNum = 0;
int ppuidNum = 0;
int uuidsame = 0;
int subHset = 0;
extern int threeType;

void subscribeClient::start()
{
    bool ret = false;
    int index = 0;
    do
    {
        ret = connectaRedis();
        if(ret == true)
        {
            redisReply *rpy = (redisReply*) redisCommand(get_redisContex(), "subscribe %s", m_subStr.c_str());
            if(rpy == NULL || rpy->type != REDIS_REPLY_ARRAY) 
            {
                ret = false;
            }
            else
            {
               
                for(index= 0; index< rpy->elements; index++)
                {
                    if(index%3==1)//3
                    {
                        redisReply *childRpy = (redisReply *)rpy->element[index];
                        if(childRpy==NULL || childRpy->type != REDIS_REPLY_STRING)
                        {
                            continue;
                        }
                
                        string str1 = childRpy->str;
                        if(str1.compare(m_subStr))
                        {   
                            continue;
                        }
                        else
                        {
                            cout << "subscribe " << str1 << endl;
                            break;
                        }
                    }
                   
                }//for(i = 0; i < rpy->elements; i++)
                
                if(index == rpy->elements) 
                { 
                    ret = false;
                }  
            }//rpy == NULL
            if(rpy != nullptr) freeReplyObject(rpy);
            if(ret  == false)
            {
      //          sleep(1);
                freeContext();
            }
        }//ret = true;
    }while(ret == false);           
}


void subscribeClient::run()
{
    redisClient redisC(m_saveDataRedisIP, m_saveDataRedisPort);
    redisC.run();
    int i;

    while(1)
    {
        start();
	    while(1)
	    {
	    try{
    		redisReply *reply;
	    	int ret = redisGetReply(get_redisContex(), (void **)&reply);
    		if(ret != REDIS_OK)
    		{
    			freeReplyObject(reply);
    			freeContext();
    		    break;
    		}

    		if(reply->type != REDIS_REPLY_ARRAY)
    		{
    			freeReplyObject(reply);
    			continue;
    		}

       
    		bool found = false;
    		for(i = 0;  i < reply->elements; i++)
    		{
    			if(i%3==1)//3
    			{
    				redisReply *childRpy = (redisReply *)reply->element[i];
    				if(childRpy->type != REDIS_REPLY_STRING) continue;
    				string publishUUID = childRpy->str;
    			
    				if(publishUUID.compare(m_subStr))  continue;
    				found = true;
    			}

    			if(found &&(i%3==2))//3
    			{
    				CommonMessage commMsg;
                    VastRequest vastReq;
                    
    				redisReply *childRpy = (redisReply *)reply->element[i];
    				if(childRpy->type != REDIS_REPLY_STRING) continue;

    				string str1 = childRpy->str;
    				commMsg.ParseFromString(str1);
    				string data = commMsg.data();
    				vastReq.ParseFromString(data);
    				string uuid = vastReq.id();
    				m_subValue = uuid;

#ifdef DEBUG                    
                    cout << "subscribe:<" << uuid << ">" << subNum++ << endl;
#endif

                    pthread_mutex_lock(&bcDataMutex);
                    map<string, bcDataRec*>::iterator bcDataIt =  bcDataRecList.find(uuid);
                    if(bcDataIt != bcDataRecList.end())//存在
                    {
                        string bcDataUuid = bcDataIt->first;
                        bcDataRec *bcData = bcDataIt->second;
                        if(bcData)
                        {
                            BCStatus bcStatus = bcData->status;
                            string bcDataBidIdAndTime =  bcData->bidIDAndTime;
                            char *recvBuf = bcData->buf;
                            int recvSize = bcData->bufSize;
                            bcData->buf = NULL;
                            delete bcData;
                            bcDataRecList.erase(bcDataIt);
                            pthread_mutex_unlock(&bcDataMutex);

                            if(bcStatus == BC_recvBid)
                            {
                                redisReply *reply = (redisReply *)redisCommand(redisC.get_redisContex(), "hset %s %s %b", bcDataUuid.c_str(),  bcDataBidIdAndTime.c_str() , recvBuf, recvSize);// 
                                delete[] recvBuf;
                                if(reply== NULL) //严重错误需要重新连接redis
                                {
                                    cout << "serious problem" << endl;
                                    redisFree(redisC.get_redisContex());
                                    if(redisC.connectaRedis() == false)
                                    {
                                        cout <<"m_redisClient.connectaRedisWithTimeout() == false)" <<endl;
                                    }
                                }
#ifdef DEBUG
                                if(reply->type == 3)
                                {
                                    cout <<"s hset times:" << subHset++ << endl;
                                }
                                else
                                {
                                    cout << "redis hset falure-:"<<reply->type << endl; 
                                }
#endif                    
                                freeReplyObject(reply);   
                            }
                        }
                        else
                        {
                            bcDataRecList.erase(bcDataIt);
                            pthread_mutex_unlock(&bcDataMutex);
                        }
                    }
                    else
                    {

                          time_t timep;
                          time(&timep);
                          bcDataRec *bData = new bcDataRec;
                          bData->status = BC_recvSub;
                          bData->buf = NULL;
                          bData->bufSize = 0;
                          bData->time = timep;

                          bcDataRecList.insert(pair<string, bcDataRec*>(uuid, bData));
                          pthread_mutex_unlock(&bcDataMutex);
                    }

                    found = false;
    			}	//if(i%3==1)
    		}//for(i = 0;  i < reply->elements; i++)
    		
          }
    		catch(...)
            {
                cout << "subscribeClient error exception" << endl;
                exit(-1);
            }
       
    	
    	}
    }
	return;
}


