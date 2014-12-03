/*
  *Copyright (c) 2014, fxxc
  *All right reserved.
  *
  *filename: bcserv.h
  *document mark :
  *summary:
  *	
  *current version:1.1
  *author:xyj
  *date:2014-11-21
  *
  *history version:1.0
  *author:xyj
  *date:2014-10-11
  */

#ifndef __BC_SERV_H__
#define __BC_SERV_H__
#include <string>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <event.h>
#include <string.h>
#include <map>
#include <set>
#include <list>
#include "bcConfig.h"
#include "hiredis.h"
#include "zmq.h"
#include "threadpoolmanager.h"
#include "redisPool.h"
#include "redisPoolManager.h"

#include "lock.h"
using namespace std;
using std::string;

const int	c_master_throttleIP=0x01;
const int	c_master_throttlePubVastPort=0x02;
const int	c_master_bcIP=0x08;
const int	c_master_bcListenBidPort=0x10;
const int   c_master_redisIP = 0x20;
const int   c_master_redisPort = 0x40;
const int   c_master_redisSaveTime=0x80;
const int	c_workerNum=0x100;
const int	c_master_subKey=0x200;

const int	c_update_throttleAddr=0x01;
const int	c_update_bcAddr=0x02;
const int	c_update_workerNum=0x04;
const int	c_update_zmqSub=0x08;
const int   c_update_forbid=0x10;
const int   c_update_redisAddr=0x20;


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


class redisDataRec
{
public:
	redisDataRec(time_t time)
	{
		m_time = time;
	}
	void add_field(string &field)
	{
		m_field.push_back(field);
	}
	vector<string> &get_field_list(){return m_field;}
	time_t get_time(){return m_time;}
	~redisDataRec(){}
private:
	vector<string> m_field;
	time_t    m_time;
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
class bufferStructure
{
public:
	bufferStructure(char *buf, int bufSize, string &hash_field)
	{
		gen_buffer(buf, bufSize, hash_field);
	}
	void gen_buffer(char *buf, int bufSize, string &hash_field)
	{
		if(bufSize > 0)
		{
			m_bufSize = bufSize;
			m_buf = new char[bufSize];
			memcpy(m_buf, buf, bufSize);
			m_hash_filed = hash_field;
		}
	}
	char *get_buf() {return m_buf;}
	int   get_bufSize() {return m_bufSize;}
	string &get_hash_field() {return m_hash_filed;}

	void release_buffer(){ delete[] m_buf;}
	~bufferStructure()
	{
		release_buffer();
	}
	
private:
	char *m_buf=NULL;
	int   m_bufSize=0;
	string m_hash_filed;
};

typedef unsigned long long mircotime_t;

class bcDataRec
{
public:
	bcDataRec()
	{
	}
	bcDataRec(mircotime_t timep, bool recv_throttle)
	{
		m_mircoTime = timep;
		m_recv_throttle = recv_throttle;
	}
	
	bcDataRec(mircotime_t timep, uint32_t overTime, bool recv_throttle)
	{
		m_mircoTime = timep;
		m_recv_throttle = recv_throttle;
		m_overtime = overTime;
	}

	void add_bidder_buf(char *buf, int bufSize, string &hash_field)
	{
		if(!m_bufStru_vec) m_bufStru_vec = new vector<bufferStructure*>;
		if(m_bufStru_vec)
		{
			bufferStructure *buf_item = new bufferStructure(buf, bufSize, hash_field);
			if(m_bufStru_vec) m_bufStru_vec->push_back(buf_item);
		}
	}


	mircotime_t get_microtime(){return m_mircoTime;}
	int get_overtime(){return m_overtime;}
	vector<bufferStructure*>* get_buf_list()
	{
		auto it = m_bufStru_vec;
		m_bufStru_vec = NULL;
		return it;
	}

	void recv_throttle_complete(){m_recv_throttle = true;}
	bool recv_throttle_info(){return m_recv_throttle;}
////////test
	void set_sub(){m_sub++;}
	void set_bid(){m_bid++;}
	int  get_sub(){return m_sub;}
	int get_bid(){return m_bid;}

///////test
	
	~bcDataRec()
	{
		if(m_bufStru_vec)
		{
			for(auto it = m_bufStru_vec->begin(); it != m_bufStru_vec->end(); it++)
			{
				bufferStructure *buf =  *it;
				if(buf) delete buf;
			}
			delete m_bufStru_vec;
		}
	}
private:
	///////test

