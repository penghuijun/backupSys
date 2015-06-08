#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <time.h>
#include <set>
#include <map>
#include <queue>
#include <iterator>

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <event.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <memory>
#include "zmq.h"

#include "connector.h"
#include "threadpoolmanager.h"
#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"
#include "AdBidderResponseTemplate.pb.h"
#include "MobileAdRequest.pb.h"
#include "MobileAdResponse.pb.h"
#include "login.h"

using namespace com::rj::protos;
using namespace rapidjson;
using namespace std;

sig_atomic_t srv_graceful_end = 0;
sig_atomic_t srv_ungraceful_end = 0;
sig_atomic_t srv_restart = 0;
sig_atomic_t sigalrm_recved = 0;;
sig_atomic_t sigusr1_recved = 0;;
static int test_count = 0;

extern shared_ptr<spdlog::logger> g_file_logger;
extern shared_ptr<spdlog::logger> g_manager_logger;
extern shared_ptr<spdlog::logger> g_worker_logger;

#ifdef TIMELOG

struct timeval localTm;
extern pthread_mutex_t tmMutex;
unsigned long long getLonglongTime()
{
    pthread_mutex_lock(&tmMutex);
    unsigned long long t = localTm.tv_sec*1000+localTm.tv_usec/1000;
    pthread_mutex_unlock(&tmMutex); 
    return t;
}
void* connectorServ::childProcessGetTime(void *arg)
{
    connectorServ *serv = (connectorServ*) arg;
    const int getTimeInterval=10*1000;    
    int times = 0;
    while(1)
    {
        usleep(getTimeInterval);
        pthread_mutex_lock(&tmMutex);
        gettimeofday(&localTm, NULL);
        times++;
        pthread_mutex_unlock(&tmMutex);

        if((times*getTimeInterval)>(serv->m_heartInterval*1000*1000))
        {
            times = 0;
            serv->m_extbid_manager.check_connect();
        }        
    }
}

void *connectorServ::getTime(void *arg)
{
    connectorServ *serv = (connectorServ*) arg;
    int times = 0;
    const int getTimeInterval=10*1000;    
    while(1)
    {
        usleep(getTimeInterval);
        pthread_mutex_lock(&tmMutex);
        gettimeofday(&localTm, NULL);
        times++;
        pthread_mutex_unlock(&tmMutex);
        if((times*getTimeInterval)>(serv->m_heartInterval*1000*1000))
        {
            times = 0;
            if(serv->m_device_manager.devDrop())
            {
         //       serv->m_config.readConfig();
         //       serv->m_device_manager.update_throttle(serv->m_config.get_throttle_info());
            }
        }
    }
}
#endif

void connectorServ::get_local_ipaddr(vector<string> &ipAddrList)
{
    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);
    while (ifAddrStruct!=NULL) {
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            ipAddrList.push_back(string(addressBuffer));
        } else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            ipAddrList.push_back(string(addressBuffer));
        } 
        ifAddrStruct=ifAddrStruct->ifa_next;
    }
}



void connectorServ::readConfigFile()
{
    m_config.readConfig();
}

void connectorServ::calSpeed()
{
     const int printNum = 1000;
     static long long test_begTime;
     static long long test_endTime;

     int couuut = 0;
     m_test_lock.lock();
     test_count++;
     couuut = test_count;
     if(test_count%printNum==1)
     {
        struct timeval btime;
        gettimeofday(&btime, NULL);
        test_begTime = btime.tv_sec*1000+btime.tv_usec/1000;
        m_test_lock.unlock();
     }
     else if(test_count%printNum==0)
     {
        struct timeval etime;
        gettimeofday(&etime, NULL);
        test_endTime = etime.tv_sec*1000+etime.tv_usec/1000;
        long long diff = test_endTime-test_begTime;
        long speed = printNum*1000/diff;
        m_test_lock.unlock();
        g_file_logger->info("num:{0:d}", couuut);
        g_file_logger->info("time:{0:d}", diff);
        g_file_logger->info("speed:{0:d}", speed);
     }
     else
     {
        m_test_lock.unlock();
     }
}
void connectorServ::updateWorker()
{
    readConfigFile();
    m_device_manager.update_bcList(m_zmq_connect, m_config.get_bc_info());
    int poolsize = m_device_manager.get_connector_config().get_connectorThreadPoolSize();
    const bidderInformation &binfo = m_config.get_bidder_info();
//    m_redis_pool_manager.redisPool_update(binfo.get_redis_ip(), binfo.get_redis_port(), poolsize+1);
}



