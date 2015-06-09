#ifndef __ZEROMQCONNECT_H__
#define __ZEROMQCONNECT_H__

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string.h>
#include <unistd.h>
#include "zmq.h"

using namespace std;

class zeromqConnect
{
public:
	zeromqConnect(){}
	void init();
	void* establishConnect(bool client, const char * transType,int zmqType
		,const char * addr,unsigned short port, int *fd);
	void* establishConnect(bool client, const char * transType,int zmqType
		,const char * addr, int *fd);
	void* establishConnect(bool client, bool identy, string& id
		, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd);
	~zeromqConnect();
private:
	void *m_zmqContext = nullptr;	
};
#endif

