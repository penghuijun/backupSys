#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <time.h>
#include <set>
#include <map>
#include <queue>
#include <iterator>
#include <utility>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <event.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include "zmq.h"
#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"
#include "AdBidderResponseTemplate.pb.h"
#include "MobileAdRequest.pb.h"
#include "throttle.h"
#include "redis/hiredis.h"
#include "thread/threadpoolmanager.h"
#include "spdlog/spdlog.h"
#include "bufferManager.h"
#include "redis/redisPoolManager.h"
using namespace com::rj::protos::manager;

using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;
using namespace spdlog;

sig_atomic_t srv_graceful_end = 0;
sig_atomic_t srv_ungraceful_end = 0;
sig_atomic_t srv_restart = 0;
sig_atomic_t sigalrm_recved = 0;;
sig_atomic_t sigusr1_recved = 0;;


extern shared_ptr<spdlog::logger> g_file_logger;
extern shared_ptr<spdlog::logger> g_manager_logger;


//#ifdef DEBUG
//get micro time
unsigned long long throttleServ::getLonglongTime()
{
    m_timeMutex.lock();
    unsigned long long t = m_localTm.tv_sec*1000+m_localTm.tv_usec/1000;
    m_timeMutex.unlock(); 
    return t;
}
//update time on regular time
void *throttleServ::getTime(void *arg)
{
    throttleServ *serv = (throttleServ*) arg;
    while(1)
    {
        usleep(1*1000);
        serv->m_timeMutex.lock();
        gettimeofday(&serv->m_localTm, NULL);
        serv->m_timeMutex.unlock();     
    }
}


void *throttleServ::logWrite(void *arg)
{
    throttleServ *serv = (throttleServ*) arg;
    while(1)
    {
        usleep(100000);
        vector<stringBuffer*> logList;
        serv->m_logManager.popLog(logList);
        for(auto it = logList.begin(); it != logList.end();)
        {
            stringBuffer* strBuf = *it;
            if(strBuf)
            {
                serv->m_logRedisPoolManger.redis_lpush("flow_log", strBuf->get_data(), strBuf->get_dataLen()); 
            }
            delete strBuf;
            it = logList.erase(it);
        }    
    }
}

//#endif
//update dev with reload configure
void throttleServ::updateConfigure()
{
    try
    {
        g_manager_logger->info("update configure......");
        m_config.readConfig();
        if(logLevelChange())
        {
            g_file_logger->set_level(m_logLevel);
            g_manager_logger->set_level(m_logLevel); 
        }

        throttleInformation &throttle_info = m_config.get_throttle_info();
        if(m_throttleManager.update(throttle_info) == true)
        {
            int oldNum = m_workerNum;
            m_workerNum = m_throttleManager.get_throttleConfig().get_throttleWorkerNum(); 
            g_manager_logger->emerg("worker number from {0:d} to {1:d}", oldNum, m_workerNum);
        }
        //m_throttleManager.updateDev(m_config.get_bidder_info(), m_config.get_bc_info(), m_config.get_connector_info());
    }
    catch(...)
    {
        g_manager_logger->error("updateConfigure exception");
    }

}

//throttleServ structure
throttleServ::throttleServ(configureObject &config):m_config(config)
{
try
{
    int logLevel = m_config.get_throttleLogLevel();

    if((logLevel>((int)spdlog::level::off) ) || (logLevel < ((int)spdlog::level::trace))) 
    {
        m_logLevel = spdlog::level::info;
    }
    else
    {
        m_logLevel = (spdlog::level::level_enum) logLevel;
    }
    
    g_file_logger->set_level(m_logLevel);
    g_manager_logger->set_level(m_logLevel);
   // spdlog::set_pattern("[%H:%M:%S %z] %v *");
    g_manager_logger->emerg("log level:{0:d}", (int)m_logLevel);

    m_displayMutex.init();
    m_zmq_connect.init();

    m_vastBusiCode         = m_config.get_vast_businessCode();
    m_mobileBusiCode       = m_config.get_mobile_businessCode();
    m_heart_interval       = m_config.get_heart_interval();

    m_logRedisOn           = m_config.get_logRedisState();
    m_logRedisIP           = m_config.get_logRedisIP();
    m_logRedisPort         = m_config.get_logRedisPort();

		
//dev manager init
    m_throttleManager.init(m_zmq_connect, m_config.get_throttle_info(), m_config.get_connector_info(), m_config.get_bidder_info()
        , m_config.get_bc_info());
    m_workerNum = m_throttleManager.get_throttleConfig().get_throttleWorkerNum();

//establish connect between master with worker    
    m_masterPushHandler = m_zmq_connect.establishConnect(false, "ipc", ZMQ_PUSH, "masterworker" , NULL);
    m_masterPullHandler = m_zmq_connect.establishConnect(false, "ipc", ZMQ_PULL, "workermaster" , &m_masterPullFd);    
    if((m_masterPushHandler == NULL)||(m_masterPullHandler == NULL))
    {
        g_manager_logger->emerg("[master push or pull exception]");
        exit(1);
    }
    g_manager_logger->info("[master push and pull success]");

//init worker progress list    
	for(auto i = 0; i < m_workerNum; i++)
	{
		BC_process_t *pro = new BC_process_t;
		if(pro == NULL) return;
		pro->pid = 0;
		pro->status = PRO_INIT;
        pro->channel[0]=pro->channel[1]=-1;
		m_workerList.push_back(pro);
	};    
}
catch(...)
{
    g_manager_logger->emerg("throttleServ structrue exception");
    exit(1);
}
}

