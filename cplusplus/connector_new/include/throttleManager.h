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
#include "zmqSubKeyManager.h"
using namespace com::rj::protos::manager;

typedef struct m_recvAdReq_t
{
	void			*m_recvAdReqFromThrottleHandler;
	int 		 	 m_recvAdReqFromThrottleFd;
	struct event	*m_recvAdReqFromThrottleEvent;
}recvAdReq_t;

class throSubKeyObject
{
public:
	throSubKeyObject(){}
	throSubKeyObject(string& key)
	{
		m_throSubKey = key;
		m_throRegisted = false;
		m_throSubscribed = false;
	}
	string& get_throSubKey(){return m_throSubKey;}
	bool	get_throRegisted(){return m_throRegisted;}
	bool 	get_throSubscribed(){return m_throSubscribed;}
	void    set_throRegisted(bool value){m_throRegisted = value;}
	void    set_throSubscribed(bool value){m_throSubscribed = value;}
	
	~throSubKeyObject(){}
private:
	string 						m_throSubKey;
	bool						m_throRegisted;
	bool    					m_throSubscribed;
};
class throttleObject
{
public:
	throttleObject(){m_throSubKeyList_lock.init();}
	throttleObject(const string& throttle_ip, unsigned short pub_port, unsigned short manager_port, unsigned short worker_num);
	string get_throttleIP(){return m_throttle_ip;}
	unsigned short get_throttlePubPort(){return m_throttle_pubPort;}
	unsigned short get_throttleManagerPort(){return m_throttle_managerPort;}	
	void connectToThrottleManagerPort(zeromqConnect &connector, string& Identify, struct event_base * base, event_callback_fn fn, void * arg);	
	void connectToThrottlePubPort(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg);	
	void sendLoginRegisterToThrottle(const string& ip,unsigned short manager_port);
	void *get_sendLoginRegisterToThrottleHandler(int fd);
	void *get_recvAdReqFromThrottleHandler(int fd);
	void set_connectorLoginToThro(bool value){m_connectorLoginToThro = value;}
	void lost_times_clear(){m_throLostHeartTimes = 0;}
	bool drop();
	void set_zmqUnsubscribeKey(string& key);
	bool add_throSubKey(string& key);
	void erase_throSubKey(string& key);
	bool set_throSubKeyRegisted(const string& throttleIP,unsigned short throManagerPort,const string& key);
	void set_throSubKeyListRegistedStatus(bool value);
	~throttleObject(){}
private:
	string              		m_throttle_ip;
	unsigned short           	m_throttle_pubPort;
	unsigned short           	m_throttle_managerPort;
	unsigned short		m_throttle_workerNum;

	void 					   *m_sendLHRToThrottleHandler = NULL; //L:Login H:Heart R:register
	int 						m_sendLHRToThrottleFd;
	struct event			   *m_sendLHRToThrottleEvent;

	vector<recvAdReq_t *>		m_recvAdReqVector;

	bool                     	m_connectorLoginToThro = false;

	int                      	m_throLostHeartTimes = 0;
	const int                	m_throLostHeartTimes_max = 5;

	vector<throSubKeyObject*>	m_throSubKey_list;
	mutex_lock					m_throSubKeyList_lock;
};
class throttleManager
{
public:
	throttleManager();
	void init(throttleInformation& throttleInfo);
	void connectToThrottleManagerPortList(zeromqConnect &connector, string& Identify, struct event_base * base, 
												event_callback_fn fn, void * arg);
	void connectToThrottlePubPortList(zeromqConnect &connector, struct event_base * base, 
												event_callback_fn fn, void * arg);
	void sendLoginRegisterToThrottleList(const string& ip,unsigned short manager_port);
	void *get_recvLoginHeartRspHandler(int fd);
	void *get_sendLoginHeartToThrottleHandler(int fd);
	void *get_recvAdReqFromThrottleHandler(int fd);
	bool add_throttle(const string& ip,unsigned short dataPort,unsigned short managerPort,
                         zeromqConnect & connector,string & Identify,struct event_base * base,event_callback_fn fn,void * arg);
	void logined(const string& throttleIP, unsigned short throttleManagerPort);
	bool recvHeartReq(const string &throttleIP, unsigned short throttleManagerPort);
	bool dropDev();
	bool update_throttleList(vector<throttleConfig*>& throConfList);
	void add_throSubKey(string& key);
	void erase_throSubKey(string& key);
	void set_throSubKeyRegisted(const string& throttleIP,unsigned short throManagerPort,const string& key);
	~throttleManager(){}
private:
	vector<throttleObject*> 	m_throttle_list;
	mutex_lock					m_throttleList_lock;
};

#endif
