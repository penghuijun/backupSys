#ifndef __BIDDERMANAGER_H__
#define __BIDDERMANAGER_H__

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

class bidderObject
{
public:
	bidderObject();
	bidderObject(const string &bidder_ip, unsigned short bidder_port);
	
	void set(const string &bidder_ip, unsigned short bidder_port);
	bool startConnectBidder(zeromqConnect& conntor,  string& bc_id, struct event_base* base,  event_callback_fn fn, void *arg);
	bool registerToBidder(string &ip, unsigned short managerPort, unsigned short dataPort);
	string&        get_bidder_ip();
	unsigned short get_bidder_port();
	int            get_fd();
	void *         get_managerHandler();
	int            get_heart_times();

	bool           devDrop(string& bcIdentify, vector<string>& unSubList)
	{
        if(m_registed)
        {
            ostringstream os;
            m_heart_times++;
            if(m_heart_times > c_lost_times_max)
            {
            	m_heart_times = 0;
            	m_registed = false;
                os<<bcIdentify<<"-"<<m_bidderIP<<":"<<m_bidderManangerPort;
                string unsub = os.str();
                unSubList.push_back(unsub);
                return true;
            }
        }	
		return false;
	}
	
	void           init_bidder()
	{
		m_registed = false;
		m_heart_times = 0;
	}

	void  set_register_status(bool registerStatus);
	bool  get_register_status(){return m_registed;}
	void  heart_times_increase();
	void  heart_times_clear();

	void  bidder_ready();
	~bidderObject();
private:
	//bidder Addr
	string          m_bidderIP;
	unsigned short  m_bidderManangerPort;

    //zeromq libevent 
	void           *m_bidderManagerHander = NULL;
	int 	  	    m_bidderManangerFd;
	struct event   *m_bidderManangerEvent=NULL;
	
	//bc registed the bidder
	bool	        m_registed;
	const  int       c_lost_times_max;		//default: 3
	//lost heart times, heart between bc and bidder
	int             m_heart_times;
};

class bidderManager
{
public:
	bidderManager();
	void init(vector<bidderConfig*>& bidderConfList);

	void startConnectToBidder(zeromqConnect& conntor,  string& bc_id, struct event_base* base,  event_callback_fn fn, void *arg);

	void loginBidder(string &ip, unsigned short managerPort, unsigned short dataPort);

	void *get_bidderManagerHandler(int fd);
	void register_sucess(const string &bidderIP, unsigned short bidderPort);
	bool recv_heart_from_bidder(const string& bidderIP, unsigned short bidderPort);
	bool devDrop(string& bcID, vector<string>& unSubList);
	bool add_bidder(const string& bidderIP, unsigned short bidderPort
				      ,zeromqConnect & conntor,string & bc_id,struct event_base * base,event_callback_fn fn,void * arg);
	int size(){return m_bidder_list.size();}
	void update_bidder(const vector<bidderConfig*>& bidderConfigList);

	~bidderManager();
private:
	vector<bidderObject*> m_bidder_list;

	mutex_lock m_bidderList_lock;
};

#endif