/*
  *name:zmq_get_message
  *argument:
  *func:get zmq_msg_t format message from zeromq socket
  *return: failure return -1, success return msglen
  */
int throttleServ::zmq_get_message(void* socket, zmq_msg_t &part, int flags)
{
     int rc = zmq_msg_init (&part);
     if(rc != 0) 
     {
        return -1;
     }

     rc = zmq_recvmsg (socket, &part, flags);
     if(rc == -1)
     {
        return -1;       
     }
     return rc;
}

/*
  *name:recvAD_callback
  *argument:fd,event,argment
  *func:if recv throttle data msg(subscribe), trigger the callback function
  *return: 
  */
void throttleServ::recvAD_callback(int _, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len;

    throttleServ *serv = (throttleServ*) pair;
    if(serv==NULL) 
    {
        g_manager_logger->emerg("serv is nullptr");
        return;
    }

    len = sizeof(events);
    void *adrsp = serv->m_throttleManager.get_throttle_request_handler();
    if(adrsp == NULL)
    {
        g_manager_logger->emerg("get_throttle_request_handler is nullptr");
        return;
    }
    
    int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_manager_logger->error("subscribe event exception");
        return;
    }

    if ( events & ZMQ_POLLIN )//recv msg
    {
        while (1)
        {
            zmq_msg_t first_part;
            int recvLen = serv->zmq_get_message(adrsp, first_part, ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                zmq_msg_close (&first_part);
                break;
            }
            zmq_msg_close(&first_part);

            zmq_msg_t part;
            recvLen = serv->zmq_get_message(adrsp, part, ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                zmq_msg_close (&part);
                break;
            }
            char *msg_data=(char *)zmq_msg_data(&part);  
             if(recvLen)
             {   
                 g_file_logger->trace("recv ad request len:{0:d}", recvLen);
                 int rc =  zmq_send(serv->m_masterPushHandler, msg_data, recvLen, ZMQ_NOBLOCK);  //ZMQ_DONTWAIT
                 if(rc<=0)
                 {
                     static int sendToWorkerLostTimes = 0;
                     sendToWorkerLostTimes++;
                     g_file_logger->warn("sendToWorkerLostTimes:{0:d}", sendToWorkerLostTimes);  
                 }
             }
             zmq_msg_close(&part);
        }
    }
}

/*
  *name:broadcastLoginOrHeartReq
  *argument:throttleserv point
  *func:send login or heart req to bidder, connector, bc
  *return: 
  */
void *throttleServ::broadcastLoginOrHeartReq(void *throttle)
{
    try
    {
        throttleServ *serv = (throttleServ*) throttle;
        if(serv == NULL)
        {
            g_manager_logger->emerg("broadcastLoginOrHeartReq serv is NULL");
            exit(1);
        }
        sleep(1);
        while(1)
        {
            if(serv->m_throttleManager.loginOrHeartReq() == false)//if there is device dorp(lost heart 3 times ), reload configure continuous
            {   
                serv->m_config.readConfig();
                serv->m_throttleManager.updateDev(serv->m_config.get_bidder_info(), serv->m_config.get_bc_info()
                    , serv->m_config.get_connector_info());
            }
            sleep(serv->m_heart_interval);
        }
    }
    catch(...)
    {
        g_manager_logger->emerg("broadcastLoginOrHeartReq exception");
    }
}


/*
  *name:broadcastLoginOrHeartReq
  *argument:throttleserv point
  *func:establish Asynchronous event using libevent
  *return: 
  */
void *throttleServ::throttleManager_handler(void *throttle)
{
    throttleServ *serv = (throttleServ*) throttle;
    struct event_base* base = event_base_new(); 
    eventArgment *arg = new eventArgment(base, throttle);

    //master pull worker handlered msg
    struct event *masterPullEvent = event_new(base, serv->m_masterPullFd, EV_READ|EV_PERSIST, masterPullMsg, throttle); 
    event_add(masterPullEvent, NULL);

    //subscribe throttle async event
    serv->m_throttleManager.throttleRequest_event_new(base, recvAD_callback, throttle);   
    //manager async event
    serv->m_throttleManager.throttleManager_event_new(base, recvManangerInfo_callback, arg);
    //recv response msg event
    serv->m_throttleManager.connectAllDev(serv->m_zmq_connect, base, recvBidderLogin_callback, throttle);
    event_base_dispatch(base);
    return NULL;  
}

bool throttleServ::masterRun()
{     
      //ad thread, expire thread, poll thread
      pthread_t pth;
      pthread_t pthl;

      pthread_create(&pth,  NULL,  broadcastLoginOrHeartReq, this);
      pthread_create(&pthl, NULL,  throttleManager_handler,  this); 
      g_manager_logger->info("==========================throttle serv start===========================");
	  return true;
}


/*
  *name:manager_from_connector_handler
  *argument:
  *func:manager messager handler from connector
  *return: if need response ,return true, else false
  */
