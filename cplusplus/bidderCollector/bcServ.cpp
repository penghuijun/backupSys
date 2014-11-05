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

using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;
const char *businessCode_VAST="5.1";

sig_atomic_t srv_graceful_end = 0;
sig_atomic_t srv_ungraceful_end = 0;
sig_atomic_t srv_restart = 0;
sig_atomic_t sigalrm_recved = 0;;
sig_atomic_t sigusr1_recved = 0;;

unsigned long long bcServ::getLonglongTime()
{
    tmLock();
    unsigned long long t = m_microSecTime.tv_sec*1000+m_microSecTime.tv_usec/1000;
    tmUnlock();
    return t;
}

void bcServ::readConfigFile()
{
    m_config.readConfig();
    
    string ip;
    unsigned short value;
  
    m_configureChange = 0;
    ip = m_config.get_throttleIP();
    if(ip != m_throttleIP) 
    {
        m_configureChange |= c_master_throttleIP;
        m_throttleIP = ip;
    }
   
    value  = m_config.get_throttlePubVastPort();
    if(m_throttlePubVastPort!= value)
    {
        m_configureChange |= c_master_throttlePubVastPort;
        m_throttlePubVastPort = value;
    }

    ip = m_config.get_bcIP();
    if(ip != m_bcIP) 
    {
        m_configureChange |= c_master_bcIP;
        m_bcIP = ip;
    }
   
    value  = m_config.get_bcListenBidderPort();
    if(m_bcListenBidderPort!= value)
    {
        m_configureChange |= c_master_bcListenBidPort;
        m_bcListenBidderPort = value;
    }

    ip = m_config.get_redisIP();
    if(ip != m_redisIP) 
    {
        m_configureChange |= c_master_redisIP;
        m_redisIP = ip;
    }
   
    value  = m_config.get_redisPort();
    if(m_redisPort != value)
    {
        m_configureChange |= c_master_redisPort;
        m_redisPort = value;
    }

    value  = m_config.get_redisSaveTime();
    if(m_redisSaveTime != value)
    {
        m_configureChange |= c_master_redisSaveTime;
        m_redisSaveTime = value;
    }
    
    m_subKey.clear();
    auto &sub_vec = m_config.get_subKey();
       // m_subKey = config.get_subKey();
    auto it = sub_vec.begin();
    for(it = sub_vec.begin(); it != sub_vec.end(); it++)
    {
        string sub =  *it;
    //    string expSub = sub+"_EXP"; 
        m_subKey.push_back(sub);
   //     m_subKey.push_back(expSub);
    }
    
    if(m_subKey != m_subKeying)
    {
        m_configureChange |= c_master_subKey;
    }

    value = m_config.get_workerNum();
    if(m_workerNum != value)
    {
            m_configureChange |= c_workerNum;
            workerList_wr_lock();
            m_workerNum = value;
            workerList_rw_unlock();
    } 
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
        m_redisConnectNum = config.get_redisConnectNum();
        
        m_workerNum = config.get_workerNum();
        auto &sub_vec = config.get_subKey();
       // m_subKey = config.get_subKey();
        auto it = sub_vec.begin();
        for(it = sub_vec.begin(); it != sub_vec.end(); it++)
        {
            string sub =  *it;
            m_subKey.push_back(sub);
        }
        pthread_mutex_init(&m_uuidListMutex, NULL);
        pthread_rwlock_init(&handler_fd_lock, NULL);
        pthread_rwlock_init(&m_workerList_lock, NULL);
        pthread_mutex_init(&bcDataMutex, NULL);
        pthread_mutex_init(&m_redisContextMutex, NULL);
        pthread_mutex_init(&m_delDataMutex, NULL); 
        pthread_mutex_init(&tmMutex, NULL);    
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
        cerr <<"bidderServ structure error"<<endl;
        exit(1);
    }
}

void bcServ::calSpeed()
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


/*
  *update m_workerList,  if m_wokerNum < workerListSize , erase the workerNode, if m_workerList update ,lock the resource
  */
void bcServ::updataWorkerList(pid_t pid)
{
    workerList_wr_lock();
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
                cout <<"reduce worker num"<<endl;
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
    workerList_rw_unlock();
}

