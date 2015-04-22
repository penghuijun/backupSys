#ifndef __THROTTLEMANAGER_H__
#define __THROTTLEMANAGER_H__
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <event.h>
#include <unistd.h>
#include <algorithm>
#include "zmq.h"
#include "adConfig.h"
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include "login.h"
#include "lock.h"
#include "spdlog/spdlog.h"
#include "managerProto.pb.h"
using namespace com::rj::protos::manager;

extern shared_ptr<spdlog::logger> g_manager_logger;


class zmqSubscribeKey
{
public:
	zmqSubscribeKey(){}
	zmqSubscribeKey(const string& bidderIP, unsigned short bidderPort,const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort);
	void set(const string& bidderIP, unsigned short bidderPort,const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort);
	string&    	   get_bidder_ip(){return m_bidderIP;}
	unsigned short get_bidder_manager_port(){return m_bidderManagerPort;}
	string&        get_bc_ip(){return m_bcIP;}
	unsigned short get_bc_manager_port(){return m_bcManagerPort;}
	unsigned short get_bc_data_port(){return m_bcDataPort;}
	string&        get_subKey(){return m_subKey;}
	bool           get_regist_status(){return m_bcRegisted;}
    bool set_register_status(string& bidderIP, unsigned short bidderManagerPort, string& bcIP,
							unsigned short bcManangerPort, unsigned short bcDataPort, bool registed);
	bool set_register_stataus(bool status);
	void set_subscribed(){m_subscribed=true;}
	bool get_subscribed(){return m_subscribed;}
	~zmqSubscribeKey()
	{
		g_manager_logger->info("<erase subkey>:{0}", m_subKey);
	}
private:
	string         m_bidderIP;
	unsigned short m_bidderManagerPort;
	string         m_bcIP;
	unsigned short m_bcManagerPort;
	unsigned short m_bcDataPort;
	string         m_subKey;
	bool           m_bcRegisted = false;
	bool           m_subscribed = false;
};

class subThroKey
{
public:
	subThroKey(){}
	subThroKey(string& key)
	{
		set(key);
	}
	
	void set(string &key){m_subKey = key;}
	const string&        get_subKey(){return m_subKey;}
	bool           get_regist_status(){return m_bcRegisted;}
	bool           set_regist_status(bool status){m_bcRegisted = status;}

	bool           get_subscribed(){return m_subscribed;}    
	bool           set_registed(){m_bcRegisted = true;}
	void           set_subscribed(){m_subscribed=true;}

	~subThroKey(){}
private:
	string         m_subKey;
	bool           m_bcRegisted = false;
	bool           m_subscribed = false;
};



class zmqSubKeyManager
{
public:
	zmqSubKeyManager();
	string& add_subKey(const string& bidder_ip, unsigned short bidder_port,const string& bc_ip,
		unsigned short bcManagerPort, unsigned short bcDataPOort);

	void erase_subKey_by_bcAddr(string& bcIP, unsigned short bcManagerPort);
	void erase_subKey_by_bidderAddr(string& bidderIP, unsigned short bidderManagerPort);
	void erase_subKey(string& bidderIP, unsigned short bidderManagerPort, string& bcIP, unsigned short bcManagerPort);
	void erase_subKey(string& subKey);
	vector<zmqSubscribeKey*> get_subKey_list(){return m_subKey_list;}
	~zmqSubKeyManager();
private:
	vector<zmqSubscribeKey*> m_subKey_list;
};




class throttleObject
{
public:
	throttleObject(){}
	throttleObject(const string& throttle_ip, unsigned short throttle_port, unsigned short manager_port);
	bool add_subkey(string &key)
	{
		for(auto it = m_subThroKey_list.begin(); it != m_subThroKey_list.end(); it++)
		{
			subThroKey* throttleKey =  *it;
			if(throttleKey==NULL) continue;
			if(key == throttleKey->get_subKey())
			{
				return false;
			}
		}
		subThroKey* subKey = new subThroKey(key);
		m_subThroKey_list.push_back(subKey);
		return true;
	}
	void set_publishKey_status(bool registed);
	bool startSubThrottle(zeromqConnect& conntor, struct event_base* base,  event_callback_fn fn, void *arg);
	void connectToThrottle(zeromqConnect &connector, string& bidderID, struct event_base * base, event_callback_fn fn, void * arg);
	void add_throttle_identify(string& throttleIP, unsigned short throttlePort, string& identify);
	bool devDrop();

	string&        get_throttle_ip(){return m_throttle_ip;}
	unsigned short get_throttle_port(){return m_throttle_pubPort;}
	unsigned short get_throttle_managerPort(){return m_throttle_managerPort;}
	int            get_fd(){return m_throttle_pubFd;}
	void*          get_handler(){return m_throtlePubHandler;}
	void*          get_manager_handler(int fd);
	int            get_manager_fd(){return m_throttleManagerFd;}
	