bool throttleServ::manager_from_connector_handler(const managerProtocol_messageType &type
  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg)
{
    const string& ip = value.ip();
    if(value.port_size() < 1) return false;
    unsigned short mangerPort = value.port(0);
    bool ret = false;
    switch(type)
    {
        case managerProtocol_messageType_REGISTER_REQ:
        {
            const string& key = value.key();
            g_manager_logger->info("[register req][throttle <- connector]:{0},{1:d},{2}", ip, mangerPort, key);
            ret = m_throttleManager.registerKey_handler(false,key, m_zmq_connect, base, recvBidderLogin_callback, arg);
            if(ret)
            {
                rspType = managerProtocol_messageType_REGISTER_RSP;
            }
            break;             
        }
        case managerProtocol_messageType_LOGIN_REQ:
        {
            //add_connector_to_throttle_request
            g_manager_logger->info("[login req][throttle <- connector]:{0},{1:d}", ip, mangerPort);
            m_throttleManager.add_dev(sys_connector, ip, mangerPort, m_zmq_connect, base, recvBidderLogin_callback ,arg);
            rspType = managerProtocol_messageType_LOGIN_RSP;
            ret = true;
            break;
        }
        case managerProtocol_messageType_LOGIN_RSP:
        {
            //add_throttle_to_connector_response
           g_manager_logger->info("[login rsp][throttle <- connector]:{0},{1:d}", ip, mangerPort);
           m_throttleManager.loginSuccess(sys_connector, ip, mangerPort);
           break;
        }
        case managerProtocol_messageType_HEART_RSP:
        {
            //throttle_heart_to_connector_response
            g_manager_logger->info("[heart rsp][throttle <- connector]:{0},{1:d}", ip, mangerPort);
            m_throttleManager.recvHeartBeatRsp(sys_connector, ip, mangerPort);
            break;
        }
        default:
        {
            g_manager_logger->info("managerProtocol_messageType from connector exception {0:d}",(int)  type);
            break;
        }
    }
    
    return ret;
}


/*
  *name:manager_from_bidder_handler
  *argument:
  *func:manager messager handler from bidder
  *return: if need response ,return true, else false
  */
bool throttleServ::manager_from_bidder_handler(const managerProtocol_messageType &type
  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg)
{
    const string& ip = value.ip();
    if(value.port_size() < 1) return false;
    unsigned short mangerPort = value.port(0);
    bool ret = false;
    switch(type)
    {
        case managerProtocol_messageType_REGISTER_REQ:
        {
            //bidder_register_to_throttle_request:
            const string& key = value.key();
            g_manager_logger->info("[register req][throttle <- bidder]:{0},{1:d},{2}", ip, mangerPort, key); 
            ret = m_throttleManager.registerKey_handler(true, key, m_zmq_connect, base, recvBidderLogin_callback, arg);

            if(ret)
            {
                rspType = managerProtocol_messageType_REGISTER_RSP;
            }
            break; 
        }
        case managerProtocol_messageType_LOGIN_REQ:
        {
            //add_bidder_to_throttle_request:
            g_manager_logger->info("[login req][throttle <- bidder]:{0},{1:d}", ip, mangerPort);  
            m_throttleManager.add_dev(sys_bidder, ip, mangerPort,m_zmq_connect, base, recvBidderLogin_callback , arg);
            rspType = managerProtocol_messageType_LOGIN_RSP;
            ret = true;
            break;
        }
        case managerProtocol_messageType_LOGIN_RSP:
        {
            //add_throttle_to_bidder_response:
           g_manager_logger->info("[login rsp][throttle <- bidder]:{0},{1:d}", ip, mangerPort);
           m_throttleManager.loginSuccess(sys_bidder,ip, mangerPort);
           break;
        }
        case managerProtocol_messageType_HEART_RSP:
        {
            //throttle_heart_to_bidder_response:
            g_manager_logger->info("[heart rsp][throttle <- bidder]:{0},{1:d}", ip, mangerPort);
            m_throttleManager.recvHeartBeatRsp(sys_bidder, ip, mangerPort);
            break;
        }
        default:
        {
            g_manager_logger->info("managerProtocol_messageType from bidder exception {0:d}",(int)  type);
            break;
        }
    }
    
    return ret;
}

/*
  *name:manager_from_BC_handler
  *argument:
  *func:manager messager handler from bc
  *return: if need response ,return true, else false
  */
bool throttleServ::manager_from_BC_handler(const managerProtocol_messageType &type
  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg)
{
    const string& ip = value.ip();
    if(value.port_size() < 1) return false;
    unsigned short mangerPort = value.port(0);
    bool ret = false;
    switch(type)
    {
        case managerProtocol_messageType_LOGIN_REQ:
        {
            //add_bc_to_throttle_request:
            g_manager_logger->info("[login req][throttle <- bc]:{0},{1:d}", ip, mangerPort);
            m_throttleManager.add_dev(sys_bc, ip, mangerPort, m_zmq_connect, base, recvBidderLogin_callback , arg);
            ret = true;
            rspType = managerProtocol_messageType_LOGIN_RSP;
            break;
        }
        case managerProtocol_messageType_LOGIN_RSP:
        {
            //add_throttle_to_bc_response:
            g_manager_logger->info("[login rsp][throttle <- bc]:{0},{1:d}", ip, mangerPort);
            m_throttleManager.loginSuccess(sys_bc, ip, mangerPort);
           break;
        }
        case managerProtocol_messageType_HEART_RSP:
        {
            //throttle_heart_to_bc_response:
            g_manager_logger->info("[heart rsp][throttle <- bc]:{0},{1:d}", ip, mangerPort);
            m_throttleManager.recvHeartBeatRsp(sys_bc, ip, mangerPort);
            break;
        }
        default:
        {
            g_manager_logger->info("managerProtocol_messageType exception {0:d}",(int) type);
            break;
        }
    }
    
    return ret;
}

/*
  *name:manager_handler
  *argument:
  *func:manager messager handler 
  *return: if need response ,return true, else false
  */
