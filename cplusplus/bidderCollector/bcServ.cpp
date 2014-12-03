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
using namespace std;


sig_atomic_t srv_graceful_end = 0;
sig_atomic_t srv_ungraceful_end = 0;
sig_atomic_t srv_restart = 0;
sig_atomic_t sigalrm_recved = 0;;
sig_atomic_t sigusr1_recved = 0;;

unsigned long long bcServ::getLonglongTime()
{
    tmMutex.lock();
    unsigned long long t = m_microSecTime.tv_sec*1000+m_microSecTime.tv_usec/1000;
    tmMutex.unlock();
    return t;
}

mircotime_t bcServ::get_microtime()
{
    tmMutex.lock();
    mircotime_t t = m_microSecTime.tv_sec*1000+m_microSecTime.tv_usec/1000;
    tmMutex.unlock();
    return t;
}


void bcServ::readConfigFile()
{
    m_config.readConfig();
    
    string ip;
    unsigned short value;
  
    m_configureChange = 0;
    ip = m_config.get_throttleIP();
    value  = m_config.get_throttlePubVastPort();  
    if((ip != m_throttleIP)||(value != m_throttlePubVastPort)) 
    {
        m_configureChange |= c_update_throttleAddr;
        m_throttleIP = ip;
        m_throttlePubVastPort = value;
    }
   
    ip = m_config.get_bcIP();
    value  = m_config.get_bcListenBidderPort();
    if((ip != m_bcIP)||(value!=m_bcListenBidderPort)) 
    {
        m_configureChange |= c_update_bcAddr;
        m_bcIP = ip;
        m_bcListenBidderPort = value;
    }
    
    m_subKey.clear();
    m_subKey = m_config.get_subKey(); 
    if(m_subKey != m_subKeying)
    {
        m_configureChange |= c_update_zmqSub;
    }


    value = m_config.get_workerNum();
    if(m_workerNum != value)
    {
         m_configureChange |= c_update_workerNum;
         m_workerList_lock.write_lock();
         m_workerNum = value;
         m_workerList_lock.read_write_unlock();
    } 

    value = m_config.get_threadPoolSize();
    if(m_thread_pool_size!=value)
    {
         m_configureChange |= c_update_forbid;
         m_thread_pool_size = value;
    }

    ip=m_config.get_redisIP();
    value=m_config.get_redisPort();
    if((ip != m_redisIP)||(value != m_redisPort))
    {
         m_configureChange |= c_update_redisAddr;
         m_redisIP = ip;
         m_redisPort = value;
    }

    
    m_redisSaveTime = m_config.get_redisSaveTime();
    m_vastBusiCode = m_config.get_vastBusiCode();
    m_mobileBusiCode = m_config.get_mobileBusiCode();
}


