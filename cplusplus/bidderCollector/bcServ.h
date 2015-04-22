/*
  *Copyright (c) 2014, fxxc
  *All right reserved.
  *
  *filename: bcserv.h
  *document mark :
  *summary:
  *	
  *current version:1.1
  *author:xyj
  *date:2014-11-21
  *
  *history version:1.0
  *author:xyj
  *date:2014-10-11
  */

#ifndef __BC_SERV_H__
#define __BC_SERV_H__
#include <string>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <event.h>
#include <string.h>
#include <map>
#include <set>
#include <list>

#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

#include "adConfig.h"
#include "hiredis.h"
#include "zmq.h"
#include "threadpoolmanager.h"
#include "redisPool.h"
#include "redisPoolManager.h"
#include "zeromqConnect.h"
#include "login.h"
#include "lock.h"
#include "bcManager.h"
#include "MobileAdRequest.pb.h"
#include "MobileAdResponse.pb.h"
#include "CommonMessage.pb.h"
#include "bufferManager.h"
#include "managerProto.pb.h"

using namespace com::rj::protos::manager;
using namespace std;
using namespace com::rj::protos::mobile;
using namespace com::rj::protos;

enum proStatus
{
	PRO_INIT,
    PRO_BUSY,
	PRO_RESTART,
	PRO_KILLING
};


typedef struct BCProessInfo
{
    pid_t               pid;
	int 			  channel[2];
    proStatus         status;
} BC_process_t; 

class adInsight
{
public:
	adInsight(){}
	void set_data(string& property, vector<string>& idList)
	{
		m_property = property;
		for(auto it = idList.begin(); it != idList.end(); it++)
		{
			string& str = *it;
			m_idsList.push_back(str);
		}
	}

	string& get_property(){return m_property;}
	vector<string>& get_idsList(){return m_idsList;}
	~adInsight()
	{
		for(auto it = m_idsList.begin(); it != m_idsList.end();)
		{
			it = m_idsList.erase(it);
		}
	}
private:
	string m_property;
	vector<string> m_idsList;
};

class adInsightManager
{
public:
	adInsightManager(){}
	void add_insight(string& property, vector<string>& idList)
	{
		for(auto it = m_adInsightList.begin(); it != m_adInsightList.end(); it++)
		{
			adInsight *insight = *it;
			if(insight == NULL) continue;
			if(insight->get_property() == property) return;
		}
		adInsight *insight = new adInsight;
		insight->set_data(property, idList);
		m_adInsightList.push_back(insight);
	}

	void eraseDislikeChannel(MobileAdResponse &mobile_response, vector<string>& dislikeIDList)
	{
	}	

	void eraseDislikeCreative(MobileAdResponse &mobile_response, vector<string>& dislikeIDList)
	{
		for(auto campIt = dislikeIDList.begin(); campIt != dislikeIDList.end(); campIt++)
		{
			string& disLikeID = *campIt;
			
			if(disLikeID.empty()) continue;
		    int bidContentSize = mobile_response.bidcontent_size();
			auto bidderContentList = mobile_response.bidcontent();
			for(auto bidIndex = 0; bidIndex < bidContentSize; bidIndex++)
			{
				 MobileAdResponse_mobileBid mobile_bidder = mobile_response.bidcontent(bidIndex);
				 int creativeSize = mobile_bidder.creative_size();
				 auto creativeList = mobile_bidder.creative();
				 for(auto creativeIndex = 0; creativeIndex < creativeSize; creativeIndex++)
				 {
				 	MobileAdResponse_Creative creative = mobile_bidder.creative(creativeIndex);
					string creativeID = creative.creativeid();
					if(disLikeID == creativeID)
					{
						creativeList.SwapElements(creativeIndex, creativeSize-1);
						creativeList.RemoveLast();
						break;
					}
				 }
			 }						   
		 }	
	}

