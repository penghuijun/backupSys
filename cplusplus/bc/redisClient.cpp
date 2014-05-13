#include <string.h>
#include <iostream>
#include <time.h>
#include <pthread.h>
#include <map>
#include <string>
#include <unistd.h>
#include "redisClient.h"
#include "zeroMqServer.h"
using namespace std;

extern pthread_mutex_t redisMutex;
extern pthread_mutex_t redisCmdMutex;
extern pthread_mutex_t bcDataMutex;
extern map<string, bcDataRec*> bcDataRecList;
extern map<string, redisDataRec*> redisRecList;

redisClient::redisClient(const char *servIp, unsigned short servPort)
{
	set_redisParam(servIp, servPort);
}


redisClient::redisClient(string servIP, unsigned short servPort)
{
    set_redisParam(servIP, servPort);
}


redisClient::redisClient(string servIP, unsigned short servPort, int savetime):delInterval(savetime)
{
     set_redisParam(servIP, servPort);    
}


void redisClient::set_redisParam(const char * servIp,unsigned short servPort)
{
	m_ip = servIp;
	m_port = servPort;
}

void redisClient::set_redisParam(string servIp,unsigned short servPort)
{
    m_ip = servIp;
	m_port = servPort;
}

bool redisClient::connectaRedis()
{
	m_rc = redisConnect(m_ip.c_str(), m_port);
	if(m_rc == NULL) return false;
	if(m_rc->err)
	{
		freeContext();
		cout <<"connectaRedis failure" << endl;
		return false;
	}

	cout << "connectaRedis success" << endl;
	return true;
}

bool redisClient::connectaRedisWithTimeout(struct timeval tv)
{
	m_rc = redisConnectWithTimeout(m_ip.c_str(), m_port, tv);
	if(m_rc == NULL) return false;
	if(m_rc->err)
	{
		freeContext();
		cout <<"connectaRedisWithTimeout failure" << endl;
		return false;
	}
	cout << "connectaRedisWithTimeout success" << endl;
	return true;
}

void redisClient::run()
{
    bool ret = false;
    do
    {
        ret = connectaRedis();
        if(ret == false)
        {
            cout << "error:: can not connect redis" << endl;
   //         sleep(1);
        }
    }while(ret==false);
}

void redisClient::delRedisItem() 
{

try{
     time_t timep;
     time(&timep);
     map<string, string> delMap;

 
     pthread_mutex_lock(&redisMutex);
     map<string, redisDataRec*>::iterator redisIt = redisRecList.begin();  
	 for(redisIt = redisRecList.begin();redisIt != redisRecList.end(); )
	 {  
	 	redisDataRec* redis = redisIt->second;  
        
        if(redis == NULL)
		{
			redisRecList.erase(redisIt++);
			continue;
		}

        string uuid = redisIt->first;
        string fieldSym = redis->bidIDAndTime;
        time_t timeout= redis->time;

        if(timep - timeout >= delInterval)
        {
            delete redis;
            redisRecList.erase(redisIt++);
            delMap.insert(pair<string, string>(uuid, fieldSym));
        }
        else
        {
            redisIt++;   
        }
	 }     
     pthread_mutex_unlock(&redisMutex);

     pthread_mutex_lock(&bcDataMutex);
     map<string, bcDataRec*>::iterator bcDataIt = bcDataRecList.begin();  
	 for(bcDataIt = bcDataRecList.begin();bcDataIt != bcDataRecList.end(); )
	 {  
	  
	 	bcDataRec* bcData = bcDataIt->second;  
   
        if(bcData == NULL)
		{
			bcDataRecList.erase(bcDataIt++);
			continue;
		}
         
        time_t tm= bcData->time;
        if(timep - tm >= 3)
        {
            if(bcData->buf) delete[] bcData->buf;
            delete bcData;    
            bcDataRecList.erase(bcDataIt++);
        }
        else
        {
            bcDataIt++;   
        }
	 }     
     pthread_mutex_unlock(&bcDataMutex);

     
     while(delMap.size())
     {
         map<string, string>::iterator delIt;
         int index = 0;
         
         for(delIt= delMap.begin(); delIt != delMap.end();)
         {
     
             string uuid = delIt->first;
             string field = delIt->second; 
             redisAppendCommand(m_rc, "hdel %s %s", uuid.c_str(), field.c_str());
             index++;
             if(index >=2000) break; // 一个管道1000个    
             delMap.erase(delIt++);
         }

         
         int i;
         for(i = 0;i < index; i++)
         {
            redisReply *ply;
            redisGetReply(m_rc, (void **) &ply);
         } 
     }
}
catch(...)
{
    pthread_mutex_unlock(&redisMutex);
    pthread_mutex_unlock(&bcDataMutex);
    cout << "delRedisItem exception" << endl;
      
}

}

