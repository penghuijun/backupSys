#ifndef __THROTTLECONFIG_H__
#define __THROTTLECONFIG_H__
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
using namespace std;

class throttleConfig
{
public:
	throttleConfig(const char* configTxt);
	void readConfig();
	void display() const;
	const string& get_configFlieName() const{return m_configName;}
	const string& get_throttleIP() const {return m_throttleIP;}
	unsigned short get_throttleAdPort() const{return m_throttleAdPort;}
	unsigned short get_throttlePubPort() const {return m_pubPort;}
	unsigned short get_throttleworkerNum() const{return m_workNum;}

	~throttleConfig()
	{
		m_infile.close();
	}
private:
	bool get_subString(string &src, char first, char end, string &dst);
	ifstream m_infile;
	unsigned short m_workNum=1;
	string m_configName;
	string m_throttleIP;
	unsigned short m_throttleAdPort=0;
	unsigned short m_pubPort=0;
};

#endif