	int m_sub=0;
	int m_bid=0;
	///////test
	mircotime_t m_mircoTime=0;
	uint32_t    m_overtime=1000;//1000ms default;
	bool   m_recv_throttle=false;
	vector<bufferStructure*> *m_bufStru_vec=NULL;
};
#define UUID_SIZE_MAX 100

//bidder server
class bcServ
{
public:
	bcServ(configureObject& config);
	void run();
	mircotime_t get_microtime();
	void check_data_record();
	int zmq_get_message(void* socket, zmq_msg_t &part, int flags);

	void update_microtime()
	{
		tmMutex.lock();
        gettimeofday(&m_microSecTime, NULL);
		tmMutex.unlock();
	}
	bool needRemoveWorker() const 
	{
		return (m_workerList.size()>m_workerNum)?true:false;
	}
	void updataWorkerList(pid_t pid);
	void start_worker();
	static void worker_thread_func(void *arg);
	bool masterRun();
	void master_net_init();
	
	void worker_net_init();

	void masterRestart(struct event_base* base);

	void *get_zmqContext() const {return m_zmqContext;}
	
//subKey incre , decre, change, lead to subscribe or unsubsribe
	void releaseConnectResource(string subKey) 
	{
		for(auto it = m_handler_fd_map.begin(); it != m_handler_fd_map.end();)
		{
			handler_fd_map *hfMap = *it;
			if(hfMap==NULL)
			{
				it = m_handler_fd_map.erase(it);	
				continue;
			}
			
			if(hfMap->subKey == subKey)
			{
			     cerr <<"del subkey:"<<subKey<<endl;
			     zmq_setsockopt(hfMap->handler, ZMQ_UNSUBSCRIBE, subKey.c_str(), subKey.size()); 
                 zmq_close(hfMap->handler);
                 event_del(hfMap->evEvent);
                 delete hfMap;
				 it = m_handler_fd_map.erase(it);
				 
                 for(auto itor = m_subKeying.begin(); itor != m_subKeying.end();)
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

	void updateWorker()
	{
		readConfigFile();
		cout<<"worker:m_configureChange:"<<m_configureChange<<endl;
		if((m_configureChange &c_update_forbid) == c_update_forbid)
		{
			m_configureChange=(m_configureChange&(~c_update_forbid));
			exit(0);
		}
		m_redisPoolManager.redisPool_update(m_config.get_redisIP(), m_config.get_redisPort(), m_config.get_threadPoolSize()+1);
		
	}

	static void workerBusiness_callback(int fd, short event, void *pair);

	static void usr1SigHandler(int fd, short event, void *arg);
	static void hupSigHandler(int fd, short event, void *arg);
	static void	intSigHandler(int fd, short event, void *arg);
	static void termSigHandler(int fd, short event, void *arg);	

	void *get_request_handler(bcServ *serv, int fd) const
	{	
		void *handler;
		serv->handler_fd_lock.read_lock();
		auto it = m_handler_fd_map.begin();
		for(it = m_handler_fd_map.begin(); it != m_handler_fd_map.end(); it++)
		{
			handler_fd_map *hfMap = *it;
			if(hfMap==NULL) continue;
			if(hfMap->fd == fd)
			{
				handler = hfMap->handler;
				serv->handler_fd_lock.read_write_unlock();
				return handler;
			}
		}
		serv->handler_fd_lock.read_write_unlock();
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
	void sendToWorker(char *buf, int size);
	void calSpeed();
	void subVastHandler(char * pbuf,int size);
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
	void handler_recv_buffer(char * buf,int size, string &uuid, string &hash_field);
private:
	void get_overtime_recond(map<string, vector<string>*>& delMap);
	void delete_redis_data(map<string, vector<string>*>& delMap);
	void * establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd);
	configureObject& m_config;
	

	vector<BC_process_t*>  m_workerList;//children progress list
	vector<string> m_subKeying;//message filtering, if establis or move success, remove or add to m_subKeying
	vector<string> m_subKey;//subKey configure , need establish message filter or remove message filter
	
	unsigned int m_configureChange=0;
	
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
	unsigned short m_thread_pool_size=0;
	
	unsigned short m_workerNum=1;
	int m_workerChannel;

	map<string, bcDataRec*> bcDataRecList;
	redisContext *m_redisContex;
	redisContext* m_delContext;
	map<string, redisDataRec*> redisRecList;

	mutex_lock bcDataMutex;
	mutex_lock m_delDataMutex;
	mutex_lock tmMutex;
	mutex_lock m_getTimeofDayMutex;
	read_write_lock handler_fd_lock;
	read_write_lock m_workerList_lock;

	int m_redisHsetNum = 0;

	redisPoolManager m_redisPoolManager;
	ThreadPoolManager m_threadPoolManger;

	struct timeval m_microSecTime;


	string m_vastBusiCode;
	string m_mobileBusiCode;
};



#endif