connectorServ::connectorServ(configureObject &config):m_config(config)
{
    try
    {
        int logLevel = m_config.get_bidderLogLevel();
        if( (logLevel>((int)spdlog::level::off) ) || (logLevel < ((int)spdlog::level::trace))) 
        {
            m_logLevel = spdlog::level::info;
        }
        else
        {
            m_logLevel = (spdlog::level::level_enum) logLevel;
        }
        g_file_logger->set_level(m_logLevel);
        g_manager_logger->set_level(m_logLevel);
        g_manager_logger->emerg("log level:{0}", m_logLevel);

        m_heartInterval = m_config.get_heart_interval();
        m_zmq_connect.init(); 
        m_device_manager.init(m_zmq_connect, m_config.get_throttle_info(), m_config.get_connector_info(), m_config.get_bc_info());
        m_workerNum = m_device_manager.get_connector_config().get_connectorWorkerNum();
        m_vastBusinessCode = m_config.get_vast_businessCode();
        m_mobileBusinessCode = m_config.get_mobile_businessCode();

        m_masterPushHandler = m_zmq_connect.establishConnect(false, "ipc", ZMQ_PUSH, "connectormasterworker" , NULL);
        m_masterPullHandler = m_zmq_connect.establishConnect(false, "ipc", ZMQ_PULL, "connectorworkermaster" , &m_masterPullFd);    
        if((m_masterPushHandler == NULL)||(m_masterPullHandler == NULL))
        {
            g_manager_logger->emerg("[master push or pull exception]");
            exit(1);
        }
        g_manager_logger->info("[master push and pull success]");

    
        m_worker_info_lock.init();
        m_test_lock.init();
        auto i = 0;
    	for(i = 0; i < m_workerNum; i++)
    	{
    		BC_process_t *pro = new BC_process_t;
    		pro->pid = 0;
    		pro->status = PRO_INIT;
    		m_workerList.push_back(pro);
    	};        
    }
    catch(...)
    {   
        g_manager_logger->emerg("connectorServ structure error");
        exit(1);
    }
}

/*
  *update m_workerList,  if m_wokerNum < workerListSize , erase the workerNode, if m_workerList update ,lock the resource
  */
void connectorServ::updataWorkerList(pid_t pid)
{
    m_worker_info_lock.write_lock();
    auto it = m_workerList.begin();
    for(it = m_workerList.begin(); it != m_workerList.end();)
    {
        BC_process_t *pro = *it;
        if(pro == NULL)
        {
            it = m_workerList.erase(it);
            continue;
        }
        if(pro->pid == pid) //find the child progress
        {
            pro->pid = 0;
            pro->status = PRO_INIT;
           
            if(m_workerList.size()>m_workerNum)
            {
                g_manager_logger->info("reduce worker num");
                delete pro;
                it = m_workerList.erase(it);
            }
            else
            {
                it++;
            }
        }
        else
        {
            it++;
        }
    }
    m_worker_info_lock.read_write_unlock();
}

void connectorServ::signal_handler(int signo)
{
    time_t timep;
    time(&timep);
    char *timeStr = ctime(&timep);
    switch (signo)
    {
        case SIGTERM:
            g_manager_logger->info("SIGTERM:{0}", timeStr);
            srv_ungraceful_end = 1;
            break;
        case SIGINT:
            g_manager_logger->info("SIGINT:{0}", timeStr);
            srv_graceful_end = 1;
            break;
        case SIGHUP:
            g_manager_logger->info("SIGHUP:{0}", timeStr);
            srv_restart = 1;
            break;
        case SIGUSR1:  
            g_manager_logger->info("SIGUSR1:{0}", timeStr);
            sigusr1_recved = 1;
            break;
        case SIGALRM:   
            g_manager_logger->info("SIGALARM:{0}", timeStr);
            sigalrm_recved = 1;
            break;
        default:
            g_manager_logger->info("signo:{0}", signo);
            break;
    }
}


