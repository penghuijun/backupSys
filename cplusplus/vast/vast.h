#ifndef __VAST_H__
#define __VAST_H__
#include<iostream>
#include<string>
#include "vastConfig.h"



using namespace std;
class vast
{
public:
	vast(configureObject &config);
	void * establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd);
	void run(int speed, int cnt);
	void calSpeed();
	int structVastFrame(string &vast);
	int structMobileFrame(string &vast);

	~vast(){}
private:
	void *m_zmqContext;
	void *m_pubHandler;
	string m_throttleIP;
	unsigned short m_throttlePort;
	int m_runTimes;
	int m_adType= 0;
	string m_ttlTime;
	int m_intervalTime=1000;


	string m_campaignID;
	string m_imps_lifetime;
	string m_imps_month;
	string m_imps_week;
	string m_imps_day;
	string m_imps_fiveminute;

	string m_width;
	string m_height;
	target m_target;
	throttleInformation m_throttle;
};


#endif