bool throttleServ::manager_handler(const managerProtocol_messageTrans &from
           ,const managerProtocol_messageType &type, const managerProtocol_messageValue &value
           ,managerProtocol_messageType &rspType,struct event_base * base, void * arg)
{
    bool ret = false;
    switch(from)
    {
        case managerProtocol_messageTrans_BC:
        {
            ret = manager_from_BC_handler(type, value, rspType, base, arg);
            break;
        }
        case managerProtocol_messageTrans_BIDDER:
        {
            ret = manager_from_bidder_handler(type, value, rspType, base, arg);
            break;
        }
        case managerProtocol_messageTrans_CONNECTOR:
        {
            ret = manager_from_connector_handler(type, value, rspType, base, arg);    
            break;
        }
        default:
        {
            g_manager_logger->info("managerProtocol_messageFrom exception {0:d}",(int)from);
            break;
        }
    }
    return ret;
}
bool throttleServ::manager_handler(void *handler, string& identify,  const managerProtocol_messageTrans &from
            ,const managerProtocol_messageType &type, const managerProtocol_messageValue &value,struct event_base * base, void * arg)
{
    managerProtocol_messageType rspType;
    bool ret = manager_handler(from, type, value, rspType, base, arg);
    if(ret)//if need response 
    {
        throttleConfig& configure = m_throttleManager.get_throttleConfig();
        const string ip = configure.get_throttleIP();
        int size = 0;
        if(rspType != managerProtocol_messageType_REGISTER_RSP)//not register response ,there is not the key
        {
            size = managerProPackage::send_response(handler, identify, managerProtocol_messageTrans_THROTTLE, rspType
                , ip , configure.get_throttleManagerPort(), configure.get_throttlePubPort());
        }
        else//register response ,there is the key
        {
            size = managerProPackage::send_response(handler, identify, managerProtocol_messageTrans_THROTTLE, rspType
                , value.key(), ip , configure.get_throttleManagerPort(), configure.get_throttlePubPort());
        }
        g_manager_logger->info("manager handler send size :{0:d}", size);
    }
    return ret;
}

/*
  *name:recvBidderLogin_callback
  *argument:
  *func:manager response messager callback
  *return:
  */
void throttleServ::recvBidderLogin_callback(int fd, short event, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len;

    throttleServ *serv = (throttleServ*) pair;
    if(serv==NULL) 
    {
        g_manager_logger->emerg("recvBidderLogin_callback param error");
        exit(1);
    }

    len = sizeof(events);
    void *adrsp = serv->m_throttleManager.get_login_handler(fd);
    if(adrsp==NULL)
    {
        g_manager_logger->emerg("recvBidderLogin_callback get handler exception");
        return;
    }
    int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {

        g_manager_logger->emerg("recvBidderLogin_callback zmq_getsockopt exception");
        return;
    }

    if ( events & ZMQ_POLLIN )
    {
        while (1)
        {
            zmq_msg_t part;
            int recvLen = serv->zmq_get_message(adrsp, part, ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                zmq_msg_close (&part);
                break;
            }
            char *msg_data=(char *)zmq_msg_data(&part);
            if(msg_data)
            {
                managerProtocol manager_pro;
                manager_pro.ParseFromArray(msg_data, recvLen);  
                const managerProtocol_messageTrans &from = manager_pro.messagefrom();
                const managerProtocol_messageType &type = manager_pro.messagetype();
                const managerProtocol_messageValue &value = manager_pro.messagevalue();
                managerProtocol_messageType rspType;
                serv->manager_handler(from, type, value, rspType);   
            }
            zmq_msg_close(&part);
        }
    }
}

/*
  *name:recvManangerInfo_callback
  *argument:
  *func:manager request messager callback
  *return:
  */
void throttleServ::recvManangerInfo_callback(int fd, short event, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len;
    eventArgment *arg = (eventArgment*) pair;

    throttleServ *serv = (throttleServ*) arg->get_serv();
    if(serv==NULL) 
    {
        g_manager_logger->emerg("recvManangerInfo_callback param error");
        exit(1);
    }

    len = sizeof(events);
    void *adrsp = serv->m_throttleManager.get_throttle_manager_handler();
    if(adrsp==NULL)
    {
        g_manager_logger->emerg("recvManangerInfo_callback get throttle handler exception");
        return;
    }
    int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_manager_logger->emerg("recvManangerInfo_callback zmq_getsockopt exception");
        return;
    }

    if ( events & ZMQ_POLLIN )
    {
        while (1)
        {
            zmq_msg_t identify_part;
            int identify_len = serv->zmq_get_message(adrsp, identify_part, ZMQ_NOBLOCK);
            if ( identify_len == -1 )
            {
                zmq_msg_close (&identify_part);
                break;
            }
            char *identify_data=(char *)zmq_msg_data(&identify_part);
            string identify(identify_data, identify_len);
            zmq_msg_close(&identify_part);     
            
            zmq_msg_t data_part;
            int data_len = serv->zmq_get_message(adrsp, data_part, ZMQ_NOBLOCK);
            if ( data_len == -1 )
            {
                zmq_msg_close (&data_part);
                break;
            }
            char *data=(char *)zmq_msg_data(&data_part);
            if(data)
            {
                managerProtocol manager_pro;
                manager_pro.ParseFromArray(data, data_len);  
                const managerProtocol_messageTrans &from = manager_pro.messagefrom();
                const managerProtocol_messageType &type = manager_pro.messagetype();
                const managerProtocol_messageValue &value = manager_pro.messagevalue();
                managerProtocol_messageType rspType;
                serv->manager_handler(adrsp, identify, from, type, value, arg->get_base(),(void *)serv);      
            }
            zmq_msg_close (&data_part);
        }
    }
}
/*
  *name:publishData
  *argument:
  *func:publish data with zeromq publish
  *return:
  */
