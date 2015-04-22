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

extern shared_ptr<spdlog::logger> g_manager_logger;



//key  reqestSym@bcIP:bcPort-bidderIP:bidderPort  eg:  1@1.1.1.1:1111-2.2.2.2:2222
class bidderCollecter
{
public:
	bidderCollecter(){}
	bidderCollecter(string &bidder_ip, unsigned short bidder_port, const string& bc_ip, unsigned short bc_managerPort, unsigned short bc_dataPort);
	void set(string &bidder_ip, unsigned short bidder_port,const string& bc_ip, unsigned short bc_managerPort, unsigned short bc_dataPort);

	bool startManagerConnectToBC(zeromqConnect& conntor, string& identify, struct event_base* base,  event_callback_fn fn, void *arg);
	bool startDataConnectToBC(zeromqConnect& conntor);
	string& get_subkey(){return m_subKey;}
	string& get_bcIP(){return m_bcIP;}
	unsigned short get_bcManagerPort(){return m_bcManagerPort;}
	
	unsigned short get_bcDataPort(){return m_bcDataPort;}
	int get_fd(){return m_bidderStateFd;}
	void *get_handler(){return m_bidderStateHander;}
	void *get_bcDataHandler(){return m_bidderDataHandler;}

	vector<string> get_throttleIdentifyList(){return m_throttleIdentifyList;}
	void erase_throttle_identify(string &id);
	void add_throttle_id(string& id);
	void set_login_status(bool status){m_bidderLoginToBC=status;}
	bool get_login_status(){return m_bidderLoginToBC;}

	bool loginOrHeartReqToBc(const string &ip, unsigned short port, vector<string> &unsubKeyList);
	int get_lost_times(){return m_lostHeartTimes;}
	void lost_time_clear(){m_lostHeartTimes = 0;}
	void *get_handler(int fd);
	~bidderCollecter()
	{
		if(m_bidderStateEvent)event_del(m_bidderStateEvent);
		if(m_bidderStateHander)zmq_close(m_bidderStateHander);
		if(m_bidderDataHandler) zmq_close(m_bidderDataHandler);
	}

private:
	string          m_bcIP;
	unsigned short  m_bcManagerPort;
	unsigned short  m_bcDataPort;
	vector<string>  m_throttleIdentifyList;	
	string          m_bidderIdentify;
	string          m_bcIdentify;
	string	        m_subKey;
	
	void	       *m_bidderStateHander = NULL;
	int 	        m_bidderStateFd;
	struct event   *m_bidderStateEvent=NULL;

	void           *m_bidderDataHandler = NULL;
	
	bool      		m_bc_recvStated = false;
	
	bool            m_bidderLoginToBC=false;
	bool      		m_registerToThrottle = false;
	int             m_lostHeartTimes = 0;
	const int       m_lostHeartTime_max = 5;
	int             m_lostLoginTimes = 0;
	const int       m_lostLoginTimes_max = 5;
};

class bcManager
{
public:
	bcManager()
	{
		m_bcList_lock.init();
	}
	void bcmanager_lock_init()
	{
		m_bcList_lock.init();		
	}
	void init(bidderConfig& bidder_conf, vector<bcConfig*>& bc_conf_list);
	void add_throttle_identify(string& throttleID);

	bool add_bc(zeromqConnect &connector, string& identify, struct event_base * base, event_callback_fn fn, void * arg, 
		string &bidder_ip, unsigned short bidder_port, const string& bc_ip, unsigned short bc_ManagerPort, unsigned short bc_dataPort);


	bool add_bc(zeromqConnect &connector, string &bidder_ip, unsigned short bidder_port,
		string& bc_ip, unsigned short bc_ManagerPort, unsigned short bc_dataPort);


	bool loginOrHeartReqToBc(const string &ip, unsigned short port, vector<string> &unsubKeyList);
	void update_bc(vector<bcConfig*>& bcConfList);

	bool startDataConnectToBC(zeromqConnect& conntor);

	bool BCManagerConnectToBC(zeromqConnect & conntor, string& identify,struct event_base * base,event_callback_fn fn,void * arg);
	void registerToThrottle(string &identify);
	int  sendAdRspToBC(string& bcIP, unsigned short bcDataPort, void* data, size_t dataSize, int flags);
	void *get_bcManager_handler(int fd);	

	bool recv_heartBeatRsp(const string& bcIP, unsigned short bcPort);

	void loginSucess(const string& bcIP, unsigned short bcPort);
	void relogin(const string& bcIP, unsigned short bcPort);

	void *get_handler(int fd);
	~bcManager()
	{
		for(auto it = m_bc_list.begin(); it != m_bc_list.end();)
		{
			delete *it;
			it = m_bc_list.erase(it);
		}
		m_bcList_lock.destroy();
	}

private:
	vector<bidderCollecter*> m_bc_list;	
	mutex_lock m_bcList_lock;
};


#endif