bcServ::bcServ(configureObject &config):m_config(config)
{
    try
    {
        m_throttleIP = config.get_throttleIP();
        m_throttlePubVastPort = config.get_throttlePubVastPort();

        m_bcIP = config.get_bcIP();
        m_bcListenBidderPort = config.get_bcListenBidderPort();

        m_redisIP = config.get_redisIP();
        m_redisPort = config.get_redisPort();
        m_redisSaveTime = config.get_redisSaveTime();
        m_thread_pool_size = config.get_threadPoolSize();
        
        m_workerNum = config.get_workerNum();
        m_vastBusiCode = config.get_vastBusiCode();
        m_mobileBusiCode = config.get_mobileBusiCode();
        auto &sub_vec = config.get_subKey();
        auto it = sub_vec.begin();
        for(it = sub_vec.begin(); it != sub_vec.end(); it++)
        {
            string sub =  *it;
            m_subKey.push_back(sub);
        }
        bcDataMutex.init();
        m_delDataMutex.init();
        tmMutex.init();
        handler_fd_lock.init();
        m_workerList_lock.init();
    	for(auto i = 0; i < m_workerNum; i++)
    	{
    		BC_process_t *pro = new BC_process_t;
    		pro->pid = 0;
    		pro->status = PRO_INIT;
    		m_workerList.push_back(pro);
    	};
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
         if(test_count%10000==1)
         {
            struct timeval btime;
            gettimeofday(&btime, NULL);
            test_begTime = btime.tv_sec*1000+btime.tv_usec/1000;
            m_getTimeofDayMutex.unlock();
  //          cerr<<btime.tv_sec<<":--start--:"<<btime.tv_usec<<endl;
         }
         else if(test_count%10000==0)
         {
            int count = test_count;
            struct timeval etime;
            gettimeofday(&etime, NULL);
            test_endTime = etime.tv_sec*1000+etime.tv_usec/1000;
            long long diff = test_endTime-test_begTime;
            m_getTimeofDayMutex.unlock();
//            cerr<<etime.tv_sec<<":--end--:"<<etime.tv_usec<<endl;
            cerr <<"send request num: "<<count<<endl;
            cerr <<"cost time: " <<diff<<endl;
        
            if(diff)
            cerr<<"sspeed: "<<10000*1000/diff<<endl;
         }
         else
         {
            m_getTimeofDayMutex.unlock();
         }
}


/*
  *update m_workerList,  if m_wokerNum < workerListSize , erase the workerNode, if m_workerList update ,lock the resource
  */
void bcServ::updataWorkerList(pid_t pid)
{
    m_workerList_lock.write_lock();
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
                cerr <<"reduce worker num"<<endl;
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
    m_workerList_lock.read_write_unlock();
}

void bcServ::signal_handler(int signo)
{
    time_t timep;
    time(&timep);
    char *timeStr = ctime(&timep);
    switch (signo)
    {
        case SIGTERM:
	        cerr << "SIGTERM: "<<timeStr << endl;
            srv_ungraceful_end = 1;
            break;
        case SIGINT:
	        cerr << "SIGINT: " <<timeStr<< endl;
            srv_graceful_end = 1;
            break;
        case SIGHUP:
          cerr << "SIGHUP: "<< timeStr << endl;
            srv_restart = 1;
            break;
        case SIGUSR1:   
          cerr <<"SIGUSR1: "<< timeStr <<"pid::"<<getpid()<< endl;
            sigusr1_recved = 1;
            break;
        case SIGALRM:   
        //  cerr <<"SIGALARM: "<< timeStr << endl;
            sigalrm_recved = 1;
            break;
        default:
            cerr<<"signo:"<<signo<<endl;
            break;
    }
}


void bcServ::hupSigHandler(int fd, short event, void *arg)
{
    cerr <<"signal hup"<<endl;
    exit(0);
}

void bcServ::intSigHandler(int fd, short event, void *arg)
{
   cerr <<"signal int:"<<endl;
   usleep(10);
   exit(0);
}

void bcServ::termSigHandler(int fd, short event, void *arg)
{
    cerr <<"signal term"<<endl;
    exit(0);
}



void bcServ::usr1SigHandler(int fd, short event, void *arg)
{
    cerr <<"signal SIGUSR1"<<endl;
    bcServ *serv = (bcServ*) arg;
    if(serv != NULL)
    {
        serv->updateWorker();
    }
}

void bcServ::check_data_record()
{
    int lost=0;
    static int lostnum=0;

    mircotime_t micro = get_microtime();
    bcDataMutex.lock();
    for(auto bcDataIt = bcDataRecList.begin();bcDataIt != bcDataRecList.end(); )
    {  
        bcDataRec* bcData = bcDataIt->second;  
        if(bcData == NULL)
        {
            bcDataRecList.erase(bcDataIt++);
            continue;
        }

        mircotime_t overTime = bcData->get_overtime();
        mircotime_t tm= bcData->get_microtime();
        mircotime_t diff = micro - tm;
        if(diff>=overTime)//if have overtime, then user overtime, if not use default overtime
        {
      ////test
          /*  int bidd = bcData->get_bid();
            int sub = bcData->get_sub();
            if((sub!=1)||(bidd!=2))
            {
                lost++;
            }*/
      ////test
            delete bcData;  
            bcDataRecList.erase(bcDataIt++);
        }
        else
        {
            bcDataIt++;   
        }
    }     
    bcDataMutex.unlock();

    /*if(lost)
    {
        lostnum+=lost;
        cout<<"lost:"<<lostnum<<endl;
    }*/
}

void *bcServ::getTime(void *arg)
{
    bcServ *serv = (bcServ *)arg;
    int hsetNum = 0;
    int cntNum = 0;
    int times = 0;
    while(1)
    {
        usleep(10*1000);
        serv->update_microtime();
        serv->check_data_record();    
    }
}

void bcServ::worker_thread_func(void *arg)
{
    throttleBuf *fun_arg = (throttleBuf*) arg;
    if(fun_arg)
    {
       char uuid[UUID_SIZE_MAX];
       char biderIDTime[UUID_SIZE_MAX];
       char* buf = fun_arg->buf;
       int   len = fun_arg->bufSize;
       bcServ *serv = (bcServ *)fun_arg->serv;
       if(buf)
       {
            if(len<=2*UUID_SIZE_MAX)
            {
                cerr<<"recv data erorr:datalen:"<<len<<endl;
                delete[] fun_arg->buf;
                delete fun_arg;
                return;
            }
            memcpy(uuid, buf, UUID_SIZE_MAX);
            memcpy(biderIDTime, buf+UUID_SIZE_MAX, UUID_SIZE_MAX);
            if(serv)
            {
                if(serv->m_redisPoolManager.redis_hset_request(uuid,biderIDTime,buf+2*UUID_SIZE_MAX,len - 2*UUID_SIZE_MAX)==true)
                {
                    serv->calSpeed();
                }
                else
                {
                    cerr<<"redis_hset_request false"<<endl;
                }
            }
       }
       delete[] fun_arg->buf;
       delete fun_arg;
    }
}

static int worker_recv_cnt=0;
void bcServ::workerBusiness_callback(int fd, short event, void *pair)
{
    bcServ *serv = (bcServ*) pair;
    if(serv==NULL) 
    {
        cerr<<"serv is nullptr"<<endl;
        return;
    }
    
    char buf[1024];
    int len;
    int dataLen;
    char uuid[UUID_SIZE_MAX];
    char biderIDTime[UUID_SIZE_MAX];
    
    while(1)
    {
    
       len = read(fd, buf, 4);   
       if(len != 4)
       {
          break;
       }

       dataLen = GET_LONG(buf); 
////////////////
       char *read_buf = new char[dataLen];
       len = read(fd, read_buf, dataLen);
       if(len <= 0)
       {
            delete[] read_buf;
            return;
       }
       
       int idx = len;
       while(idx < dataLen) 
       {
           cout<<"recv data exception:"<<dataLen <<"  idx:"<<idx<<endl;
           len = read(fd, read_buf+idx, dataLen-idx);
           if(len <= 0)
           {
                delete[] read_buf;
                return;
           }
           idx += len;
       }
//////////////////////     
 /*      char *read_buf = new char[dataLen];
       len = read(fd, read_buf, dataLen);
       if(len != dataLen) 
       {
           cerr<<"recv data exception:"<<dataLen <<"len:"<<len<<endl;
           delete[] read_buf;
           break;
       }*/
   //    serv->calSpeed();
       //hset  
       throttleBuf *arg = new throttleBuf;
       arg->buf = read_buf;
       arg->bufSize = dataLen;
       arg->serv = serv;
       if(serv->m_threadPoolManger.Run(worker_thread_func,(void *) arg)!=0)
       {
            cerr<<"lost some worker_thread_func"<<endl;
            delete[] read_buf;
            delete arg;
       }
     }            
}


void *bcServ::work_event_new(void *arg)
{
    bcServ* serv = (bcServ*) arg;

    serv->m_getTimeofDayMutex.init();
    struct event_base* base = event_base_new();    
    struct event * hup_event = evsignal_new(base, SIGHUP, serv->hupSigHandler, arg);
    struct event * int_event = evsignal_new(base, SIGINT, serv->intSigHandler, arg);
    struct event * term_event = evsignal_new(base, SIGTERM, serv->termSigHandler, arg);
    struct event * usr1_event = evsignal_new(base, SIGUSR1, serv->usr1SigHandler, arg);
    struct event *clientPullEvent = event_new(base, serv->m_workerChannel, EV_READ|EV_PERSIST, serv->workerBusiness_callback, arg);

    event_add(clientPullEvent, NULL);
    evsignal_add(hup_event, NULL);
    evsignal_add(int_event, NULL);
    evsignal_add(term_event, NULL);
    evsignal_add(usr1_event, NULL);
    event_base_dispatch(base);    
}

void bcServ::start_worker()
{
    worker_net_init();
    pthread_t pth1, pth2, pth3;    
    pthread_create(&pth2, NULL, work_event_new, this);  
    pthread_join(pth2, NULL);
}

void bcServ::masterRestart(struct event_base* base)
{
    cerr <<"masterRestart::m_throConfChange:"<<m_configureChange<<endl;
    if((m_configureChange&c_update_throttleAddr)==c_update_throttleAddr)
    {
        m_configureChange = (m_configureChange&(~c_update_throttleAddr));
        cerr <<"c_master_throttleIP or c_master_throttlePort change"<<endl;
        for(auto itor = m_handler_fd_map.begin(); itor != m_handler_fd_map.end();)
        {
            handler_fd_map *hfMap = *itor;
            if(hfMap)
            {
                string subKey = hfMap->subKey;
                zmq_setsockopt(hfMap->handler, ZMQ_UNSUBSCRIBE, subKey.c_str(), subKey.size()); 
                zmq_close(hfMap->handler);
                event_del(hfMap->evEvent);
                delete hfMap;
                itor = m_handler_fd_map.erase(itor);

            }
            else
            {
                itor=m_handler_fd_map.erase(itor);
            }
         }
         m_subKeying.clear();

         for(auto it =  m_subKey.begin(); it !=  m_subKey.end(); it++)
         {
            string subKey = *it;
            if(zmq_connect_subscribe(subKey))
            {
                handler_fd_lock.read_lock();
                handler_fd_map *hfMap = get_request_handler(subKey);
                struct event * pEvRead = event_new(base, hfMap->fd, EV_READ|EV_PERSIST, recvRequest_callback, this); 
                hfMap->evEvent = pEvRead;
                handler_fd_lock.read_write_unlock();
                event_add(pEvRead, NULL);
            }
         }
    }

    if((m_configureChange&c_update_zmqSub)==c_update_zmqSub)//change subsrcibe
    {
        m_configureChange = (m_configureChange&(~c_update_zmqSub));
        vector<string> delVec;
        vector<string> addVec;
        for(auto it = m_subKeying.begin(); it != m_subKeying.end(); it++)
        {
            string &subKey = *it;
            auto et = find(m_subKey.begin(),m_subKey.end(), subKey);
            if(et == m_subKey.end())//not find ,unsubsribe
            {
                delVec.push_back(subKey);
            }
        }

        for(auto it = m_subKey.begin(); it != m_subKey.end(); it++)
        {
            string &subKey = *it;
            auto et = find(m_subKeying.begin(), m_subKeying.end(), subKey);
            if(et == m_subKeying.end())
            {
                addVec.push_back(subKey);
            }
        }

        //m_subkeying exit, m_subkey not exit , this means must the subkey must unsubscribe
        for(auto it = delVec.begin(); it != delVec.end(); it++)
        {
            string &subKey = *it;
            handler_fd_lock.write_lock();
            releaseConnectResource(subKey);
            handler_fd_lock.read_write_unlock();
        }

        for(auto it = addVec.begin(); it != addVec.end(); it++)
        {
            string subKey = *it;
            cerr <<"add subkey:"<<subKey<<endl; 
            handler_fd_lock.write_lock();
            handler_fd_map *hfMap = zmq_connect_subscribe(subKey);
            if(hfMap)
            {
                struct event * pEvRead = event_new(base, hfMap->fd, EV_READ|EV_PERSIST, recvRequest_callback, this);  
                event_add(pEvRead, NULL);
                hfMap->evEvent = pEvRead;
            }
            handler_fd_lock.read_write_unlock();
        }
    } 

    if((m_configureChange&c_update_bcAddr)==c_update_bcAddr)
    {
        m_configureChange = (m_configureChange&(~c_update_bcAddr));
        zmq_close(m_bidderRspHandler);
        event_del(m_bidderEvent);       
        m_bidderRspHandler = bindOrConnect(false, "tcp", ZMQ_ROUTER, m_bcIP.c_str(), m_bcListenBidderPort, &m_bidderFd);//client sub 
        m_bidderEvent = event_new(base, m_bidderFd, EV_READ|EV_PERSIST, recvBidder_callback, this);  
        event_add(m_bidderEvent, NULL);     
    }
    cerr <<"masterRestart::m_throConfChange:"<<m_configureChange<<endl; 
}

redisContext* bcServ::connectToRedis()
{
	redisContext* context = redisConnect(m_redisIP.c_str(), m_redisPort);
	if(context == NULL) return NULL;
	if(context->err)
	{
		redisFree(context);
		cerr <<"connectaRedis failure" << endl;
		return NULL;
	}

	cerr << "connectaRedis success" << endl;
	return context;
}


void bcServ::worker_net_init()
{ 
    const int tastPoolSize = 10000;
    m_redisPoolManager.connectorPool_init(m_redisIP.c_str(), m_redisPort,  m_thread_pool_size+1);
    m_threadPoolManger.Init(tastPoolSize, m_thread_pool_size, m_thread_pool_size);
    cerr <<"worker net init success:" <<endl;
}

void bcServ::sendToWorker(char *buf, int size)
{
    BC_process_t* pro = NULL;
    int chanel;   
    int rc;
    static int workerID = 0;
      
    try
    {
        m_workerList_lock.read_lock();
        unsigned int workIdx= (++workerID)%m_workerNum;
        pro = m_workerList.at(workIdx);
        if(pro == NULL) return;
        chanel = pro->channel[0];
        m_workerList_lock.read_write_unlock();

        do
        {   
            rc = write(chanel, buf, size);
            if(rc>0)
            { 
                break;
            }
            
            m_workerList_lock.read_lock();
            workIdx= (++workerID)%m_workerNum;
            pro = m_workerList.at(workIdx);
            chanel = pro->channel[0]; 
            m_workerList_lock.read_write_unlock();   
       }while(1);
    }
    catch(std::out_of_range &err)
    {
        cerr<<err.what()<<"LINE:"<<__LINE__  <<" FILE:"<< __FILE__<<endl;
        workerID = 0;
        sendToWorker(buf, size);
    }
    catch(...)
    {
        cerr <<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&"<<endl;
        throw;
    }
}

void bcServ::handler_recv_buffer(char * buf,int size, string &uuid, string &hash_field)
{
     time_t timep;
     time(&timep);

     int recvSize = size+2*UUID_SIZE_MAX+4;//len+uuid+field+buf 
     char *recvBuf = new char[recvSize];
     memset(recvBuf, 0x00, recvSize);
     PUT_LONG(recvBuf ,recvSize-4);
     memcpy(recvBuf+4, uuid.c_str(), uuid.size());
     memcpy(recvBuf+UUID_SIZE_MAX+4, hash_field.c_str(), hash_field.size());
     memcpy(recvBuf+2*UUID_SIZE_MAX+4,buf, size);
    //send to worker
     sendToWorker(recvBuf, recvSize);
     delete[] recvBuf;

     redisDataRec *redis = NULL;
     
     m_delDataMutex.lock();//record redis hset record, delete hash hset data after expire
     auto bcDataIt = redisRecList.find(uuid);
     if(bcDataIt != redisRecList.end())
     {
        redis = bcDataIt->second;
        if(redis==NULL)
        {
            redis = new redisDataRec(timep);
        }
     }
     else
     {
         redis = new redisDataRec(timep);    
     }
     redis->add_field(hash_field);
     redisRecList.insert(pair<string, redisDataRec*>(uuid, redis));
     m_delDataMutex.unlock();
}

void bcServ::bidderHandler(char * pbuf,int size)
{
    ostringstream os;
    CommonMessage commMsg;
    string rsp;
    string uuid;
    string bidID;
    string sockID;
    string busiCode;

    
    commMsg.ParseFromArray(pbuf, size);  
    busiCode=commMsg.businesscode();
    string ttl=commMsg.ttl();
    uint32_t overTime = atoi(ttl.c_str());;
   // cout<<"b ttl:"<<overTime<<endl;
    rsp = commMsg.data();   
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
   //     bidID=mobile_rsp.bidderid();
    }
    else
    {
        cerr<<"bid::invalid business code:"<<busiCode<<endl;
        return;
    }
    if(uuid.empty())
    {
        cerr<<"bid::uuid empty"<<endl;
        return;
    }
    
    mircotime_t recvBidTime = get_microtime();
    os.str("");
    os<<bidID<<":"<<"<"<<recvBidTime<<">";
    string bidIDandTime = os.str();

  //  cerr<<"bidd:"<<uuid<<"--"<<bidIDandTime<<endl;
    bcDataMutex.lock();
    auto bcDataIt = bcDataRecList.find(uuid);
    if(bcDataIt != bcDataRecList.end())//finu uuid
    {
         bcDataRec *bcData = bcDataIt->second;
         if(bcData)
         {  
         ////test
                bcData->set_bid();
        ////test
              if(bcData->recv_throttle_info())//recv throttle
              {  
                   auto bidder_buf = bcData->get_buf_list();
                   bcDataMutex.unlock();
                   handler_recv_buffer(pbuf, size, uuid, bidIDandTime);

                   if(bidder_buf)//can not recv there,
                   {
                        cerr<<"error:bidderHandler get bidder_buf is not null"<<endl;
                        for(auto it = bidder_buf->begin(); it != bidder_buf->end(); it++)
                        {
                            bufferStructure *buf_stru= *it;
                            if(buf_stru)
                            {
                                handler_recv_buffer(buf_stru->get_buf(), buf_stru->get_bufSize(), uuid, buf_stru->get_hash_field());
                            }
                        }
                        delete bidder_buf;
                   }
               }
               else//have not recv throttle, so record the bidder response data;
               {
                  if(bcData)
                  {
                       bcData->add_bidder_buf(pbuf, size, bidIDandTime);
                  }
                  bcDataMutex.unlock();

               }
         
          }
          else//invalid bcData ,assert , throw error
          {
              bcDataRecList.erase(bcDataIt);
              bcDataMutex.unlock();

          }
     }
     else// can not find uuid,represent throttle info have not recved
     {
           bcDataRec *bData = new bcDataRec(recvBidTime, overTime, false);
           bData->add_bidder_buf(pbuf, size, bidIDandTime);
////test
           bData->set_bid();
////test
           bcDataRecList.insert(pair<string, bcDataRec*>(uuid, bData)); 
           bcDataMutex.unlock();          
     }   
}
/*
  *subscribte throttle request
  */