void throttleServ::publishData(char *msgData, int msgLen)
{
    char uuid[PUBLISHKEYLEN_MAX];
    memset(uuid, 0, sizeof(uuid));
    memcpy(uuid, msgData, sizeof(uuid));
    static int sendNum = 1;
    
    displayRecord();

    /*
      *each request just send to bidder and connector by hash random
      */
    #if 0   
    string bidderKey;
    string connectorKey;
    if(m_throttleManager.get_throttle_publishKey(uuid, bidderKey, connectorKey))
    {
        int dataLen = msgLen - PUBLISHKEYLEN_MAX;
        void *pubVastHandler = m_throttleManager.get_throttle_publish_handler();
        g_file_logger->debug("master publish:{0},{1},{2},{3:d}", uuid, bidderKey, connectorKey, sendNum++);            
        if(pubVastHandler)
        {
            if(bidderKey.empty()==false)
            {
                //publish data to bidder and BC
                zmq_send(pubVastHandler, bidderKey.c_str(), bidderKey.size(), ZMQ_SNDMORE|ZMQ_DONTWAIT);
                zmq_send(pubVastHandler, msgData+PUBLISHKEYLEN_MAX, dataLen, ZMQ_DONTWAIT);  
            }
            if(connectorKey.empty()==false)
            {
                //publish data to connector and BC
                zmq_send(pubVastHandler, connectorKey.c_str(), connectorKey.size(), ZMQ_SNDMORE|ZMQ_DONTWAIT);
                zmq_send(pubVastHandler, msgData+PUBLISHKEYLEN_MAX, dataLen, ZMQ_DONTWAIT);  
            }
        }
    }
    else
    {
        static int failureTimes = 0;
        failureTimes++;
        g_file_logger->warn("req failure:{0}, {1:d}", uuid, failureTimes);  
    }
    #endif


    /*
      *each request send to all bidder and connector by loop
      */
    #if 1
    void *pubVastHandler = m_throttleManager.get_throttle_publish_handler();
    if(pubVastHandler)
    {
        g_file_logger->debug("master publish:{0}, {1:d}", uuid, sendNum++);
        m_throttleManager.publishData(pubVastHandler, msgData, msgLen);
    }
    else
    {
        g_file_logger->error("pubVastHandler null");
    }
    #endif
    
}

/*
  *name:workerPublishData
  *argument:
  *func:publish data with zeromq publish
  *return:
  */
void throttleServ::workerPublishData(char *msgData, int msgLen)
{
    char uuid[PUBLISHKEYLEN_MAX];
    memset(uuid, 0, sizeof(uuid));
    memcpy(uuid, msgData, sizeof(uuid));
    static int sendNum = 1;
    
    //displayRecord();

    /*
      *each request just send to bidder and connector by hash random
      */
    #if 0   
    string bidderKey;
    string connectorKey;
    if(m_throttleManager.get_throttle_publishKey(uuid, bidderKey, connectorKey))
    {
        int dataLen = msgLen - PUBLISHKEYLEN_MAX;
        void *pubVastHandler = m_throttleManager.get_throttle_publish_handler();
        g_file_logger->debug("master publish:{0},{1},{2},{3:d}", uuid, bidderKey, connectorKey, sendNum++);            
        if(pubVastHandler)
        {
            if(bidderKey.empty()==false)
            {
                //publish data to bidder and BC
                zmq_send(pubVastHandler, bidderKey.c_str(), bidderKey.size(), ZMQ_SNDMORE|ZMQ_DONTWAIT);
                zmq_send(pubVastHandler, msgData+PUBLISHKEYLEN_MAX, dataLen, ZMQ_DONTWAIT);  
            }
            if(connectorKey.empty()==false)
            {
                //publish data to connector and BC
                zmq_send(pubVastHandler, connectorKey.c_str(), connectorKey.size(), ZMQ_SNDMORE|ZMQ_DONTWAIT);
                zmq_send(pubVastHandler, msgData+PUBLISHKEYLEN_MAX, dataLen, ZMQ_DONTWAIT);  
            }
        }
    }
    else
    {
        static int failureTimes = 0;
        failureTimes++;
        g_file_logger->warn("req failure:{0}, {1:d}", uuid, failureTimes);  
    }
    #endif


    /*
      *each request send to all bidder and connector by loop
      */
    #if 1
    void *pubVastHandler = m_throttleManager.get_throttle_publish_handler();
    if(pubVastHandler)
    {
        g_file_logger->debug("woker publish:{0}, {1:d}", uuid, sendNum++);
        m_throttleManager.workerPublishData(pubVastHandler, msgData, msgLen);
    }
    else
    {
        g_file_logger->error("pubVastHandler null");
    }
    #endif
    
}



/*
  *name:masterPullMsg
  *argument:
  *func:master pull message
  *return:
  */

void throttleServ::masterPullMsg(int _, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len;

    throttleServ *serv = (throttleServ*) pair;
    if(serv==NULL) 
    {
        g_manager_logger->emerg("masterPullMsg param error");
        return;
    }

    len = sizeof(events);
    void *adrsp = serv->m_masterPullHandler;
    if(adrsp == NULL)
    {
        g_manager_logger->emerg("masterPullHandler is null");
        exit(1);
    }
    int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_manager_logger->emerg("masterPullMsg zmq_getsockopt exception");
        return;
    }

    if ( events & ZMQ_POLLIN )
    {
        while (1)
        {
            zmq_msg_t first_part;
            int recvLen = serv->zmq_get_message(adrsp, first_part, ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                zmq_msg_close (&first_part);
                break;
            }

            char *msg_data=(char *)zmq_msg_data(&first_part);
            if(msg_data)
            {
                serv->publishData(msg_data, recvLen);
            }
            zmq_msg_close(&first_part);
        }
    }
}

