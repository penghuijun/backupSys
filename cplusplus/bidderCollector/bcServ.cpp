/*
  *Copyright (c) 2014, fxxc
  *All right reserved.
  *
  *filename: bcserv.cpp
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
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <time.h>
#include <set>
#include <map>
#include <iterator>

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <event.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include "zmq.h"
#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"
#include "AdBidderResponseTemplate.pb.h"
#include "expireddata.pb.h"
#include "bcServ.h"
#include "threadpoolmanager.h"
#include "redisPool.h"
#include "redisPoolManager.h"
#include "MobileAdRequest.pb.h"
#include "MobileAdResponse.pb.h"

using namespace com::rj::protos::msg;
using namespace com::rj::protos::mobile;
using namespace com::rj::protos;
using namespace com::rj::protos::manager;
using namespace std;

sig_atomic_t srv_graceful_end = 0;
sig_atomic_t srv_ungraceful_end = 0;
sig_atomic_t srv_restart = 0;
sig_atomic_t sigalrm_recved = 0;;
sig_atomic_t sigusr1_recved = 0;;
extern shared_ptr<spdlog::logger> g_file_logger;
extern shared_ptr<spdlog::logger> g_manager_logger;


//get micro time
mircotime_t bcServ::get_microtime()
{
    m_tmMutex.lock();
    mircotime_t t = m_microSecTime.tv_sec*1000+m_microSecTime.tv_usec/1000;
    m_tmMutex.unlock();
    return t;
}

//update time
void *bcServ::getTime(void *arg)
{
    bcServ *serv = (bcServ *)arg;
    int times = 0;
    int getTimeInterval=1*1000;
    while(1)
    {
        usleep(getTimeInterval);
        times++;
        serv->update_microtime();
        serv->check_data_record(); 
        if((times*getTimeInterval)>(serv->m_heart_interval*1000*1000))
        {
            times = 0;
            if(serv->m_bc_manager.devDrop())
            {
                serv->m_config.readConfig();
                serv->m_bc_manager.update_dev(serv->m_config.get_throttle_info(), serv->m_config.get_bidder_info());
            }
            
        }
    }
}

bcServ::bcServ(configureObject &config):m_config(config)
{
    try
    {
        int logLevel = m_config.get_bcLogLevel();
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
        g_manager_logger->emerg("log level:{0:d}", (int)m_logLevel);


        m_zmq_connect.init();
        m_bcDataMutex.init();
        m_tmMutex.init();
        handler_fd_lock.init();
        m_workerList_lock.init();
        m_getTimeofDayMutex.init();

        m_vastBusiCode = m_config.get_vast_businessCode();
        m_mobileBusiCode = m_config.get_mobile_businessCode();
        m_heart_interval = m_config.get_heart_interval();
        m_logRedisOn = m_config.get_logRedisOn();
        m_logRedisIP  = m_config.get_logRedisIP();
        m_logRedisPort = m_config.get_logRedisPort();
        m_bc_manager.init(m_zmq_connect, m_config.get_throttle_info(), m_config.get_bidder_info(), m_config.get_bc_info());
        const int tastPoolSize = 10000;
		int threadPoolSize = m_bc_manager.get_bc_config().get_bcThreadPoolSize();
		m_redisPoolManager.connectorPool_init(m_bc_manager.get_redis_ip(), m_bc_manager.get_redis_port(), threadPoolSize+10);
        if(m_logRedisIP.empty() == false)
        {
            m_redisLogManager.connectorPool_init(m_logRedisIP, m_logRedisPort, threadPoolSize+10);
        }
        m_threadPoolManger.Init(tastPoolSize, threadPoolSize, threadPoolSize);
    }
    catch(...)
    {   
        cerr <<"bidderServ structure error"<<endl;
        exit(1);
    }
}

void bcServ::calSpeed()
{
    static long long test_begTime;
    static long long test_endTime;
    static int test_count = 0;
    m_getTimeofDayMutex.lock();
    test_count++;
    const int intervalCnt = 1000;
    if(test_count%intervalCnt==1)
    {
        struct timeval btime;
        gettimeofday(&btime, NULL);
        test_begTime = btime.tv_sec*1000+btime.tv_usec/1000;
        m_getTimeofDayMutex.unlock();
    }
    else if(test_count%intervalCnt==0)
    {
        int count = test_count;
        struct timeval etime;
        gettimeofday(&etime, NULL);
        test_endTime = etime.tv_sec*1000+etime.tv_usec/1000;
        long long diff = test_endTime-test_begTime;
        int speed = 0;
        if(diff) speed = intervalCnt*1000/diff;
        m_getTimeofDayMutex.unlock();
        g_file_logger->warn("send requst num, const time, speed:{0:d}, {1:d}, {2:d}", count, diff, speed);
    }
    else
    {
        m_getTimeofDayMutex.unlock();
    }
}


//change log level
bool bcServ::logLevelChange()
{
     int logLevel = m_config.get_bcLogLevel();
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

void bcServ::usr1SigHandler(int fd, short event, void *arg)
{
    g_manager_logger->info("signal SIGUSR1");
    bcServ *serv = (bcServ*) arg;
    if(serv == NULL) return;
    if(serv->logLevelChange())
    {
         g_file_logger->set_level(serv->m_logLevel);
         g_manager_logger->set_level(serv->m_logLevel); 
    }
    
}

//check data list , if time out ,erase 
void bcServ::check_data_record()
{
    int lost=0;
    static int lostnum=0;

    mircotime_t micro = get_microtime();
    m_bcDataMutex.lock();
    for(auto bcDataIt = m_bcDataRecList.begin();bcDataIt != m_bcDataRecList.end(); )
    {  
        bcDataRec* bcData = bcDataIt->second;  
        if(bcData == NULL)
        {
            m_bcDataRecList.erase(bcDataIt++);
            continue;
        }

        mircotime_t overTime = bcData->get_overtime();
        mircotime_t tm= bcData->get_microtime();
        mircotime_t diff = micro - tm;
        if(diff>=overTime)//if have overtime, then user overtime, if not use default overtime
        {            
            delete bcData;  
            m_bcDataRecList.erase(bcDataIt++);
        }
        else
        {
            bcDataIt++;   
        }
    }     
    m_bcDataMutex.unlock();
}


//erase dislike campaign and creative ,and hset the result
void bcServ::worker_thread_func(void *arg)
{
    adResponseValue *response_value = (adResponseValue*) arg;
    if(response_value)
    { 
       bcServ *serv = (bcServ *)response_value->get_serv(); 
       if(serv)
       {
            CommonMessage commesg;
            commesg.ParseFromArray(response_value->get_data(), response_value->get_dataSize());  
            const string &dataStr = commesg.data();
            MobileAdResponse mobile_response;
            mobile_response.ParseFromString(dataStr);
            response_value->get_insightManager();
            adInsightManager& insight_manager = response_value->get_insightManager();
            insight_manager.eraseDislikeAd(mobile_response);//erase dislike campaign ,creative

            int responseSizze = mobile_response.ByteSize();
            char *responseDaata = new char[responseSizze];
            mobile_response.SerializeToArray(responseDaata, responseSizze);  

            commesg.clear_data();
            commesg.set_data(responseDaata, responseSizze);
            responseSizze = commesg.ByteSize();
            char* comMessBuf = new char[responseSizze];
            commesg.SerializeToArray(comMessBuf, responseSizze);

            //hset the ad response to redis
            if(serv->m_redisPoolManager.redis_hset_request(response_value->get_uuid().c_str(), response_value->get_field().c_str(),
                comMessBuf, responseSizze, serv->m_bc_manager.get_redis_timeout())==true)
            {
                g_file_logger->debug("hset success:{0}", response_value->get_uuid());
                serv->calSpeed();
                if(serv->m_logRedisOn)
                {
                    ostringstream os;
                    os<<response_value->get_uuid()<<"_"<<serv->get_microtime()<<"_bc*recv";
                    string logValue = os.str();
                    serv->m_redisLogManager.redis_lpush("flow_log", logValue.c_str(), logValue.size());
                }
            }
            else
            {
                g_file_logger->error("worker_thread_func exception");
            }
            delete[] responseDaata;
            delete[] comMessBuf;
       }
    }
    delete response_value;
}

void *bcServ::register_update_config_event()
{
    struct event_base* base = event_base_new();    
    struct event * usr1_event = evsignal_new(base, SIGUSR1, usr1SigHandler, this);
    evsignal_add(usr1_event, NULL);
    event_base_dispatch(base);    
}

void bcServ::handler_recv_buffer(adInsightManager* insightManager, stringBuffer *string_buf, string &uuid, string &hash_field)
{
     time_t timep;
     time(&timep);
     
     adResponseValue *arg = new adResponseValue(insightManager, uuid, hash_field, this, string_buf);
     if(m_threadPoolManger.Run(worker_thread_func, (void *) arg)!=0)
     {
          g_file_logger->error("lost some worker_thread_func");
          delete arg;
          return ;
     }
}


//recv bidder data handler
void bcServ::bidderHandler(char * pbuf,int size)
{
    ostringstream os;
    CommonMessage commMsg;
    string uuid;
    string bidID;
    string sockID;
    string campaignId;

    commMsg.ParseFromArray(pbuf, size);  
    const string &busiCode=commMsg.businesscode();
    const string &ttl=commMsg.ttl();
    uint32_t overTime = atoi(ttl.c_str());;
    const string &rsp = commMsg.data();   
    const uint32_t overTime_max=1000;
    const uint32_t overTime_min=100;
    if((overTime>overTime_max)||(overTime<overTime_min)) overTime=overTime_max;

    if(busiCode==m_vastBusiCode)
    {
        BidderResponse bidRsp;
        bidRsp.ParseFromString(rsp);
        uuid = bidRsp.id();
        bidID = bidRsp.bidderid();
    }
    else if(busiCode==m_mobileBusiCode)
    {
        MobileAdResponse mobile_rsp;
        mobile_rsp.ParseFromString(rsp);
        uuid=mobile_rsp.id();
        bidID=mobile_rsp.bidid(); 
        MobileAdResponse_mobileBid mobileBid;
        mobileBid=mobile_rsp.bidcontent(0);
        campaignId = mobileBid.campaignid();
    }
    else
    {
        g_file_logger->error("recv bidder invalid business code:{0}", busiCode);
        return;
    }
    if(uuid.empty())
    {
        g_file_logger->error("invalid uuid");
        return;
    }
    g_file_logger->debug("recv bidder uuid:{0}", uuid);
 
    mircotime_t recvBidTime = get_microtime();
    os.str("");
    static int idses = 0;
    idses++;
    os<<idses<<":"<<"<"<<recvBidTime<<">";
    string bidIDandTime = os.str();
    m_bcDataMutex.lock();
    auto bcDataIt = m_bcDataRecList.find(uuid);
    if(bcDataIt != m_bcDataRecList.end())//finu uuid
    {
         bcDataRec *bcData = bcDataIt->second;
         if(bcData)
         {  
              if(bcData->recv_throttle_info())//recv throttle
              {  
                   auto bidder_buf = bcData->get_buf_list();
                   m_bcDataMutex.unlock();
                   stringBuffer *string_buf = new stringBuffer(pbuf, size);
                   handler_recv_buffer(bcData->get_adInsightManager(), string_buf, uuid, bidIDandTime);

                   if(bidder_buf)//can not recv there,
                   {
                        g_file_logger->debug("more than one bidder response for a request");
                        for(auto it = bidder_buf->begin(); it != bidder_buf->end(); it++)
                        {
                            bufferStructure *buf_stru= *it;
                            if(buf_stru)
                            {
                                stringBuffer *str_buf = buf_stru->stringBuf_pop();
                                if(str_buf)
                                {
                                    handler_recv_buffer(bcData->get_adInsightManager(), str_buf
                                                , uuid, buf_stru->get_hash_field());
                                }
                            }
                        }
                        delete bidder_buf;
                   }
               }
               else//have not recv throttle, so record the bidder response data;
               {
                  g_file_logger->debug("have not recv throttle!");
                  if(bcData)
                  {
                       bcData->add_bidder_buf(pbuf, size, bidIDandTime);
                  }
                  m_bcDataMutex.unlock();
               }
         
          }
          else//invalid bcData ,assert , throw error
          {
              g_file_logger->debug("invalid bcData !");
              m_bcDataRecList.erase(bcDataIt);
              m_bcDataMutex.unlock();
          }
     }
     else// can not find uuid,represent throttle info have not recved
     {
           g_file_logger->debug("DataRecList can not find uuid:{0} && campaignId:{1}",uuid,campaignId);
           bcDataRec *bData = new bcDataRec(recvBidTime, overTime, false);
           bData->add_bidder_buf(pbuf, size, bidIDandTime);
           m_bcDataRecList.insert(pair<string, bcDataRec*>(uuid, bData)); 
           m_bcDataMutex.unlock();          
     }   
}
/*
  *subscribte throttle request
  */