bool parse_publishKey(string& origin, string& bcIP, unsigned short &bcManagerPort, unsigned short &bcDataPort,
                                    string& bidderIP, unsigned short& bidderPort, int recvLen)
{
    int idx = origin.find("-");
    if(idx == string::npos) return false;
    string bcAddr = origin.substr(0, idx++);
    string bidderAddr = origin.substr(idx, recvLen-idx);

    idx = bcAddr.find(":");
    if(idx == string::npos) return false;
    bcIP = bcAddr.substr(0, idx++);
    string portStr = bcAddr.substr(idx, bcAddr.size()-idx);
    idx = portStr.find(":");
    if(idx == string::npos) return false;
    string mangerPortStr = portStr.substr(0, idx++);
    string dataPortStr = portStr.substr(idx, portStr.size()-idx);
    bcManagerPort = atoi(mangerPortStr.c_str());
    bcDataPort = atoi(dataPortStr.c_str());
 
    idx = bidderAddr.find(":");
    if(idx == string::npos) return false;
    bidderIP = bidderAddr.substr(0, idx++);
    string bidderPortstr = bidderAddr.substr(idx, bidderAddr.size()-idx);
    bidderPort = atoi(bidderPortstr.c_str());
    return true;
}


void connectorServ::hupSigHandler(int fd, short event, void *arg)
{
    g_worker_logger->info("signal hup");
    exit(0);
}

void connectorServ::intSigHandler(int fd, short event, void *arg)
{
   g_worker_logger->info("signal int");
   usleep(10);
   exit(0);
}

void connectorServ::termSigHandler(int fd, short event, void *arg)
{
    g_worker_logger->info("signal term");
    exit(0);
}

void connectorServ::usr1SigHandler(int fd, short event, void *arg)
{
    g_worker_logger->info("signal SIGUSR1");
    connectorServ *serv = (connectorServ*) arg;
    if(serv != NULL)
    {
        serv->updateWorker();
    }
}

void connectorServ::frequency_display(shared_ptr<spdlog::logger>& file_logger,
    const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_Frequency >&frequency)
{
    for(auto it = frequency.begin(); it != frequency.end();it++)
    {
       auto fre = *it;
       file_logger->debug("property-id:{0}-{1}", fre.property(), fre.id());
       auto value = fre.frequencyvalue();
       for(auto iter = value.begin(); iter != value.end(); iter++)
       {
           auto fre_value = *iter;
           file_logger->debug("frequencyType:{0}    times:{1}", fre_value.frequencytype(), fre_value.times()); 
       }
    }
}

void connectorServ::worker_parse_master_data(char *data, unsigned int dataLen)
{
    if((data == NULL)||(dataLen <= PUBLISHKEYLEN_MAX)) return;

    char subsribe[PUBLISHKEYLEN_MAX];    
    memset(subsribe, 0x00, sizeof(subsribe));
    memcpy(subsribe, data, PUBLISHKEYLEN_MAX);
    string publishKey = subsribe;
    char * probufData = data+PUBLISHKEYLEN_MAX;
    int probufDataLen = dataLen - PUBLISHKEYLEN_MAX;
    
    CommonMessage commMsg;
    commMsg.ParseFromArray(probufData, probufDataLen);     
    const string &dataStr=commMsg.data();
    const string &tbusinessCode = commMsg.businesscode();
    const string &codeType = commMsg.datacodingtype();
//   get iso stadard

    bidRequestObject bidObj;    
    if(tbusinessCode == m_mobileBusinessCode)
    {
        bidObj.parse_mobile(dataStr);       
    }
    else
    {
        return;
    }

    string request_json;
    bidObj.toJson(request_json);
    m_extbid_manager.send(tbusinessCode, codeType, publishKey, request_json);
    g_worker_logger->trace("request json:{0}",request_json);

}

void  connectorServ::handle_ad_request(void *arg)
{
    messageBuf *msg = (messageBuf*) arg;
    if(msg)
    {
        char *buf = msg->get_stringBuf().get_data();
        int   dataLen = msg->get_stringBuf().get_dataLen();
        connectorServ *serv = (connectorServ*) msg->get_serv();
        if(serv)
        {
            serv->worker_parse_master_data(buf, dataLen);
        }
    }
    delete msg;
}

/*
  *worker sync handler
  *handler: recv dataLen, then read data, throw to thread handler
  */
void connectorServ::workerBusiness_callback(int fd, short event, void *pair)
{
    try
    {
        connectorServ *serv = (connectorServ*) pair;
        if(serv == NULL) 
        {
            g_worker_logger->emerg("workerBusiness_callback param is null");
            exit(1);
        }
        
        uint32_t events;
        size_t len = sizeof(events);
        void *adrsp = serv->m_workerPullHandler;
        if(adrsp == NULL)
        {
            g_worker_logger->emerg("pull master exception");
            exit(1);
        }
        
        int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
        if(rc == -1)
        {
            g_worker_logger->error("workerBusiness_callback zmq_getsockopt return -1");
            return;
        }

        if(events & ZMQ_POLLIN ) //message come
        {
            while (1)
            {
                zmq_msg_t part;
                int recvLen = serv->zmq_get_message(adrsp, part, ZMQ_NOBLOCK);//read
                if ( recvLen == -1 )
                {
                    zmq_msg_close (&part);
                    break;
                }
                
                char *msg_data=(char *)zmq_msg_data(&part);
                messageBuf *msg= new messageBuf(msg_data, recvLen, serv);
                if(serv->m_thread_manager.Run(handle_ad_request,(void *)msg) !=0 )//not return 0, failure
                {
                     g_worker_logger->error("drop request");
                     delete msg;
                }
                zmq_msg_close (&part);
            }
        }
    }
    catch(...)
    {
        g_worker_logger->emerg("workerBusiness_callback exception");
        exit(1);
    }
}

