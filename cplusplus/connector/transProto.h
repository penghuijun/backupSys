#ifndef __TRANSPROTO_H__
#define __TRANSPROTO_H__
#include <iostream>
#include <string>
#include <event.h>
#include "adConfig.h"
#include "redisPool.h"
using namespace std;
#define BUFSIZE 4096

enum transErrorStatus
{
	error_socket_close = 0,
	error_recv_null = -1,
	error_cannot_find_fd = -2,
	error_data_imcomplete = -3,
	error_complete = -4
};

class transProtocol
{
public:
	transProtocol(){}
	transProtocol(const string& ip, unsigned short port)
	{
		set(ip, port);
	}

	virtual bool pro_connect(){};
    virtual bool pro_connect_asyn(struct event_base * base, event_callback_fn fn, void * arg)
	{
	}	
	virtual bool pro_close(){};
	virtual int  pro_send(const char * str){};
	virtual	transErrorStatus  recv_async(stringBuffer &recvBuf){};
	const string& get_ip() const{return m_ip;}
	unsigned short get_port() const{return m_port;}
	int          get_fd() const{return m_fd;}
	bool get_sending()
	{
		if(m_sending) return false;
		m_sending = true;
		return true;
	}

	virtual bool update_connect(int fd)
	{
		if(m_fd == fd)
		{
			m_sending = false;
			return true;
		}
		return false;
	}

	void check_connect(struct event_base * base, event_callback_fn fn, void * arg)
	{
		if((m_fd <= 0)||(m_event == NULL))
		{
			pro_connect_asyn(base, fn, arg);
		}
	}
	void set(const string& ip, unsigned short port)
	{
		m_ip = ip;
		m_port = port;
	}
	virtual ~transProtocol(){};
protected:
	string m_ip;
	unsigned short m_port;
	struct event *m_event = NULL;//event
	int    m_fd = -1;
	bool   m_sending = false;
};
#endif