void bcServ::subVastHandler(char * pbuf,int size)
{
    CommonMessage commMsg;
    uint32_t overTime = 100;
    const uint32_t overTime_max=1000;
    const uint32_t overTime_min=100;

    commMsg.ParseFromArray(pbuf, size);
    const string& busiCode = commMsg.businesscode();
    const string& data = commMsg.data();
    const string& ttl = commMsg.ttl();
    overTime = atoi(ttl.c_str());;
    if((overTime>overTime_max)||(overTime<overTime_min)) overTime=overTime_max;

    adInsightManager *adInsight_manager = new adInsightManager;
    string uuid;
    if(busiCode==m_vastBusiCode)//business code can configure by configure file
    {
        VastRequest vastReq;
        vastReq.ParseFromString(data);
        uuid = vastReq.id();

    }
    else if(busiCode==m_mobileBusiCode)
    {
        MobileAdRequest mobileReq;  
        mobileReq.ParseFromString(data);
        uuid = mobileReq.id();
        int req_adInsightSize = mobileReq.adinsight_size();
        for(auto i = 0; i < req_adInsightSize; i++)
        {
            MobileAdRequest_AdInsight req_adInsight = mobileReq.adinsight(i);
            string insight_pro = req_adInsight.property();
            int ids_size = req_adInsight.ids_size();
            if(ids_size == 0) continue;
            vector<string> stringList;
            for(auto j = 0; j < ids_size; j++)
            {
                string ids = req_adInsight.ids(j);
                stringList.push_back(ids);
            }
            adInsight_manager->add_insight(insight_pro, stringList);
        }     
    }
    else
    {
        g_file_logger->error("recv throttle invalid business code:{0:d}", busiCode);
        return;
    }

    if(uuid.empty())
    {
        g_file_logger->error("recv throttle invalid uuid");
        return;
    }
    g_file_logger->debug("recv throttle:{0}", uuid);
    m_bcDataMutex.lock();
    auto bcDataIt = m_bcDataRecList.find(uuid);//find uuid, this is represent, bidder is come first, or two same uuid request come from throttle in overtime
    if(bcDataIt != m_bcDataRecList.end())//
    {
        bcDataRec *bcData = bcDataIt->second;
        if(bcData)
        {
            if(bcData->recv_throttle_info()== false)// the first recv throttle info, then....
            {
                bcData->recv_throttle_complete();//change recv throtlle symbol
                auto bidder_buf = bcData->get_buf_list();//get bidder response list,
                bcData->add_insightManager(adInsight_manager);
                m_bcDataMutex.unlock();
                if(bidder_buf)//if bidder response first come, bidder_buf is not null, then haddler the recv bidder info;
                {
                     for(auto it = bidder_buf->begin(); it != bidder_buf->end(); it++)//bidder buf list ,record buf info and field, use in redis hash hset command
                     {
                         bufferStructure *buf_stru= *it;
                         if(buf_stru)
                         {
                             handler_recv_buffer(adInsight_manager, buf_stru->stringBuf_pop(), uuid, buf_stru->get_hash_field());//handler bidder info
                         }
                     }
                     delete bidder_buf;
                 }
             }
             else  //recv same uuid throttle, drop it
             {
                 m_bcDataMutex.unlock();     
             }
         }
         else// NULL data
         {
             m_bcDataRecList.erase(bcDataIt);
             m_bcDataMutex.unlock();
         }
     }
     else// uuid request come first
     {
        mircotime_t recvSubTime = get_microtime();  //record recv time , then wo know delete the infomation when
        bcDataRec *bData = new bcDataRec(recvSubTime, overTime, true);//timep tell we when delete redis record, overtime, tell we free data info at expired
        m_bcDataRecList.insert(pair<string, bcDataRec*>(uuid, bData));
        bData->add_insightManager(adInsight_manager);
        m_bcDataMutex.unlock();
     }
}