void connectorServ::recv_exbidder_adResponse(int fd, short event, void *pair)
{
   stringBuffer adRspBuf;
   connectorServ *serv = (connectorServ*)pair;
   if(serv==NULL) 
   {
       g_worker_logger->emerg("recv_exbidder_adResponse paramet exception");
       exit(1);
   }

   if(event == EV_TIMEOUT)//timeout
   {
       serv->m_extbid_manager.libevent_timeout(fd);
       return;
   }
   
   while(1)
   {
       transErrorStatus errstatus = serv->m_extbid_manager.recv_async(fd, adRspBuf);
       if(errstatus == error_socket_close)
       {
           g_worker_logger->emerg("remote serv close connect:{0:d}", fd);
           serv->m_extbid_manager.close(fd);
           return;
       }
       else if(errstatus == error_complete)
       {
           break;
       }
       else if((errstatus == error_recv_null)||(errstatus == error_cannot_find_fd))
       {
            return;
       }
   }

   char *sendHttpRsp = adRspBuf.get_data();
   if(sendHttpRsp != NULL)
   {
        g_worker_logger->trace("sendHttpRsp:{0}", sendHttpRsp);
        rapidjson::Document document;
        document.Parse<0>(sendHttpRsp);
        if(document.HasParseError())
        {
            g_worker_logger->error("rapidjson parse error::{0}", document.GetParseError());
            return;
        }
        adRequestInfo *reqInfo = serv->m_extbid_manager.adRequest_pop(fd);
        const string& busicode = reqInfo->get_busicode();
        const string& datacode = reqInfo->get_codeType();
        if(reqInfo == NULL)
        {
            g_worker_logger->error("can not find httpInfo");
            return;
        }

        bidResponseObject bidRsp;
        bidRsp.parse(document);
        CommonMessage commMsg;
        if(bidRsp.toProtobuf(busicode, datacode, commMsg) == false) return;
    
        char comBuf[BUF_SIZE];
        commMsg.SerializeToArray(comBuf, sizeof(comBuf));
        int rspAdSize = commMsg.ByteSize();

        string subscribeKey(reqInfo->get_publishkey());
        string bcIP;
        unsigned short bcManagerPort;
        unsigned short bcDataPort;
        string bidderIP;
        unsigned short bidderPort;
        bool parse_result = parse_publishKey(subscribeKey, bcIP, bcManagerPort, bcDataPort,
                    bidderIP , bidderPort, subscribeKey.size());
        delete reqInfo;
        if(parse_result == false)
        {
            return ;
        }
        
        int ssize = serv->m_device_manager.sendAdRspToBC(bcIP, bcDataPort, comBuf,  rspAdSize, ZMQ_NOBLOCK);
        if(ssize>0)
        {      
            g_worker_logger->debug("send to bc:{0:d}",ssize);
            g_worker_logger->debug("recv valid campaign, send to BC:{0:d},{1},{2}"
                , ssize, document["id"].GetString(), serv->m_device_manager.get_bidder_identify());
            serv->calSpeed();
        }
        else
        {
           g_worker_logger->error("send response to BC failure");   
        }
        serv->m_extbid_manager.send();
     }  
}

