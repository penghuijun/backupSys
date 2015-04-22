#ifndef __BIDDERMANAGER_H__
#define __BIDDERMANAGER_H__

#include "bcManager.h"
#include "throttleManager.h"
#include "adConfig.h"

class bidderManager
{
public:
	bidderManager()
	{
		m_subKey_lock.init();
	}
	void init(zeromqConnect &connector, throttleInformation& throttle_info, bidderInformation& bidder_info, bcInformation& bc_info);
	void determine_bidder_address(zeromqConnect &connector, bidderInformation& bidder_info, vector<bcConfig*>& bcList);
	void login_event_new(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg);
	void subscribe_throttle_event_new(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg);
	bool update(bidderInformation &new_bidder_info);
	void registerBCToThrottle();
	void registerBCToThrottle(string &throttle_ip, unsigned short throttle_mangerPort);
	void dataConnectToBC(zeromqConnect & conntor);
	void connectAll(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg);
	void *get_throttle_handler(int fd);
	void *get_throttle_manager_handler(int fd);
	void add_throttle_identify(string& throttleIP, unsigned short throttlePort, string& identify);
	bool add_bc(zeromqConnect &connector, struct event_base * base, event_callback_fn fn,  void * arg,
				const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPort);
	bool add_throttle(zeromqConnect &connector, struct event_base * base, event_callback_fn fn,event_callback_fn fn1, void * arg,
				const string& ip, unsigned short managerPort, unsigned short dataPort);
	bool add_subKey(const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPOort);
	void update_bcList(zeromqConnect &connector, bcInformation& bcInfo);
	void loginOrHeartReqToBc(configureObject& config);
	bool dropDev();
	bool update_throttle(throttleInformation& throttle_info);

	void set_publishKey_registed(const string& ip, unsigned short managerPort,const string& publishKey);
	void loginedThrottle(const string& throttleIP, unsigned short throttleManagerPort);

	void *         get_bidderLoginHandler();
	int            sendAdRspToBC(string& bcIP, unsigned short bcDataPort, void* data, size_t dataSize, int flags);
	void *         get_bcManager_handler(int fd);
	string&        get_redis_ip();
	unsigned short get_redis_port() ;
	bidderConfig&  get_bidder_config() ;
	string& get_bidder_identify();

	void recv_BCHeartBeatRsp(const string& bcIP, unsigned short bcPort);

	void loginedBC(const string& bcIP, unsigned short bcPort);
	bool recv_throttleHeartReq(const string &ip, unsigned short port);
	void init_bidder(){m_bc_manager.bcmanager_lock_init();};
	~bidderManager();
private:
	
	bidderConfig    m_bidderConfig;
	string          m_bidderIdentify;
	void*           m_bidderLoginHandler = NULL;
	int 	  		m_bidderLoginFd;
	struct event*   m_bidderLoginEvent=NULL;

	string          m_redisIP;
	unsigned short  m_redisPort;

	read_write_lock  m_subKey_lock;
	zmqSubKeyManager m_subKey_manager;
	bcManager        m_bc_manager;
	throttleManager  m_throttle_manager;
};

#endif