//subscribe throttle callback
void bcServ::recvRequest_callback(int fd, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len=sizeof(int);

    bcServ *serv = (bcServ*)pair;
    if(serv==NULL) 
    {
        g_file_logger->error("recvRequest_callback param error");
        return;
    }

    len = sizeof(events);
    void *handler = serv->m_bc_manager.get_throttle_handler(fd);
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_file_logger->error("recvRequest_callback get event exception");
        return;
    }

    if(events & ZMQ_POLLIN)
    {
        while (1)
        {
           zmq_msg_t first_part;
           int recvLen = serv->zmq_get_message(handler, first_part, ZMQ_NOBLOCK);
           if ( recvLen == -1 )
           {
               zmq_msg_close (&first_part);
               break;
           }
           zmq_msg_close(&first_part);

           zmq_msg_t part;
           recvLen = serv->zmq_get_message(handler, part, ZMQ_NOBLOCK);
           if ( recvLen == -1 )
           {
               zmq_msg_close (&part);
               break;
           }
           char *msg_data=(char *)zmq_msg_data(&part);
            if(recvLen)
            {
                serv->subVastHandler(msg_data, recvLen);
            }
            zmq_msg_close(&part);
        }
    }
}

void *bcServ::throttle_request_handler(void *arg)
{ 
    bcServ *serv = (bcServ*) arg;
    struct event_base* base = event_base_new(); 
    serv->m_bc_manager.startSubThrottle(serv->m_zmq_connect, base, recvRequest_callback, arg);
    event_base_dispatch(base);
    return NULL;  
}

