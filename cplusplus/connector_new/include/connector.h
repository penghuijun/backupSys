#ifndef __CONNECTOR_H__
#define __CONNECTOR_H__
#include <string>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <event.h>
#include <map>
#include <queue>
#include <set>
#include <list>

#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>


#include <json/json.h>
#include <redis/redisPoolManager.h>
#include <thread/threadpool.h>
#include <thread/threadpoolmanager.h>
#include <mysql.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "urlcode.h"
#include "zmq.h"
#include "adConfig.h"
#include "lock.h"
#include "throttleManager.h"
#include "bcManager.h"
#include "connectorManager.h"
#include "zmqSubKeyManager.h"
#include "DSPmanager.h"
#include "GYinGetTarget.h"
#include "MobileAdRequest.pb.h"
#include "CommonMessage.pb.h"
#include "MobileAdResponse.pb.h"
#include "GMobileAdRequestResponse.pb.h"

#include "http_parse.h"

using namespace com::rj::protos::mobile::request;
using namespace com::rj::protos::mobile::response;
using namespace com::rj::adsys::dsp::connector::obj::g::proto;
using namespace com::rj::protos;
using namespace std;


#define PUBLISHKEYLEN_MAX 100
#define EPSINON 0.000001
extern vector<map<int,string>> SQL_MAP;
extern map<int,string> Creative_template;

class eventArgment
{
public:
	eventArgment(){}
	eventArgment(struct event_base* base, void *serv)
	{
		set(base, serv);
	}	
	void set(struct event_base* base, void *serv)
	{
		m_base = base;
		m_serv = serv;
	}
	struct event_base* get_base(){return m_base;}
	void *get_serv(){return m_serv;}
private:
	struct event_base* m_base;
	void *m_serv;
};

enum proStatus
{
	PRO_INIT,
    	PRO_BUSY,
	PRO_RESTART,
	PRO_KILLING
};

enum smRspType
{
	TXT,
	IMG,
	RICHMEDIA	
};

#if 0
enum dataType
{
	DATA_JSON,
	DATA_PROTOBUF
};
#endif

typedef struct BCProessInfo
{
    pid_t               pid;
	int 			  channel[2];
    proStatus         status;
} BC_process_t; 

struct messageBuf
{
public:
	messageBuf(){}
	messageBuf(char* data, int dataLen, void *serv)
	{
		message_new(data, dataLen, serv);
	}
	void message_set(char* data, int dataLen, void *serv)
	{
		m_stringBuf.string_set(data, dataLen);
		m_serv = serv;
	}

	void message_new(char* data, int dataLen, void *serv)
	{
		m_stringBuf.string_new(data, dataLen);
		m_serv = serv;	
	}
	stringBuffer& get_stringBuf(){return m_stringBuf;}
	void *        get_serv(){return m_serv;}
	~messageBuf(){}
private:
	stringBuffer m_stringBuf;
	void *m_serv;
};

struct commMsgRecord
{
	char *data;
	int datalen;
	struct timeval tv;
	long long ttl;	//ms
};
class connectorServ
{
public:
	connectorServ(configureObject& config);
	void readConfigFile();
	void run();	
	//ContentCategory GYin_AndroidgetTargetCat(string& cat);
	//ContentCategory GYin_IOSgetTargetCat(string& cat);
	//void mapInit();
	static void signal_handler(int signo);
	static void hupSigHandler(int fd, short event, void *arg);
	static void	intSigHandler(int fd, short event, void *arg);
	static void termSigHandler(int fd, short event, void *arg);
	static void usr1SigHandler(int fd, short event, void *arg);
	static void workerHandle_callback(int fd, short event, void *arg);	
	static void handle_recvLoginHeartReq(int fd,short event,void *arg);	
	static void handle_recvLoginHeartRsp(int fd,short event,void *arg);
	static void handle_recvAdRequest(int fd,short event,void *arg);
	static void handle_recvAdResponseTele(int fd,short event,void *arg);
	static void handle_recvAdResponseGYin(int sock,short event,void *arg);
	static void handle_recvAdResponseSmaato(int sock,short event,void *arg);
	static void *connectToOther(void *arg);
	static void *sendLoginHeartToOther(void *arg);
	static void *recvLoginHeartFromOther(void *arg);
	static void *recvAdRequestFromThrottle(void *arg);
	static void *getTime(void *arg);
	static void *checkTimeOutCommMsg(void *arg);
	static void *checkConnectNum(void *arg);
	
	int zmq_get_message(void* socket, zmq_msg_t &part, int flags);
	void updataWorkerList(pid_t pid);
	bool logLevelChange();
	void updateConfigure();
	void updateWorker();
	void *get_recvLoginHeartReqHandler(){return m_recvLoginHeartReqHandler;}
	void *get_sendLoginHeartToOtherHandler(int fd);
	
