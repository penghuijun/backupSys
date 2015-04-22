#ifndef __BIDDERMANAGER_H__
#define __BIDDERMANAGER_H__
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <event.h>
#include <unistd.h>
#include "zmq.h"
#include "adConfig.h"
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

#include "zeromqConnect.h"
#include "login.h"
#include "lock.h"

class bidderObject
{
public:
	bidderObject();
	bidderObject(const string& ip, unsigned short port);
	void set(const string& ip, unsigned short managerPort);
	
	bool loginOrHeart(const string& tip, unsigned short tmanagerPort, unsigned short tdataport);
	bool connect(zeromqConnect& conntor, string& identify, struct event_base* base, event_callback_fn fn, void *arg);
	bool loginedSucess(const string& bidderIP, unsigned short bidderPort);
	
	int            get_loginFd() const;
	void*          get_loginHandler() const;
	const string&  get_bidderIP() const;
	unsigned short get_bidderPort() const;
	
	void  lostTimesClear(){m_lostHeartTimes=0;}
	void set_login_status(bool res){m_throttleLoginedBidder = res;}
	bool get_login_status(){return m_throttleLoginedBidder;}
	
	~bidderObject()	;
private:

	string         m_bidderIP;
	unsigned short m_bidderLoginPort;
	void	 	  *m_bidderLoginHander = NULL;
	int 	       m_bidderLoginFd;
	int            m_lostHeartTimes=0;
	const    int   m_lostHeartTime_max =3;

	struct event  *m_bidderLoginEvent=NULL;	
	bool           m_throttleLoginedBidder = false;

};

class bidderManager
{
public:
	bidderManager();
	void init( bidderInformation& bidderInfo);
	void run(zeromqConnect& conntor, string& identify, struct event_base* base, event_callback_fn fn, void *arg);
	bool loginOrHeart(const string& tip, unsigned short tmanagerPort, unsigned short tdataport, vector<ipAddress> &LostAddrList);
	void *get_login_handler(int fd);
	bool recvHeartFromBidder(const string& bidderIP, unsigned short bidderPort);
	void loginedSucess(const string& bidderIP, unsigned short bidderPort);
	void add_bidder(const string& bidderIP, unsigned short bidderPort, zeromqConnect& conntor, string& throttleID, 
							struct event_base* base,  event_callback_fn fn, void *arg);
	void updateBidder(const vector<bidderConfig*>& bidderConfigList);
	void set_login_status(bool status);
	void get_logined_address(vector<ipAddress*> &addr);
	void reloginDevice(const string& ip, unsigned short port);

	~bidderManager()
	{
		g_manager_logger->info("bidderManager erase");
		m_bidderList_lock.lock();
		for(auto it = m_bidder_list.begin(); it != m_bidder_list.end(); )
		{
			delete *it;
			it = m_bidder_list.erase(it);
		}
		m_bidderList_lock.unlock();
		m_bidderList_lock.destroy();
	}

private:	
	vector<bidderObject*> m_bidder_list;	
	mutex_lock            m_bidderList_lock;
};

#endif