void bcServ::signal_handler(int signo)
{
    time_t timep;
    time(&timep);
    char *timeStr = ctime(&timep);
    switch (signo)
    {
        case SIGTERM:
	        cout << "SIGTERM: "<<timeStr << endl;
            srv_ungraceful_end = 1;
            break;
        case SIGINT:
	        cout << "SIGINT: " <<timeStr<< endl;
            srv_graceful_end = 1;
            break;
        case SIGHUP:
          cout << "SIGHUP: "<< timeStr << endl;
            srv_restart = 1;
            break;
        case SIGUSR1:   
          cout <<"SIGUSR1: "<< timeStr <<"pid::"<<getpid()<< endl;
            sigusr1_recved = 1;
            break;
        case SIGALRM:   
        //  cout <<"SIGALARM: "<< timeStr << endl;
            sigalrm_recved = 1;
            break;
        default:
            cout<<"signo:"<<signo<<endl;
            break;
    }
}


void bcServ::hupSigHandler(int fd, short event, void *arg)
{
    cout <<"signal hup"<<endl;
    exit(0);
}

void bcServ::intSigHandler(int fd, short event, void *arg)
{
   cout <<"signal int:"<<endl;
   usleep(10);
   exit(0);
}

void bcServ::termSigHandler(int fd, short event, void *arg)
{
    cout <<"signal term"<<endl;
    exit(0);
}



void bcServ::usr1SigHandler(int fd, short event, void *arg)
{
    cout <<"signal SIGUSR1"<<endl;
    bcServ *serv = (bcServ*) arg;
    if(serv != NULL)
    {
        serv->reloadWorker();
        if(((serv->m_configureChange&c_master_redisIP) == c_master_redisIP)||((serv->m_configureChange&c_master_redisPort) == c_master_redisPort) )
        {
            serv->m_redisPool.connectorPool_earse();
            serv->m_redisPool.connectorPool_init(serv->m_redisIP.c_str(), serv->m_redisPort, serv->m_redisConnectNum);
        }
    }
}


void *bcServ::getTime(void *arg)
{
    bcServ *serv = (bcServ *)arg;
    int hsetNum = 0;
    int cntNum = 0;
    int times = 0;
    while(1)
    {
        usleep(2*1000);
        serv->update_microtime();
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
            memcpy(uuid, buf, UUID_SIZE_MAX);
            memcpy(biderIDTime, buf+UUID_SIZE_MAX, UUID_SIZE_MAX);
            if(serv)
            {
                serv->m_redisPool.redis_hset_request(uuid,biderIDTime,buf+2*UUID_SIZE_MAX,len - 2*UUID_SIZE_MAX); 
            }
       }
       delete[] fun_arg->buf;
       delete fun_arg;
    }
}

void bcServ::workerBusiness_callback(int fd, short event, void *pair)
{
    bcServ *serv = (bcServ*) pair;
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
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
       char *read_buf = new char[dataLen];
       len = read(fd, read_buf, dataLen);
       if(len != dataLen) 
       {
           cout<<"recv data exception:"<<dataLen <<"len:"<<len<<endl;
           delete[] read_buf;
           break;
       }

       serv->calSpeed();
       //hset  
       throttleBuf *arg = new throttleBuf;
       arg->buf = read_buf;
       arg->bufSize = dataLen;
       arg->serv = serv;
       if(serv->m_threadPoolManger.Run(worker_thread_func,(void *) arg)!=0)
       {
            delete[] read_buf;
            delete arg;
       }
     }            
}