void bcServ::subVastHandler(char * pbuf,int size)
{
  //  time_t timep;
 //   time(&timep);
    CommonMessage commMsg;

    uint32_t overTime = 100;
    const uint32_t overTime_max=1000;
    const uint32_t overTime_min=100;

    commMsg.ParseFromArray(pbuf, size);
    string busiCode = commMsg.businesscode();
    string data = commMsg.data();
    string ttl = commMsg.ttl();
    overTime = atoi(ttl.c_str());;
   // cout<<"s ttl:"<<overTime<<endl;
    if((overTime>overTime_max)||(overTime<overTime_min)) overTime=overTime_max;

    
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
    }
    else
    {
        cerr<<"sub::invalid business code:"<<busiCode<<endl;
        return;
    }

    if(uuid.empty())
    {
        cerr<<"sub::uuid empty"<<endl;
        return;
    }
    //cerr<<"vast:"<<uuid<<endl;

    
    bcDataMutex.lock();
    auto bcDataIt = bcDataRecList.find(uuid);//find uuid, this is represent, bidder is come first, or two same uuid request come from throttle in overtime
    if(bcDataIt != bcDataRecList.end())//
    {
        bcDataRec *bcData = bcDataIt->second;
        if(bcData)
        {
                ////test
                   bcData->set_sub();
        ////test
            if(bcData->recv_throttle_info()== false)// the first recv throttle info, then....
            {
                bcData->recv_throttle_complete();//change recv throtlle symbol
                auto bidder_buf = bcData->get_buf_list();//get bidder response list,
                bcDataMutex.unlock();
                if(bidder_buf)//if bidder response first come, bidder_buf is not null, then haddler the recv bidder info;
                {
                     for(auto it = bidder_buf->begin(); it != bidder_buf->end(); it++)//bidder buf list ,record buf info and field, use in redis hash hset command
                     {
                         bufferStructure *buf_stru= *it;
                         if(buf_stru)
                         {
                             handler_recv_buffer(buf_stru->get_buf(), buf_stru->get_bufSize(), uuid, buf_stru->get_hash_field());//handler bidder info
                         }
                     }
                     delete bidder_buf;
                 }
             }
             else  //recv same uuid throttle, drop it
             {
                 bcDataMutex.unlock();     
             }
         }
         else// NULL data
         {
             bcDataRecList.erase(bcDataIt);
             bcDataMutex.unlock();
         }
     }
     else// uuid request come first
     {
        mircotime_t recvSubTime = get_microtime();  //record recv time , then wo know delete the infomation when
        bcDataRec *bData = new bcDataRec(recvSubTime, overTime, true);//timep tell we when delete redis record, overtime, tell we free data info at expired
        bcDataRecList.insert(pair<string, bcDataRec*>(uuid, bData));
        ////test
                   bData->set_sub();
        ////test


        bcDataMutex.unlock();
     }
}

