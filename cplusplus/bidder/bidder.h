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
#include "MobileAdResponse.pb.h"
#include "MobileAdRequest.pb.h"
#include "redisPoolManager.h"
#include "lock.h"

using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;


const int	c_update_throttleAddr=0x01;
const int	c_update_bcAddr=0x02;
const int	c_update_workerNum=0x04;
const int	c_update_zmqSub=0x08;
const int   c_update_forbid=0x10;


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


enum requestState
{
	request_invalid,
	request_waiting,
	request_sended,
	request_over
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


	void intToString(int num, string& str)
	{
    	ostringstream os;
   	 	os<<num;
    	str = os.str();
	}

	int zmq_get_message(void* socket, zmq_msg_t &part, int flags);

	void doubleToString(double num, string& str)
	{
    	ostringstream os;
   	 	os<<num;
    	str = os.str();
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
	void updateWorker();
	static void usr1SigHandler(int fd, short event, void *arg);
	static void *throttle_request_handler(void *arg);
	static void recvRequest_callback(int _, short __, void *pair);
	static void workerBusiness_callback(int fd, short event, void *pair);
	static void func(int fd, short __, void *pair);
	void *bindOrConnect(bool client,const char * transType,int zmqType,const char * addr,unsigned short port,int * fd);
	handler_fd_map* zmq_connect_subscribe(string &subkey);
	static void hupSigHandler(int fd, short event, void *arg);
	static void	intSigHandler(int fd, short event, void *arg);
	static void termSigHandler(int fd, short event, void *arg);
	static void signal_handler(int signo);
	
	bool jsonToProtobuf_compaign(vector<string> &camp_vec, string &tbusinessCode, string &data_coding_type, string &uuid, CommonMessage &commMsg);
	static void handle_ad_request(void* arg);
	bool gen_mobile_response_protobuf(string &business_code, string &data_code,  string& ttl_time, string &data_str);
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

	redisPoolManager m_redis_pool_manager;
	ThreadPoolManager m_thread_manager;
	struct event_base* m_base;
	unsigned short m_thread_pool_size;
	unsigned short m_target_converce_num = 0;
	string m_vastBusinessCode;
	string m_mobileBusinessCode;
	string m_bidder_id;
};
#endif


