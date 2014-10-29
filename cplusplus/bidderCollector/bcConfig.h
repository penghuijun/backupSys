/*
  *bcConfig
  *auth:yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#ifndef __BCCONFIG_H__
#define __BCCONFIG_H__
#include<iostream>
#include<fstream>
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
	unsigned short get_redisConnectNum() const {return m_redisConnectNum;}
	const vector<string>& get_subKey() const {return m_subKey;}

	~configureObject()
	{
		m_infile.close();
		m_subKey.clear();
	}
private:
	bool get_subString(string &src, char first, char end, string &dst);
	ifstream m_infile;
	string m_configName;
	
	string m_throttleIP;
	unsigned short m_throttlePubVastPort=0;

	string m_bcIP;
	unsigned short m_bcListenBidderPort;

	string m_redisIP;
	unsigned short m_redisPort;
	unsigned short m_redisSaveTime;
	unsigned short m_redisConnectNum;
	
	unsigned short m_workNum=1;
	vector<string> m_subKey;
};

#endif
