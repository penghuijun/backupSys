#ifndef __BC_SERV_H__
#define __BC_SERV_H__
#include <string>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <event.h>
#include <map>
#include <set>
#include <list>
#include "bcConfig.h"
#include "hiredis.h"
#include "zmq.h"
#include "threadpoolmanager.h"
#include "redisPool.h"

using namespace std;
using std::string;
#define BUFSIZE 4096


const int	c_master_throttleIP=0x01;
const int	c_master_throttlePubVastPort=0x02;
const int	c_master_bcIP=0x08;
const int	c_master_bcListenBidPort=0x10;
const int   c_master_redisIP = 0x20;
const int   c_master_redisPort = 0x40;
const int   c_master_redisSaveTime=0x80;
const int	c_workerNum=0x100;
const int	c_master_subKey=0x200;



enum proStatus
{
	PRO_INIT,
    PRO_BUSY,
	PRO_RESTART,
	PRO_KILLING
};


typedef struct BCProessInfo
{
    pid_t               pid;
	int 			  channel[2];
    proStatus         status;
} BC_process_t; 

typedef struct 
{
    pid_t              pid;
	int                chanel;
	bool 			   sended;
    char      *buf;
	int                bufSize;
	unsigned int       id;
	unsigned long long sendTime;
	int                sendCnts;
} sendBufUnit; 


struct redisDataRec
{
	string bidIDAndTime;
	int    time;
};

class threadPoolFuncArg
{
public:
	threadPoolFuncArg(char *key, char* field, char *data, int dataLen)
	{
		set(key, field, data, dataLen);
	}
	void set(char *key, char* field, char *data, int dataLen)
	{
		m_key = key;
		m_field = field;
		m_data = data;
		m_dataLen = dataLen;
	}

	string &get_key(){return m_key;}
	string &get_field(){return m_field;}
	char*   get_data(){return m_data;}
	int     get_dataLen(){return m_dataLen;}
	void free_data()
	{
		delete[] m_data;
	}
	~threadPoolFuncArg()
	{
		free_data();
	}
private:
	string m_key;
	string m_field;
	char*  m_data;
	int    m_dataLen;
};

struct throttleBuf
{
	char *buf;
	unsigned int bufSize;
	void *serv;
	int channel;
};

struct servReadWorkerEvent
{
	struct event *ev;
	int fd;
};

struct eventCallBackParam
{
	void *serv;
	int ev_handlerMsgCount;
	struct event_base *ev_base;
	struct event *ev_adEvent;
	struct event *ev_expEvent;
	struct event *ev_clientPullEvent;
	vector<servReadWorkerEvent *>ev_servPullEventList;
};

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



#define PUBLISHKEYLEN_MAX 50

struct handler_fd_map
{
	string subKey;
	void *handler;
	int fd;
	struct event *evEvent;
};

class bcServ;

struct eventParam
{
	struct event_base* base;
	bcServ *serv;
};
enum class BCStatus
{
	BC_recvBid,
	BC_recvSub,
	BC_recvExpire,
	BC_recvBidAndSub,
	BC_recvBidAndExp,
	BC_recvSubAndExp
};

struct bcDataRec
{
	string bidIDAndTime;
	time_t time;
	BCStatus status;
	char *buf;
	int bufSize;
//#ifdef DEBUG
	unsigned long long subTime;
	unsigned long long bidTime;
	unsigned long long expTime;
//#endif
};
#define UUID_SIZE_MAX 100


//bidder server
class bcServ
{
public:
	bcServ(configureObject& config);
	void run();

	void update_microtime()
	{
		tmLock();
        gettimeofday(&m_microSecTime, NULL);
        tmUnlock();
	}
	bool needRemoveWorker() const 
	{
		return (m_workerList.size()>m_workerNum)?true:false;
	}
	void updataWorkerList(pid_t pid);
	void start_worker();
	static void worker_thread_func(void *arg);


	void bcDataLock(){pthread_mutex_lock(&bcDataMutex);}
	void bcDataUnlock(){pthread_mutex_unlock(&bcDataMutex);}
	void redisContextLock(){pthread_mutex_lock(&m_redisContextMutex);}
	void redisContextUnlock(){pthread_mutex_unlock(&m_redisContextMutex);}
	void delDataLock(){pthread_mutex_lock(&m_delDataMutex);}
	void delDataUnlock(){pthread_mutex_unlock(&m_delDataMutex);}

	
	void fd_rd_lock(){ pthread_rwlock_rdlock(&handler_fd_lock);}
	void fd_wr_lock(){ pthread_rwlock_wrlock(&handler_fd_lock);}
	void fd_rw_unlock(){ pthread_rwlock_unlock(&handler_fd_lock);}

	void workerList_rd_lock(){ pthread_rwlock_rdlock(&m_workerList_lock);}
	void workerList_wr_lock(){ pthread_rwlock_wrlock(&m_workerList_lock);}
	void workerList_rw_unlock(){ pthread_rwlock_unlock(&m_workerList_lock);}

	void tmLock(){pthread_mutex_lock(&tmMutex);}
	void tmUnlock(){pthread_mutex_unlock(&tmMutex);}
	void lock(){pthread_mutex_lock(&m_uuidListMutex);}
	void unlock(){pthread_mutex_unlock(&m_uuidListMutex);}
	bool masterRun();
	void master_net_init();
	
	void worker_net_init();

	void masterRestart(struct event_base* base);