/*
  *name:parseAdRequest
  *argument:
  *func:parse ad request
  *return:
  */
void throttleServ::parseAdRequest(char *msgData, int msgLen)
{
    CommonMessage commMsg;
    commMsg.ParseFromArray(msgData, msgLen);     
    const string& vastStr=commMsg.data();
    const string& tbusinessCode = commMsg.businesscode();
    string uuid;
    string publishKey;
              
    if(tbusinessCode == m_vastBusiCode)  
    {
       VastRequest vast;
       vast.ParseFromString(vastStr);
       uuid = vast.id();
    }
    else if(tbusinessCode == m_mobileBusiCode)
    {
       MobileAdRequest mobile;
       mobile.ParseFromString(vastStr);
       uuid = mobile.id();
    }
    else
    {
       g_file_logger->warn("businescode can  not anasis");
       return;
    }           

    if(m_logRedisOn)
    {
        ostringstream os;
        os<<uuid<<"_"<<getLonglongTime()<<"_throttle*recv";
        string logValue = os.str();
        m_logManager.addLog(logValue);
    }
    
    g_file_logger->trace("worker recv request uuid:{0},{1:d}", uuid, getpid());
    int tmpBufLen = PUBLISHKEYLEN_MAX+msgLen;
    char *tmpBuf = new char[tmpBufLen];
    memset(tmpBuf, 0x00, tmpBufLen);
    memcpy(tmpBuf, uuid.c_str(), uuid.size());
    memcpy(tmpBuf+PUBLISHKEYLEN_MAX, msgData, msgLen);  

	#if 0
    int rc = zmq_send(m_workerPushHandler, tmpBuf, tmpBufLen, ZMQ_NOBLOCK);
    
    if(rc <= 0)
    {
        static int sendToMastLostTimes = 0;
        sendToMastLostTimes++;
        g_file_logger->warn("sendToMastLostTimes:{0:d}", sendToMastLostTimes);         
    }
	#endif

	workerPublishData(tmpBuf, tmpBufLen);
	
    delete[] tmpBuf; 
}

/*
  *name:workerPullMsg
  *argument:
  *func:worker pull ad request
  *return:
  */

void throttleServ::workerPullMsg(int _, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len;

    throttleServ *serv = (throttleServ*) pair;
    if(serv==NULL) 
    {
        g_manager_logger->emerg("workerPullMsg param error");
        exit(1);
    }

    len = sizeof(events);
    void *adrsp = serv->m_workerPullHandler;
    if(adrsp == NULL)
    {
        g_manager_logger->emerg("workerPullHandler is null");
        exit(1);
    }

    int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_manager_logger->emerg("workerPullMsg zmq_getsockopt exception");
        return;
    }

    if ( events & ZMQ_POLLIN )
    {
        while (1)
        {
            zmq_msg_t part;
            int recvLen = serv->zmq_get_message(adrsp, part, ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                zmq_msg_close (&part);
                break;
            }
            char *msg_data=(char *)zmq_msg_data(&part);
            if(msg_data)
            {
                serv->parseAdRequest(msg_data, recvLen);
            }
            zmq_msg_close(&part); 
        }
    }
}

/*
  *name:hupSigHandler
  *argument:
  *func:worker recv hup sig handler
  *return:
  */
void throttleServ::hupSigHandler(int fd, short event, void *arg)
{
    throttleServ *serv = (throttleServ*) arg;
    g_file_logger->info("signal hup");
    exit(0);
}

/*
  *name:intSigHandler
  *argument:
  *func:worker recv int sig handler
  *return:
  */
void throttleServ::intSigHandler(int fd, short event, void *arg)
{
    throttleServ *serv = (throttleServ*) arg;
    g_file_logger->info("signal int");
    usleep(100);
    exit(0);
}

/*
  *name:termSigHandler
  *argument:
  *func:worker recv term sig handler
  *return:
  */
void throttleServ::termSigHandler(int fd, short event, void *arg)
{
    throttleServ *serv = (throttleServ*) arg;
    g_file_logger->info("signal term");
    exit(0);
}

/*
  *name:usr1SigHandler
  *argument:
  *func:worker recv usr1 sig handler(kill -10)
  *return:
  */

void throttleServ::usr1SigHandler(int fd, short event, void *arg)
{
    throttleServ *serv = (throttleServ*) arg;
    if(serv)
    {
        g_file_logger->info("signal SIGUSR1");    
        serv->workerRestart(arg);
    }
}

void throttleServ::workerRestart(void *arg)
{
    m_config.readConfig();
    if(logLevelChange())
    {
        g_file_logger->set_level(m_logLevel);
        g_manager_logger->set_level(m_logLevel);
    }
}

void throttleServ::signal_handler(int signo)
{
    time_t timep;
    time(&timep);
    char *timeStr = ctime(&timep);
    switch (signo)
    {
        case SIGTERM:
            g_manager_logger->info("SIGTERM");
            srv_ungraceful_end = 1;
            break;
        case SIGINT:
            g_manager_logger->info("SIGINT");
            srv_graceful_end = 1;
            break;
        case SIGHUP:
            g_manager_logger->info("SIGHUP");
            srv_restart = 1;
            break;
        case SIGUSR1:   
            g_manager_logger->info("SIGUSR1");
            sigusr1_recved = 1;
            break;
        case SIGALRM: 
//            g_manager_logger->info("SIGALRM");
            sigalrm_recved = 1;
            break;
    }
}

