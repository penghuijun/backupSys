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
#include "throttleConfig.h"
#include "hiredis.h"
#include "zmq.h"
#include "threadpoolmanager.h"


using namespace std;
using std::string;
#define BUFSIZE 8192


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

class throttleServ;

struct eventParam
{
	struct event_base* base;
	throttleServ *serv;
};

struct fdEvent
{
	int fd;
	struct event *evEvent;
};


class throttleServ
{
public:
	throttleServ(throttleConfig& config);
	
	const char *get_throttleIP() const {return m_throttleIP.c_str();}
	unsigned short get_throttlePort() const {return m_throttleAdPort;}

	void lock(){pthread_mutex_lock(&uuidListMutex);}
	void unlock(){pthread_mutex_unlock(&uuidListMutex);}

	void workerListLock(){pthread_mutex_lock(&workerListMutex);}
	void workerListUnlock(){pthread_mutex_unlock(&workerListMutex);}

	void speed()
	{
	
	static long long m_begTime=0;
	static long long m_endTime=0;
	static int m_count = 0;
		 lock();
		 m_count++;
		 if(m_count%10000==1)
		 {
		 	struct timeval btime;
		 	gettimeofday(&btime, NULL);
			m_begTime = btime.tv_sec*1000+btime.tv_usec/1000;
			 	 unlock();
		 }
		 else if(m_count%10000==0)
		 {
		 	struct timeval etime;
		 	gettimeofday(&etime, NULL);
			m_endTime = etime.tv_sec*1000+etime.tv_usec/1000;
			 	 unlock();
			cout <<"send request num: "<<m_count<<endl;
			cout <<"cost time: " << m_endTime-m_begTime<<endl;
			cout<<"sspeed: "<<10000*1000/(m_endTime-m_begTime)<<endl;
		 }
		 else
		 	{
	 	 unlock();
		 	}
	}
	bool masterRun();
	void masterRestart(struct event_base* base);
	void workerRestart(eventCallBackParam *param);

	void *getAdRspHandler()const{return m_adRspHandler;}
	int  getAdfd() const{return m_adFd;}
	void *getPublishVastHandler() const {return m_publishHandler;}

	int getWorkerNum() const{return m_config.get_throttleworkerNum();}
	
	void closeZmqHandlerResource()
	{
		zmq_close(m_adRspHandler);           
        zmq_close(m_publishHandler);      
		zmq_ctx_destroy(m_zmqContext);	
	}

	vector<BC_process_t*>& getWorkerProList() { return m_workerList;}
	bool needRemoveWorker() const
	{
		return (m_workerList.size()>m_workerNum)?true:false;
	}
	void run();
	void start_worker();

	void addSendUnitToList(void *data, unsigned int dataLen, int workIdx, unsigned int sendID);

	void sendToWorker(char *buf, int size);
	int getWorkChannel() {return workChannel;}
	void listenWorkerEvent(struct event_base* base);

	static void listenEventState(int fd, short __, void *pair);
	static void *listenVastEvent(void *throttle);
    static void *recvFromWorker(void *throttle);
	static void recvFromWorker_cb(int fd, short event, void *pair);
	static void recvAD_callback(int _, short __, void *pair);
	static void signal_handler(int signo);
	static void worker_recvFrom_master_cb(int fd, short event, void *pair);
	static void hupSigHandler(int fd, short event, void *arg);
	static void intSigHandler(int fd, short event, void *arg);
	static void termSigHandler(int fd, short event, void *arg);
	static void usr1SigHandler(int fd, short event, void *arg);


	~throttleServ()
	{
		closeZmqHandlerResource(); 
		pthread_mutex_destroy(&uuidListMutex);
		pthread_mutex_destroy(&workerListMutex);
	}
	
	
private:
	void * establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd);
	void * bindOrConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd);
	void readConfigFile();
	void startNetworkResource();
	void updataWorkerList(pid_t pid);
	
	throttleConfig& m_config;
	void *m_zmqContext = nullptr;	

	string m_throttleIP;
	unsigned short m_throttleAdPort;
	void *m_adRspHandler = nullptr;
	int   m_adFd = 0;
	struct event *m_adEvent=NULL;

	unsigned short m_publishPort=5010;
	void *m_publishHandler=nullptr;

	pthread_mutex_t uuidListMutex;
	pthread_mutex_t workerListMutex;
	
	ThreadPoolManager m_manager;


	unsigned int m_throConfChange = 0;

	int m_workerNum=1;
	vector<BC_process_t*>  m_workerList;
	pid_t m_masterPid;

	unsigned int m_sendToWorkerID=0;

	int workChannel;
	int m_eventFd[2];
	int m_listenWorkerNumFd[2];
	vector<fdEvent*> m_fdEvent;
};



#endif


