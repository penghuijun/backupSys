#ifndef __THROTTLE_H__
#define __THROTTLE_H__
#include <string>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <map>
#include <set>
#include "throttleConfig.h"
#include "hiredis.h"
#include "zmq.h"
#include "threadpoolmanager.h"

using namespace std;
using std::string;
#define BUFSIZE 4096


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
};

#define PUBLISHKEYLEN_MAX 20


class throttleServ
{
public:
	throttleServ(throttleConfig& config);
	const char *get_throttleIP() const {return m_throttleIP.c_str();}
	unsigned short get_throttlePort() const {return m_throttleAdPort;}
	const void *get_zmqContext() const{return m_zmqContext;}
	void *  establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port);

	void *expireServWork();
	void *servPollWork();
	void *adWork();

	void startPull();
	bool masterRun();
	void workerStart();
	void workerHandler(char *buf, unsigned int size);

	int workerRecvPullData(char *buf,int bufsize)
	{
		return zmq_recv(m_clientPullHandler, buf, bufsize,0);
	}

	int workerPushData(char *buf,int bufsize)
	{
		return zmq_send(m_clientPushHandler, buf, bufsize,0);
	}

	~throttleServ()
	{
		if(m_adRspHandler!=nullptr) zmq_close(m_adRspHandler);
		if(m_zmqExpireRspHandler!=nullptr) zmq_close(m_zmqExpireRspHandler);
		if(m_publishHandler!=nullptr) zmq_close(m_publishHandler);
		if(m_servPushHandler!=nullptr) zmq_close(m_servPushHandler);
		if(m_servPollHandler!=nullptr) zmq_close(m_servPollHandler);
		if(m_clientPullHandler!=nullptr) zmq_close(m_clientPullHandler);
		if(m_clientPushHandler!=nullptr) zmq_close(m_clientPushHandler);
		if(m_zmqContext!=nullptr) zmq_ctx_destroy(m_zmqContext);
	}
	
private:
	void *m_zmqContext = nullptr;	

	string m_throttleIP;
	unsigned short m_throttleAdPort;
	void *m_adRspHandler = nullptr;

	unsigned short m_throttleExpirePort;
	void *m_zmqExpireRspHandler = nullptr;

	unsigned short m_publishPort=5010;
	void *m_publishHandler=nullptr;

	unsigned short m_servPushPort=5557;
	unsigned short m_servPullPort=5558;
	void *m_clientPullHandler=nullptr;
	void *m_clientPushHandler=nullptr;
	void *m_servPushHandler=nullptr;
	void *m_servPollHandler=nullptr;

	ThreadPoolManager m_manager;

};
#endif