/*
  *name:start_worker
  *argument:
  *func:worker init
  *return:
  */
void throttleServ::start_worker()
{
    try
    {
        m_timeMutex.init();
        m_zmq_connect.init();
        pid_t pid = getpid();


        pthread_t pth2;
        pthread_t pth3;
        pthread_create(&pth2, NULL,  getTime,  this); 
        pthread_create(&pth3, NULL,  logWrite,  this); 

        m_logRedisPoolManger.connectorPool_init(m_logRedisIP, m_logRedisPort, 10);
        //g_file_logger = spdlog::rotating_logger_mt("worker", "logs/debugfile", 1048576*500, 3, true); 
        g_file_logger = spdlog::daily_logger_mt("worker", "logs/debugfile", true); 
        g_file_logger->set_level(m_logLevel);    

        m_workerPullHandler = m_zmq_connect.establishConnect(true, "ipc", ZMQ_PULL,  "masterworker",  &m_workerPullFd);
        m_workerPushHandler = m_zmq_connect.establishConnect(true, "ipc", ZMQ_PUSH,  "workermaster",  NULL);    
        if((m_workerPullHandler == NULL)||(m_workerPushHandler == NULL))
        {
            g_file_logger->info("worker push or pull exception");
            exit(1); 
        }
        g_file_logger->info("worker {0:d} push or pull success", pid);
        
        struct event_base* base = event_base_new();    
        struct event * hup_event = evsignal_new(base, SIGHUP, hupSigHandler, this);
        struct event * int_event = evsignal_new(base, SIGINT, intSigHandler, this);
        struct event * term_event = evsignal_new(base, SIGTERM, termSigHandler, this);
        struct event * usr1_event = evsignal_new(base, SIGUSR1, usr1SigHandler, this);
        struct event *clientPullEvent = event_new(base, m_workerPullFd, EV_READ|EV_PERSIST, workerPullMsg, this);

        event_add(clientPullEvent, NULL);
        evsignal_add(hup_event, NULL);
        evsignal_add(int_event, NULL);
        evsignal_add(term_event, NULL);
        evsignal_add(usr1_event, NULL);
        event_base_dispatch(base);
    }
    catch(...)
    {
        g_file_logger->emerg("start_worker exception");
        exit(1);
    }
}

/*
  *name:displayRecord
  *argument:
  *func:record speed, cost time and so on
  *return:
  */

void throttleServ::displayRecord()
{
    static long long m_begTime=0;
    static long long m_endTime=0;
    static int m_count = 0;
    const int statistics_fre = 10000;
    

    m_displayMutex.lock();
    m_count++;
    if(m_count%statistics_fre==1)
    {
        struct timeval btime;
        gettimeofday(&btime, NULL);
        m_begTime = btime.tv_sec*1000+btime.tv_usec/1000;
        m_displayMutex.unlock();

    }
    else if(m_count%statistics_fre==0)
    {
        struct timeval etime;
        gettimeofday(&etime, NULL);
        m_endTime = etime.tv_sec*1000+etime.tv_usec/1000;
        auto cnt = m_count;
        auto diff = m_endTime-m_begTime;
        auto speed = statistics_fre*1000/diff;
        m_displayMutex.unlock();
        
        g_file_logger->info("handler reqeust num, const time, speed:{0:d}, {1:d}, {2:d}", cnt, diff, speed);   
    }
    else
    {
       m_displayMutex.unlock();
    }
}

/*
  *name:updataWorkerList
  *argument:
  *func:update worker list 
  *return:
  */
void throttleServ::updataWorkerList(pid_t pid)
{
    for(auto it = m_workerList.begin(); it != m_workerList.end(); it++)
    {
        BC_process_t *pro = *it;
        if(pro&&(pro->pid == pid))
        {
            pro->pid = 0;
            pro->status = PRO_INIT;
            close(pro->channel[0]);
            if(needRemoveWorker())
            {
                delete pro;
                m_workerList.erase(it);
                break;
            }
        }
    }

}

inline bool throttleServ::needRemoveWorker() const
{
    return (m_workerList.size()>m_workerNum)?true:false;
}


/*
  *name:logLevelChange
  *argument:
  *func:changer log level
  *return:
  */
bool throttleServ::logLevelChange()
{
     int logLevel = m_config.get_throttleLogLevel();
     spdlog::level::level_enum log_level;
     if((logLevel>((int)spdlog::level::off) ) || (logLevel < ((int)spdlog::level::trace))) 
     {
         log_level = spdlog::level::info;
     }
     else
     {
         log_level = (spdlog::level::level_enum) logLevel;
     }
     
     if(log_level != m_logLevel)
     {
         g_manager_logger->emerg("log level from level {0:d} to {1:d}", (int)m_logLevel, (int)log_level);
         m_logLevel = log_level;
         return true;
     }
     return false;
}


