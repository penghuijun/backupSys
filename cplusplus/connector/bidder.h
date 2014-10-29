#ifndef __BIDDER_H__
#define __BIDDER_H__
#include <string>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <event.h>
#include <map>
#include <utility>
#include <set>
#include <list>
#include "bidderConfig.h"
#include "hiredis.h"
#include "zmq.h"
#include "threadpoolmanager.h"
#include "connectorPool.h"
#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"
#include "AdBidderResponseTemplate.pb.h"
#include "document.h"		// rapidjson's DOM-style API
#include "prettywriter.h"	// for stringify JSON
#include "filestream.h"	// wrapper of C stream for prettywriter as output
#include "stringbuffer.h"



using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace rapidjson;
using namespace std;
using std::string;
#define BUFSIZE 4096


const int	c_master_throttleIP=0x01;
const int	c_master_throttlePort=0x02;
const int	c_master_bcIP=0x04;
const int	c_master_bcPort=0x08;
const int	c_workerNum=0x10;
const int	c_master_subKey=0x20;




enum proStatus
{
	PRO_INIT,
    PRO_BUSY,
	PRO_RESTART,
	PRO_KILLING
};


enum requestState
{
	request_invalid,
	request_waiting,
	request_sended,
	request_over
};

struct uuid_subsribe_info
{
	string business_code;
	string data_coding_type;
	string publishKey;
	requestState state;
	string vastStr;
	exchangeConnector* connector;
	string jsonStr;
};
typedef struct BCProessInfo
{
    pid_t               pid;
	int 			  channel[2];
    proStatus         status;
} BC_process_t; 

typedef struct 
{
    pid_t              pid;
	int                chanel;
	bool 			   sended;
    char      *buf;
	int                bufSize;
	unsigned int       id;
	unsigned long long sendTime;
	int                sendCnts;
} sendBufUnit; 


struct redisDataRec
{
	string bidIDAndTime;
	int    time;
};

struct throttleBuf
{
	char *buf;
	unsigned int bufSize;
	void *serv;
	int channel;
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

class bidderServ;

struct eventParam
{
	struct event_base* base;
	bidderServ *serv;
};

//bidder server
class bidderServ
{
public:
	bidderServ(configureObject& config);
	void readConfigFile();//read configure file
	void worker_parse_master_data(char *data, unsigned int dataLen);
	uuid_subsribe_info *get_request_info(int fd)
	{
		//cout<<"++++sssize:"<<m_request_info_list.size()<<endl;
		requestList_rd_lock();
		auto it = m_request_info_list.begin();
		for(it = m_request_info_list.begin(); it != m_request_info_list.end(); it++)
		{
			uuid_subsribe_info *info = *it;
			if(info == NULL) continue;
			exchangeConnector *conn = info->connector;
			if(conn==NULL) 
			{
			//	cout<<"cccc can not fine conn:"<<info->state<<endl;
				continue;
			}
			if(info && (conn->get_sockfd()==fd) &&(info->state == request_sended))
			{
				requestList_rw_unlock();
				return info;
			}
		}
		requestList_rw_unlock();
		return NULL;
	}

	void earse_request_info(int fd)
	{
		requestList_wr_lock();
		auto it = m_request_info_list.begin();
		for(it = m_request_info_list.begin(); it != m_request_info_list.end();)
		{
			uuid_subsribe_info *info = *it;
			if(info)
			{
				exchangeConnector *conn = info->connector;
				if(conn && (conn->get_sockfd()==fd) &&(info->state == request_sended))
				{
					delete info;
					m_request_info_list.erase(it++);
					requestList_rw_unlock();
					return;
				}
				else
				{
					it++;
				}
			}
			else
			{
			    m_request_info_list.erase(it++);	
			}
		}
		requestList_rw_unlock();
	}
	bool needRemoveWorker() const 
	{
		return (m_workerList.size()>m_workerNum)?true:false;
	}
	void updataWorkerList(pid_t pid);
	void start_worker();
	void run();
	bool masterRun();
	void masterRestart(struct event_base* base);
	void master_net_init();
	void worker_net_init();
	void *get_zmqContext() const {return m_zmqContext;}
	void *get_request_handler(int fd) 
	{	
		void *handler;
		fd_rd_lock();
		auto it = m_handler_fd_map.begin();
		for(it = m_handler_fd_map.begin(); it != m_handler_fd_map.end(); it++)
		{
			handler_fd_map *hfMap = *it;
			if(hfMap && (hfMap->fd == fd))
			{
				handler = hfMap->handler;
				fd_rw_unlock();
				return handler;
			}
		}
		fd_rw_unlock();
		return NULL;
	}

