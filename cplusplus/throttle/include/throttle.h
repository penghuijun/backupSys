#ifndef __THROTTLE_H__
#define __THROTTLE_H__
#include <string>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <event.h>
#include <map>
#include <set>
#include <list>
#include <queue>
#include "adConfig.h"
#include "redis/hiredis.h"
#include "zmq.h"
#include "thread/threadpoolmanager.h"
#include "throttleManager.h"
#include "lock.h"
#include "spdlog/spdlog.h"
#include "managerProto.pb.h"
#include "redis/redisPoolManager.h"
#include "bufferManager.h"
using namespace com::rj::protos::manager;


using namespace std;
using std::string;

const int	c_master_throttleIP=0x01;
const int	c_master_adPort=0x02;
const int	c_master_publishVast=0x04;
const int	c_workerNum=0x08;

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
	~eventArgment(){}
private:
	struct event_base* m_base;
	void *m_serv;
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



#define PUBLISHKEYLEN_MAX 100

class throttleServ;

struct eventParam
{
	struct event_base* base;
	throttleServ *serv;
};



class fdEventMap
{
public:
	fdEventMap(){}
	fdEventMap(int fd, struct event* event)
	{
		set(fd, event);
	}
	void set(int fd, struct event* event)
	{
		m_fd = fd;
		m_evEvent = event;
	}

	int get_fd(){ return m_fd;}
	struct event* get_event(){return m_evEvent;}
	
	~fdEventMap()
	{
		close(m_fd);
		event_free(m_evEvent);
	}
private:
	int m_fd;
	struct event *m_evEvent=NULL;
};

class throttleServ
{
public:
	throttleServ(configureObject& config);
	vector<BC_process_t*>& getWorkerProList() { return m_workerList;}
	int          zmq_get_message(void* socket, zmq_msg_t &part, int flags);
	static void *broadcastLoginOrHeartReq(void *throttle);
	static void *getTime(void *arg);
	unsigned long long getLonglongTime();
	bool logLevelChange();

	void         displayRecord();
	bool         masterRun();
	void         workerRestart(void *arg);
	bool         needRemoveWorker() const;
	void         run();
	void         start_worker();
	static void  signal_handler(int signo);
	//pthread
	static void *throttleManager_handler(void *throttle);

	//libevent
	static void  recvBidderLogin_callback(int fd, short event, void *pair);
	static void  recvManangerInfo_callback(int fd, short event, void *pair);
	static void  recvAD_callback(int _, short __, void *pair);
	static void  masterPullMsg(int _, short __, void *pair);
	static void  workerPullMsg(int fd, short event, void *pair);
	void publishData(char *msgData, int msgLen);
	void parseAdRequest(char *msgData, int msgLen);

	//sig event
	static void  hupSigHandler(int fd, short event, void *arg);
	static void  intSigHandler(int fd, short event, void *arg);
	static void  termSigHandler(int fd, short event, void *arg);
	static void  usr1SigHandler(int fd, short event, void *arg);

	bool manager_handler(const managerProtocol_messageTrans &from
			   ,const managerProtocol_messageType &type, const managerProtocol_messageValue &value
			   ,managerProtocol_messageType &rspType,struct event_base * base = NULL, void * arg = NULL);
	bool manager_handler(void *handler, string& identify, const managerProtocol_messageTrans &from
				,const managerProtocol_messageType &type, const managerProtocol_messageValue &value
				,struct event_base * base=NULL, void * arg = NULL);
	bool manager_from_connector_handler(const managerProtocol_messageType &type
	  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg);
	bool manager_from_bidder_handler(const managerProtocol_messageType &type
	  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg);
	bool manager_from_BC_handler(const managerProtocol_messageType &type
	  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg);
	static void *logWrite(void *arg);

	~throttleServ()
	{	
		m_displayMutex.destroy();
		m_timeMutex.destroy();
	}
	
	
private:
	void updateConfigure();
	void updataWorkerList(pid_t pid);
	struct timeval m_localTm;	
	configureObject&       m_config;
	mutex_lock             m_displayMutex;
	mutex_lock             m_timeMutex;
	int                    m_workerNum=0;
	vector<BC_process_t*>  m_workerList;	

	string                 m_vastBusiCode;
	string                 m_mobileBusiCode;
	int                    m_heart_interval;
	zeromqConnect          m_zmq_connect;
	
	throttleManager        m_throttleManager;
	redisPoolManager       m_logRedisPoolManger;

	spdlog::level::level_enum  m_logLevel;

	bool            m_logRedisOn;
	string          m_logRedisIP;
	unsigned short  m_logRedisPort;
	logManager      m_logManager;
	
	void           *m_masterPushHandler = NULL;
	void		   *m_masterPullHandler = NULL;
	int             m_masterPullFd;

	void           *m_workerPullHandler = NULL;
	void           *m_workerPushHandler = NULL;
	int             m_workerPullFd;
};



#endif


