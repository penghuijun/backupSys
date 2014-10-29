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
#define BUFSIZE 4096


const int	c_master_throttleIP=0x01;
const int	c_master_adPort=0x02;
const int	c_master_expPort=0x04;
const int	c_master_servPush=0x08;
const int	c_master_servPoll=0x10;
const int	c_master_publishVast=0x20;
const int	c_master_publishExp=0x40;
const int	c_workerNum=0x80;

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


class throttleServ
{
public:
	throttleServ(throttleConfig& config);
	void reloadConfig()
	{
		m_throConfChange = 0;
		readConfigFile();
	}
	
	const char *get_throttleIP() const {return m_throttleIP.c_str();}
	unsigned short get_throttlePort() const {return m_throttleAdPort;}
	const void *get_zmqContext() const{return m_zmqContext;}

	void lock(){pthread_mutex_lock(&uuidListMutex);}
	void unlock(){pthread_mutex_unlock(&uuidListMutex);}

	void workerListLock(){pthread_mutex_lock(&workerListMutex);}
	void workerListUnlock(){pthread_mutex_unlock(&workerListMutex);}

	void uuidListInsert(string uuid, string pub)
	{
		 lock();
		// uuidList.insert(pair<string, string>(uuid, pub)); 
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
	void masterRestart();
	void workerRestart(eventCallBackParam *param);
	void workerHandler();

	bool getPublishKey(string& uuid, string &publishKey)
	{
		lock();
		auto it = uuidList.find(uuid);
		if(it != uuidList.end())
		{
			publishKey = it->second;
			uuidList.erase(it);
			unlock();
			return true;
		}
		else 
		{
			unlock();
		}
		return false;
	}

	void stopWorkerData(eventCallBackParam *param);
	void *getAdRspHandler()const{return m_adRspHandler;}
	int  getAdfd() const{return m_adFd;}
	int  getExpFd() const{return m_expFd;}
	void *getExpRspHandler() const{return m_zmqExpireRspHandler;}
	void *getPublishVastHandler() const {return m_publishHandler;}
	void *getPublishExpHandler() const {return m_publishExpireHandler;}

	int getWorkerNum() const{return m_config.get_throttleworkerNum();}
	
	void closeZmqHandlerResource()
	{
		int rc1 = zmq_close(m_adRspHandler); 
        int rc2 =zmq_close(m_zmqExpireRspHandler);     
        int rc3=zmq_close(m_publishHandler);      
        int rc4=zmq_close(m_publishExpireHandler);  
		int rc = zmq_ctx_destroy(m_zmqContext);	
		cout <<"zmq_ctx_destroy::"<<rc1<<":"<<rc2<<":"<<rc3<<":"<<rc4<<":"<<":"<<rc<<endl;
	}

	vector<BC_process_t*>& getWorkerProList() { return m_workerList;}
	bool needRemoveWorker() const
	{
		return (m_workerList.size()>m_workerNum)?true:false;
	}
	void run();
	void workerLoop();

	void addSendUnitToList(void *data, unsigned int dataLen, int workIdx, unsigned int sendID);

	void sendToWorker(char *buf, int size);
	int getWorkChannel() {return workChannel;}

	bool getWorkerChannelChanged()
	{
		bool ch ;
		workerListLock();
		ch = m_workerChannelChanged;
		workerListUnlock();
		return ch;
	}
	void setWorkerChannelChanged(bool state)
	{
		workerListLock();
		m_workerChannelChanged = state;
		workerListUnlock();
	}
	bool updateServWorkerEvent(void *pair);

	~throttleServ()
	{
		closeZmqHandlerResource(); 
		pthread_mutex_destroy(&uuidListMutex);
		pthread_mutex_destroy(&workerListMutex);
	}
	
private:
	void * establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd);
	void * bindOrConnect(unsigned int configure, bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd);
	void readConfigFile();
	void startNetworkResource();
	void startWorkerConnect();
	void updataWorkerList(pid_t pid);
	throttleConfig& m_config;
	void *m_zmqContext = nullptr;	

	string m_throttleIP;
	unsigned short m_throttleAdPort;
	void *m_adRspHandler = nullptr;
	int   m_adFd = 0;

	unsigned short m_throttleExpirePort;
	void *m_zmqExpireRspHandler = nullptr;
	int m_expFd=0;

	unsigned short m_publishPort=5010;
	void *m_publishHandler=nullptr;

	unsigned short m_publishExpirePort=5020;
	void *m_publishExpireHandler=nullptr;
/*
	unsigned short m_servPushPort=5557;
	unsigned short m_servPullPort=5558;
	void *m_clientPullHandler=nullptr;
	void *m_clientPushHandler=nullptr;
	void *m_servPushHandler=nullptr;
	void *m_servPollHandler=nullptr;
	int m_clientPullFd = 0;
	int m_servPullFd = 0;
*/
	pthread_mutex_t uuidListMutex;
	pthread_mutex_t workerListMutex;
	map<string, string> uuidList;
	
	ThreadPoolManager m_manager;

	long long m_begTime;
	long long m_endTime;
	int m_count = 0;

	unsigned int m_throConfChange = 0;

	pthread_t m_adVastPthread;
	pthread_t m_adExpPthread;
	pthread_t m_servPullPthread;

	int m_workerNum=1;
	vector<BC_process_t*>  m_workerList;
	pid_t m_masterPid;
	bool m_masterStart=false;
	unsigned int m_sendToWorkerID=0;

	int workChannel;

	bool m_workerChannelChanged=false;
};



#endif