void bcServ::recvRequest_callback(int fd, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len=sizeof(int);

    bcServ *serv = (bcServ*)pair;
    if(serv==NULL) 
    {
        cerr<<"serv is nullptr"<<endl;
        return;
    }

    len = sizeof(events);
    void *handler = serv->get_request_handler(serv, fd);
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        cerr <<"adrsp is invalid" <<endl;
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

void* bcServ::establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd)
{
        ostringstream os;
        string pro;
        int rc;
        void *handler = nullptr;
        size_t size;
    
    
        os.str("");
        os << transType << "://" << addr << ":" << port;
        pro = os.str();
    
        handler = zmq_socket (m_zmqContext, zmqType);
        if(client)
        {
           rc = zmq_connect(handler, pro.c_str());
        }
        else
        {
           rc = zmq_bind (handler, pro.c_str());
        }
    
        if(rc!=0)
        {
            cerr << "<" << addr << "," << port << ">" << "can not "<< ((client)? "connect":"bind")<< " this port:"<<zmq_strerror(zmq_errno())<<endl;
            zmq_close(handler);
            return nullptr;     
        }
    
        if(fd != nullptr&& handler!=nullptr)
        {
            size = sizeof(int);
            rc = zmq_getsockopt(handler, ZMQ_FD, fd, &size);
            if(rc != 0 )
            {
                cerr<< "<" << addr << "," << port << ">::" <<"ZMQ_FD faliure::" <<zmq_strerror(zmq_errno())<<"::"<<endl;
                zmq_close(handler);
                return nullptr; 
            }
        }
        return handler;
}