	void eraseDislikeCampaign(MobileAdResponse &mobile_response, vector<string>& dislikeIDList)
	{
		for(auto it = dislikeIDList.begin(); it != dislikeIDList.end(); it++)
		{
			string& disLikeID = *it;
			
			if(disLikeID.empty()) continue;
		    int bidContentSize = mobile_response.bidcontent_size();
			auto bidderContentList = mobile_response.mutable_bidcontent();
			for(auto bidIndex = 0; bidIndex < bidContentSize; )
			{
				 MobileAdResponse_mobileBid mobile_bidder = mobile_response.bidcontent(bidIndex);
				 string cam_id = mobile_bidder.campaignid();
				// cout<<"****:"<<cam_id<<endl;
				 if(disLikeID == cam_id)
				 {
				 //	 cout<<"dislike campid:"<<disLikeID<<endl;
					 if(bidIndex != bidContentSize-1)
					 {
			     	 	bidderContentList->SwapElements(bidIndex, bidContentSize-1);
					 }
				     bidderContentList->RemoveLast();
					 bidContentSize = mobile_response.bidcontent_size();
					// cout<<"====:"<<mobile_response.ByteSize()<<endl;
					// break;
				 }
				 else
				 {
				 	bidIndex++;	
				 }
			 }						   
		 }	
	}

	void eraseDislikeAd(MobileAdResponse &mobile_response)
	{
		 for(auto it = m_adInsightList.begin(); it != m_adInsightList.end(); it++)
		 {
			 adInsight *insight = *it;
			 if(insight == NULL) continue;
			 string property = insight->get_property();
			 vector<string> dislikeIDList = insight->get_idsList();
			 if(property == "dln")
			 {
			// 	cout<<"dln"<<endl;
				 eraseDislikeCampaign(mobile_response, dislikeIDList);		
			 }
			 else if(property == "dlr")
		     {
		    // 	cout<<"dlr"<<endl;
		     	eraseDislikeCreative(mobile_response, dislikeIDList);
			 }
			 else if(property == "dll")
			 {
		   //  	eraseDislikeChannel(mobile_response, dislikeIDList);			 	
			 }
		 }
	}
	

	vector<adInsight*>& get_adInsightList(){return m_adInsightList;}
	~adInsightManager()
	{
		for(auto it = m_adInsightList.begin(); it != m_adInsightList.end();)
		{
			adInsight *insight = *it;
			delete insight;
			it = m_adInsightList.erase(it);
		}
	}
private:
	vector<adInsight*> m_adInsightList;
};


class redisDataRec
{
public:
	redisDataRec(time_t time)
	{
		m_time = time;
	}
	void add_field(string &field)
	{
		m_field.push_back(field);
	}
	vector<string> &get_field_list(){return m_field;}
	time_t get_time(){return m_time;}
	~redisDataRec(){}
private:
	vector<string> m_field;
	time_t    m_time;
};

class threadPoolFuncArg
{
public:
	threadPoolFuncArg(char *key, char* field, char *data, int dataLen)
	{
		set(key, field, data, dataLen);
	}
	void set(char *key, char* field, char *data, int dataLen)
	{
		m_key = key;
		m_field = field;
		m_data = data;
		m_dataLen = dataLen;
	}

	string &get_key(){return m_key;}
	string &get_field(){return m_field;}
	char*   get_data(){return m_data;}
	int     get_dataLen(){return m_dataLen;}
	void free_data()
	{
		delete[] m_data;
	}
	~threadPoolFuncArg()
	{
		free_data();
	}
private:
	string m_key;
	string m_field;
	char*  m_data;
	int    m_dataLen;
};

class adResponseValue
{
public:
	adResponseValue(){}
	adResponseValue(adInsightManager *manager, string& uuid, string& field, void *serv, stringBuffer *string_buf)
	{
		set_uuid(uuid);
		set_field(field);
		set_serv(serv);
		set_data(string_buf);
		if(manager)
		{
			auto insigtList = manager->get_adInsightList();
			for(auto it  = insigtList.begin(); it != insigtList.end(); it++)
			{
				adInsight* insight = *it;
				if(insight == NULL) continue;
				m_adInsightManager.add_insight(insight->get_property(), insight->get_idsList());
			}
		}
	}
	void set_uuid(string& uuid)
	{
		m_uuid = uuid;
	}
	void set_field(string& field)
	{
		m_field = field;
	}
	void set_serv(void *serv)
	{
		m_serv = serv;
	}
	void set_data(stringBuffer *string_buf)
	{
		m_stringBuf = string_buf;		
	}	
	char* get_data()
	{
		return m_stringBuf->get_data();
	}
	unsigned int get_dataSize()
	{
		return m_stringBuf->get_dataLen();
	}
	string& get_uuid()
	{
		return m_uuid;
	}
	string& get_field()
	{
		return m_field;
	}
	void *get_serv()
	{
		return m_serv;
	}