void *bcServ::work_event_new(void *arg)
{
    bcServ* serv = (bcServ*) arg;

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
 
    cout <<"masterRestart::m_throConfChange:"<<m_configureChange<<endl;
    if(((m_configureChange&c_master_throttleIP)==c_master_throttleIP)|| ((m_configureChange&c_master_throttlePubVastPort)==c_master_throttlePubVastPort))
    {
        cout <<"c_master_throttleIP or c_master_throttlePort change"<<endl;
        auto it = m_subKeying.begin();
        for(it = m_subKeying.begin(); it != m_subKeying.end();)
        {
            string subKey = *it;
            auto itor = m_handler_fd_map.begin();
            for(itor = m_handler_fd_map.begin(); itor != m_handler_fd_map.end();)
            {
                handler_fd_map *hfMap = *itor;
                if(hfMap==NULL)
                {
                    itor = m_handler_fd_map.erase(itor);
                    continue;
                }
                if(hfMap->subKey == subKey)
                {
                     zmq_setsockopt(hfMap->handler, ZMQ_UNSUBSCRIBE, subKey.c_str(), subKey.size()); 
                     zmq_close(hfMap->handler);
                     event_del(hfMap->evEvent);
                     delete hfMap;
                     itor = m_handler_fd_map.erase(itor);
                }
                else
                {
                    itor++;
                }
            }

            it = m_subKeying.erase(it);
         }
         m_subKeying.clear();
         m_handler_fd_map.clear();

         for(it =  m_subKey.begin(); it !=  m_subKey.end(); it++)
         {
            string subKey = *it;
            if(zmq_connect_subscribe(subKey))
            {
                fd_rd_lock();
                handler_fd_map *hfMap = get_request_handler(subKey);
                struct event * pEvRead = event_new(base, hfMap->fd, EV_READ|EV_PERSIST, recvRequest_callback, this); 
                hfMap->evEvent = pEvRead;
                fd_rw_unlock();
                event_add(pEvRead, NULL);
            }
         }
    }

    if((m_configureChange&c_master_subKey)==c_master_subKey)//change subsrcibe
    {
        vector<string> delVec;
        vector<string> addVec;
        auto it = m_subKeying.begin();
        for(it = m_subKeying.begin(); it != m_subKeying.end(); it++)
        {
            string subKey = *it;
            auto et = find(m_subKey.begin(),m_subKey.end(), subKey);
            if(et == m_subKey.end())//not find ,unsubsribe
            {
                delVec.push_back(subKey);
            }
        }

        for(it = m_subKey.begin(); it != m_subKey.end(); it++)
        {
            string subKey = *it;
            auto et = find(m_subKeying.begin(), m_subKeying.end(), subKey);
            if(et == m_subKeying.end())
            {
                addVec.push_back(subKey);
            }
        }

        //m_subkeying exit, m_subkey not exit , this means must the subkey must unsubscribe
        for(it = delVec.begin(); it != delVec.end(); it++)
        {
            string subKey = *it;
            fd_wr_lock();
            releaseConnectResource(subKey);
            fd_rw_unlock();
        }

        for(it = addVec.begin(); it != addVec.end(); it++)
        {
            string subKey = *it;
            cout <<"add subkey:"<<subKey<<endl; 
            fd_wr_lock();
            handler_fd_map *hfMap = zmq_connect_subscribe(subKey);
            if(hfMap)
            {
                struct event * pEvRead = event_new(base, hfMap->fd, EV_READ|EV_PERSIST, recvRequest_callback, this);  
                event_add(pEvRead, NULL);
                hfMap->evEvent = pEvRead;
            }
            fd_rw_unlock();
        }

        delVec.clear();
        addVec.clear();
    } 

    if(((m_configureChange&c_master_bcIP)==c_master_bcIP)|| ((m_configureChange&c_master_bcListenBidPort)==c_master_bcListenBidPort))
    {
        zmq_close(m_bidderRspHandler);
        event_del(m_bidderEvent);       
        m_bidderRspHandler = bindOrConnect(false, "tcp", ZMQ_ROUTER, m_bcIP.c_str(), m_bcListenBidderPort, &m_bidderFd);//client sub 
        m_bidderEvent = event_new(base, m_bidderFd, EV_READ|EV_PERSIST, recvBidder_callback, this);  
        event_add(m_bidderEvent, NULL);     
    }
}

redisContext* bcServ::connectToRedis()
{
	redisContext* context = redisConnect(m_redisIP.c_str(), m_redisPort);
	if(context == NULL) return NULL;
	if(context->err)
	{
		redisFree(context);
		cout <<"connectaRedis failure" << endl;
		return NULL;
	}

	cout << "connectaRedis success" << endl;
	return context;
}


void bcServ::worker_net_init()
{ 
    const int tastPoolSize = 10000;
    m_redisPool.connectorPool_init(m_redisIP.c_str(), m_redisPort,  m_redisConnectNum);
    m_threadPoolManger.Init(tastPoolSize, m_redisConnectNum, m_redisConnectNum);
    cout <<"worker net init success:" <<endl;
}