void* bcServ::bindOrConnect( bool client,const char * transType,int zmqType,const char * addr,unsigned short port,int * fd)
{
        int connectTimes = 0;
        const int connectMax=3;
    
        void *zmqHandler = NULL;
    
        do
        {
           zmqHandler = establishConnect(client, transType, zmqType, addr, port, fd);
           if(zmqHandler == nullptr)
           {
               if(connectTimes++ >= connectMax)
               {
                    cerr <<"try to connect to MAX"<<endl;     
                    exit(1);
               }
               cerr <<"<"<<addr<<","<<port<<">"<<((client)? "connect":"bind")<<"failure"<<endl;
               sleep(1);
           }
        }while(zmqHandler == nullptr);
    
        cerr << "<" << addr << "," << port << ">" << ((client)? "connect":"bind")<< " to this port:"<<endl;
        return zmqHandler;
}



void bcServ::configureUpdate(int fd, short __, void *pair)
{
    eventParam *param = (eventParam *) pair;
    bcServ *serv = (bcServ*)param->serv;
    char buf[1024]={0};
    int rc = read(fd, buf, sizeof(buf));
    cerr<<buf<<endl;
    if(memcmp(buf,"event", 5) == 0)
    {
        cerr <<"update connect:"<<getpid()<<endl;
        serv->masterRestart(param->base);
    }    
 }

 /*
  *function name: zmq_connect_subscribe
  *param: subsribe key
  *return:handler_fd_map*
  *function: establish subscribe connection
  *all right reserved
  */