	adInsightManager &get_insightManager(){return m_adInsightManager;}
	~adResponseValue()
	{
		delete m_stringBuf;
	}
private:
	stringBuffer *m_stringBuf = NULL;
	void *m_serv=NULL;
	string m_uuid;
	string m_field;
	adInsightManager m_adInsightManager;
};

struct servReadWorkerEvent
{
	struct event *ev;
	int fd;
};

struct eventCallBackParam
{
	void *serv;
	int ev_handlerMsgCount;
	struct event_base *ev_base;
	struct event *ev_adEvent;
	struct event *ev_expEvent;
	struct event *ev_clientPullEvent;
	vector<servReadWorkerEvent *>ev_servPullEventList;
};

typedef unsigned char byte;

#define PUT_LONG(buffer, value) { \
    (buffer) [0] =  (((value) >> 24) & 0xFF); \
    (buffer) [1] =  (((value) >> 16) & 0xFF); \
    (buffer) [2] =  (((value) >> 8)  & 0xFF); \
    (buffer) [3] =  (((value))       & 0xFF); \
    }

#define GET_LONG(buffer) \
      ((byte)(buffer) [0] << 24) \
    + ((byte)(buffer) [1] << 16) \
    + ((byte)(buffer) [2] << 8)  \
    +  (byte)(buffer) [3]



#define PUBLISHKEYLEN_MAX 50

struct handler_fd_map
{
	string subKey;
	void *handler;
	int fd;
	struct event *evEvent;
};

class bcServ;

struct eventParam
{
	struct event_base* base;
	bcServ *serv;
};

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


class bufferStructure
{
public:
	bufferStructure(char *buf, int bufSize, string &hash_field)
	{
		gen_buffer(buf, bufSize, hash_field);
	}
	void gen_buffer(char *buf, int bufSize, string &hash_field)
	{
		if(bufSize > 0)
		{
			m_stringBuffer = new stringBuffer(buf, bufSize);
			m_hash_filed = hash_field;
		}
	}

	stringBuffer *stringBuf_pop()
	{
		stringBuffer* bufPtr = m_stringBuffer;
		m_stringBuffer = NULL;
		return bufPtr;
	}
	string &get_hash_field() {return m_hash_filed;}

	void release_buffer()
	{
		delete m_stringBuffer;
	}
	~bufferStructure()
	{
		release_buffer();
	}
	
private:
	stringBuffer *m_stringBuffer = NULL;
	string m_hash_filed;
};


typedef unsigned long long mircotime_t;

class bcDataRec
{
public:
	bcDataRec()
	{
	}
	bcDataRec(mircotime_t timep, bool recv_throttle)
	{
		m_mircoTime = timep;
		m_recv_throttle = recv_throttle;
	}
	
	bcDataRec(mircotime_t timep, uint32_t overTime, bool recv_throttle)
	{
		m_mircoTime = timep;
		m_recv_throttle = recv_throttle;
		m_overtime = overTime;
	}

	void add_bidder_buf(char *buf, int bufSize, string &hash_field)
	{
		if(!m_bufStru_vec) m_bufStru_vec = new vector<bufferStructure*>;
		if(m_bufStru_vec)
		{
			bufferStructure *buf_item = new bufferStructure(buf, bufSize, hash_field);
			if(m_bufStru_vec) m_bufStru_vec->push_back(buf_item);
		}
	}
	
	mircotime_t get_microtime(){return m_mircoTime;}
	int get_overtime(){return m_overtime;}
	vector<bufferStructure*>* get_buf_list()
	{
		vector<bufferStructure*>* bufPtr = m_bufStru_vec;
		m_bufStru_vec = NULL;
		return bufPtr;
	}