	handler_fd_map *get_request_handler(string subKey) const
	{	
		void *handler;
		auto it = m_handler_fd_map.begin();
		for(it = m_handler_fd_map.begin(); it != m_handler_fd_map.end(); it++)
		{
			handler_fd_map *hfMap = *it;
			if(hfMap==NULL) continue;
			if(hfMap->subKey == subKey)
			{
				return hfMap;
			}
		}
		return NULL;
	}

//subKey incre , decre, change, lead to subscribe or unsubsribe
	void releaseConnectResource(string subKey) 
	{
		auto it = m_handler_fd_map.begin();
		for(it = m_handler_fd_map.begin(); it != m_handler_fd_map.end();)
		{
			handler_fd_map *hfMap = *it;
			if(hfMap==NULL)
			{
				it = m_handler_fd_map.erase(it);	
				continue;
			}
			
			if(hfMap->subKey == subKey)
			{
			     cout <<"del subkey:"<<subKey<<endl;
			     zmq_setsockopt(hfMap->handler, ZMQ_UNSUBSCRIBE, subKey.c_str(), subKey.size()); 
                 zmq_close(hfMap->handler);
                 event_del(hfMap->evEvent);
                 delete hfMap->evEvent;
                 delete hfMap;
				 it = m_handler_fd_map.erase(it);
				 
                 auto itor = m_subKeying.begin();
                 for(itor = m_subKeying.begin(); itor != m_subKeying.end();)
                 {
                    if(subKey ==  *itor)
                    {
                        itor = m_subKeying.erase(itor);
                    }
                    else
                    {
                        itor++;
                    }
                 }  
				
			}
			else
			{
				it++;
			}
		}
	}

	void fd_rd_lock(){ pthread_rwlock_rdlock(&handler_fd_lock);}
	void fd_wr_lock(){ pthread_rwlock_wrlock(&handler_fd_lock);}
	void fd_rw_unlock(){ pthread_rwlock_unlock(&handler_fd_lock);}

	void workerList_rd_lock(){ pthread_rwlock_rdlock(&m_workerList_lock);}
	void workerList_wr_lock(){ pthread_rwlock_wrlock(&m_workerList_lock);}
	void workerList_rw_unlock(){ pthread_rwlock_unlock(&m_workerList_lock);}

	void requestList_rd_lock(){ pthread_rwlock_rdlock(&m_requestList_lock);}
	void requestList_wr_lock(){ pthread_rwlock_wrlock(&m_requestList_lock);}
	void requestList_rw_unlock(){ pthread_rwlock_unlock(&m_requestList_lock);}

	

	
	void lock(){pthread_mutex_lock(&m_uuidListMutex);}
	void unlock(){pthread_mutex_unlock(&m_uuidListMutex);}
	void write_worker_channel(void *data, unsigned int dataLen, int chanelFd, string &subsribe);
	void sendToWorker(char *buf, int size, string& subsribe);
	void calSpeed()
	{
		 static long long test_begTime;
		 static long long test_endTime;
		 static int test_count = 0;
		 lock();
		 test_count++;
		 if(test_count%10000==1)
		 {
		 	struct timeval btime;
		 	gettimeofday(&btime, NULL);
			test_begTime = btime.tv_sec*1000+btime.tv_usec/1000;
		    unlock();
		 }
		 else if(test_count%10000==0)
		 {
		 	struct timeval etime;
		 	gettimeofday(&etime, NULL);
			test_endTime = etime.tv_sec*1000+etime.tv_usec/1000;
			unlock();
			cout <<"send request num: "<<test_count<<endl;
			cout <<"cost time: " << test_endTime-test_begTime<<endl;
			cout<<"sspeed: "<<10000*1000/(test_endTime-test_begTime)<<endl;
		 }
		 else
		 {
	 	 	unlock();
		 }
	}

