#ifndef __CONNECTORMANAGER_H__
#define __CONNECTORMANAGER_H__
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

class connectorObject
{
public:
	connectorObject();
	connectorObject(const string& ip, unsigned short port);
	void set(const string& ip, unsigned short managerPort);
	bool loginOrHeart(const string &ip, unsigned short managerPort, unsigned short dataPort);
	bool connect(zeromqConnect& conntor, string& identify, struct event_base* base, event_callback_fn fn, void *arg);
	bool loginSucess(const string& ip, unsigned short port);
	int            get_loginFd()const;
	void*          get_loginHandler()const;
	const string&        get_devIP()const;
	unsigned short get_managerPort()const;
	void  lostTimesClear(){m_lostHeartTimes=0;}
	void set_login_status(bool res){m_throttleLogined = res;}
	bool get_login_status(){return m_throttleLogined;}
	~connectorObject()	;
private:

	string         m_devIP;
	unsigned short m_managerPort;
	void	 	  *m_managerHander = NULL;
	int 	       m_managerFd;
	int            m_lostHeartTimes=0;
	const    int   m_lostHeartTime_max =3;

	struct event  *m_managerEvent=NULL;	
	bool           m_throttleLogined = false;

};

class connectorManager
{
public:
	connectorManager();
	void init(connectorInformation& connInfo);
	void run(zeromqConnect& conntor, string& identify, struct event_base* base, event_callback_fn fn, void *arg);
	bool loginOrHeart(const string &ip, unsigned short managerPort, unsigned short dataPort, vector<ipAddress> &LostAddrList);
	void *get_login_handler(int fd);
	bool recvHeartRsp(const string& ip, unsigned short port);
	void loginSucess(const string& ip, unsigned short port);
	void add_connector(const string& ip, unsigned short managerPort, zeromqConnect& conntor, string& throttleID, 
							struct event_base* base,  event_callback_fn fn, void *arg);
	void update(const vector<connectorConfig*>& configList);
	void set_login_status(bool status);
	void reloginDevice(const string& ip, unsigned short port);

	~connectorManager()
	{
		m_manager_lock.lock();
		for(auto it = m_dev_list.begin(); it != m_dev_list.end(); )
		{
			delete *it;
			it = m_dev_list.erase(it);
		}
		m_manager_lock.unlock();
	}

private:	
	vector<connectorObject*> m_dev_list;	
	mutex_lock               m_manager_lock;
};

#endif