int bcServ::zmq_get_message(void* socket, zmq_msg_t &part, int flags)
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

//recv bidder callback
void bcServ::recvBidder_callback(int fd, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len=sizeof(int);

    bcServ *serv = (bcServ*)pair;
    if(serv==NULL) 
    {
        g_manager_logger->error("recvBidder_callback param error");
        return;
    }

    len = sizeof(events);
    void *handler = serv->m_bc_manager.get_adRspHandler();
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_manager_logger->error("recvBidder_callback get event exception");        
        return;
    }
   
    if(events & ZMQ_POLLIN)
    {
        while (1)
        {
           zmq_msg_t first_part;
           int recvLen = serv->zmq_get_message(handler, first_part, ZMQ_NOBLOCK);
           if ( recvLen == -1 )
           {
               zmq_msg_close (&first_part);
               break;
           }
           char *id_str=(char *)zmq_msg_data(&first_part);
           string identify(id_str, recvLen);
           g_file_logger->debug("[BidRsponse identify]: {0}", identify);
           zmq_msg_close(&first_part);

           zmq_msg_t part;
           recvLen = serv->zmq_get_message(handler, part, ZMQ_NOBLOCK);
           if ( recvLen == -1 )
           {
               zmq_msg_close (&part);
               break;
           }
           char *msg_data=(char *)zmq_msg_data(&part);
            if(recvLen)
            {
                serv->bidderHandler(msg_data, recvLen);
            }
            
            zmq_msg_close(&part);
        }
    }
}


