#ifndef __THROTTLEMANAGER_H__
#define __THROTTLEMANAGER_H__

#include "bcManager.h"
#include "bidderManager.h"
#include "connectorManager.h"
#include "lock.h"
#include "pubKeyManager.h"

class throttleManager
{
public:
	throttleManager();
	void init(zeromqConnect &connector, throttleInformation& thro_info, connectorInformation& conn_info
		,  bidderInformation& bidder_info, bcInformation& bc_info);
	bool update( throttleInformation &throttle_info);
	void updateDev(bidderInformation& bidder_info,  bcInformation& bc_info, connectorInformation& conn_info);
	void address_init(zeromqConnect &connector, throttleInformation& thro_info);
	bool loginOrHeartReq();

	void recvHeartBeatRsp(bidderSymDevType sysType, const string& ip, unsigned short port);
	void loginSuccess(bidderSymDevType sysType, const string& ip, unsigned short port);
	void connectAllDev(zeromqConnect& connector, struct event_base* base,  event_callback_fn fn, void *arg);
	void reloginDevice(bidderSymDevType sysType, const string& ip, unsigned short port);

	void *get_throttle_request_handler();
	void *get_throttle_publish_handler();
	void *get_throttle_manager_handler();
	void *get_login_handler(int fd);
	bool  get_throttle_publishKey(const char* uuid, string& bidderKey, string& connectorKey);
	void reloginAllDev();
	throttleConfig& get_throttleConfig();
	string&         get_throttle_identify();
	
	void throttleRequest_event_new(struct event_base* base,  event_callback_fn fn, void *arg);
	void throttleManager_event_new(struct event_base* base,  event_callback_fn fn, void *arg);

	void add_throttle_publish_key(bool fromBidder, const string& bidderIP, unsigned short bidderPort,const string& bcIP,
		unsigned short bcManagerPort, unsigned short bcDataPort);

	void add_dev(bidderSymDevType sysType,const string& ip, unsigned short port, zeromqConnect& connector
    , struct event_base* base,  event_callback_fn fn, void *arg);	
	
	bool registerKey_handler(bool fromBidder, const string &key, zeromqConnect& connector
		, struct event_base* base,	event_callback_fn fn, void *arg);
	void delete_bc(string bcIP, unsigned short bcPort);
	
	~throttleManager()
	{
		event_del(m_throttleAdEvent);
		zmq_close(m_throttleAdHandler);
		event_del(m_throttleManagerEvent);
		zmq_close(m_throttleManagerHandler);
		zmq_close(m_throttlePubHandler);
		g_manager_logger->info("throttleManager erase");
	}

private:
	throttleConfig  m_throttle_config;
	string          m_throttle_identify;

	void           *m_throttleAdHandler = NULL;
	struct event   *m_throttleAdEvent = NULL;
	int             m_throttleAdFd;

	void           *m_throttleManagerHandler = NULL;
	struct event   *m_throttleManagerEvent = NULL;
	int             m_throttleManagerFd;

	void           *m_throttlePubHandler = NULL;
	throttlePubKeyManager m_throttlePublish;
//	throttlePublish m_connectorPublish;

	bidderManager    m_bidder_manager;
	bcManager        m_bc_manager;
	connectorManager m_connector_manager;
};

#endif

