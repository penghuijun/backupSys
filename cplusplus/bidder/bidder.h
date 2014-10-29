#ifndef __BIDDER_H__
#define __BIDDER_H__
#include <string>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <event.h>
#include <map>
#include <queue>
#include <set>
#include <list>
#include "bidderConfig.h"
#include "hiredis.h"
#include "zmq.h"
#include "threadpoolmanager.h"
#include "redisPool.h"
#include "document.h"		// rapidjson's DOM-style API
#include "prettywriter.h"	// for stringify JSON
#include "filestream.h"	// wrapper of C stream for prettywriter as output
#include "stringbuffer.h"
#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"
#include "AdBidderResponseTemplate.pb.h"
#include "AdMobileResponse.pb.h"
#include "AdMobileRequest.pb.h"
#include "redisPool.h"

using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;
#define BUFSIZE 4096


const int	c_master_throttleIP=0x01;
const int	c_master_throttlePort=0x02;
const int	c_master_bcIP=0x04;
const int	c_master_bcPort=0x08;
const int	c_workerNum=0x10;
const int	c_master_subKey=0x20;



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

enum requestState
{
	request_invalid,
	request_waiting,
	request_sended,
	request_over
};

struct uuid_subsribe_info
{
	string business_code;
	string data_coding_type;
	string publishKey;
	queue<int> compaignID_vec;
	requestState state;
	string vastStr;
	exchangeConnector* connector;
	string jsonStr;
};


struct redisDataRec
{
	string bidIDAndTime;
	int    time;
};

struct messageBuf
{
	char *buf;
	unsigned int bufSize;
	void *serv;
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


#define PUBLISHKEYLEN_MAX 50

struct handler_fd_map
{
	string subKey;
	void *handler;
	int fd;
	struct event *evEvent;
};

class bidderServ;

struct eventParam
{
	struct event_base* base;
	bidderServ *serv;
};

class read_write_lock
{
public:
	read_write_lock(){}
	int init()
	{
		return pthread_rwlock_init(&m_rw_lock, NULL);
	}
	int read_lock()
	{
		return pthread_rwlock_rdlock(&m_rw_lock);
	}
	int write_lock()
	{
		return pthread_rwlock_wrlock(&m_rw_lock);
	}
	int read_trylock()
	{
		return pthread_rwlock_tryrdlock(&m_rw_lock);
	}
	int write_trylock()
	{
		return pthread_rwlock_trywrlock(&m_rw_lock);
	}

	
	int read_write_unlock()
	{
		return pthread_rwlock_unlock(&m_rw_lock);
	}
	int destroy()
	{
		return pthread_rwlock_destroy(&m_rw_lock);
	}
	~read_write_lock(){destroy();}
private:
	pthread_rwlock_t m_rw_lock;	
};

class mutex_lock
{
public:
	mutex_lock(){}
	int init()
	{
		return pthread_mutex_init(&m_mutex_lock, NULL);
	}
	int lock()
	{
		return pthread_mutex_lock(&m_mutex_lock);
	}
	int trylock()
	{
		return pthread_mutex_trylock(&m_mutex_lock);
	}

	int unlock()
	{
		return pthread_mutex_unlock(&m_mutex_lock);	
	}
	int destroy()
	{
		return pthread_mutex_destroy(&m_mutex_lock);
	}