void bcServ::addSendUnitToList(void * data,unsigned int dataLen,int workIdx)
{
    BC_process_t* pro = NULL;
    char *sendFrame=NULL;
    int frameLen;
    int chanel;   
    int rc;
    try
    {
        workerList_rd_lock();
        pro = m_workerList.at(workIdx%m_workerNum);
        if(pro == NULL) return;
        chanel = pro->channel[0];
        workerList_rw_unlock();
        frameLen = dataLen;
        sendFrame= (char *)data;

        do
        {   
            rc = write(chanel, sendFrame, frameLen);
            if(rc>0)
            { 
            //    calSpeed();
                break;
            }
            
            workerList_rd_lock();
            pro = m_workerList.at((++workIdx)%m_workerNum);
            chanel = pro->channel[0]; 
            workerList_rw_unlock();
            
       }while(1);
      // delete[] sendFrame;
    }
    catch(std::out_of_range &err)
    {
        cerr<<err.what()<<"LINE:"<<__LINE__  <<" FILE:"<< __FILE__<<endl;
        delete[] sendFrame;
        addSendUnitToList(data, dataLen, 0);
    }
    catch(...)
    {
        cout <<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&"<<endl;
        throw;
    }
}

void bcServ::sendToWorker(char * buf,int bufLen)
{
    static int workerID = 0;
    try
    {
        workerList_rd_lock();
        unsigned idx = (++workerID)%m_workerNum;
        workerList_rw_unlock();
        addSendUnitToList(buf, bufLen, idx);
    }
    catch(...)
    {
        addSendUnitToList(buf, bufLen, 0);
        cout <<"sendToWorker exception!"<<endl;
        throw;
    }
}


static int bbb=0;
void bcServ::bidderHandler(char * pbuf,int size)
{
    CommonMessage commMsg;
    string rsp;
    BidderResponse bidRsp;
    string uuid;
    string bidID;
    string sockID;

    commMsg.ParseFromArray(pbuf, size);     
    rsp = commMsg.data();
    bidRsp.ParseFromString(rsp);
    uuid = bidRsp.id();
    bidID = bidRsp.bidderid();
 //       printf("bb:%d\n", ++bbb);
    if(uuid.empty())
    {
        printf("bidder:: uuid null");
        return;
    }
    time_t timep;
    time(&timep);

    string bidIDandTime=bidID+":" ;
    char timebuf[32] ={0};
    sprintf(timebuf, "<%d>", timep);
    bidIDandTime += timebuf;
    bcDataLock();
    auto bcDataIt = bcDataRecList.find(uuid);
    if(bcDataIt != bcDataRecList.end())
    {
         string bcDataUuid = bcDataIt->first;
         bcDataRec *bcData = bcDataIt->second;
         if(bcData)
         {  

              BCStatus status = bcData->status;
              if(status == BCStatus::BC_recvSub)//只处理这种情况
              {  
              //   printf("b:%d\n", ++bbb);
                    bcData->bidTime=getLonglongTime();
                    auto diff = bcData->bidTime-bcData->subTime;
                    if(diff>=30)
                    {
              //          printf("%s----b:%lld-----s:%lld\n", uuid.c_str(), bcData->bidTime, bcData->subTime);
                    }
                   bcData->status= BCStatus::BC_recvBidAndSub;
                   delete[] bcData->buf;
                   delete bcData;
                   bcDataRecList.erase(bcDataIt);  
                   bcDataUnlock();
                   int recvSize = size+2*UUID_SIZE_MAX+4;
                
                   char *recvBuf = new char[recvSize];
                   memset(recvBuf, 0x00, recvSize);
                   PUT_LONG(recvBuf ,recvSize-4);
                   memcpy(recvBuf+4, uuid.c_str(), uuid.size());
                   memcpy(recvBuf+UUID_SIZE_MAX+4, bidIDandTime.c_str(), bidIDandTime.size());
                   memcpy(recvBuf+2*UUID_SIZE_MAX+4,pbuf, size);
    
                  //send to worker
                   sendToWorker(recvBuf, recvSize);
                   delete[] recvBuf;

                   redisDataRec *redis = new redisDataRec;
                   if(redis)
                   {  
                       redis->bidIDAndTime = bidIDandTime;
                       redis->time = timep;
                       delDataLock();
                       redisRecList.insert(pair<string, redisDataRec*>(uuid, redis));
                       delDataUnlock();
                   } 
               }
               else if(status == BCStatus::BC_recvExpire)
               {
                   bcData->status= BCStatus::BC_recvBidAndExp;
                   bcDataUnlock();   
               }
               else if(status == BCStatus::BC_recvSubAndExp)
               {
                    delete[] bcData->buf;
                    delete bcData;
                    bcDataRecList.erase(bcDataIt);
                    bcDataUnlock();
               }
               else
               {
                  bcDataUnlock();
               }
         
          }
          else
          {
              bcDataRecList.erase(bcDataIt);
              bcDataUnlock();
          }
     }
     else
     {
         //      printf("b:%d\n", ++bbb);
             bcDataRec *bData = new bcDataRec;
             time(&timep);
             bData->status = BCStatus::BC_recvBid;
             bData->buf = new char[size];
             memcpy(bData->buf, pbuf, size);
             bData->bufSize = size;
             bData->bidTime=getLonglongTime();
             bData->subTime = 0;
             bData->time = timep;
             bData->bidIDAndTime = bidIDandTime;
             bcDataRecList.insert(pair<string, bcDataRec*>(uuid, bData)); 
             bcDataUnlock();           
     }   
}