void connectorServ::start_worker()
{
try{
    srand(getpid());
    worker_net_init();
    m_test_lock.init();
    m_base = event_base_new(); 

    m_zmq_connect.init();
    pid_t pid = getpid();

    g_worker_logger = spdlog::rotating_logger_mt("worker", "logs/cdebugfile", 1048576*500, 3, true);  
    g_worker_logger->set_level(m_logLevel);  
    g_worker_logger->info("worker start:{0:d}", getpid());

    m_workerPullHandler = m_zmq_connect.establishConnect(true, "ipc", ZMQ_PULL,  "connectormasterworker",  &m_workerPullFd);
    m_workerPushHandler = m_zmq_connect.establishConnect(true, "ipc", ZMQ_PUSH,  "connectorworkermaster",  NULL);    
    if((m_workerPullHandler == NULL)||(m_workerPushHandler == NULL))
    {
        g_worker_logger->info("worker push or pull exception");
        exit(1); 
    }
    g_worker_logger->info("worker {0:d} push or pull success", pid);
    
    struct event * hup_event = evsignal_new(m_base, SIGHUP, hupSigHandler, this);
    struct event * int_event = evsignal_new(m_base, SIGINT, intSigHandler, this);
    struct event * term_event = evsignal_new(m_base, SIGTERM, termSigHandler, this);
    struct event * usr1_event = evsignal_new(m_base, SIGUSR1, usr1SigHandler, this);
    struct event *clientPullEvent = event_new(m_base, m_workerPullFd, EV_READ|EV_PERSIST, workerBusiness_callback, this);

    event_add(clientPullEvent, NULL);
    evsignal_add(hup_event, NULL);
    evsignal_add(int_event, NULL);
    evsignal_add(term_event, NULL);
    evsignal_add(usr1_event, NULL);

    int poolSize = m_device_manager.get_connector_config().get_connectorThreadPoolSize();
    m_thread_manager.Init(10000, poolSize, poolSize);
    m_extbid_manager.init(m_config.get_externBid_info(), m_base, recv_exbidder_adResponse, this);

// 初始化对照表
    
    event_base_dispatch(m_base);
}
catch(...)
{
    g_worker_logger->emerg("start worker exception");
    exit(1);
}
}



void connectorServ::masterRestart(struct event_base* base)
{ 
}

void connectorServ::worker_net_init()
{ 
    m_zmq_connect.init();
    m_device_manager.init_bidder();

    m_device_manager.dataConnectToBC(m_zmq_connect);
    
#ifdef TIMELOG
    pthread_t pth;
    pthread_create(&pth, NULL, childProcessGetTime, this);
#endif
}

void connectorServ::sendToWorker(char* subKey, int keyLen, char * buf,int bufLen)
{
    try
    { 
        if(keyLen > PUBLISHKEYLEN_MAX)return;
        int frameLen = bufLen+PUBLISHKEYLEN_MAX;
        char* sendFrame = new char[frameLen];
        memset(sendFrame, 0x00, frameLen);
        memcpy(sendFrame, subKey, keyLen);
        memcpy(sendFrame+PUBLISHKEYLEN_MAX, buf, bufLen);
        zmq_send(m_masterPushHandler, sendFrame, frameLen, ZMQ_NOBLOCK);
        delete[] sendFrame;
    }
    catch(...)
    {
        g_manager_logger->error("sendToWorker exception!");
        throw;
    }
}

int connectorServ::zmq_get_message(void* socket, zmq_msg_t &part, int flags)
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

void connectorServ::recvRequest_callback(int fd, short __, void *pair)
{
    uint32_t events;
    size_t len=sizeof(events);

    connectorServ *serv = (connectorServ*)pair;
    if(serv==NULL) 
    {
        g_manager_logger->emerg("recvRequest_callback param is null");
        return;
    }

    void *handler = serv->m_device_manager.get_throttle_handler(fd);
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_manager_logger->error("get_throttle_handler return NULL:{0:d}", fd);
        return;
    }

    if(events & ZMQ_POLLIN)
    {
        while (1)
        {
           zmq_msg_t subscribe_key_part;
           int sub_key_len = serv->zmq_get_message(handler, subscribe_key_part, ZMQ_NOBLOCK);
           if (sub_key_len == -1 )
           {
               zmq_msg_close (&subscribe_key_part);
               break;
           }
           char *subscribe_key = (char *)zmq_msg_data(&subscribe_key_part);

           zmq_msg_t data_part;
           int data_len = serv->zmq_get_message(handler, data_part, ZMQ_NOBLOCK);
           if ( data_len == -1 )
           {
               zmq_msg_close(&subscribe_key_part);
               zmq_msg_close (&data_part);
               break;
           }
           char *data=(char *)zmq_msg_data(&data_part);
           g_manager_logger->trace("recv request from throttle:{0:d}", data_len);
           if((sub_key_len>0) &&(data_len>0))
           {
               serv->sendToWorker(subscribe_key, sub_key_len, data, data_len);
           }
           zmq_msg_close(&subscribe_key_part);
           zmq_msg_close(&data_part);
        }
    }
}


