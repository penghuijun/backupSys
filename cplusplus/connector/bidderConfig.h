/*
  *bidderConfig
  *auth:yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#ifndef __BIDDERCONFIG_H__
#define __BIDDERCONFIG_H__
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
using namespace std;

struct exBidderInfo
{
	vector<string> subKey;
	string exBidderIP;
	unsigned short exBidderPort;
	unsigned short connectNum;
};



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
	unsigned short get_throttlePort() const{return m_throttlePort;}

	const string& get_bidderCollectorIP() const {return m_bidderCollectorIP;}
	unsigned short get_bidderCollectorPort() const{return m_bidderCollectorPort;}
	unsigned short get_bidderworkerNum() const{return m_workNum;}
	vector<string> get_subKey() const {return m_subKey;}
	const vector<exBidderInfo*>& get_exBidderList() const{return m_exBidderList;} 

	~configureObject()
	{
		m_infile.close();
		m_subKey.clear();
	}
private:
	bool get_subString(string &src, char first, char end, string &dst);
	ifstream m_infile;
	unsigned short m_workNum=1;
	string m_configName;
	string m_throttleIP;
	unsigned short m_throttlePort=0;
	string m_bidderCollectorIP;
	unsigned short m_bidderCollectorPort=0;
	vector<string> m_subKey;

	vector<exBidderInfo*> m_exBidderList;
};

#endif