//handler manager info from throttle
bool bcServ::managerProto_from_throttle_handler(const managerProtocol_messageType &type
  , const managerProtocol_messageValue &value,  managerProtocol_messageType &rspType, struct event_base * base, void * arg)
{
    const string& ip = value.ip();
    if(value.port_size() <= 1) return false;
    unsigned short managerPort = value.port(0);
    unsigned short dataPort = value.port(1);
    bool ret = false;
    switch(type)
    {
        case managerProtocol_messageType_LOGIN_RSP:
        {
            //add_bc_to_throttle_response:
             g_manager_logger->info("[login rsp][bc <- throttle]:{0},{1:d}", ip, managerPort);
             m_bc_manager.loginThrottleSuccess(ip, managerPort);
            break;
        }
        case managerProtocol_messageType_LOGIN_REQ:
        {
            g_manager_logger->info("[login req][bc <- throttle]:{0},{1:d},{2:d}", ip, managerPort, dataPort);
            m_bc_manager.add_throttle(ip, managerPort, dataPort, m_zmq_connect, base, recvRequest_callback, arg);
            rspType = managerProtocol_messageType_LOGIN_RSP;
            ret = true;
            break;
        }
        case managerProtocol_messageType_HEART_REQ:
        {
            //throttle_heart_to_bc_request:
            g_manager_logger->info("[heart req][bc <- throttle]:{0},{1:d}", ip, managerPort);
            ret = m_bc_manager.recv_heartBeat(managerProtocol_messageTrans_THROTTLE, ip, managerPort);
            rspType = managerProtocol_messageType_HEART_RSP;
            break;
        }
        default:
        {
            g_manager_logger->info("##manager protocol type from throttle is ivalid:{0:d}", (int)type);            
            break;
        }
    }
    
    return ret;
}