static int iiii=0;
static int jjjj = 0;
static int kkkk = 0;

void bcServ::subVastHandler(char * pbuf,int size)
{                  
    CommonMessage commMsg;
    VastRequest vastReq;

    commMsg.ParseFromArray(pbuf, size);
    string busiCode = commMsg.businesscode();
    string data = commMsg.data();
    vastReq.ParseFromString(data);
    string uuid = vastReq.id();
   // printf("uuid==:%s\n", uuid.c_str());
    time_t timep;
    time(&timep);
  //     printf("ii:%d\n", ++iiii);
    if(uuid.empty())
    {
        printf("sub::uuid null");
        return;
    }
    
    bcDataLock();
    auto bcDataIt = bcDataRecList.find(uuid);
    if(bcDataIt != bcDataRecList.end())//存在
    {
        bcDataRec *bcData = bcDataIt->second;
        if(bcData)
        {
            BCStatus bcStatus = bcData->status;
            string bcDataBidIdAndTime =  bcData->bidIDAndTime;

            if(bcStatus == BCStatus::BC_recvBid)
            {
            //    printf("jj:%d\n", ++jjjj);
                bcData->subTime = getLonglongTime();
                auto diff = bcData->subTime- bcData->bidTime;
                if(diff>=30)
                {
                 //   printf("%s----s:%lld-----b:%lld\n", uuid.c_str(), bcData->subTime, bcData->bidTime);
                }
                int recvSize = bcData->bufSize+2*UUID_SIZE_MAX+4;
                char *pptr = bcData->buf;   
                delete bcData;
                bcDataRecList.erase(bcDataIt);
                bcDataUnlock();   
                
                char *recvBuf = new char[recvSize];
                memset(recvBuf, 0x00, recvSize);
                PUT_LONG(recvBuf,recvSize-4);
                memcpy(recvBuf+4, uuid.c_str(), uuid.size());
                memcpy(recvBuf+UUID_SIZE_MAX+4, bcDataBidIdAndTime.c_str(), bcDataBidIdAndTime.size());
                memcpy(recvBuf+2*UUID_SIZE_MAX+4, bcData->buf, bcData->bufSize);

                //send to worker
                sendToWorker(recvBuf, recvSize);
                delete[] pptr;
                delete[] recvBuf;

                redisDataRec *redis = new redisDataRec;
                if(redis)
                {  
                   redis->bidIDAndTime = bcDataBidIdAndTime;
                   redis->time = timep;
                   delDataLock();
                   redisRecList.insert(pair<string, redisDataRec*>(uuid, redis));
                   delDataUnlock();
                } 
            }
            else if(bcStatus == BCStatus::BC_recvExpire)
            { 
          //  printf("3c33\n");
               bcData->status = BCStatus::BC_recvSubAndExp;
               bcDataUnlock();
            }
            else if(bcStatus == BCStatus::BC_recvBidAndExp)
            {
           //     printf("aaaa\n");

                delete[] bcData->buf;
                delete bcData;
                bcDataRecList.erase(bcDataIt);
                bcDataUnlock();   
            }
            else
            {
          //      cout<<uuid<<"=========="<<bcDataIt->first<<endl;
         //   printf("***:%d---%s   %d, %d,%d\n", bcStatus, uuid.c_str(), bcData->subTime, bcData->bidTime, bcData->expTime);
                  delete[] bcData->buf;
                  delete bcData;
                  bcDataRecList.erase(bcDataIt);
                  bcDataUnlock();     
            }
         }
         else
         {
             delete[] bcData->buf;
             delete bcData;
             bcDataRecList.erase(bcDataIt);
             bcDataUnlock();
         }
     }
     else
     {
    //     printf("kk:%d\n", ++kkkk);

        time_t timep;
        time(&timep);
        bcDataRec *bData = new bcDataRec;
        bData->status = BCStatus::BC_recvSub;
        bData->buf = NULL;
        bData->bufSize = 0;
        bData->subTime = getLonglongTime();
        bData->bidTime = 0;
        bData->time = timep;
        bcDataRecList.insert(pair<string, bcDataRec*>(uuid, bData));
        bcDataUnlock();
     }
}

