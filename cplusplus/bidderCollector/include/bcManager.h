#ifndef __BCMANAGER_H__
#define __BCMANAGER_H__

#include "throttleManager.h"
#include "bidderManager.h"
#include "connectorManager.h"
#include "managerProto.pb.h"
using namespace com::rj::protos::manager;

class bcManager
{
public:
	bcManager();
	void init(zeromqConnect &connector, throttleInformation& throttle_info, bidderInformation& bidder_info, connectorInformation& connector_info, bcInformation& bc_info);
	void determine_bc_address(zeromqConnect &connector, bcInformation& bc_info);
	void AdRsp_event_new(struct event_base * base, event_callback_fn fn, void * arg);
	void manager_event_new(struct event_base * base, event_callback_fn fn, void * arg);
	void connectAll(	zeromqConnect& conntor, struct event_base* base,  event_callback_fn fn, void *arg);
	void login();
	void loginThrottleSuccess(const string &ip, unsigned short managerPort);

	void register_sucess(bool bidder, const string &ip, unsigned short managerPort);

	void startSubThrottle(zeromqConnect & conntor,struct event_base * base,event_callback_fn fn,void * arg);
	void*          get_bidderManagerHandler(int fd);
	void*          get_throttle_handler(int fd);
	void*          get_bcManagerHandler();
	void*          get_adRspHandler();
	string&        get_redis_ip();
	unsigned short get_redis_port();
	unsigned short get_redis_timeout();
	bcConfig&      get_bc_config();
	string&        get_bcIdentify();
	string&        get_bcIPPortID();
	bool devDrop();
	bool recv_heartBeat(managerProtocol_messageTrans trans,const string& ip, unsigned short managerPort);
	void add_subKey(const string& bidderIP, unsigned short bidderPort, string &subKey);

	bool add_bidder(const string& bidderIP, unsigned short bidderPort
		    ,zeromqConnect & conntor, struct event_base * base,event_callback_fn fn,void * arg);
	void add_throttle(const string& throtteleIP, unsigned short throttleManagerPort, unsigned short throttlePubPort
    ,zeromqConnect & conntor, struct event_base * base,event_callback_fn fn,void * arg);
	void add_connector(const string& ip, unsigned short port
		,zeromqConnect & zmqConn, struct event_base * base,event_callback_fn fn,void * arg);
	void update_dev(throttleInformation& thro_info, bidderInformation& bidder_info);
	~bcManager();
private:
	bcConfig        m_bcConfig;
	string          m_bcIdentify;
	string          m_bcIPPortID;
	
	void*           m_bcManagerHandler = NULL;
	int 	  		m_bcManagerFd;
	struct event*   m_bcManagerEvent = NULL;

	void*           m_bcAdRspHandler = NULL;
	int             m_bcAdRspFd;
	struct event*   m_bcAdRspEvent = NULL;

	string          m_redisIP;
	unsigned short  m_redisPort;
	unsigned short  m_redisTimeout;

	zmqSubKeyManager m_subKey_manager;
	throttleManager  m_throttle_manager;
	connectorManager m_connector_manager;
	bidderManager    m_bidder_manager;
};

#endif