//handler manager info from connector
bool bcServ::managerProto_from_connector_handler(const managerProtocol_messageType &type
  , const managerProtocol_messageValue &value,  managerProtocol_messageType &rspType, struct event_base * base, void * arg)
{
    const string& ip = value.ip();
    if(value.port_size() == 0) return false;
    unsigned short port = value.port(0);
    bool ret = false;
    switch(type)
    {
        case managerProtocol_messageType_LOGIN_RSP:
        {
            g_manager_logger->info("[login rsp][bc <- connector]:{0},{1:d}", ip, port);
            m_bc_manager.register_sucess(false, ip, port);
            break;
        }
        case managerProtocol_messageType_LOGIN_REQ:
        {
            g_manager_logger->info("[login req][bc <- connector]:{0},{1:d}", ip, port);
            m_bc_manager.add_connector(ip, port, m_zmq_connect, base, recvRegisterRsp_callback, arg);
            rspType = managerProtocol_messageType_LOGIN_RSP;
            ret = true;
            break;
        }
        case managerProtocol_messageType_HEART_REQ:
        {
            g_manager_logger->info("[heart req][bc <- connector]:{0},{1:d}", ip, port);
            ret = m_bc_manager.recv_heartBeat(managerProtocol_messageTrans_CONNECTOR, ip, port);
            rspType = managerProtocol_messageType_HEART_RSP;
            break;
        }
        default:
        {
            g_manager_logger->info("##manager protocol type from throttle is ivalid:{0:d}", (int)type);    
            break;
        }
    }
    return ret;
}

/*name:managerProto_from_bidder_handler
 *function: handler msg from bidder
 *return: if need response to request device ,return true , otherwise false
 */
