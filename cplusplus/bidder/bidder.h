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
#include "bidderManager.h"
#include "campaign.h"
#include "managerProto.pb.h"
using namespace com::rj::protos::manager;

using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;

#define PUBLISHKEYLEN_MAX 100

const int	c_update_throttleAddr=0x01;
const int	c_update_bcAddr=0x02;
const int	c_update_workerNum=0x04;
const int	c_update_zmqSub=0x08;
const int   c_update_forbid=0x10;

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
	void *m_serv;
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
	bool needRemoveWorker() const 
	{
		return (m_workerList.size()>m_workerNum)?true:false;
	}
	bool parse_publishKey(string& origin, string& bcIP, unsigned short &bcManagerPort, unsigned short &bcDataPort,
										string& bidderIP, unsigned short& bidderPort, int recvLen);
	bool logLevelChange();

	bool get_bidder_target(const MobileAdRequest& mobile_request, operationTarget &target_operation_set, verifyTarget &target_verify_set);
    void frequency_display(shared_ptr<spdlog::logger>& file_logger, const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_Frequency >&frequency);
	void appsession_display(shared_ptr<spdlog::logger>& file_logger,
		const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_AppSession>& appsession);

	static void *getTime(void *arg);
	static void* childProcessGetTime(void *arg);
	unsigned long long getchildMicroTime();
	unsigned long long getMasterMicroTime();


	void intToString(int num, string& str)
	{
    	ostringstream os;
   	 	os<<num;
    	str = os.str();
	}

	void updateConfigure();
	int zmq_get_message(void* socket, zmq_msg_t &part, int flags);
	void updataWorkerList(pid_t pid);
	void start_worker();
	void run();
	bool masterRun();
	void worker_net_init();
	void sendToWorker(char* subKey, int keyLen, char * buf,int bufLen);
	void calSpeed();
	void updateWorker();
	static void usr1SigHandler(int fd, short event, void *arg);
	static void *throttle_request_handler(void *arg);
	static void recvRequest_callback(int _, short __, void *pair);
	static void workerBusiness_callback(int fd, short event, void *pair);
	static void hupSigHandler(int fd, short event, void *arg);
	static void	intSigHandler(int fd, short event, void *arg);
	static void termSigHandler(int fd, short event, void *arg);
	static void signal_handler(int signo);
	static void bidderManagerMsg_handler(int fd, short __, void *pair);
	static void* sendHeartToBC(void *bidder);
	static void *bidderManagerHandler(void *arg);
	static void handle_ad_request(void* arg);
	bool mobile_request_handler(const char* pubKey, const CommonMessage& request_commMsg);
	char* gen_mobile_response_protobuf(const char* pubKey, campaignInfoManager &ad_campaignInfoManager, 
		const CommonMessage& commMsg, MobileAdRequest& mobile_request, int &dataLen);
	void get_local_ipaddr(vector<string> &ipAddrList)
	{
		struct ifaddrs * ifAddrStruct=NULL;
	    void * tmpAddrPtr=NULL;

	    getifaddrs(&ifAddrStruct);
	    while (ifAddrStruct!=NULL) {
	        if (ifAddrStruct->ifa_addr->sa_family==AF_INET) { // check it is IP4
	            // is a valid IP4 Address
	            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
	            char addressBuffer[INET_ADDRSTRLEN];
	            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
				ipAddrList.push_back(string(addressBuffer));
	        } else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6) { // check it is IP6
	            // is a valid IP6 Address
	            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
	            char addressBuffer[INET6_ADDRSTRLEN];
	            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
				ipAddrList.push_back(string(addressBuffer));
	        } 
	        ifAddrStruct=ifAddrStruct->ifa_next;
	    }
	}
	static void recvRsponse_callback(int fd, short __, void *pair);
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

	~bidderServ()
	{
		m_childtime_lock.destroy();
		m_mastertime_lock.destroy();
	}
private:
	configureObject& m_config;

	string m_throttleIP;
	unsigned short m_throttlePort;
	
	string m_bidderCollectorIP;
	unsigned short m_bidderCollectorPort;

	int m_workerNum;
	
	vector<handler_fd_map*> m_handler_fd_map;
	vector<BC_process_t*>  m_workerList;//children progress list
	vector<string> m_subKeying;//message filtering, if establis or move success, remove or add to m_subKeying
	vector<string> m_subKey;//subKey configure , need establish message filter or remove message filter
		
	void *m_response_handler = nullptr;

	mutex_lock      m_diplayLock;
	mutex_lock      m_childtime_lock;
	struct timeval  m_childlocalTm;;

	mutex_lock      m_mastertime_lock;
	struct timeval  m_masterlocalTm;;
	redisPoolManager m_redis_pool_manager;
	redisPoolManager m_log_redis_manager;
	ThreadPoolManager m_thread_manager;
	
	struct event_base* m_base;
	unsigned short m_thread_pool_size;
	unsigned short m_target_converce_num = 0;
	string m_vastBusinessCode;
	string m_mobileBusinessCode;
	string m_bidder_id;

	bidderInformation   m_bidder_infomation;
	bcInformation       m_bc_infomation;
	void *m_zmq_login_handler=NULL;
	zeromqConnect m_zmq_connect;
	int m_login_fd;
	bidderConfig m_bidder_config;
	string m_zmq_login_identify;
	struct event *m_login_event=NULL;

	bidderManager m_bidder_manager;
	unsigned short m_heartInterval;

	spdlog::level::level_enum m_logLevel;	

	bool           m_logRedisOn=false;
	string         m_logRedisIP;
	unsigned short m_logRedisPort;

	void		   *m_masterPushHandler = NULL;
	void		   *m_masterPullHandler = NULL;
	int 			m_masterPullFd;

	void           *m_workerPullHandler = NULL;
	void           *m_workerPushHandler = NULL;
	int             m_workerPullFd;	
};
#endif


