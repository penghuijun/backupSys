/*
*/
#ifndef __REDIS_CLIENT__
#define __REDIS_CLIENT__
#include "hiredis.h"
#include <string>
using std::string;
/*
enum class redisReplyType
{
	redisReplyString,
	redisReplyArray,
	redisReplyInt,
	redisReplyNil,
	redisReplyStatus,
	redisReplyErr
};*/


class redisClient
{
public:
	redisClient()=default;
	redisClient(const char *servIp, unsigned short servPort);
	redisClient(string servIP, unsigned short servPort);
	redisClient(string servIP, unsigned short servPort, int savetime);

	void set_redisParam(const char *servIp, unsigned short servPort);
	void set_redisParam(string servIp, unsigned short servPort);
	bool connectaRedis();
	bool connectaRedisWithTimeout(struct timeval tv);
	virtual void start(){};
	virtual void run();
	redisContext *get_redisContex()const
	{
		return m_rc;
	}

	void freeContext(){if(m_rc!=NULL)redisFree(m_rc);};
	void delRedisItem();
	int getDelInterval()const{return delInterval;}
	~redisClient(){	freeContext();}
private:
	int delInterval = 0;
	redisContext *m_rc=nullptr;
	string m_ip;
	unsigned short m_port;
};




#endif
