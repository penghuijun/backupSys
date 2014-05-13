/*
*/
#ifndef __SUBSCRIBECLIENT_H__
#define __SUBSCRIBECLIENT_H__
#include <string>
#include <map>
#include "redisClient.h"


using std::string;

class subscribeClient: public redisClient
{
public:
	subscribeClient()=default;
	subscribeClient(const char *ip, unsigned short port, const char *str, const char *redisIP, unsigned short redisPort);
	subscribeClient(string ip, unsigned short port, string str, string redisIP, unsigned short redisPort);
	void set_subStr(const char *str)
	{
		m_subStr = str;
	}
	void set_subStr(string str)
	{
		m_subStr = str;
	}
	virtual void start();
	virtual void run();
private:
	string m_subStr;
	string m_subValue;
	string m_saveDataRedisIP;
	unsigned short m_saveDataRedisPort;
};


#endif
