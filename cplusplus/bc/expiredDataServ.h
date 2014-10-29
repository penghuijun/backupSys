#ifndef __EXPIREDATASERV_H__
#define __EXPIREDATASERV_H__
#include "zeroMqServer.h"
#include <string>
using std::string;

class expireDataServ: public zeroMqServer
{
public:
	expireDataServ(const char *ip, unsigned short port):zeroMqServer(ip, port){}
	expireDataServ(string ip, unsigned short port):zeroMqServer(ip, port){}
	//using zeroMqServer::zeroMqServer;
	virtual bool  run();
private:

};

#endif