	void recv_throttle_complete(){m_recv_throttle = true;}
	bool recv_throttle_info(){return m_recv_throttle;}
	void add_insightManager(adInsightManager* manager){m_insightManager = manager;}
	adInsightManager* get_adInsightManager(){return m_insightManager;}
	~bcDataRec()
	{
		if(m_bufStru_vec)
		{
			for(auto it = m_bufStru_vec->begin(); it != m_bufStru_vec->end(); it++)
			{
				bufferStructure *buf =  *it;
				if(buf) delete buf;
			}
			delete m_bufStru_vec;
		}
		delete m_insightManager;
	}
private:
	mircotime_t     m_mircoTime=0;
	uint32_t        m_overtime=1000;//1000ms default;
	bool            m_recv_throttle=false;
	adInsightManager *m_insightManager = NULL;
	vector<bufferStructure*> *m_bufStru_vec=NULL;
};
#define UUID_SIZE_MAX 100

//bidder server
class bcServ
{
public:
	bcServ(configureObject& config);
	void run();
	mircotime_t get_microtime();
	void check_data_record();
	int zmq_get_message(void* socket, zmq_msg_t &part, int flags);
	bool logLevelChange();

	void update_microtime()
	{
		m_tmMutex.lock();
        gettimeofday(&m_microSecTime, NULL);
		m_tmMutex.unlock();
	}
	static void worker_thread_func(void *arg);
	bool masterRun();
		
	static void* bc_manager_handler(void *arg);
	
	static void recvRegisterRsp_callback(int fd, short __, void *pair);
	static void managerEventHandler(int fd, short __, void *pair);
	static void recvBidder_callback(int fd, short __, void *pair);
	static void recvRequest_callback(int _, short __, void *pair);
	
	static void usr1SigHandler(int fd, short event, void *arg);
	static void hupSigHandler(int fd, short event, void *arg);
	static void	intSigHandler(int fd, short event, void *arg);
	static void termSigHandler(int fd, short event, void *arg);	
	
	static void *brocastLogin(void *arg);

	void calSpeed();
	void subVastHandler(char * pbuf,int size);
	void bidderHandler(char * pbuf,int size);
	static void *throttle_request_handler(void *arg);

	void *register_update_config_event();
	static void *getTime(void *arg);
	void manager_handler(void *handler, string& identify, const managerProtocol_messageTrans &from
	, const managerProtocol_messageType &type , const managerProtocol_messageValue &value
	, struct event_base * base = NULL, void * arg = NULL);
	
	bool manager_handler(const managerProtocol_messageTrans &from, const managerProtocol_messageType &type
			 , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType,struct event_base * base, void * arg);

	bool managerProto_from_throttle_handler(const managerProtocol_messageType &type
	, const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base = NULL, void * arg = NULL);
	
	bool managerProto_from_connector_handler(const managerProtocol_messageType &type
	, const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base = NULL, void * arg = NULL);
	
	bool managerProto_from_bidder_handler(const managerProtocol_messageType &type
	, const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base = NULL, void * arg = NULL);

	void handler_recv_buffer(adInsightManager* insightManager, stringBuffer *string_buf, string &uuid, string &hash_field);
private:
	configureObject& m_config;
	
	map<string, bcDataRec*>    m_bcDataRecList;

	mutex_lock          m_bcDataMutex;
	mutex_lock          m_tmMutex;
	mutex_lock          m_getTimeofDayMutex;
	read_write_lock     handler_fd_lock;
	read_write_lock     m_workerList_lock;

	struct timeval      m_microSecTime;
	string              m_vastBusiCode;
	string              m_mobileBusiCode;
	zeromqConnect       m_zmq_connect;

	unsigned short      m_heart_interval;
	redisPoolManager    m_redisPoolManager;
	redisPoolManager    m_redisLogManager;
	ThreadPoolManager   m_threadPoolManger;
	bcManager           m_bc_manager;

	bool                m_logRedisOn=false;
	string              m_logRedisIP;
	unsigned short      m_logRedisPort;
	
	spdlog::level::level_enum m_logLevel;	
};



#endif