	void masterRun();
	void workerRun();
	bool manager_handler(void *handler, string& identify, const managerProtocol_messageTrans &from
					,const managerProtocol_messageType &type, const managerProtocol_messageValue &value
					,struct event_base * base=NULL, void * arg = NULL);
	bool manager_handler(const managerProtocol_messageTrans &from
				   ,const managerProtocol_messageType &type, const managerProtocol_messageValue &value
				   ,managerProtocol_messageType &rspType, struct event_base * base = NULL, void * arg = NULL);
	bool manager_from_BC_handler(const managerProtocol_messageType &type
		  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg);
	bool manager_from_throttle_handler(const managerProtocol_messageType &type
		  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg);
	void sendToWorker(char *subkey, int keylen, char *buf, int bufLen);	
	void httpPostRequest(const char* hostname, const char* api, const char* parameters);
	bool getStringFromSQLMAP(vector<string>& str_buf,const MobileAdRequest_Device& request_dev,const MobileAdRequest_GeoInfo& request_geo);
	void Tele_AdReqJsonAddApp(Json::Value &app, const MobileAdRequest& mobile_request);
	void Tele_AdReqJsonAddImp(Json::Value &impArray,const MobileAdRequest &mobile_request);
	bool Tele_AdReqJsonAddDevice(Json::Value &device,const MobileAdRequest& mobile_request);
	bool GYin_AdReqProtoMutableApp(App *app,const MobileAdRequest& mobile_request);
	bool GYin_AdReqProtoMutableDev(Device *device,const MobileAdRequest& mobile_request);
	void mobile_AdRequestHandler(const char *pubKey,const CommonMessage& request_commMsg);
	static void thread_handleAdRequest(void *arg);	
	void handle_recvAdResponse(int sock, short event, void *arg, dspType type);
	void handle_BidResponseFromDSP(dspType type,char *data,int dataLen, const CommonMessage& request_commMsg);		
	char* convertTeleBidResponseJsonToProtobuf(char *data,int dataLen,int& ret_dataLen,string& uuid);
	char* convertGYinBidResponseProtoToProtobuf(char *data,int dataLen,int& ret_dataLen,string& uuid);
	char* convertSmaatoBidResponseXMLtoProtobuf(char *data,int dataLen,int& ret_dataLen,string& uuid, const CommonMessage& request_commMsg);
	char* xmlParseAds(xmlNodePtr &adsNode, int& ret_dataLen, string& uuid, const CommonMessage& request_commMsg);
	bool convertProtoToTeleJson(string &reqTeleJsonData,const MobileAdRequest& mobile_request);	
	bool convertProtoToGYinProto(BidRequest& bidRequest,const MobileAdRequest& mobile_request);
	bool convertProtoToHttpGETArg(char *buf, const MobileAdRequest& mobile_request);
	bool mutableAction(MobileAdRequest &mobile_request,MobileAdResponse_Action *mobile_action,Json::Value &action);
	bool creativeAddEvents(MobileAdResponse_Creative  *mobile_creative,Json::Value &temp,string& nurl);
	bool GYIN_mutableAction(MobileAdRequest &mobile_request,MobileAdResponse_Action *mobile_action,Bid &GYIN_bid);
	bool GYIN_creativeAddEvents(MobileAdRequest &mobile_request,MobileAdResponse_Creative  *mobile_creative,Bid &GYIN_bid);
	bool SMAATO_mutableAction(MobileAdResponse_Action *mobile_action, xmlNodePtr &adNode, smRspType adType);
	bool SMAATO_creativeAddEvents(MobileAdResponse_Creative  *mobile_creative, xmlNodePtr &adNode, smRspType adType);
	commMsgRecord* checkValidId(const string& str_id);
	void displayCommonMsgResponse(shared_ptr<spdlog::logger> &logger,char *data,int dataLen);
	void displayGYinBidRequest(const char *data,int dataLen);
	void displayGYinBidResponse(const char *data,int dataLen);	
	void hashGetBCinfo(string& uuid,string& bcIP,unsigned short& bcDataPort);
	void genFundaCommands(vector<string>& sql_commands);
	void genAppumpCommands(vector<string>& sql_commands);
	void versionConvert(string &Dest,const char *Src);
	int getDataFromMysql_Funda();
	int getDataFromMysql_Appump();		
	
	~connectorServ(){}
private:
	configureObject& 			m_config;

	mutex_lock      			m_mastertime_lock;
	mutex_lock      			m_diplay_lock;

	spdlog::level::level_enum 	m_logLevel;

	bool           				m_logRedisOn=false;
	string         				m_logRedisIP;
	unsigned short 				m_logRedisPort;
	string 						m_vastBusinessCode;
	string 						m_mobileBusinessCode;
	unsigned short 				m_heartInterval;

	int 						m_workerNum;
	throttleManager				m_throttle_manager;
	bcManager					m_bc_manager;
	connectorManager			m_connector_manager;	
	zmqSubKeyManager			m_zmqSubKey_manager;	

	vector<BC_process_t*>  		m_workerList;

	zeromqConnect 				m_zmq_connect;

	void		   			   *m_masterPushHandler = NULL;
	void		   			   *m_masterPullHandler = NULL;
	int 						m_masterPullFd;

	void           			   *m_workerPullHandler = NULL;
	void           			   *m_workerPushHandler = NULL;
	int            				m_workerPullFd;			

	void 					   *m_recvLoginHeartReqHandler = NULL;
	int 						m_recvLoginHeartReqFd;

	void 					   *m_recvAdRequestHandler = NULL;
	int 						m_recvAdRequestFd;

	struct event_base* 			m_base;
	struct event_base* 			pth1_base;

	ThreadPoolManager 			m_thread_manager;
	
	struct event			   *m_conChinaTelecomDspEvent;
	
	//vector<commMsgRecord*>		commMsgRecordList;
	map<string, commMsgRecord*>		commMsgRecordList;
	mutex_lock					commMsgRecordList_lock;

	dspManager					m_dspManager;
	list<string>				requestUuidList;
	
};



#endif