	void *get_zmqContext() const {return m_zmqContext;}
	
//subKey incre , decre, change, lead to subscribe or unsubsribe
	void releaseConnectResource(string subKey) 
	{
		auto it = m_handler_fd_map.begin();
		for(it = m_handler_fd_map.begin(); it != m_handler_fd_map.end();)
		{
			handler_fd_map *hfMap = *it;
			if(hfMap==NULL)
			{
				it = m_handler_fd_map.erase(it);	
				continue;
			}
			
			if(hfMap->subKey == subKey)
			{
			     cout <<"del subkey:"<<subKey<<endl;
			     zmq_setsockopt(hfMap->handler, ZMQ_UNSUBSCRIBE, subKey.c_str(), subKey.size()); 
                 zmq_close(hfMap->handler);
                 event_del(hfMap->evEvent);
                 delete hfMap->evEvent;
                 delete hfMap;
				 it = m_handler_fd_map.erase(it);
				 
                 auto itor = m_subKeying.begin();
                 for(itor = m_subKeying.begin(); itor != m_subKeying.end();)
                 {
                    if(subKey ==  *itor)
                    {
                        itor = m_subKeying.erase(itor);
                    }
                    else
                    {
                        itor++;
                    }
                 }  
				
			}
			else
			{
				it++;
			}
		}
	}

	void reloadWorker()
	{
	    readConfigFile();
	}
	static void workerBusiness_callback(int fd, short event, void *pair);

	static void usr1SigHandler(int fd, short event, void *arg);
	static void hupSigHandler(int fd, short event, void *arg);
	static void	intSigHandler(int fd, short event, void *arg);
	static void termSigHandler(int fd, short event, void *arg);	

	void *get_request_handler(bcServ *serv, int fd) const
	{	
		void *handler;
		serv->fd_rd_lock();
		auto it = m_handler_fd_map.begin();
		for(it = m_handler_fd_map.begin(); it != m_handler_fd_map.end(); it++)
		{
			handler_fd_map *hfMap = *it;
			if(hfMap==NULL) continue;
			if(hfMap->fd == fd)
			{
				handler = hfMap->handler;
				serv->fd_rw_unlock();
				return handler;
			}
		}
		serv->fd_rw_unlock();
		return NULL;
	}
	handler_fd_map *get_request_handler(string subKey) const
	{	
		void *handler;
		auto it = m_handler_fd_map.begin();
		for(it = m_handler_fd_map.begin(); it != m_handler_fd_map.end(); it++)
		{
			handler_fd_map *hfMap = *it;
			if(hfMap==NULL) continue;
			if(hfMap->subKey == subKey)
			{
				return hfMap;
			}
		}
		return NULL;
	}
	void addSendUnitToList(void *data, unsigned int dataLen, int workIdx);
	void sendToWorker(char *buf, int size);
	void calSpeed();
	void subHandler(char * pbuf,int size);
	void subVastHandler(char * pbuf,int size);
	void subExpHandler(char * pbuf,int size);
	void bidderHandler(char * pbuf,int size);
	static void recvBidder_callback(int fd, short __, void *pair);
	static void recvRequest_callback(int _, short __, void *pair);
	void *bindOrConnect(bool client,const char * transType,int zmqType,const char * addr,unsigned short port,int * fd);
	static void configureUpdate(int fd, short __, void *pair);
	handler_fd_map* zmq_connect_subscribe(string subkey);
	static void *throttle_request_handler(void *arg);
	static void *bidder_response_handler(void *arg);
	static void  *deleteRedisDataHandle(void *arg);
	static void signal_handler(int signo);
	void readConfigFile();
	redisContext* connectToRedis();
	int get_hsetNum() const{return m_redisHsetNum;}
	int init_hsetNum() {m_redisHsetNum=0;}
	redisContext *get_redisContext(){return m_redisContex;}
	static void *work_event_new(void *arg);
	unsigned long long getLonglongTime();
	static void *getTime(void *arg);
private:
	#define BUSI_VAST "5.1"
	#define BUSI_EXP  "5.1.2"


	void get_overtime_recond(map<string, string>& delMap);
	void delete_redis_data(map<string, string>& delMap);
	void delete_invalid_recond();
	void * establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd);
	configureObject& m_config;
	

	vector<BC_process_t*>  m_workerList;//children progress list
	vector<string> m_subKeying;//message filtering, if establis or move success, remove or add to m_subKeying
	vector<string> m_subKey;//subKey configure , need establish message filter or remove message filter
	
	unsigned int m_configureChange=0;
	
	pthread_mutex_t m_uuidListMutex;
	pthread_rwlock_t handler_fd_lock;
	pthread_rwlock_t m_workerList_lock;
	
	int m_vastEventFd[2];
	pid_t m_master_pid;

	void *m_zmqContext = nullptr;	
	string m_throttleIP;
	unsigned short m_throttlePubVastPort=0;
	vector<handler_fd_map*> m_handler_fd_map;

	string m_bcIP;
	unsigned short m_bcListenBidderPort;
	void *m_bidderRspHandler;
	int m_bidderFd;
	struct event * m_bidderEvent;

	string m_redisIP;
	unsigned short m_redisPort;
	unsigned short m_redisSaveTime;
	unsigned short m_redisConnectNum;
	
	unsigned short m_workerNum=1;
	int m_workerChannel;

	map<string, bcDataRec*> bcDataRecList;
	redisContext *m_redisContex;
	redisContext* m_delContext;
	map<string, redisDataRec*> redisRecList;
	pthread_mutex_t bcDataMutex;
	pthread_mutex_t m_delDataMutex;
	pthread_mutex_t m_redisContextMutex;
	pthread_mutex_t tmMutex;

	int m_redisHsetNum = 0;

	redisPool m_redisPool;
	ThreadPoolManager m_threadPoolManger;

	struct timeval m_microSecTime;
};



#endif


