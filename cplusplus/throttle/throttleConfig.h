#ifndef __THROTTLECONFIG_H__
#define __THROTTLECONFIG_H__
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include "hiredis.h"
using namespace std;

class throttleRedisPublishKey
{
public:
	throttleRedisPublishKey()=default;
	throttleRedisPublishKey(string business, string country, string language, string os, string browser):m_business(business), m_country(country),\
		m_language(language), m_os(os), m_browser(browser){}
	throttleRedisPublishKey(const throttleRedisPublishKey &pubkey):m_business(pubkey.m_business),m_country(pubkey.m_country), m_language(pubkey.m_language),\
		m_os(pubkey.m_os), m_browser(pubkey.m_browser){}
	void set_business(string business) {m_business = business;}
	void set_country(string country) {m_country = country;}
	void set_language(string language) {m_language = language;}
	void set_os(string os) {m_os = os;}
	void set_browser(string browser) {m_browser = browser;}
	
	const string& get_business() const {return m_business;}
	const string& get_country() const{return m_country;}
	const string& get_language() const{return m_language;}
	const string& get_os() const {return m_os ;}
	const string& get_browser() const{return m_browser;}

	bool operator==(throttleRedisPublishKey& pubKey) const
	{
		return ((m_business==pubKey.m_business)&&(m_country == pubKey.m_country)&&(m_language ==pubKey.m_language)&&(m_os==pubKey.m_os)&&(m_browser==pubKey.m_browser));
	}
private:
	string m_business;
	string m_country;
	string m_language;
	string m_os;
	string m_browser;	
};


class throttleInfo
{
public:
	throttleInfo()=default;
	throttleInfo(string redisIP, unsigned short port):m_redisIP(redisIP), m_redisPort(port){}
	throttleInfo(throttleInfo &info):m_redisIP(info.m_redisIP), m_redisPort(info.m_redisPort)
		{
				publishKeyList = info.publishKeyList;
		}
	void set_redisIP(string redisIP) {m_redisIP = redisIP;}
	void set_redisPort(unsigned short port) {m_redisPort = port;}
	void set_publishKey(throttleRedisPublishKey &publishKey)
	{
		vector<throttleRedisPublishKey>::iterator it = publishKeyList.begin();
		for(it = publishKeyList.begin(); it != publishKeyList.end(); it++)
		{
			throttleRedisPublishKey pKey = *it;
			if(pKey==publishKey) break;
		}
		if(it==publishKeyList.end())
		{	
			publishKeyList.push_back(publishKey);
		}
	}

	void set_publishKey(string business, string country, string language, string os, string browser)
	{
		throttleRedisPublishKey pubKey(business, country, language, os, browser);
		set_publishKey(pubKey);
	}
	
	void set_redisContext(redisContext *redisContext){m_redisContext = redisContext;}
	redisContext* get_redisContext()const{return m_redisContext;}
	const string& get_redisIP() const {return m_redisIP;}
	unsigned short get_redisPort() const {return m_redisPort;}

	redisContext *findRedisContext(string business, string country, string language, string os, string browser) const
	{
		vector<throttleRedisPublishKey>::const_iterator it=publishKeyList.begin();
		for(it = publishKeyList.begin(); it != publishKeyList.end(); it++)
		{
			throttleRedisPublishKey key = *it;
			if(key.get_business()== business && key.get_country() == country && key.get_language()==language&&key.get_os()==os&&key.get_browser()==browser)
			{
				return m_redisContext;
			}
		}
		return nullptr;
	}

	bool findRedisExsit(string business, string country, string language, string os, string browser)
	{
		vector<throttleRedisPublishKey>::iterator it=publishKeyList.begin();
		for(it = publishKeyList.begin(); it != publishKeyList.end(); it++)
		{
			throttleRedisPublishKey key = *it;
			string tbusi=key.get_business();
			string tcoun=key.get_country();
			string tlang=key.get_language();
			string tos = key.get_os();
			string tbrow = key.get_browser();
			if((tbusi == "*" || tbusi== business) && (tcoun == "*" ||tcoun == country) && (tlang == "*" || tlang==language)&& (tos == "*" ||tos==os)&&(tbrow == "*" ||tbrow==browser))
			{
				cout << "findRedisExsit" << endl;
				return true;
			}
		}
		cout << "not findRedisExsit" << endl;
		return false;
	}
	
	throttleInfo &operator=(const throttleInfo& info)
	{
		if(this !=  &info)
		{
			m_redisIP = info.m_redisIP;
			m_redisPort = info.m_redisPort;
			publishKeyList = info.publishKeyList;
		}
		return *this;
	}

	void display() const
	{
		cout << "redis IP:" << m_redisIP << endl;
		cout << "redis Port:" << m_redisPort << endl;
		cout << "redis context:"<< m_redisContext << endl;
		vector<throttleRedisPublishKey>::const_iterator it = publishKeyList.begin();
		for(it = publishKeyList.begin(); it != publishKeyList.end(); it++)
		{
			throttleRedisPublishKey key = *it;
			cout<<key.get_business()<<'-'<<key.get_country()<<'-'<<key.get_language()<<'-'<<key.get_os()<<'-'<<key.get_browser()<<endl;
		}
	}
	
private:
	string m_redisIP;
	unsigned short m_redisPort;
	redisContext *m_redisContext=nullptr;
	vector<throttleRedisPublishKey> publishKeyList;
};


class throttleConfig
{
public:
	throttleConfig(const char* configTxt);
	void readConfig();
	void display() const;
	const string& get_configFlieName() const{return m_configName;}
	const string& get_throttleIP() const {return m_throttleIP;}
	unsigned short get_throttleAdPort() const{return m_throttleAdPort;}
	unsigned short get_throttleExpirePort() const{return m_throttleExpirePort;}
	unsigned short get_throttleServPushPort() const{return m_servPushPort;}
	unsigned short get_throttleServPullPort() const{return m_servPullPort;}
	unsigned short get_throttlePubPort() const {return m_pubPort;}
	unsigned short get_throttlePubExpPort() const {return m_pubExpPort;}
	unsigned short get_throttleworkerNum() const{return m_workNum;}

	~throttleConfig()
	{
		m_infile.close();
	}
private:
	ifstream m_infile;
	unsigned short m_workNum=1;
	string m_configName;
	string m_throttleIP;
	unsigned short m_throttleAdPort=0;
	unsigned short m_throttleExpirePort=0;
	unsigned short m_servPushPort=0;
	unsigned short m_servPullPort=0;
	unsigned short m_pubPort=0;
	unsigned short m_pubExpPort=0;
};

#endif
