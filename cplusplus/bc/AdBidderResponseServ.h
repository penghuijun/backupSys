#ifndef __ADBIDDERRESPONSESERV_H__
#define __ADBIDDERRESPONSESERV_H__
#include "zeroMqServer.h"
#include "redisClient.h"
#include <string>
using std::string;
class adBidderRspServ: public zeroMqServer
{
public:
	
	adBidderRspServ(const char *ip, unsigned short port,const char *redisIP, unsigned short redisPort );
	adBidderRspServ(string ip, unsigned short port,string redisIP, unsigned short redisPort );
	virtual bool  run();
	void * worker_routine(void *context);
private:
	string m_redisServIp;
	unsigned short m_redisServPort;

};
unsigned long long get_cycles();
#endif