handler_fd_map*  bcServ::zmq_connect_subscribe(string subkey)
{
    int fd;
    int rc;
    size_t size = sizeof(int);
    int hwm = 30000;
    
    void *handler = bindOrConnect(true, "tcp", ZMQ_SUB, m_throttleIP.c_str(), m_throttlePubVastPort, &fd);//client sub 
    //rc = zmq_setsockopt(handler, ZMQ_RCVHWM, &hwm, sizeof(hwm));
    rc =  zmq_setsockopt(handler, ZMQ_SUBSCRIBE, subkey.c_str(), subkey.size());
    if(rc!=0)
    {
        cerr<<"sucsribe "<< subkey <<" failure::"<<zmq_strerror(zmq_errno())<<endl;
        return NULL;
    }
    else
    {
        cerr <<"sucsribe "<< subkey <<" success"<<endl;
        m_subKeying.push_back(subkey);//push to m_subKeying
    } 
    
    handler_fd_map *hfMap = new handler_fd_map;//establish subkey fd handler information
    hfMap->fd = fd;
    hfMap->handler = handler;
    hfMap->subKey = subkey;
    m_handler_fd_map.push_back(hfMap);
    return hfMap;
}


void bcServ::master_net_init()
{ 
    m_zmqContext =zmq_ctx_new();
    int hwm = 30000;
    auto it = m_subKey.begin();  
    for(it = m_subKey.begin(); it != m_subKey.end();it++)
    {
        zmq_connect_subscribe(*it);
    }   
    cerr <<"adVast bind success" <<endl;
    
    m_bidderRspHandler = bindOrConnect(false, "tcp", ZMQ_ROUTER, m_bcIP.c_str(), m_bcListenBidderPort, &m_bidderFd);//client sub 
    int   rc = zmq_setsockopt(m_bidderRspHandler, ZMQ_RCVHWM, &hwm, sizeof(hwm));
    
    cerr <<"bidder rsp bind success" <<endl;
}

