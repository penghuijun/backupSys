#ifndef __BCMANAGER_H__
#define __BCMANAGER_H__
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
#include "managerProto.pb.h"
using namespace com::rj::protos::manager;

class bidderCollecter
{
public:
	bidderCollecter();
	bidderCollecter(const string& bc_ip, unsigned short managerPort);
	void set(const string& bc_ip, unsigned short managerPort);
	bool connect(zeromqConnect& conntor, string& throttleID, struct event_base* base,  event_callback_fn fn, void *arg);
	bool loginOrHeart(const string& tip, unsigned short tmanagerPort, unsigned short tdataport);
	bool loginedSuccess(const string& bcIP, unsigned short bcPort);
	void *			get_handler(int fd) const ;
	const string&   get_bcIP()const;
	unsigned short  get_bcManagerPort()const;
	bool bcManagerAddr_compare(string& bcIP, unsigned short bcMangerPort);
	void lostHeartTimeClear(){m_lostHeartTimes = 0;}
	void set_login_success(bool status)
	{
		m_throttleLoginedBC = status;
	}
	~bidderCollecter();

private:
	string         m_bcIP;
	unsigned short m_bcManagerPort;		
	void	 	  *m_bcManagerHander = NULL;
	int 	       m_bcManagerFd;
	struct event  *m_bcManagerEvent=NULL;
	bool           m_throttleLoginedBC=false;
	int            m_lostHeartTimes = 0;
	const int      m_lostHeartTime_max = 3;
};

class bcManager
{
public:
	bcManager();
	void init( bcInformation& bcInfo);
	void run(zeromqConnect& conntor, string& throttleID, struct event_base* base, event_callback_fn fn, void *arg);
	void *get_manger_handler(int fd);
	void loginedSuccess(const string& ip, unsigned short port);
	void erase_bc(string &bcIP, unsigned short bcManagerPort);
	bool loginOrHeart(const string& tip, unsigned short tmanagerPort, unsigned short tdataport , vector<ipAddress> &LostAddrList);
	void add_bc(const string& bcIP, unsigned short bcManagerPort, zeromqConnect& conntor, string& throttleID, 
						struct event_base* base,  event_callback_fn fn, void *arg);
	bool recvHeartFromBC(const string& bcIP, unsigned short bcPort);	
	void updateBC(const vector<bcConfig*>& bcConfigList);
	void set_login_status(bool status);
	void reloginDevice(const string& ip, unsigned short port);

	~bcManager()
	{
		m_bcList_lock.lock();
		for(auto it = m_bidderCollecter_list.begin(); it != m_bidderCollecter_list.end();)
		{
			delete *it;
			it = m_bidderCollecter_list.erase(it);
		}
		m_bcList_lock.unlock();
		m_bcList_lock.destroy();
	}

private:
	mutex_lock               m_bcList_lock;
	vector<bidderCollecter*> m_bidderCollecter_list;
};

#endif
