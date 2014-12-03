/*
  *bidderConfig
  *auth:yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#ifndef __BIDDERCONFIG_H__
#define __BIDDERCONFIG_H__
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
	bool readFileToString(const char* file_name, string& fileData);
	void readConfig();
	void display() const;
	const string& get_configFlieName() const{return m_configName;}
	const string& get_bidderID() const{return m_bidderID;}	
	const string& get_throttleIP() const {return m_throttleIP;}
	unsigned short get_throttlePort() const{return m_throttlePort;}

	const string& get_bidderCollectorIP() const {return m_bidderCollectorIP;}
	unsigned short get_bidderCollectorPort() const{return m_bidderCollectorPort;}

	const string& get_redisIP() const{return m_redis_ip;}
	unsigned short get_redisPort() const {return m_redis_port;}
	unsigned short get_threadPoolSize() const{return m_threadPoolSize;}
	unsigned short get_bidderworkerNum() const{return m_workNum;}
	unsigned short get_converceNum() const{return m_converce_num;}	
	const string& get_vastBusiCode() const{return m_vastBusiCode;}
	const string& get_mobileBusiCode() const{return m_mobileBusiCode;}

	vector<string> get_subKey() const {return m_subKey;}
	bool parseSubInfo(string& subStr, string& oriStr, vector<string>& m_subList);
	~configureObject()
	{
		m_infile.close();
		m_subKey.clear();
	}
private:
	bool get_subString(string &src, char first, char end, string &dst);
	ifstream m_infile;
	string m_bidderID;
	unsigned short m_workNum=1;
	string m_configName;
	string m_throttleIP;
	unsigned short m_throttlePort=0;
	string m_bidderCollectorIP;
	unsigned short m_bidderCollectorPort=0;
	vector<string> m_subKey;
	string m_redis_ip;
	unsigned short m_redis_port=0;
	unsigned short m_threadPoolSize=0;
	string m_vastBusiCode;
	string m_mobileBusiCode;
	unsigned short m_converce_num=0;

//	configureSubInfo m_configSubInfo;
};

#endif
