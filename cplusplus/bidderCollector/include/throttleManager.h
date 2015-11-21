#ifndef __THROTTLEMANAGER_H__
#define __THROTTLEMANAGER_H__
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
#include "zmq.h"
#include "adConfig.h"
#include "login.h"
#include "lock.h"
#include "spdlog/spdlog.h"
#include "managerProto.pb.h"
using namespace com::rj::protos::manager;

extern shared_ptr<spdlog::logger> g_manager_logger;

class zmqSubscribeKey
{
public:
	zmqSubscribeKey();
	zmqSubscribeKey(const string& bidderIP, unsigned short bidderPort, string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort);
	void set(const string& bidderIP, unsigned short bidderPort, string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort);

	string&    	   get_bidder_ip();
	unsigned short get_bidder_manager_port();
	string&        get_bc_ip();
	unsigned short get_bc_manager_port();
	unsigned short get_bc_data_port();
	string&        get_subKey();
	bool           get_regist_status();
	bool           set_registed(){m_registerToBidder = true;}

	bool subKey_equal(const string &bidderIP, unsigned short bidderPort
		, const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort);

	bool subKey_compare(string& bidderIP, unsigned short bidderPort, string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort)
	{
		if((bidderIP == m_bidderIP)&&(bidderPort == m_bidderManagerPort)
			&&(bcIP == m_bcIP)&&(bcManagerPort == m_bcManagerPort)&&(bcDataPort == m_bcDataPort))
		{
			return true;
		}
		return false;
	}

	~zmqSubscribeKey();
private:
//bcIP:bcManagerPort:bcDataPort-bidderIP:bidderManangerPort
//every subkey must register to bidder
	string         m_bidderIP;
	unsigned short m_bidderManagerPort;
	string         m_bcIP;
	unsigned short m_bcManagerPort;
	unsigned short m_bcDataPort;
	string         m_subKey;
	bool           m_registerToBidder;
};


class zmqSubKeyManager
{
public:
	zmqSubKeyManager();
	void add_subKey(const string& bidder_ip, unsigned short bidder_port, string& bc_ip,
		unsigned short bcManagerPort, unsigned short bcDataPOort, string& subKey);

	void erase_subKey_by_bcAddr(string& bcIP, unsigned short bcManagerPort);
	void erase_subKey_by_bidderAddr(string& bidderIP, unsigned short bidderManagerPort);
	void erase_subKey(string& bidderIP, unsigned short bidderManagerPort, string& bcIP, unsigned short bcManagerPort);
	void erase_subKey(string& subKey);
	vector<zmqSubscribeKey*>& get_subKey_list(){return m_subKey_list;}
	~zmqSubKeyManager();
private:
	vector<zmqSubscribeKey*> m_subKey_list;
};

class throttleObject
{
public:
	throttleObject();
	throttleObject(const string& throttleIP, unsigned short throttleManagerPort, unsigned short throttlePubPort);
	bool set_subscribe_opt(string &key);
	bool set_unsubscribe_opt(string &key);
	bool startSubThrottle(vector<zmqSubscribeKey*>& subKeyList, zeromqConnect& conntor, 
		struct event_base* base,  event_callback_fn fn, void *arg);
	void startConnectToThrottle(zeromqConnect& conntor,  string& bc_id, struct event_base* base,  event_callback_fn fn, void *arg);
	int            get_fd();
	string&        get_throttleIP();
	unsigned short get_throttlePubPort();
	void *         get_throttlePubHandler();

	unsigned short get_throttleManagerPort(){return m_throttleManagerPort;}
	void *         get_throttleManagerHandler(int fd);
	int            get_heart_times(){return m_heart_times;}

	void loginThrottle(const string &ip, unsigned short managerPort, unsigned short dataPort);

	void heart_times_increase(){m_heart_times++;}
	void heart_times_clear(){m_heart_times=0;}
	bool get_loginThrottle(){return m_loginThrottled;}
	void set_loginThrottle(bool logined){m_loginThrottled = logined;}
	~throttleObject();
private:
	string                   m_throttleIP;
	unsigned short           m_throttlePubPort;
	int                      m_throttlepubFd;//throttle zeromq publish fd
	void*                    m_throtllePubHandler = NULL;
	struct event*            m_throttlePubEvent = NULL;

	unsigned short           m_throttleManagerPort;
	int                      m_throttleManagerFd;
	void*                    m_throttleManagerHandler=NULL;
	struct event*            m_throttleManagerEvent=NULL;
	int                      m_heart_times;
	bool                     m_loginThrottled;
};


class throttleManager
{
public:
	throttleManager();
	void init(vector<throttleConfig*>& throttleList);

	void run(vector<zmqSubscribeKey*> subkeylist, zeromqConnect & conntor, struct event_base * base, event_callback_fn fn, void * arg);

	void add_bidder(string& bidderIP, unsigned short bidderPort, bcConfig& bc_conf);
	void* get_throttle_handler(int fd);
	void *get_throttleManagerHandler(int fd);

	void startConnectToThrottle(zeromqConnect& conntor,  string& bc_id, struct event_base* base,  event_callback_fn fn, void *arg);

	void add_throttle(const string& throttleIP, unsigned short throttleManagerPort, unsigned short throttlePubPort,
		vector<zmqSubscribeKey*>& subKeyList ,zeromqConnect & conntor, struct event_base * base,event_callback_fn fn,void * arg);

	void set_subscribe_opt(string &key);
	void loginThrottle(const string &ip, unsigned short managerPort, unsigned short dataPort);
	void loginThrottleSuccess(const string &ip, unsigned short managerPort);
	bool update_throttle(vector<throttleConfig*>& throConfList);

	bool recv_heart_from_throttle(const string& ip, unsigned short port);
	bool recv_throttle_heart_time_increase();
	void unSubscribe(vector<string> unsubList);
	~throttleManager();
private:
	const int               c_lost_times_max;	//default: 3
	vector<throttleObject*> m_throttle_list;
	mutex_lock              m_throttleList_lock;
};

#endif