void connectorServ::register_throttle_response_handler(int fd, short __, void *pair)
{
     uint32_t events;
     size_t len=sizeof(events);
     int recvLen = 0;
     ostringstream os;
     connectorServ *serv = (connectorServ*)pair;
     if(serv==NULL) 
     {
        g_manager_logger->emerg("register_throttle_response_handler param is null");
        exit(1);
     }

      void *handler = serv->m_device_manager.get_throttle_manager_handler(fd);
      if(handler == NULL)
      {
        g_manager_logger->error("register_throttle_response_handler exception");
        return;
      }
      
      int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
      if(rc == -1)
      {
         g_manager_logger->error("subscribe fd:{0:d} error!", fd);
         return;
      }
    
      if(events & ZMQ_POLLIN)
      {
          while (1)
          {      
             zmq_msg_t part;
             recvLen = serv->zmq_get_message(handler, part, ZMQ_NOBLOCK);
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

void *connectorServ::throttle_request_handler(void *arg)
{ 
      connectorServ *serv = (connectorServ*) arg;
      if(!serv) return NULL;
      struct event_base* base = event_base_new(); 
      serv->m_device_manager.subscribe_throttle_event_new(serv->m_zmq_connect, base, recvRequest_callback, arg);
      event_base_dispatch(base);
      return NULL;  
}


void connectorServ::bidderManagerMsg_handler(int fd, short __, void *pair)
{
    uint32_t events;
    size_t len=sizeof(events);
    int recvLen = 0;

    eventArgment *argment = (eventArgment*) pair;
    connectorServ *serv = (connectorServ*)argment->get_serv();
    
    if(serv==NULL) 
    {
        g_manager_logger->emerg("serv is nullptr");
        exit(1);
    }

    void *handler = serv->m_device_manager.get_bidderLoginHandler();
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_manager_logger->error("bidderManagerMsg_handler zmq_getsockopt exception");
        return;
    }

    if(events & ZMQ_POLLIN)
    {
        while (1)
        {
           zmq_msg_t identify_part;
           int identify_len = serv->zmq_get_message(handler, identify_part, ZMQ_NOBLOCK);
           if ( identify_len == -1 )
           {
               zmq_msg_close (&identify_part);
               break;
           }
           char *identify_str = (char*) zmq_msg_data(&identify_part);
           string identify(identify_str, identify_len); 
           zmq_msg_close(&identify_part);
            
           zmq_msg_t part;
           recvLen = serv->zmq_get_message(handler, part, ZMQ_NOBLOCK);
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
                serv->manager_handler(handler, identify, from, type, value, argment->get_base(), pair);           
           }  
           zmq_msg_close(&part);
        }
    }
}


void *connectorServ::bidderManagerHandler(void *arg)
{ 
      connectorServ *serv = (connectorServ*) arg;
      if(!serv) return NULL;
      struct event_base* base = event_base_new(); 
      eventArgment *event_arg = new eventArgment(base, arg);
  
      serv->m_device_manager.subscribe_throttle_event_new(serv->m_zmq_connect, base, recvRequest_callback, arg);
      serv->m_device_manager.connectAll(serv->m_zmq_connect, base, recvRsponse_callback, event_arg);      
      serv->m_device_manager.login_event_new(serv->m_zmq_connect, base, bidderManagerMsg_handler, event_arg);
      event_base_dispatch(base);
      return NULL;  
}

void *connectorServ::sendHeartToBC(void *bidder)
{
    connectorServ *serv = (connectorServ*) bidder;
    sleep(1);
    while(1)
    {
        serv->m_device_manager.loginOrHeartReqToBc(serv->m_config);
        serv->m_device_manager.registerBCToThrottle();
        sleep(serv->m_heartInterval);
    }
}

bool connectorServ::manager_from_BC_handler(const managerProtocol_messageType &type
  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg)
{
    const string& ip = value.ip();
    if(value.port_size() != 2) return false;
    unsigned short mangerPort = value.port(0);
    unsigned short dataPort = value.port(1);
    bool ret = false;
    switch(type)
    {
        case managerProtocol_messageType_LOGIN_REQ:
        {
            //add_bc_to_bidder_request
            g_manager_logger->info("[login request from BC]:{0},{1:d},{2:d}", ip, mangerPort, dataPort);    
            m_device_manager.add_bc(m_zmq_connect, base, recvRsponse_callback, arg, ip, mangerPort, dataPort); 
            rspType = managerProtocol_messageType_LOGIN_RSP;
            ret = true;
            for (auto it = m_workerList.begin(); it != m_workerList.end(); it++)
            {
                BC_process_t *pro = *it;
                if(pro == NULL) continue;
                if(pro->pid == 0) 
                {
                    continue;
                }
                kill(pro->pid, SIGUSR1); 
            }
            break;
        }
        case managerProtocol_messageType_LOGIN_RSP:
        {
            g_manager_logger->info("[add connector response from bc ]:{0},{1:d}", ip, mangerPort);
            m_device_manager.bidderLoginedBcSucess(ip, mangerPort);
            break;
        }
        case managerProtocol_messageType_HEART_RSP:
        {
            g_manager_logger->info("[heart response from bc]:{0},{1:d}", ip, mangerPort);
            m_device_manager.lostheartTimesWithBC_clear(ip, mangerPort);
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

bool connectorServ::manager_from_throttle_handler(const managerProtocol_messageType &type
  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg)
{
    const string& ip = value.ip();
    if(value.port_size() != 2) return false;
    unsigned short mangerPort = value.port(0);
    unsigned short dataPort = value.port(1);
    bool ret = false;
    switch(type)
    {
        case managerProtocol_messageType_LOGIN_REQ:
        {
            //add_throttle_to_bidder_request
            g_manager_logger->info("[login req][throttle -> connector]:{0},{1:d},{2:d}", ip, mangerPort, dataPort);
            m_device_manager.add_throttle(m_zmq_connect, base, register_throttle_response_handler, recvRequest_callback, arg, 
                   ip, mangerPort, dataPort);
            m_device_manager.throttle_init(ip, mangerPort);  
            rspType = managerProtocol_messageType_LOGIN_RSP;
            ret = true;
            break;
        }
        case managerProtocol_messageType_LOGIN_RSP:
        {
            //add_bidder_to_throttle_response
            g_manager_logger->info("[login rsp][throttle -> connector]:{0},{1:d},{2:d}", ip, mangerPort, dataPort);
            m_device_manager.loginToThroSucess(ip, mangerPort);
            break;
        }
        case managerProtocol_messageType_HEART_REQ:
        {
            //throttle_heart_to_bidder_requset:
            g_manager_logger->info("[heart req][throttle -> connector]:{0}, {1:d}", ip, mangerPort);
            ret = m_device_manager.recv_heart_from_throttle(ip, mangerPort);
            rspType = managerProtocol_messageType_HEART_RSP;
            break;
        }
        case managerProtocol_messageType_REGISTER_RSP:
        {
            //bidder_register_to_throttle_response
            const string& key = value.key();
            g_manager_logger->info("[register response][throttle -> connector]:{0},{1:d},{2:d}", ip, mangerPort, dataPort);
            m_device_manager.set_publishKey_registed(ip, mangerPort, key);
            break;
        }
        default:
        {
            g_manager_logger->info("managerProtocol_messageType from throttle exception {0:d}",(int)  type);
            break;
        }
    }
    
    return ret;
}

bool connectorServ::manager_handler(const managerProtocol_messageTrans &from
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
        case managerProtocol_messageTrans_THROTTLE:
        {
            ret = manager_from_throttle_handler(type, value, rspType, base, arg);
            break;
        }
        default:
        {
            g_manager_logger->info("managerProtocol_messageFrom exception {0:d}", (int)from);
            break;
        }
    }
    return ret;
}

bool connectorServ::manager_handler(void *handler, string& identify,  const managerProtocol_messageTrans &from
            ,const managerProtocol_messageType &type, const managerProtocol_messageValue &value,struct event_base * base, void * arg)
{
    managerProtocol_messageType rspType;
    bool ret = manager_handler(from, type, value, rspType, base, arg);
    if(ret)
    {
        const connectorConfig& configure = m_device_manager.get_connector_config();
        managerProPackage::send_response(handler, identify, managerProtocol_messageTrans_CONNECTOR, rspType
            , configure.get_connectorIP(), configure.get_connectorManagerPort());
    }
    return ret;
}

void connectorServ::recvRsponse_callback(int fd, short event, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len;
    eventArgment *event_arg = (eventArgment*) pair;

    connectorServ *serv = (connectorServ*) event_arg->get_serv();
    if(serv==NULL) 
    {
        g_manager_logger->emerg("serv is nullptr");
        exit(1);
    }

    len = sizeof(events);
    void *adrsp = serv->m_device_manager.get_bcManager_handler(fd);
    if(adrsp==NULL)
    {
        g_manager_logger->error("get_bcManager_handler return null");
        return;
    }
    int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_manager_logger->info("adrsp is invalid");
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
                const managerProtocol_messageTrans &from  =  manager_pro.messagefrom();
                const managerProtocol_messageType &type   =  manager_pro.messagetype();
                const managerProtocol_messageValue &value =  manager_pro.messagevalue();
                managerProtocol_messageType responseType;
                serv->manager_handler(from, type, value, responseType);
            }
            zmq_msg_close(&part);
        }
    }
}