	void registerBCToThrottle(const string &devIP, unsigned short devPort);


	void subThrottle(string& bidderIP, unsigned short bidderPort, string &bcIP,
											unsigned short bcManagerPort, unsigned short bcDataPort);


	bool set_publishKey_registed(const string& ip, unsigned short port,const string& publishKey);

	void *get_handler(int fd);
	
	bool set_subscribe_opt(string &key);
	bool set_unsubscribe_opt(string &key);
	void erase_subKey(string &key);

	void lost_times_increase(){m_lost_heart_times++;}
	int  get_lost_times(){return m_lost_heart_times;}
	void lost_times_clear(){m_lost_heart_times = 0;}

	bool get_bidderLoginToThro(){return m_bidderLoginToThro;}
	void set_bidderLoginToThro(bool status) {m_bidderLoginToThro = status;}
	void throttle_init()
	{
		m_lost_heart_times = 0;
		for(auto it = m_subThroKey_list.begin(); it != m_subThroKey_list.end(); it++)
		{
			subThroKey *subKey = *it;
			if(subKey) subKey->set_regist_status(false);
		}
	}
	~throttleObject()
	{	
		for(auto it = m_subThroKey_list.begin(); it != m_subThroKey_list.end();)
		{
			delete *it;
			it = m_subThroKey_list.erase(it);
		}

		if(m_throttleEvent) event_del(m_throttleEvent);
		if(m_throtlePubHandler) zmq_close(m_throtlePubHandler);
		if(m_throttleManagerEvent) event_del(m_throttleManagerEvent);
		if(m_throtleManagerHandler) zmq_close(m_throtleManagerHandler);

	}

private:
	string                   m_throttle_ip;
	unsigned short           m_throttle_pubPort;
	unsigned short           m_throttle_managerPort;

	int                      m_throttle_pubFd;//throttle zeromq publish fd
	void*                    m_throtlePubHandler = NULL;
	struct event*            m_throttleEvent = NULL;

	int                      m_throttleManagerFd;//throttle zeromq publish fd
	void*                    m_throtleManagerHandler = NULL;
	struct event*            m_throttleManagerEvent = NULL;	

	int                      m_lost_heart_times = 0;
	vector<subThroKey*>      m_subThroKey_list;
	bool                     m_bidderLoginToThro = false;
	const int                c_lost_times_max = 3;

};

class throttleManager
{
public:
	throttleManager()
	{
		m_throttleList_lock.init();
	}
	throttleManager(vector<throttleConfig*>& thro_conf);
	void init(const vector<throttleConfig*>& thro_conf);
	void run(zeromqConnect& conntor, struct event_base * base, event_callback_fn fn, void * arg);

	void connectToThrottle(zeromqConnect &connector, string& bidderID, struct event_base * base, 
												event_callback_fn fn, void * arg);

	void* get_throttle_handler(int fd);
	void* get_throttle_manager_handler(int fd);
	void add_throttle_identify(string& throttleIP, unsigned short throttlePort, string& identify);
	void registerBCToThrottle(const string &devIP, unsigned short devPort);
	void registerBCToThrottle(const string &devIP, unsigned short devPort,const string &throttle_ip, unsigned short throttle_mangerPort);
	bool update_throttle(const vector<throttleConfig*>& throConfList);

	void loginToThroSucess(const string& throttleIP, unsigned short throttleManagerPort)	;
	void set_publishKey_registed(const string& ip, unsigned short port,const string& publishKey);
	void add_subKey(string& key)
	{
		m_throttleList_lock.lock();
		for(auto it = m_throttle_list.begin(); it != m_throttle_list.end(); it++)
		{
			throttleObject *throttle =  *it;
			if(throttle)
			{
				throttle->add_subkey(key);
			}
		}
		m_throttleList_lock.unlock();
	}
	void throttle_init(string &throttle_ip, unsigned short throttle_mangerPort);	
	void *get_handler(int fd);
	void erase_bidder_publishKey(vector<string>& unsubKeyList);
	void add_throttle(string &throttleIP, unsigned short throttlePort, unsigned short managerPort);
	bool add_throttle(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, event_callback_fn fn1,void * arg, string& bidderIdentify,
	const	string& throttleIP, unsigned short managerPort, unsigned short dataPort);	

	bool devDrop();
	bool recv_heart_from_throttle(const string &ip, unsigned short port);
	~throttleManager();
private:

	vector<throttleObject*> m_throttle_list;
	mutex_lock              m_throttleList_lock;
};


#endif

