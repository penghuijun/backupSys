#ifndef __DEVICEMANAGER_H__
#define __DEVICEMANAGER_H__

#include "bcManager.h"
#include "throttleManager.h"
#include "exBidManager.h"

class deviceManager
{
public:
	deviceManager()
	{
		m_subKey_lock.init();
	}
	void init(zeromqConnect &connector,const throttleInformation& throttle_info
							,const connectorInformation& connector_info,const bcInformation& bc_info);
	const connectorConfig& get_connector_config();

	void determine_local_address(zeromqConnect &connector,const connectorInformation& connector_info, const vector<bcConfig*>& bcList);
	void login_event_new(zeromqConnect &connector, struct event_base * base, event_callback_fn fn, void * arg);
	void subscribe_throttle_event_new(zeromqConnect &connector, struct event_base * base, 
												event_callback_fn fn, void * arg);
	void throttle_init(const string &throttle_ip, unsigned short throttle_mangerPort);

	void registerBCToThrottle();
	void registerBCToThrottle(string &throttle_ip, unsigned short throttle_mangerPort);
	void dataConnectToBC(zeromqConnect & conntor);
	void connectAll(zeromqConnect & conntor,struct event_base * base,event_callback_fn fn,void * arg);
	void *get_throttle_handler(int fd);
	void *get_throttle_manager_handler(int fd);
	void add_throttle_identify(string& throttleIP, unsigned short throttlePort, string& identify);
	bool add_bc(zeromqConnect &connector, struct event_base * base, event_callback_fn fn,  void * arg,
				 const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPort);
	bool add_throttle(zeromqConnect &connector, struct event_base * base, event_callback_fn fn,event_callback_fn fn1, void * arg,
				const string& ip, unsigned short managerPort, unsigned short dataPort);
	bool add_subKey(const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPOort);
	void update_bcList(zeromqConnect &connector,const bcInformation& bcInfo);
	void loginOrHeartReqToBc(configureObject& config);
	bool devDrop();
	bool update_throttle(const throttleInformation& throttle_info);

	void set_publishKey_registed(const string& ip, unsigned short port, const string& publishKey);
	void loginToThroSucess(const string& throttleIP, unsigned short throttleManagerPort);

	void *         get_bidderLoginHandler();
	int            sendAdRspToBC(string& bcIP, unsigned short bcDataPort, void* data, size_t dataSize, int flags);
	void *         get_bcManager_handler(int fd);
	string&        get_redis_ip();
	unsigned short get_redis_port();
	string& get_bidder_identify();

	void lostheartTimesWithBC_clear(const string& bcIP, unsigned short bcPort);

	void bidderLoginedBcSucess(const string& bcIP, unsigned short bcPort);
	bool recv_heart_from_throttle(const string &ip, unsigned short port);
	bool parse_protocal(const char* ori_str, int recvLen, int &protoType, string& protoValue);
	bool parse_bidderIdentify(string &origin, string& ip, unsigned short &port);
	bool parse_bcOrThrottelIdentify(string& origin, string& ip, unsigned short &managerPort, unsigned short &dataPort);
	void init_bidder(){m_bc_manager.init_bidder();};
	~deviceManager();
private:
	
	connectorConfig m_connectorConfig;	
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
	exBidManager     m_exBidder_manager;
};

#endif