void bcServ::subHandler(char * pbuf,int size)
{                  
   // CommonMessage commMsg;

   // commMsg.ParseFromString(pbuf);
 //   string busiCode = commMsg.businesscode();
 //   if(busiCode == BUSI_VAST)
    {
        subVastHandler(pbuf, size);
    }
  //  else if(busiCode == BUSI_EXP)
    {
    //    subExpHandler(pbuf,size);
    }
}



void bcServ::subExpHandler(char * pbuf,int size)
{   
     time_t timep;
     time(&timep);

     CommonMessage commMsg;
     commMsg.ParseFromArray(pbuf, size);
     string rsp = commMsg.data();
     ExpiredMessage expireMsg;
     expireMsg.ParseFromString(rsp);
     string uuid = expireMsg.uuid();
     string expiredata = expireMsg.status();                                 

     bcDataLock();
     auto bcDataIt = bcDataRecList.find(uuid);
     if(bcDataIt != bcDataRecList.end())
     {
          bcDataRec *bcData = bcDataIt->second;
          if(bcData)
          {
                BCStatus status = bcData->status;
                if(status == BCStatus::BC_recvBid)
                {
                    bcData->status = BCStatus::BC_recvBidAndExp;
                }
                else if(status == BCStatus::BC_recvSub)
                {
                    bcData->status = BCStatus::BC_recvSubAndExp;
                }
                else if(status == BCStatus::BC_recvBidAndSub)
                {
                    delete[] bcData->buf;
                    delete bcData;
                    bcDataRecList.erase(bcDataIt);
                }
                else
                {
                    bcDataRecList.erase(bcDataIt);
                }
          }
          bcDataUnlock();

      }
      else
      {
        //   cout <<"exception expTimeout first come"<<m_test_expTimeout++<<endl;
           bcDataRec *bData = new bcDataRec;
           time(&timep);
           bData->status = BCStatus::BC_recvExpire;
           bData->buf = nullptr;
           bData->time = timep;
           bcDataRecList.insert(pair<string, bcDataRec*>(uuid, bData));    
           bcDataUnlock();
      }
}


void bcServ::recvRequest_callback(int fd, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len=sizeof(int);
    char buf[BUFSIZE];

    bcServ *serv = (bcServ*)pair;
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
        return;
    }

    len = sizeof(events);
    void *handler = serv->get_request_handler(serv, fd);
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        cout <<"adrsp is invalid" <<endl;
        return;
    }

    if(events & ZMQ_POLLIN)
    {
        while (1)
        {
            int recvLen = zmq_recv(handler, buf, sizeof(buf), ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                break;
            }
      
            recvLen = zmq_recv(handler, buf, sizeof(buf), ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                break;
            }  
            if(recvLen)
            {
                serv->subHandler(buf,recvLen);
            }  
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
            cout << "<" << addr << "," << port << ">" << "can not "<< ((client)? "connect":"bind")<< " this port:"<<zmq_strerror(zmq_errno())<<endl;
            zmq_close(handler);
            return nullptr;     
        }
    
        if(fd != nullptr&& handler!=nullptr)
        {
            size = sizeof(int);
            rc = zmq_getsockopt(handler, ZMQ_FD, fd, &size);
            if(rc != 0 )
            {
                cout<< "<" << addr << "," << port << ">::" <<"ZMQ_FD faliure::" <<zmq_strerror(zmq_errno())<<"::"<<endl;
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
                    cout <<"try to connect to MAX"<<endl;
                    system("killall throttle");     
                    exit(1);
               }
               cout <<"<"<<addr<<","<<port<<">"<<((client)? "connect":"bind")<<"failure"<<endl;
               sleep(1);
           }
        }while(zmqHandler == nullptr);
    
        cout << "<" << addr << "," << port << ">" << ((client)? "connect":"bind")<< " to this port:"<<endl;
        return zmqHandler;
}



