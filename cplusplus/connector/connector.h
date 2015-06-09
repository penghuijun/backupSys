#ifndef __CONNECTOR_H__
#define __CONNECTOR_H__
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

#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>


#include "adConfig.h"
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
#include "zeromqConnect.h"
#include "login.h"
#include "deviceManager.h"
#include "rtbkit.h"

using namespace com::rj::protos;
using namespace std;
using namespace rapidjson;

#define PUBLISHKEYLEN_MAX 100

class eventArgment
{
public:
	eventArgment(){}
	eventArgment(struct event_base* base, void *serv)
	{
		set(base, serv);
	}	
	void set(struct event_base* base, void *serv)
	{
		m_base = base;
		m_serv = serv;
	}
	struct event_base* get_base(){return m_base;}
	void *get_serv(){return m_serv;}
private:
	struct event_base* m_base;
	void *m_serv;
};

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


class messageBuf
{
public:
	messageBuf(){}
	messageBuf(char* data, int dataLen, void *serv)
	{
		message_new(data, dataLen, serv);
	}
	void message_set(char* data, int dataLen, void *serv)
	{
		m_stringBuf.string_set(data, dataLen);
		m_serv = serv;
	}

	void message_new(char* data, int dataLen, void *serv)
	{
		m_stringBuf.string_new(data, dataLen);
		m_serv = serv;	
	}
	stringBuffer& get_stringBuf(){return m_stringBuf;}
	void *        get_serv(){return m_serv;}
	~messageBuf(){}
private:
	stringBuffer m_stringBuf;
	void*        m_serv;
};



//bidder server
class connectorServ
{
public:
	connectorServ(configureObject& config);
	void readConfigFile();//read configure file
	bool needRemoveWorker() const 
	{
		return (m_workerList.size()>m_workerNum)?true:false;
	}

    void frequency_display(shared_ptr<spdlog::logger>& file_logger, const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_Frequency >&frequency);
	static void *getTime(void *arg);
	static void* childProcessGetTime(void *arg);
	int zmq_get_message(void* socket, zmq_msg_t &part, int flags);

	void updataWorkerList(pid_t pid);
	void start_worker();
	void run();
	bool masterRun();
	void masterRestart(struct event_base* base);
	void worker_net_init();;
	void sendToWorker(char* subKey, int keyLen, char * buf,int bufLen);
	void calSpeed();
	void updateWorker();
	static void usr1SigHandler(int fd, short event, void *arg);
	static void *throttle_request_handler(void *arg);
	static void recvRequest_callback(int _, short __, void *pair);
	static void workerBusiness_callback(int fd, short event, void *pair);
	static void recv_exbidder_adResponse(int fd, short event, void *pair);
	static void hupSigHandler(int fd, short event, void *arg);
	static void	intSigHandler(int fd, short event, void *arg);
	static void termSigHandler(int fd, short event, void *arg);
	static void signal_handler(int signo);
	static void bidderManagerMsg_handler(int fd, short __, void *pair);
	static void* sendHeartToBC(void *bidder);
	static void *bidderManagerHandler(void *arg);
	static void recvRsponse_callback(int fd, short event, void *pair);
	static void handle_ad_request(void* arg);
	
	void get_local_ipaddr(vector<string> &ipAddrList);

	void worker_parse_master_data(char *data, unsigned int dataLen);	
	static void register_throttle_response_handler(int fd, short __, void *pair);

	bool manager_handler(void *handler, string& identify, const managerProtocol_messageTrans &from
				,const managerProtocol_messageType &type, const managerProtocol_messageValue &value
				,struct event_base * base=NULL, void * arg = NULL);
	bool manager_handler(const managerProtocol_messageTrans &from
			   ,const managerProtocol_messageType &type, const managerProtocol_messageValue &value
			   ,managerProtocol_messageType &rspType, struct event_base * base = NULL, void * arg = NULL);

	bool manager_from_BC_handler(const managerProtocol_messageType &type
	  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg);
	bool manager_from_throttle_handler(const managerProtocol_messageType &type
	  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg);

	const string& get_vastBusiCode() const {return m_vastBusinessCode;}
	const string& get_mobileBusiCode()const{return m_mobileBusinessCode;}
	~connectorServ()
	{
		for(auto it = m_workerList.begin(); it != m_workerList.end();)
		{
			delete *it;
			it = m_workerList.erase(it);
		}
		m_test_lock.destroy();
		m_worker_info_lock.destroy();
	}
private:
	configureObject&          m_config;

	int                       m_workerNum;	
	vector<BC_process_t*>     m_workerList;//children progress list
		
	mutex_lock                m_test_lock;
	read_write_lock           m_worker_info_lock;
	zeromqConnect             m_zmq_connect;	
	struct event_base*        m_base = NULL;
	string                    m_vastBusinessCode;
	string                    m_mobileBusinessCode;
	unsigned short            m_heartInterval;
	spdlog::level::level_enum m_logLevel;	

//	redisPoolManager    m_redis_pool_manager;
	ThreadPoolManager         m_thread_manager;
	deviceManager             m_device_manager;
	exBidManager              m_extbid_manager;
	
//master worker communication 
	void*                     m_masterPushHandler = NULL;
	void*                     m_masterPullHandler = NULL;
	int 			          m_masterPullFd;
	void*                     m_workerPullHandler = NULL;
	void*                     m_workerPushHandler = NULL;
	int                       m_workerPullFd;	
};
#endif


