#ifndef __CONNECTORMANAGER_H__
#define __CONNECTORMANAGER_H__

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <event.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include "adConfig.h"
#include "login.h"
#include "zmq.h"
#include "lock.h"
#include "spdlog/spdlog.h"
#include "managerProto.pb.h"
using namespace com::rj::protos::manager;


extern shared_ptr<spdlog::logger> g_manager_logger;

class connectorObject
{
public:
	connectorObject();
	connectorObject(const string &ip, unsigned short port);
	
	void set(const string &ip, unsigned short port);
	bool connect(zeromqConnect& conntor,  string& bc_id, struct event_base* base,  event_callback_fn fn, void *arg);
	bool login(string &ip, unsigned short managerPort, unsigned short dataPort);
	const string&        get_connector_ip() const;
	unsigned short get_connector_port() const;
	int            get_fd();
	void *         get_managerHandler();
	int            get_heart_times();
	void           init_connector()
	{
		m_registed = false;
		m_heart_times = 0;
	}

	void  set_register_status(bool registerStatus);
	bool  get_register_status(){return m_registed;}
	void  heart_times_increase();
	void  heart_times_clear();
	~connectorObject();
private:
	//bidder Addr
	string          m_ip;
	unsigned short  m_manangerPort;

    //zeromq libevent 
	void           *m_managerHander = NULL;
	int 	  	    m_manangerFd;
	struct event   *m_manangerEvent=NULL;
	
	//bc registed the bidder
	bool	        m_registed;
	
	//lost heart times, heart between bc and bidder
	int             m_heart_times;
};

class connectorManager
{
public:
	connectorManager();
	void init(vector<connectorConfig*>& confList);

	void connect(zeromqConnect& zmqconn,  string& bc_id, struct event_base* base,  event_callback_fn fn, void *arg);

	void login(string &ip, unsigned short managerPort, unsigned short dataPort);

	void *get_managerHandler(int fd);
	void register_sucess(const string &ip, unsigned short port);
	bool recv_heart_time_increase(string& bcID, vector<string>& unSubList);
	bool recv_heart(const string& ip, unsigned short port);
	bool add_connector(const string& ip, unsigned short port,zeromqConnect& conntor
                                ,string & bc_id,struct event_base * base,event_callback_fn fn,void * arg);
	int size(){return m_connector_list.size();}
	void update(const vector<connectorConfig*>& configList);

	~connectorManager();
private:
	vector<connectorObject*> m_connector_list;
	const  int c_lost_times_max;	//default: 3
	mutex_lock m_manager_lock;
};

#endif