bool bcServ::managerProto_from_bidder_handler(const managerProtocol_messageType &type
  , const managerProtocol_messageValue &value,  managerProtocol_messageType &rspType, struct event_base * base, void * arg)
{
    const string& ip = value.ip();
    if(value.port_size() == 0) return false;
    unsigned short port = value.port(0);
    bool ret = false;
    switch(type)
    {
        case managerProtocol_messageType_LOGIN_RSP:
        {
            //add_bc_to_bidder_response:
            g_manager_logger->info("[login rsp][bc <- bidder]:{0},{1:d}",ip, port);
            m_bc_manager.register_sucess(true, ip, port);
            break;
        }
        case managerProtocol_messageType_LOGIN_REQ:
        { 
            //add_bidder_to_bc_request:
            g_manager_logger->info("[login req][bc <- bidder]:{0},{1:d}", ip, port);
            ret = m_bc_manager.add_bidder(ip, port, m_zmq_connect, base, recvRegisterRsp_callback, arg);
            rspType = managerProtocol_messageType_LOGIN_RSP;
            break;
        }
        case managerProtocol_messageType_HEART_REQ:
        {
            //bidder_heart_to_bc_request
            g_manager_logger->info("[heart req][bc <- bidder]:{0},{1:d}", ip, port);
            ret = m_bc_manager.recv_heartBeat(managerProtocol_messageTrans_BIDDER, ip, port);
            rspType = managerProtocol_messageType_HEART_RSP;
            break;
        }
        default:
        {
            g_manager_logger->info("##manager protocol type from throttle is ivalid:{0:d}", (int)type);    
            break;
        }
    }
    return ret;
}

 /*name:manager_handler
   *function:handler msg from other type device
   *return : if need response to request device ,return true , otherwise false
   */
bool bcServ::manager_handler(const managerProtocol_messageTrans &from, const managerProtocol_messageType &type
         , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType,struct event_base * base, void * arg)
{

    bool ret = false;
    switch(from)
    {
        case managerProtocol_messageTrans_THROTTLE:
        {
            //msg from throttle
            ret = managerProto_from_throttle_handler(type, value, rspType, base, arg);
            break;
        }
        case managerProtocol_messageTrans_CONNECTOR:
        {
            //msg from connector
            ret = managerProto_from_connector_handler(type, value, rspType, base, arg);
            break;
        }
        case managerProtocol_messageTrans_BIDDER:
        {
            //msg from bidder
            ret = managerProto_from_bidder_handler(type, value, rspType, base, arg);
            break;
        }
        default:
        {
            //invalid msg
            g_manager_logger->info("###manager_handler from is ivalid:{0:d}", (int)from);  
            break;
        }
    }
    return ret;
}


/*
  *name:manager_handler
  *function:handler msg from other type device, if need ,response to request device
  */
void bcServ::manager_handler(void *handler, string& identify, const managerProtocol_messageTrans &from, const managerProtocol_messageType &type
         , const managerProtocol_messageValue &value, struct event_base * base, void * arg)
{

    bool ret = false;
    managerProtocol_messageType rspType;
    ret = manager_handler(from, type, value, rspType, base,  arg);
    if(ret)
    {
        //need response to request device
       // g_manager_logger->trace("identify:{0}, from:{1:d}, type:{2:d}",identify, (int)from, (int) rspType);
        bcConfig& bcc = m_bc_manager.get_bc_config();
        managerProPackage::send_response(handler, identify, managerProtocol_messageTrans_BC, rspType, bcc.get_bcIP()
            ,bcc.get_bcMangerPort(), bcc.get_bcDataPort());
    }
}
void bcServ::recvRegisterRsp_callback(int fd, short __, void *pair)
{
    ostringstream os;
    zmq_msg_t msg;
    uint32_t events;
    size_t len=sizeof(int);

    bcServ *serv = (bcServ*)pair;
    if(serv==NULL) 
    {
        g_manager_logger->error("recvRegisterRsp_callback param error");
        return;
    }

    len = sizeof(events);
    void *handler = serv->m_bc_manager.get_bidderManagerHandler(fd);
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_manager_logger->error("recvRegisterRsp_callback get event exception");
        return;
    }

    if(events & ZMQ_POLLIN)
    {
        while (1)
        {
           zmq_msg_t first_part;
           int recvLen = serv->zmq_get_message(handler, first_part, ZMQ_NOBLOCK);
           if ( recvLen == -1 )
           {
               zmq_msg_close (&first_part);
               break;
           }
           char *msg_data=(char *)zmq_msg_data(&first_part);
           if(msg_data)
           {
               managerProtocol manager_pro;
               manager_pro.ParseFromArray(msg_data, recvLen);  
               const managerProtocol_messageTrans &from = manager_pro.messagefrom();
               const managerProtocol_messageType &type = manager_pro.messagetype();
               const managerProtocol_messageValue &value = manager_pro.messagevalue();
               string id;
               serv->manager_handler(handler, id, from, type, value); 
           }
           zmq_msg_close(&first_part);
        }
    }
}