	void reloadWorker()
	{
	    readConfigFile();
		if(((m_configureChange &c_master_bcIP) == c_master_bcIP)||((m_configureChange&c_master_bcPort)==c_master_bcPort))
		{
			zmq_close(m_response_handler);
			m_response_handler = bindOrConnect(true, "tcp", ZMQ_DEALER, m_bidderCollectorIP.c_str(), m_bidderCollectorPort, NULL);
		}

		auto listVec = m_config.get_exBidderList();
    	auto it = listVec.begin();
    	for(it = listVec.begin(); it != listVec.end(); it++)
    	{
        	exBidderInfo *info = *it;
        	if(info)
        	{
            	m_connectorPool.update_connector_pool(info->exBidderIP,info->exBidderPort, info->subKey, m_base, recvFromExBidderHandlerint , (void *)this, info->connectNum);
        	}
    	}
    	display_connectPool();
	}

	static void usr1SigHandler(int fd, short event, void *arg);
	static void *throttle_request_handler(void *arg);
	static void recvRequest_callback(int _, short __, void *pair);
	static void workerBusiness_callback(int fd, short event, void *pair);
	static void func(int fd, short __, void *pair);
	void *bindOrConnect(bool client,const char * transType,int zmqType,const char * addr,unsigned short port,int * fd);
	handler_fd_map* zmq_connect_subscribe(string subkey);
	static void hupSigHandler(int fd, short event, void *arg);
	static void	intSigHandler(int fd, short event, void *arg);
	static void termSigHandler(int fd, short event, void *arg);
	static void signal_handler(int signo);
	
	bool RTB_mobile_http_request_json(string &request, string &business_data);
	bool RTB_video_http_request_json(string &request, string &business_data);

	exchangeConnector*  send_request_to_bidder(string businessCode, string &publishKey, string &vastStr,socket_state &sockState);
	static void recvFromExBidderHandlerint(int fd, short event, void *pair);

	exchangeConnector *get_exchangeConnect(int fd)
	{
		return m_connectorPool.get_CP_exchange_connector(fd);
	}

	void sendTo_externBid_next();

	void update_uuid_state(string &uuid, requestState state)
	{
		auto it = m_uuidMap.begin();
		for(it = m_uuidMap.begin(); it != m_uuidMap.end(); it++)
		{
			string str = it->first;
			if(str == uuid)
			{
				uuid_subsribe_info *uinfo = it->second;
				uinfo->state =  state;
			}
		}
	}

	bool find_externBid_from_pubKey(string &ip, unsigned short &port, string &pubKey)
	{
		
	}

	void display_connectPool()
	{
		m_connectorPool.display();
	}

	bool transJsonToProtoBuf( rapidjson::Document &document, string &tbusinessCode, string &data_coding_type, string &vastStr, CommonMessage &commMsg);
	static void *check_connector(void *arg);
	static void connector_maintain(int fd, short __, void *pair);

	bool connectorPool_update(){return m_connectorPool.traval_connector_pool();}

	
	void connector_libevent_add(struct event_base* base , libevent_fun fun, void *serv)
	{
		 m_connectorPool.connector_pool_libevent_add(base, fun, serv);
	}
	
	void dis_connector_fd(int fd)
	{
		m_connectorPool.erase_connector(fd);
	}
private:
	#define SUBSCRIBE_STR_LEN 100
	void * establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd);
	configureObject& m_config;

	string m_throttleIP;
	unsigned short m_throttlePort;
	
	string m_bidderCollectorIP;
	unsigned short m_bidderCollectorPort;

	int m_workerNum;
	int m_workerChannel;
	
	vector<handler_fd_map*> m_handler_fd_map;
	vector<BC_process_t*>  m_workerList;//children progress list
	vector<string> m_subKeying;//message filtering, if establis or move success, remove or add to m_subKeying
	vector<string> m_subKey;//subKey configure , need establish message filter or remove message filter
	
	unsigned int m_configureChange=0;
	
	void *m_zmqContext = nullptr;	
	void *m_response_handler = nullptr;
	
	pthread_mutex_t m_uuidListMutex;
	pthread_rwlock_t handler_fd_lock;
	pthread_rwlock_t m_workerList_lock;
	pthread_rwlock_t m_requestList_lock;

	int m_eventFd[2];
	int m_connector_eventFd[2];
	pid_t m_master_pid;

	connectorPool m_connectorPool;
	ThreadPoolManager m_maneger;

	map<string, uuid_subsribe_info*> m_uuidMap;

	list<uuid_subsribe_info*> m_request_info_list;

	struct event_base* m_base;
};



#endif