void bcServ::configureUpdate(int fd, short __, void *pair)
{
    eventParam *param = (eventParam *) pair;
    bcServ *serv = (bcServ*)param->serv;
    char buf[1024]={0};
    int rc = read(fd, buf, sizeof(buf));
    cout<<buf<<endl;
    if(memcmp(buf,"event", 5) == 0)
    {
        cout <<"update connect:"<<getpid()<<endl;
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
        cout <<"sucsribe "<< subkey <<" success"<<endl;
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
    cout <<"adVast bind success" <<endl;
    
    m_bidderRspHandler = bindOrConnect(false, "tcp", ZMQ_ROUTER, m_bcIP.c_str(), m_bcListenBidderPort, &m_bidderFd);//client sub 
    int   rc = zmq_setsockopt(m_bidderRspHandler, ZMQ_RCVHWM, &hwm, sizeof(hwm));
    
    cout <<"bidder rsp bind success" <<endl;
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

      serv->fd_rd_lock();
      auto it = serv->m_handler_fd_map.begin();
      for(it = serv->m_handler_fd_map.begin(); it != serv->m_handler_fd_map.end(); it++)
      {
         handler_fd_map *hfMap = *it;
         if(hfMap== NULL)continue;
         struct event * pEvRead = event_new(base, hfMap->fd, EV_READ|EV_PERSIST, recvRequest_callback, arg);  
         event_add(pEvRead, NULL);
         hfMap->evEvent = pEvRead;
      }
      serv->fd_rw_unlock();
      event_base_dispatch(base);
      return NULL;  
}



void bcServ::recvBidder_callback(int fd, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len=sizeof(int);
    char buf[BUFSIZE];

    bcServ *serv = (bcServ*)pair;
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
        return;
    }

    len = sizeof(events);
    void *handler = serv->m_bidderRspHandler;
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        cout <<"adrsp is invalid" <<endl;
        return;
    }
   
    if(events & ZMQ_POLLIN)
    {
        while (1)
        {
            int recvLen = zmq_recv(handler, buf, sizeof(buf), ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                break;
            }
                  memset(buf, 0, sizeof(buf));
            recvLen = zmq_recv(handler, buf, sizeof(buf), ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                break;
            }  
   
            if(recvLen)
            {
                serv->bidderHandler(buf, recvLen);
            }
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

void bcServ::get_overtime_recond(map<string, string>& delMap)
{
    time_t timep;
    time(&timep);
    delDataLock();
    auto redisIt = redisRecList.begin();  
    for(redisIt = redisRecList.begin();redisIt != redisRecList.end(); )
    {  
        redisDataRec* redis = redisIt->second;  
    
        if(redis == NULL)
        {
            redisRecList.erase(redisIt++);
            continue;
        }
    
        string uuid = redisIt->first;
        string fieldSym = redis->bidIDAndTime;
        time_t timeout= redis->time;
    
        if(timep - timeout >= m_redisSaveTime)
        {
            delete redis;
            redisRecList.erase(redisIt++);
            delMap.insert(pair<string, string>(uuid, fieldSym));
        }
        else
        {
            redisIt++;   
        }   
    }   
    delDataUnlock();

}

void bcServ::delete_invalid_recond()
{
    time_t timep;
    time(&timep);

    bcDataLock();
    auto bcDataIt = bcDataRecList.begin();  
    for(bcDataIt = bcDataRecList.begin();bcDataIt != bcDataRecList.end(); )
    {  
        bcDataRec* bcData = bcDataIt->second;  
    
        if(bcData == NULL)
        {
            bcDataRecList.erase(bcDataIt++);
            continue;
        }
     
        time_t tm= bcData->time;
        if(timep - tm >= 10)
        {
             if(bcData->subTime==0)
             {
                static int lost_vast_num = 0;
		lost_vast_num++;
		cout<<"s:"<<lost_vast_num<<endl;
             }
	     else if(bcData->bidTime==0)
	     {
		static int lost_bid_num=0;
		lost_bid_num++;
		cout<<"b:"<<lost_bid_num<<endl;
	     }
	     else
	     {
            	cout <<"del "<<bcDataIt->first<<" bid:"<<bcData->bidTime<< " sub: " << bcData->subTime<< " status:"<<(int)bcData->status<<endl;
	     }
            if(bcData->buf) delete[] bcData->buf;
            delete bcData;    
            bcDataRecList.erase(bcDataIt++);
        }
        else
        {
            bcDataIt++;   
        }
    }     
    bcDataUnlock();

}

void bcServ::delete_redis_data(map<string, string>& delMap)
{
    while(delMap.size())
    {
        map<string, string>::iterator delIt;
        int index = 0;
     
        for(delIt= delMap.begin(); delIt != delMap.end();)
        {
    
            string uuid = delIt->first;
            string field = delIt->second; 
            redisAppendCommand(m_delContext, "hdel %s %s", uuid.c_str(), field.c_str());
            index++;
            if(index >=2000) break; // 一个管道1000个    
            delMap.erase(delIt++);
        }
    
        int i;
        for(i = 0;i < index; i++)
        {
            redisReply *ply;
            redisGetReply(m_delContext, (void **) &ply);
            if(ply!=nullptr)
            {
                freeReplyObject(ply);    
            }
            else
            {
                redisFree(m_delContext);
                m_delContext = connectToRedis();
                if(m_delContext == nullptr)
                {
                    cout << "error::deleteRedisData can not connect redis" << endl;
                    sleep(1);
                }
            }
        } 
    }

}

void* bcServ::deleteRedisDataHandle(void *arg)
{   
    bcServ *serv = (bcServ*) arg;
    map<string, string> delMap; 
    
    do
    {
        serv->m_delContext = serv->connectToRedis();
        if(serv->m_delContext == nullptr)
        {
            cout << "error::deleteRedisData can not connect redis" << endl;
            sleep(1);
        }
    }while(serv->m_delContext == nullptr);

    while(1)
    {
     try{

            sleep(serv->m_redisSaveTime);
            serv->get_overtime_recond(delMap);
            serv->delete_redis_data(delMap);
            serv->delete_invalid_recond();
        }
        catch(...)
        {
             serv->bcDataUnlock();
             serv->delDataUnlock();
             cout << "delRedisItem exception" << endl;
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
                cout <<pid << "---exit" << endl;
                updataWorkerList(pid);
            }

            if (srv_graceful_end || srv_ungraceful_end)//CTRL+C ,killall throttle
            {
                cout <<"srv_graceful_end"<<endl;
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
                cout <<"srv_restart"<<endl;
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
                cout <<"master progress recv sigusr1"<<endl;
    
                
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
               
                      //cout <<"pro->pid:" << pro->pid<<"::"<< getpid()<<endl;
                      if(((m_configureChange&c_master_bcIP)==c_master_bcIP)||((m_configureChange&c_master_bcListenBidPort)==c_master_bcListenBidPort) )
                      {
                           kill(pro->pid, SIGKILL);   
                      }
                      else
                      {
                          if(count >= m_workerNum)
                          {
                               kill(pro->pid, SIGKILL); 
                          }
                          else
                          {
                               kill(pro->pid, SIGUSR1); 
                          }
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
                        workerList_wr_lock();
				        m_workerList.push_back(pro); 
                        workerList_rw_unlock();
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
                        cout<<"+++++++++++++++++++++++++++++++++++"<<endl;
                    }
                    break;
                    case 0:
                    {  
                        /*
                        cpu_set_t mask;
                        CPU_ZERO(&mask);
                        CPU_SET(workIndex, &mask);
                        sched_setaffinity(0, sizeof(cpu_set_t), &mask);    
                        closeZmqHandlerResource();*/
                        close(m_vastEventFd[0]);
                        close(m_vastEventFd[1]);
                        close(pro->channel[0]);
                        m_workerChannel = pro->channel[1];
                        int flags = fcntl(m_workerChannel, F_GETFL);
                        fcntl(m_workerChannel, F_SETFL, flags | O_NONBLOCK); 
                        is_child = 1; 
                        pro->pid = getpid();
                        cout<<"worker restart"<<endl;
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
        cout << "master exit"<<endl;
        exit(0);    
    }
    start_worker();
}