bool connectorServ::masterRun()
{
      int *ret;
      pthread_t pth1;   
      pthread_t pth2; 
      pthread_t pth3;       
      pthread_create(&pth1, NULL, bidderManagerHandler, this); 
      pthread_create(&pth2, NULL, sendHeartToBC, this); 
      pthread_create(&pth3, NULL, getTime, this);
	  return true;
}


/*all right reserved
  *auth: yanjun.xiang
  *date:2014-7-22
  *function name: run
  *return void
  *param: void 
  *function: start the bidder
  */
  
void connectorServ::run()
{
    int num_children = 0;//the number of children process
    int restart_finished = 1;//restart finish symbol
    int is_child = 0;//child 1, father 0    
    bool master_started = false;
   // register signal handler

    struct sigaction act;
    bzero(&act, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_handler = signal_handler;//The signal processing function
               
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
                g_manager_logger->info("chil process exit:{0}", pid); 
                updataWorkerList(pid);
            }

            if (srv_graceful_end || srv_ungraceful_end)//CTRL+C ,killall throttle
            {
                g_manager_logger->info("srv_graceful_end"); 
                if (num_children == 0)
                {
                    break;    //get out of while(true)
                }

                auto it = m_workerList.begin();
                for (it = m_workerList.begin(); it != m_workerList.end(); it++)
                {
                    BC_process_t *pro = *it;
                    if(pro&&pro->pid != 0)
                    {
                        kill(pro->pid, srv_graceful_end ? SIGINT : SIGTERM);
                    }
                }
                continue;    //this is necessary.
            }

      
            if (srv_restart)//all child worker restart, restart one by one
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

            if (!restart_finished) //not finish , restart child process one by one
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
            
                // all child progress restart over
                if (it == m_workerList.end())
                {
                    restart_finished = 1;
                } 
                else
                {
                    close(pro->channel[0]);
                    kill(pro->pid, SIGHUP);
                }
            }

            if(is_child==0 &&sigusr1_recved)//update confiure dynamic
            {    
                g_manager_logger->info("master progress recv sigusr1");    
                readConfigFile();//read configur file
                auto count = 0;
                auto it = m_workerList.begin();
                for (it = m_workerList.begin(); it != m_workerList.end(); it++, count++)
                {
                      BC_process_t *pro = *it;
                      if(pro == NULL) continue;
                      if(pro->pid == 0) 
                      {
                          continue;
                      }
               
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
                        m_worker_info_lock.write_lock();
				        m_workerList.push_back(pro); 
                        m_worker_info_lock.read_write_unlock();
                    }
                }    
            }
 
        }

        if(is_child==0 &&sigusr1_recved)//fathrer process and recv sig usr1
        {
            sigusr1_recved = 0;
        }

        int workIndex = 1;

        auto it = m_workerList.begin();
        for (it = m_workerList.begin(); it != m_workerList.end(); it++)
        {
            workIndex++;
            BC_process_t *pro = *it;
            if(pro == NULL) continue;
            if (pro->status == PRO_INIT)
            {
                if (socketpair(AF_UNIX, SOCK_STREAM, 0, pro->channel) == -1)
                {
                    return;
                }

                pid = fork();

                switch (pid)
                {
                    case -1:
                    {
                        g_manager_logger->error("error in fork");
                    }
                    break;
                    case 0:
                    {   
                        close(pro->channel[0]);
                        is_child = 1; 
                        pro->pid = getpid();
                    }
                    break;
                    default:
                    {
                        close(pro->channel[1]);
                        int flags = fcntl(pro->channel[0], F_GETFL);
                        fcntl(pro->channel[0], F_SETFL, flags | O_NONBLOCK); 
                        pro->pid = pid;
                        ++num_children;
                    }
                    break;
                }
                pro->status = PRO_BUSY;
                if (!pid) break; 
            }
        }

        
        if(is_child==0 && master_started==false)
        {
             //master serv start    
             master_started = true; 
             masterRun(); 
        }

        if (!pid) break; 
    }

    if (!is_child)    
    {
        g_manager_logger->emerg("master exit:{0:d}", getpid());
        exit(0);    
    }
    start_worker();
}

