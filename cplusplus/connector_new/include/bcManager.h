#ifndef __BCMANAGER_H__
#define __BCMANAGER_H__
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

class hashFnv1a64
{
public:
	static uint32_t hash_fnv1a_64(const char *key, size_t key_length);
	static uint32_t get_rand_index(const char* uuid);

private:
	static const uint64_t FNV_64_INIT = UINT64_C(0xcbf29ce484222325);
	static const uint64_t FNV_64_PRIME = UINT64_C(0x100000001b3);
};


class bcObject
{
public:
	bcObject(){}
	bcObject(const string& bc_ip, unsigned short date_Port, unsigned short manager_port,unsigned short thread_PoolSize = 0);
	string get_bcIP(){return m_bc_ip;}
	unsigned short get_bcDataPort(){return m_bc_datePort;}
	unsigned short get_bcManagerPort(){return m_bc_managerPort;}	
	unsigned short get_bcThreadPoolSize(){return m_bc_threadPoolSize;} 		
	void connectToBC(zeromqConnect &connector, string& Identify, struct event_base * base, 
												event_callback_fn fn, void * arg);
	void connectToBCDataPort(zeromqConnect &connector);
	void sendLoginHeartToBC(const string& ip,unsigned short manager_port);
	void *get_sendLoginHeartToBCHandler(int fd);
	void *get_bcDataHandler(){return m_bcDataHandler;};
	void set_connectorLoginToBC(bool value){m_connectorLoginToBC = value;}
	void lost_time_clear(){m_lostBCHeartTimes = 0;}
	bool drop(const string& connectorIP,unsigned short conManagerPort,vector<string> &unsubKeyList);	
	~bcObject(){}
private:
	string              		m_bc_ip;
	unsigned short           	m_bc_datePort;
	unsigned short           	m_bc_managerPort;
	unsigned short				m_bc_threadPoolSize;

	void 					   *m_sendLoginHeartToBCHandler = NULL; //L:Login H:Heart 
	int 						m_sendLoginHeartToBCFd;
	struct event			   *m_sendLoginHeartToBCEvent;
	bool 						m_connectorLoginToBC = false;

	void 					   *m_bcDataHandler = NULL;

	int             			m_lostBCHeartTimes = 0;
	const int       			m_lostBCHeartTime_max = 3;
};
class bcManager
{
public:
	bcManager();
	void init(bcInformation& bcInfo);
	bool add_bc(zeromqConnect &connector, string& bc_ip, unsigned short bc_ManagerPort, unsigned short bc_dataPort,unsigned short bc_threadPoolSize);
	bool add_bc(const string& ip,unsigned short dataPort,unsigned short managerPort,
                         zeromqConnect & connector,string & Identify,struct event_base * base,event_callback_fn fn,void * arg);
	bool dropDev(const string& connectorIP,unsigned short conManagerPort,vector<string> &unsubKeyList);
	void update_bcList(zeromqConnect &connector, bcInformation& bcInfo);
	void update_bcList(vector<bcConfig*>& bcConfList);
	void connectToBCList(zeromqConnect &connector, string& Identify, struct event_base * base, 
												event_callback_fn fn, void * arg);
	void connectToBCListDataPort(zeromqConnect &connector);
	void sendLoginHeartToBCList(const string& ip,unsigned short manager_port);
	void *get_sendLoginHeartToBCHandler(int fd);	
	void logined(const string& bcIP, unsigned short bcManagerPort);
	bool recvHeartRsp(const string& bcIP, unsigned short bcPort);
	void hashGetBCinfo(string& uuid,string& bcIP,unsigned short& bcDataPort);
	int sendAdRspToBC(string& bcIP, unsigned short bcDataPort, void* data, size_t dataSize, int flags);
	~bcManager(){}
private:
	vector<bcObject*> 			m_bc_list;
	mutex_lock					m_bcList_lock;
};

#endif

