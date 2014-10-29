#ifndef __BCCONFIG__
#define __BCCONFIG__
#include<iostream>
#include<fstream>
#include<string>
using namespace std;

class bcConfig
{
public:
	bcConfig(string configTxt);
	void display() const;
	const string& get_configFlieName() const{return m_configTxt;}
	const string& get_bcServIP() const {return m_bcServIP;}
	const unsigned short &get_bcListenBidPort() const{return m_bcListenBidPort;}
	const unsigned short &get_bcListenExpirePort() const{return m_bcListenExpirePort;}
	const string &get_redisSaveIP() const {return m_redisSaveIP;}
	const unsigned short &get_redisSavePort() const {return m_redisSavePort;}
	const unsigned long &get_redisSaveTime() const {return m_redisSaveTime;}
	const string &get_redisPublishIP()const {return m_redisPublishIP;}
	const unsigned short &get_redisPublishPort() const {return m_redisPublisPort;}
	const string &get_redisPublishKey() const {return m_redisPublishKey;}
	
	~bcConfig(){m_infile.close();}
private:
	ifstream m_infile;
	string m_configTxt;
	string m_bcServIP;
	unsigned short m_bcListenBidPort;
	unsigned short m_bcListenExpirePort;
	string m_redisSaveIP;
	unsigned short m_redisSavePort;
	unsigned long m_redisSaveTime;
	string m_redisPublishIP;
	unsigned short m_redisPublisPort;
	string m_redisPublishKey;
};

#endif