//bc manager event handler
//recv message request from other type device
//pair include event base and server point
void bcServ::managerEventHandler(int fd, short __, void *pair)
{
    try{
        zmq_msg_t msg;
        uint32_t events;
        size_t len=sizeof(int);
        eventArgment *argument = (eventArgment*) pair;
        if(argument == NULL) throw "managerEventHandler argment exception";
        
        bcServ *serv = (bcServ*)argument->get_serv();
        if(serv==NULL)  throw "managerEventHandler argment serv param exception";
        
        len = sizeof(events);
        void *handler = serv->m_bc_manager.get_bcManagerHandler();
        int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
        if(rc == -1) throw string("managerEventHandler get event exception");

        if(events & ZMQ_POLLIN)
        {
            while (1)
            {
               zmq_msg_t id_part;
               int id_len = serv->zmq_get_message(handler, id_part, ZMQ_NOBLOCK);
               if ( id_len == -1 )
               {
                   zmq_msg_close (&id_part);
                   break;
               }
               char *id_str=(char *)zmq_msg_data(&id_part);
               string identify(id_str, id_len);
               g_manager_logger->info("[Manager identify]: {0}", identify);
               zmq_msg_close(&id_part);
 
               zmq_msg_t data_part;
               int dataLen = serv->zmq_get_message(handler, data_part, ZMQ_NOBLOCK);
               if ( dataLen == -1 )
               {
                   zmq_msg_close (&data_part);
                   break;
               }
               char *data=(char *)zmq_msg_data(&data_part);
               if(data)
               {
                    managerProtocol manager_pro;
                    manager_pro.ParseFromArray(data, dataLen);  
                    const managerProtocol_messageTrans &from = manager_pro.messagefrom();
                    const managerProtocol_messageType &type = manager_pro.messagetype();
                    const managerProtocol_messageValue &value = manager_pro.messagevalue();
                    managerProtocol manager_rsp_pro;
                    serv->manager_handler(handler, identify, from, type, value, argument->get_base(), (void*)serv); 
               }
               zmq_msg_close(&data_part);
            }
        }
    }
    catch(const string& msg)
    {
        g_manager_logger->error(msg);
    }
    catch(const char* msg)
    {
        g_manager_logger->error(msg);
        exit(1);
    }    
    catch(...)
    {
        g_manager_logger->error("managerEventHandler unknow expception");
        exit(1);
    }
}

void *bcServ::bc_manager_handler(void *arg)
{ 
      bcServ *serv = (bcServ*) arg;
      struct event_base* base = event_base_new(); 
      eventArgment *argment = new eventArgment(base, arg);
      
      serv->m_bc_manager.startSubThrottle(serv->m_zmq_connect, base, recvRequest_callback, arg);//SUBSCRIBE THROTTLE
      serv->m_bc_manager.AdRsp_event_new(base, recvBidder_callback, arg);
      serv->m_bc_manager.manager_event_new(base, managerEventHandler, argment);
      serv->m_bc_manager.connectAll(serv->m_zmq_connect, base, recvRegisterRsp_callback, arg);
      event_base_dispatch(base);
      return NULL;  
}

void *bcServ::brocastLogin(void *arg)
{
    bcServ *serv = (bcServ *)arg;
    if(!serv) return NULL;
    sleep(1);
    while(1)
    {
        serv->m_bc_manager.login();
        sleep(serv->m_heart_interval);
    }
}

bool bcServ::masterRun()
{
      int *ret;
      pthread_t pth1; 
      pthread_t pth2; 
      pthread_t pth3; 

      pthread_create(&pth1, NULL, bc_manager_handler, this); //handler every manager informatin from other dev, include heart with bidder
      pthread_create(&pth2, NULL, brocastLogin, this); //register BC itself to bidder
      pthread_create(&pth3, NULL, getTime, this); 
	  return true;
}

void bcServ::run()
{
    masterRun(); 
    register_update_config_event();      
}





