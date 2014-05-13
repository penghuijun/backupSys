/**/
#ifndef __ZEROMQSERVER__
#define __ZEROMQSERVER__
#include<zmq.h>
#include <string>
#include "redisClient.h"

#define MQ_BUF_LEN 1024
using std::string;

enum BCStatus
{
	BC_Invalid,
	BC_recvBid,
	BC_recvSub,
	BC_recvBidAndSub	
};

struct bcDataRec
{
	string bidIDAndTime;
	int time;
	BCStatus status;
	char *buf;
	int bufSize;
};


struct redisDataRec
{
	string bidIDAndTime;
	int    time;
};



class zeroMqServer
{
public:
	zeroMqServer()=default;
	zeroMqServer(const char *ip, unsigned short port);
	zeroMqServer(string ip, unsigned short port);
	void set(const char *ip, unsigned short port);
	void set(string ip, unsigned short port);
	virtual bool run()=0;
	~zeroMqServer()
	{
		zmq_close(m_rspHandler);
		zmq_term(m_context);
	}

	void *get_rspHander() const{return m_rspHandler;}
	void *get_context() const{return m_rspHandler;}
	const char *get_ip() const{return m_ip.c_str();}
	unsigned short get_port() const {return m_port;}

	void set_context(void * context){ m_context = context;}
	void set_rspHandler(void *handler){m_rspHandler = handler;}
private:
	void *m_context=nullptr;
	void *m_rspHandler=nullptr;
	string m_ip;
	unsigned short m_port;
};


#endif