void *bcServ::throttle_request_handler(void *arg)
{ 
      bcServ *serv = (bcServ*) arg;
      struct event_base* base = event_base_new(); 

      eventParam *param = new eventParam;
      param->base = base;
      param->serv = serv;
      int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, serv->m_vastEventFd);
      struct event * pEvRead = event_new(base, serv->m_vastEventFd[0], EV_READ|EV_PERSIST, configureUpdate, param); 
      event_add(pEvRead, NULL);

      serv->handler_fd_lock.read_lock();
      auto it = serv->m_handler_fd_map.begin();
      for(it = serv->m_handler_fd_map.begin(); it != serv->m_handler_fd_map.end(); it++)
      {
         handler_fd_map *hfMap = *it;
         if(hfMap== NULL)continue;
         struct event * pEvRead = event_new(base, hfMap->fd, EV_READ|EV_PERSIST, recvRequest_callback, arg);  
         event_add(pEvRead, NULL);
         hfMap->evEvent = pEvRead;
      }
      serv->handler_fd_lock.read_write_unlock();;
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
void bcServ::recvBidder_callback(int fd, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len=sizeof(int);

    bcServ *serv = (bcServ*)pair;
    if(serv==NULL) 
    {
        cerr<<"serv is nullptr"<<endl;
        return;
    }

    len = sizeof(events);
    void *handler = serv->m_bidderRspHandler;
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        cerr <<"adrsp is invalid" <<endl;
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
                serv->bidderHandler(msg_data, recvLen);
            }
            zmq_msg_close(&part);
        }
    }
}


void *bcServ::bidder_response_handler(void *arg)
{ 
      bcServ *serv = (bcServ*) arg;
      struct event_base* base = event_base_new(); 
      serv->m_bidderEvent = event_new(base, serv->m_bidderFd, EV_READ|EV_PERSIST, recvBidder_callback, arg); 
      event_add(serv->m_bidderEvent, NULL);
      event_base_dispatch(base);
      return NULL;  
}

void bcServ::get_overtime_recond(map<string, vector<string>*>& delMap)
{
try
{
    time_t timep;
    time(&timep);
    m_delDataMutex.lock();
    for(auto redisIt = redisRecList.begin();redisIt != redisRecList.end(); )
    {  
        redisDataRec* redis = redisIt->second;  
    
        if(redis == NULL)
        {
            redisRecList.erase(redisIt++);
            continue;
        }
    
        string uuid = redisIt->first;
        time_t timeout = redis->get_time();
        if(timep - timeout >= m_redisSaveTime)
        {
            vector<string> *str_vector = new vector<string>;
            auto field_list = redis->get_field_list();
            for(auto it = field_list.begin(); it != field_list.end(); it++)
            {
                str_vector->push_back(*it);
            }
            delMap.insert(pair<string, vector<string>*>(uuid, str_vector));   
            delete redis;
            redisRecList.erase(redisIt++);
        }
        else
        {
            redisIt++;
        }

    }   
    m_delDataMutex.unlock();
}
catch(...)
{
    cerr<<"get_overtime_recond exception"<<endl;
    m_delDataMutex.unlock();
}
}


