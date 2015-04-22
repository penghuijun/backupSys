/*
  *bcConfig
  *auth:yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#ifndef __BCCONFIG_H__
#define __BCCONFIG_H__
#include <iostream>
#include <sstream>
#include <fstream>
#include<string>
#include<vector>
using namespace std;

/*
  *configureObject
  *configure class, file operation
  */
class configureObject
{
public:
	configureObject(const char* configTxt);
	void readConfig();
	void display() const;
	const string& get_configFlieName() const{return m_configName;}

	const string& get_throttleIP() const {return m_throttleIP;}
	unsigned short get_throttlePubVastPort() const{return m_throttlePubVastPort;}

	const string& get_bcIP() const{return m_bcIP;}
	unsigned short get_bcListenBidderPort() const {return m_bcListenBidderPort;}

	const string& get_redisIP() const{return m_redisIP;}
	unsigned short get_redisPort() const{return m_redisPort;}
	unsigned short get_redisSaveTime() const {return m_redisSaveTime;}
	
	unsigned short get_workerNum() const{return m_workNum;}
	unsigned short get_threadPoolSize() const {return m_threadPoolSize;}
	const vector<string>& get_subKey() const {return m_subKey;}

	const string& get_vastBusiCode() const {return m_vastBusiCode;}
	const string& get_mobileBusiCode() const {return m_mobileBusiCode;}

	~configureObject()
	{
		m_infile.close();
		m_subKey.clear();
	}
private:
	bool get_subString(string &src, char first, char end, string &dst);
	bool parseSubInfo(string& subStr, string& oriStr, vector<string>& m_subList);

	ifstream m_infile;
	string m_configName;
	
	string m_throttleIP;
	unsigned short m_throttlePubVastPort=0;

	string m_bcIP;
	unsigned short m_bcListenBidderPort;

	string m_redisIP;
	unsigned short m_redisPort;
	unsigned short m_redisSaveTime;
	
	unsigned short m_threadPoolSize;
	
	unsigned short m_workNum=1;
	vector<string> m_subKey;

	string m_vastBusiCode;
	string m_mobileBusiCode;
	
};

#endif