	void operator=(pthread_mutex_t &mutex_init)
	{
		m_mutex_lock = mutex_init;
	}
	~mutex_lock()
	{
		destroy();
	}
private:
	pthread_mutex_t m_mutex_lock;	
};


/*
class proess_info
{
public:
	process_info(){}
	process_info(pid_t pid, int chanel[], proStatus status)
	{
		set(pid, chanel, status);	
	}
	void set(pid_t pid, int chanel[], proStatus status)
	{
		m_pid = pid;
		m_channel[0] = chanel[0];
		m_channel[1] = chanel[1];
		m_status = status;
	}
	~process_info(){}
private:
    pid_t             m_pid;
	int 			  m_channel[2];
    proStatus         m_status;
} process_info_t; 


class masterWorkerModel
{
public:
	masterWorkerModel(){}
	masterWorkerModel(int worker_cnt)
	{
		m_worker_number = worker_cnt;
	}
	void add_process(pid_t pid, int chanel[], proStatus status)
	{
		proess_info *process = new proess_info(pid, chanel, status);
		m_workerList.push_back(process);
	}
	~masterWorkerModel(){}
private:
	int m_worker_number;
	vector<proess_info*>  m_workerList;		
};*/


//bidder server
class bidderServ
{
public:
	bidderServ(configureObject& config);
	void readConfigFile();//read configure file
	void master_subscribe_async_event_register(struct event_base* base, event_callback_fn func, void *arg);
	bool needRemoveWorker() const 
	{
		return (m_workerList.size()>m_workerNum)?true:false;
	}
	void updataWorkerList(pid_t pid);
	void start_worker();
	void run();
	bool masterRun();
	void masterRestart(struct event_base* base);
	void master_net_init();
	void worker_net_init();
	void *get_zmqContext() const {return m_zmqContext;}
	void *get_master_subscribe_zmqHandler(int fd);
	handler_fd_map *get_master_subscribe_zmqHandler(string subKey);
	void releaseConnectResource(string subKey) ;
	void addSendUnitToList(void *data, unsigned int dataLen, int channel);
	void sendToWorker(char *buf, int size);
	void calSpeed();
	void reloadWorker();
	static void usr1SigHandler(int fd, short event, void *arg);
	static void *throttle_request_handler(void *arg);
	static void recvRequest_callback(int _, short __, void *pair);
	static void workerBusiness_callback(int fd, short event, void *pair);
	static void func(int fd, short __, void *pair);
	void *bindOrConnect(bool client,const char * transType,int zmqType,const char * addr,unsigned short port,int * fd);
	handler_fd_map* zmq_connect_subscribe(string subkey);
	static void hupSigHandler(int fd, short event, void *arg);
	static void	intSigHandler(int fd, short event, void *arg);
	static void termSigHandler(int fd, short event, void *arg);
	static void signal_handler(int signo);
	
	bool jsonToProtobuf_compaign(vector<string> &camp_vec, string &tbusinessCode, string &data_coding_type, string &uuid, CommonMessage &commMsg);
	static void handle_ad_request(void* arg);
	bool gen_mobile_response_protobuf(string &business_code, string &data_code, string &data_str, CommonMessage &comm_msg);
	bool gen_vast_response_protobuf(string &business_code, string &data_code, string &data_str, CommonMessage &comm_msg);

	~bidderServ()
	{
	}
private:

	void * establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd);
	configureObject& m_config;

	string m_throttleIP;
	unsigned short m_throttlePort;
	
	string m_bidderCollectorIP;
	unsigned short m_bidderCollectorPort;

	int m_workerNum;
	int m_workerChannel;
	
	vector<handler_fd_map*> m_handler_fd_map;
	vector<BC_process_t*>  m_workerList;//children progress list
	vector<string> m_subKeying;//message filtering, if establis or move success, remove or add to m_subKeying
	vector<string> m_subKey;//subKey configure , need establish message filter or remove message filter
	
	unsigned int m_configureChange=0;
	
	void *m_zmqContext = nullptr;	
	void *m_response_handler = nullptr;

	mutex_lock      m_worker_zeromq_lock;
	mutex_lock      m_test_lock;
	read_write_lock m_zeromq_sub_lock;
	read_write_lock m_worker_info_lock;
	
	int m_eventFd[2];
	pid_t m_master_pid;

	redisPool m_redis_pool;
	redisPool m_buySideRedis_pool;
	ThreadPoolManager m_thread_manager;
	struct event_base* m_base;
	unsigned short m_thread_num;
};
#endif