void bcServ::delete_redis_data(map<string, vector<string>*>& delMap)
{
    while(delMap.size())
    {
        int index = 0;
        for(auto delIt= delMap.begin(); delIt != delMap.end();)
        {
            string uuid = delIt->first;
            auto field_vector = delIt->second;
            if(field_vector)
            {
                for(auto it = field_vector->begin(); it != field_vector->end(); it++)
                {
                    string field_key = *it;
                    redisAppendCommand(m_delContext, "hdel %s %s", uuid.c_str(), field_key.c_str());
                    index++;
                }
                delete field_vector;
                delMap.erase(delIt++);
                if(index >=2000) break; // 一个管道1000个  
            }
            else
            {
                delIt++;
            }
        }
        
        for(int i = 0;i < index; i++)
        {
            redisReply *ply;
            int ret = redisGetReply(m_delContext, (void **) &ply);
            if(ret != REDIS_OK)
            {
                redisFree(m_delContext);
                m_delContext = connectToRedis();  
                cerr << "error::deleteRedisData can not connect redis" << endl;
                return;        
            }
            else
            {
                freeReplyObject(ply);    
            }
        } 
    }

}

void* bcServ::deleteRedisDataHandle(void *arg)
{   
    bcServ *serv = (bcServ*) arg;
    map<string, vector<string>*> delMap; 
    
    do
    {
        serv->m_delContext = serv->connectToRedis();
        if(serv->m_delContext == nullptr)
        {
            cerr << "error::deleteRedisData can not connect redis" << endl;
            sleep(1);
        }
    }while(serv->m_delContext == nullptr);

    while(1)
    {
         sleep(serv->m_redisSaveTime);
         serv->get_overtime_recond(delMap);
         serv->delete_redis_data(delMap); 
         if(((serv->m_configureChange) &c_update_redisAddr) == c_update_redisAddr)
		 {
		    (serv->m_configureChange)=((serv->m_configureChange)&(~c_update_redisAddr));
		    redisFree(serv->m_delContext);
            do
            {
                serv->m_delContext = serv->connectToRedis();
                if(serv->m_delContext == nullptr)
                {
                    cerr << "error::deleteRedisData can not connect redis" << endl;
                    sleep(1);
                }
            }while(serv->m_delContext == nullptr);
            cout<<"deleteRedisDataHandle:"<<(serv->m_configureChange)<<endl;
		 }	
    }

}


bool bcServ::masterRun()
{
      int *ret;
      pthread_t pth1; 
      pthread_t pth2; 
      pthread_t pth3; 
      pthread_t pth4;
      master_net_init();
      pthread_create(&pth1, NULL, throttle_request_handler, this);      
      pthread_create(&pth2, NULL, bidder_response_handler, this);  
      pthread_create(&pth3, NULL, deleteRedisDataHandle, this);
      pthread_create(&pth4, NULL, getTime, this);  
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
void bcServ::run()
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

    m_master_pid = getpid();
    
    while(true)
    {
        pid_t pid;
        if(num_children != 0)
        {  
            pid = wait(NULL);
            if(pid != -1)
            {
                num_children--;
                cerr <<pid << "---exit" << endl;
                updataWorkerList(pid);
            }

            if (srv_graceful_end || srv_ungraceful_end)//CTRL+C ,killall throttle
            {
                cerr <<"srv_graceful_end"<<endl;
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
                cerr <<"srv_restart"<<endl;
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
                cerr <<"master progress recv sigusr1"<<endl;
    
                
                readConfigFile();//read configur file
                auto count = 0;
                for (auto it = m_workerList.begin(); it != m_workerList.end(); it++, count++)
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
                        m_workerList_lock.write_lock();
				        m_workerList.push_back(pro); 
                        m_workerList_lock.read_write_unlock();
                    }
                }    
            }
 
        }

        if(is_child==0 &&sigusr1_recved)//fathrer process and recv sig usr1
        {
            write(m_vastEventFd[1],"event", 5);
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
                        cerr<<"+++++++++++++++++++++++++++++++++++"<<endl;
                    }
                    break;
                    case 0:
                    {  
                        close(m_vastEventFd[0]);
                        close(m_vastEventFd[1]);
                        close(pro->channel[0]);
                        m_workerChannel = pro->channel[1];
                        int flags = fcntl(m_workerChannel, F_GETFL);
                        fcntl(m_workerChannel, F_SETFL, flags | O_NONBLOCK); 
                        is_child = 1; 
                        pro->pid = getpid();
                        cerr<<"worker restart"<<endl;
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
        cerr << "master exit"<<endl;
        exit(0);    
    }
    start_worker();
}