void throttleServ::run()
{
    bool worker_exit = false;
    int  num_children = 0;
    int  restart_finished = 1;
    int  is_child = 0;
    bool master_started = false;  
   // register signal handler
    struct sigaction act;
   
    bzero(&act, sizeof(act));
    act.sa_handler = signal_handler;
               
    sigaction(SIGALRM, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT , &act, NULL);
    sigaction(SIGHUP , &act, NULL);
    sigaction(SIGUSR1, &act, NULL); 
              
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGALRM);
    sigdelset(&set, SIGTERM);
    sigdelset(&set, SIGINT);
    sigdelset(&set, SIGHUP);
    sigdelset(&set, SIGUSR1);
    sigprocmask(SIG_SETMASK, &set, NULL);
                  
    struct itimerval timer;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 10;
    timer.it_interval.tv_usec = 0;          
    setitimer(ITIMER_REAL, &timer, NULL);
    
    while(true)
    {
        pid_t pid;
        if(num_children != 0)
        {  
            pid = wait(NULL);
            if(pid != -1)
            {
                num_children--;
                g_manager_logger->info("child process {0:d} exit", pid);
                worker_exit = true;   
                updataWorkerList(pid);
            }

            if (srv_graceful_end || srv_ungraceful_end)//CTRL+C ,killall throttle
            {
                g_manager_logger->info("srv_graceful_end:{0:d}", srv_graceful_end);
                if (num_children == 0)
                {
                    break;    //get out of while(true)
                }

                for (auto it = m_workerList.begin(); it != m_workerList.end(); it++)
                {
                    BC_process_t *pro = *it;
                    if(pro&&pro->pid != 0)
                    {
                        g_manager_logger->info("kill -9  {0:d}", pro->pid);
                        kill(pro->pid, srv_graceful_end ? SIGINT : SIGTERM);
                    }
                }
                continue;    //this is necessary.
            }

      
            if (srv_restart)
            {
                g_manager_logger->info("srv_restart");
                srv_restart = 0; 
                if (restart_finished)
                {
                    restart_finished = 0;
                    auto it = m_workerList.begin();
                    for (it = m_workerList.begin(); it != m_workerList.end(); it++)
                    {
                        BC_process_t *pro = *it;
                        if(pro == nullptr) continue;
                        if(pro->status == PRO_BUSY)  pro->status = PRO_RESTART; 
                    }
                }
            }

            if (!restart_finished) //not finish
            {
                BC_process_t *pro=NULL;

                auto it = m_workerList.begin();
                for (it = m_workerList.begin(); it != m_workerList.end(); it++)
                {
                    pro = *it;
                    if(pro == nullptr) continue;
                    if(pro->status == PRO_RESTART)
                    {
                        break;
                    }
                }
            
                // 上一次发送SIGHUP时正处于工作的worker已经全部
                // 重启完毕.
                if (it == m_workerList.end())
                {
                    restart_finished = 1;
                } 
                else
                {
                //    close(pro->channel[0]);
                    kill(pro->pid, SIGHUP);
                    //child_info[ndx].state = 3;  重启中                 
                    //还是为了尽量避免连续的SIGHUP的不良操作带来的颠簸
                    //,所以决定取消(3重启中)这个状态
                    //并不是说连续SIGHUP会让程序出错,只是不断的挂掉新进程很愚蠢
                }
            }

            if(is_child==0 &&sigusr1_recved)//master and notice worker reload configure
            {    
                g_manager_logger->info("master progress recv sigusr1");         
                updateConfigure();//read configur file
                auto count = 0;
                for (auto it = m_workerList.begin(); it != m_workerList.end(); it++, count++)
                {
                      BC_process_t *pro = *it;
                      if(pro == NULL) continue;
                      if(pro->pid == 0) 
                      {
                          continue;
                      }
               
                      //cout <<"pro->pid:" << pro->pid<<"::"<< getpid()<<endl;
                      if(count >= m_workerNum)
                      {
                           kill(pro->pid, SIGKILL); 
                      }
                      else
                      {
                           kill(pro->pid, SIGUSR1); 
                      }
                 }

                int workerSize = m_workerList.size();
                for (count=0; count < m_workerNum; count++)
                {
                    if(count >= workerSize)//worker pro increase
                    {
				        BC_process_t *pro = new BC_process_t;
				        if(pro == NULL) return;
				        pro->pid = 0;
				        pro->status = PRO_INIT;
                        pro->channel[0]=pro->channel[1]=-1;
				        m_workerList.push_back(pro); 
                    }
                }    
            }
 
        }
        int workIndex = 1;

        //fork child progress and start run progess
        bool proInit = false;
        for (auto it = m_workerList.begin(); it != m_workerList.end(); it++)
        {
            workIndex++;
            BC_process_t *pro = *it;
            if(pro == NULL) continue;
            if (pro->status == PRO_INIT)
            {
                proInit = true;
                if (socketpair(AF_UNIX, SOCK_STREAM, 0, pro->channel) == -1)
                {
                    return;
                }

                pid = fork();

                switch (pid)
                {
                    case -1:
                    {
                        g_manager_logger->info("fork exception");  
                    }
                    break;
                    case 0:
                    {     
                        close(pro->channel[0]);
                        is_child = 1; 
                        g_manager_logger->info("<worker restart>");
                        pro->pid = getpid();
                    }
                    break;
                    default:
                    {
                        close(pro->channel[1]);
                        pro->pid = pid;
                        ++num_children;
                    }
                    break;
                }
                pro->status = PRO_BUSY;
                if (!pid) break; 
            }
        }

        
        if(is_child==0 && master_started==false)//master run init 
        {    
             //master serv start  
             sleep(1);
             master_started = true;  
             masterRun();   
        }

        if(((is_child==0)&&worker_exit)||(is_child==0 &&sigusr1_recved))
        {
            worker_exit = false;
            sigusr1_recved = 0;
        } 

        if (!pid) break; 
    }
 
    if (!is_child)    
    {
        g_manager_logger->info("<master exit>");
        exit(0);    
    }

    start_worker();
}

