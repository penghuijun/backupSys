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
#include "expireddata.pb.h"
#include "MobileAdRequest.pb.h"
#include "throttle.h"
#include "hiredis.h"
#include "threadpoolmanager.h"

using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;

sig_atomic_t srv_graceful_end = 0;
sig_atomic_t srv_ungraceful_end = 0;
sig_atomic_t srv_restart = 0;
sig_atomic_t sigalrm_recved = 0;;
sig_atomic_t sigusr1_recved = 0;;


#ifdef DEBUG
struct timeval tm;
extern pthread_mutex_t tmMutex;
unsigned long long getLonglongTime()
{
    pthread_mutex_lock(&tmMutex);
    unsigned long long t = tm.tv_sec*1000+tm.tv_usec/1000;
    pthread_mutex_unlock(&tmMutex); 
    return t;
}
#endif

void throttleServ::readConfigFile()
{
    if(m_throConfChange != 0)
    {
        cout <<"can not update configuration now, change the state of throttle or restart:"<<m_throConfChange<<endl;
        return ;
    }
    m_config.readConfig();
    string throIP = m_config.get_throttleIP();
    if(throIP != m_throttleIP) 
    {
        m_throConfChange |= c_master_throttleIP;
        m_throttleIP = throIP;
    }
   
    unsigned short port  = m_config.get_throttleAdPort();
    cout <<port<<":"<< m_throttleAdPort<<endl;
    if(m_throttleAdPort != port)
    {
            m_throConfChange |= c_master_adPort;
            m_throttleAdPort = port;
    }

    port = m_config.get_throttlePubPort();
    if(m_publishPort != port)
    {
            m_throConfChange |= c_master_publishVast;
            m_publishPort = port;
    }  

    unsigned short value = m_config.get_throttleworkerNum();
    if(m_workerNum!= value)
    {
            m_throConfChange |= c_workerNum;
            m_workerNum = value;
    } 

    cout <<m_throConfChange<<endl;
}

throttleServ::throttleServ(throttleConfig &config):m_config(config)
{
    pthread_mutex_init(&uuidListMutex, NULL);
    pthread_mutex_init(&workerListMutex, NULL);

    m_throttleIP = config.get_throttleIP();   
    m_throttleAdPort  = config.get_throttleAdPort();
    m_publishPort = config.get_throttlePubPort();  
    m_workerNum = config.get_throttleworkerNum();
    m_vastBusiCode = config.get_vastBusiCode();
    m_mobileBusiCode = config.get_mobileBusiCode();
    m_publishPipeNum = config.get_pubPipeNum();

	auto i = 0;
	for(i = 0; i < m_workerNum; i++)
	{
		BC_process_t *pro = new BC_process_t;
		if(pro == NULL) return;
		pro->pid = 0;
		pro->status = PRO_INIT;
        pro->channel[0]=pro->channel[1]=-1;
		m_workerList.push_back(pro);
	};
    m_throConfChange = 0;	
}


void throttleServ::sendToWorker(char * buf,int bufLen)
{
    try
    {
        unsigned int sendId = m_sendToWorkerID++;
        workerListLock();
        unsigned idx = sendId%m_workerNum;
        workerListUnlock();
        addSendUnitToList(buf, bufLen, idx, sendId);
    }
    catch(...)
    {
        unsigned int sendId = m_sendToWorkerID++;
        addSendUnitToList(buf, bufLen, 0, sendId);
        cout <<"sendToWorker exception!"<<endl;
        throw;
    }
}

void throttleServ::addSendUnitToList(void * data,unsigned int dataLen,int workIdx,unsigned int sendID)
{
    BC_process_t* pro = NULL;
    char *sendFrame=NULL;
    int frameLen;
    int chanel;   
    int sendTimes = 0;
    try
    {
        workerListLock();
        pro = m_workerList.at(workIdx%m_workerNum);
        if(pro == NULL) return;
        chanel = pro->channel[0];
        workerListUnlock();
        
        frameLen = dataLen+4;
        sendFrame = new char[frameLen];
        PUT_LONG(sendFrame, dataLen);

        memcpy(sendFrame+4, data, dataLen);
        do
        {   
            int rc = write(chanel, sendFrame, frameLen);
         
            if(rc>0)
            {
                break;
            }
        //    if(sendTimes++>=1000) break;
            workerListLock();
            pro = m_workerList.at((workIdx++)%m_workerNum);
            chanel = pro->channel[0]; 
            workerListUnlock();
       }while(1);
       delete[] sendFrame;
    }
    catch(std::out_of_range &err)
    {
        cerr<<err.what()<<"LINE:"<<__LINE__  <<" FILE:"<< __FILE__<<endl;
        delete[] sendFrame;
        addSendUnitToList(data, dataLen, 0, sendID);
    }
    catch(...)
    {

        cout <<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&"<<endl;
        throw;
    }
}

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

void throttleServ::recvAD_callback(int _, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len;

    throttleServ *serv = (throttleServ*) pair;
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
        return;
    }

    len = sizeof(events);
    void *adrsp = serv->getAdRspHandler();
    int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {

            cout <<"adrsp is invalid" <<endl;
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
                 serv->sendToWorker(msg_data, recvLen);
             }
             zmq_msg_close(&part);
        }
    }
}


void throttleServ::recvFromWorker_cb(int fd, short event, void *pair)
{
    static int sentTbcTimes=0;
    zmq_msg_t msg;
    uint32_t events;
    int len;
    const int datalenSize=4;
    char buf[datalenSize];
    char publish[PUBLISHKEYLEN_MAX];
    char uuid[PUBLISHKEYLEN_MAX];
    string adUUID;

    throttleServ *serv = (throttleServ*) pair;
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
        return;
    }


    int dataLen = 0;
    while(1)
    {
        len = read(fd, buf, sizeof(buf));
        if(len != datalenSize)
        {
         //   cout <<"err len:" << len<<endl;
            return;
        }
        dataLen = GET_LONG(buf);

        char *data_buf = new char[dataLen];
        len = read(fd, data_buf, dataLen);
        if(len <= 0)
        {
             delete[] data_buf;
             return;
        }
        int idx = len;
        while(idx < dataLen) 
        {
            cout<<"recvFromWorker_cb data exception:"<<dataLen <<"  idx:"<<idx<<endl;
            len = read(fd, data_buf+idx, dataLen-idx);
            if(len <= 0)
            {
                 delete[] data_buf;
                 return;
            }
            idx += len;
        }

        serv->speed();
        memset(publish, 0, sizeof(publish));
        memcpy(publish, data_buf, sizeof(publish));
        dataLen -= PUBLISHKEYLEN_MAX;
        void *pubVastHandler = serv->getPublishVastHandler();
        zmq_send(pubVastHandler, publish, strlen(publish), ZMQ_SNDMORE);
        zmq_send(pubVastHandler, data_buf+PUBLISHKEYLEN_MAX, dataLen, 0);
        delete[] data_buf;
    }
   
}

void throttleServ::listenEventState(int fd, short __, void *pair)
{
    eventParam *param = (eventParam *) pair;
    throttleServ *serv = (throttleServ*)param->serv;
    char buf[1024]={0};
    int rc = read(fd, buf, sizeof(buf));
    if(rc == -1) return;
    cout<<"state:"<<buf<<endl;
    if(memcmp(buf,"event", 5) == 0)
    {
        cout <<"update connect:"<<getpid()<<endl;
        serv->masterRestart(param->base);
    }    
 
    if(memcmp(buf,"delworker", 9) == 0)
    {
        cout <<"delworker:"<<getpid()<<endl;
        serv->listenWorkerEvent(param->base);
    }    
}

void *throttleServ::recvFromWorker(void *throttle)
{
    throttleServ *serv = (throttleServ*) throttle;

    struct event_base* base = event_base_new(); 
    eventParam *param = new eventParam;
    param->base = base;
    param->serv = serv;
    int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, serv->m_listenWorkerNumFd);
    struct event * pEvRead = event_new(base, serv->m_listenWorkerNumFd[0], EV_READ|EV_PERSIST, listenEventState, param); 
    event_add(pEvRead, NULL);
    serv->listenWorkerEvent(base);
    event_base_dispatch(base);
}

void *throttleServ::listenVastEvent(void *throttle)
{
      throttleServ *serv = (throttleServ*) throttle;

      struct event_base* base = event_base_new(); 
      eventParam *param = new eventParam;
      param->base = base;
      param->serv = serv;
      int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, serv->m_eventFd);
      struct event * pEvRead = event_new(base, serv->m_eventFd[0], EV_READ|EV_PERSIST, listenEventState, param); 
      event_add(pEvRead, NULL);
      serv->m_adEvent = event_new(base, serv->m_adFd, EV_READ|EV_PERSIST, recvAD_callback, throttle); 
      event_add(serv->m_adEvent, NULL);

      event_base_dispatch(base);
      return NULL;  
}


static uint64_t FNV_64_INIT = UINT64_C(0xcbf29ce484222325);
static uint64_t FNV_64_PRIME = UINT64_C(0x100000001b3);

uint32_t
hash_fnv1a_64(const char *key, size_t key_length)
{
    uint32_t hash = (uint32_t) FNV_64_INIT;
    size_t x;

    for (x = 0; x < key_length; x++) {
      uint32_t val = (uint32_t)key[x];
      hash ^= val;
      hash *= (uint32_t) FNV_64_PRIME;
    }

    return hash;
}

uint32_t get_publish_pipe_index(const char* uuid)
{
    const int keyLen=32;
    char key[keyLen];
    for(int i= 0; i < keyLen; i++)
    {
        char ch = *(uuid+i);
        if(ch != '-') key[i]=ch;
    }
    uint32_t secret = hash_fnv1a_64(key, keyLen);
    return secret;
}

void throttleServ::worker_recvFrom_master_cb(int fd, short event, void *pair)
{
    ostringstream os;
    eventCallBackParam *param = (eventCallBackParam*) pair;
    if(param==NULL) 
    {
        cout<<"param is nullptr"<<endl;
        return;
    }
    throttleServ *serv = (throttleServ*) param->serv;
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
        return;
    }

    char buf[4096];
    int len;
    int dataLen;
    while(1)
    {
       len = read(fd, buf, 4);
       if(len == -1)
       {
          return;
       }
   
       if(len !=4) break;
       dataLen = GET_LONG(buf); 

       char *data_buf = new char[dataLen];
       len = read(fd, data_buf, dataLen);
       if(len <= 0)
       {
            delete[] data_buf;
            return;
       }
       int idx = len;
       while(idx < dataLen) 
       {
           cout<<"recv data exception:"<<dataLen <<"  idx:"<<idx<<endl;
           len = read(fd, data_buf+idx, dataLen-idx);
           if(len <= 0)
           {
                delete[] data_buf;
                return;
           }
           idx += len;
       }

      CommonMessage commMsg;
      commMsg.ParseFromArray(data_buf, dataLen);     
      string vastStr=commMsg.data();
      string tbusinessCode = commMsg.businesscode();
      string tcountry;
      string tlanguage;
      string tos;
      string tbrowser;
      string uuid;
      string publishKey;
             
     // cout<<"buisCode:"<< tbusinessCode<<"--"<<tbusinessCode.size()<<endl;
      if(tbusinessCode == serv->m_vastBusiCode)  
     {
         VastRequest vast;
         vast.ParseFromString(vastStr);
         uuid = vast.id();
         VastRequest_Device dev=vast.device();
         VastRequest_User user = vast.user();
         tcountry = user.countrycode();
         tlanguage = dev.lang();
         tos = dev.os();
         tbrowser = dev.browser();
      }
      else if(tbusinessCode == serv->m_mobileBusiCode)
      {
        MobileAdRequest mobile;
        mobile.ParseFromString(vastStr);
        uuid = mobile.id();
        MobileAdRequest_User user = mobile.user();
       // tcountry = user.country();
      }
      else
      {
         cout<<"businescode can  not anasis"<<endl;
         delete[] data_buf;
         return;
      }                       
    //  cout<<uuid<<endl;
     // printf("uuid:%s\n", uuid.c_str());
      uint32_t pipe_index = (get_publish_pipe_index(uuid.c_str())%serv->m_publishPipeNum);
     // cout <<"index:"<< pipe_index<<endl;
      os.str("");
      os <<tbusinessCode;//<<"-"<<tcountry;
      os <<"_";
      os << pipe_index;
      os <<"SUB";
      publishKey = os.str();
    
  //    cout<<publishKey<<endl;
      int tmpBufLen = PUBLISHKEYLEN_MAX+dataLen+sizeof(int);
      char *tmpBuf = new char[tmpBufLen];
      memset(tmpBuf, 0x00, tmpBufLen);
      PUT_LONG(tmpBuf, tmpBufLen-sizeof(int));
      memcpy(tmpBuf+sizeof(int), publishKey.c_str(), publishKey.size());
      memcpy(tmpBuf+PUBLISHKEYLEN_MAX+sizeof(int), data_buf, dataLen);  
 
      int rc = 0;
      do
      {
           rc = write(fd, tmpBuf, tmpBufLen); 
           if(rc > 0 ) break;
                        
      }while(1);
      delete[] data_buf; 
      delete[] tmpBuf; 
  }            
}


void throttleServ::hupSigHandler(int fd, short event, void *arg)
{
    cout <<"signal hup"<<endl;
    exit(0);
}


void throttleServ::intSigHandler(int fd, short event, void *arg)
{
    cout <<"signal int:"<<endl;
   usleep(100);
   exit(0);
}

void throttleServ::termSigHandler(int fd, short event, void *arg)
{
    cout <<"signal term"<<endl;
    exit(0);
}

void throttleServ::usr1SigHandler(int fd, short event, void *arg)
{
    cout <<"signal SIGUSR1"<<endl;
    eventCallBackParam *param = (eventCallBackParam *) arg;
    if(!param) return;
    throttleServ *serv = (throttleServ*) param->serv;
    if(serv)
    {
        serv->workerRestart(param);
    }
}

void* throttleServ::establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd)
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
        cout << "<" << addr << "," << port << ">" << "can not "<< ((client)? "connect":"bind")<< " this port:"<<"<"<<getpid()<<">:"<<zmq_strerror(zmq_errno())<<endl;
        zmq_close(handler);
        return nullptr;     
    }

    if(fd != nullptr&& handler!=nullptr)
    {
    	size = sizeof(int);
        rc = zmq_getsockopt(handler, ZMQ_FD, fd, &size);
        if(rc != 0 )
        {
            cout<< "<" << addr << "," << port << ">::" <<"ZMQ_FD faliure::" <<zmq_strerror(zmq_errno())<<endl;
            zmq_close(handler);
            return nullptr; 
        }

    }
    return handler;
}

void throttleServ::workerRestart(eventCallBackParam *param)
{
    m_config.readConfig();
    m_vastBusiCode = m_config.get_vastBusiCode();
    m_mobileBusiCode = m_config.get_mobileBusiCode();
}

#ifdef DEBUG
void *getTime(void *arg)
{
    throttleServ *serv = (throttleServ*) arg;
    while(1)
    {
        usleep(10*1000);
        pthread_mutex_lock(&tmMutex);
        gettimeofday(&tm, NULL);
        pthread_mutex_unlock(&tmMutex);
    }
}
#endif

void throttleServ::listenWorkerEvent(struct event_base* base)
{
    workerListLock();
    auto it1 = m_fdEvent.begin();
    for(it1 = m_fdEvent.begin(); it1 != m_fdEvent.end();)
    {
        fdEvent *ev = *it1;
        if(ev == NULL) continue;
        cout<<"ev:"<<ev->fd<<endl;
        auto etor1 = m_workerList.begin();
        for(etor1 = m_workerList.begin(); etor1 != m_workerList.end();etor1++)
        {
           BC_process_t *pro = *etor1;
           if(pro == NULL) continue;
           cout<<"===:"<<pro->channel[0]<<endl;
           if(ev->fd == pro->channel[0])break;
        }
    
        if(etor1 == m_workerList.end())
        {
              event_free(ev->evEvent);
              close(ev->fd); 
              it1 = m_fdEvent.erase(it1);
        }
        else
        {
              it1++;
        }
     } 

    
    auto it = m_workerList.begin();
    for(it = m_workerList.begin(); it != m_workerList.end();it++)
    {
        BC_process_t *pro = *it;
        if(pro == NULL) continue;
        auto etor = m_fdEvent.begin();
        for(etor = m_fdEvent.begin(); etor != m_fdEvent.end(); etor++)
        {
            fdEvent *ev = *etor;
            if(ev == NULL)continue;
            if(ev->fd == pro->channel[0])break;
        }

        if(etor == m_fdEvent.end())
        {
           struct event * pEvRead = event_new(base, pro->channel[0], EV_READ|EV_PERSIST, recvFromWorker_cb, this); 
           cout <<"new event:"<<pro->channel[0]<<endl;
           fdEvent *fd_event = new fdEvent;
           fd_event->fd = pro->channel[0];
           fd_event->evEvent = pEvRead;
           m_fdEvent.push_back(fd_event);
           event_add(pEvRead, NULL);
        }
      }   
 
      workerListUnlock();

}

void throttleServ::masterRestart(struct event_base* base)
{
    cout <<"masterRestart::m_throConfChange:"<<m_throConfChange<<endl;
    if(((m_throConfChange&c_master_throttleIP)==c_master_throttleIP) || (m_throConfChange&c_master_adPort)==c_master_adPort)
    {   
        zmq_close(m_adRspHandler); 
        event_del(m_adEvent);
        m_adRspHandler = bindOrConnect(false, "tcp", ZMQ_ROUTER, m_throttleIP.c_str(), m_throttleAdPort, &m_adFd);
        m_adEvent = event_new(base, m_adFd, EV_READ|EV_PERSIST, recvAD_callback, this); 
        event_add(m_adEvent, NULL);
        m_throConfChange &=(~(c_master_throttleIP|c_master_adPort));
        cout <<"advast reconnect to throttle success"<<endl;
    }

    if((m_throConfChange&c_master_publishVast)==c_master_publishVast)
    {
        zmq_close(m_publishHandler);    
        m_publishHandler = bindOrConnect(false, "tcp", ZMQ_PUB, "*", m_publishPort, nullptr);
        m_throConfChange &=(~c_master_publishVast);
        cout <<"publish reconnect to throttle success"<<endl;
    }

    if((m_throConfChange&c_workerNum)==c_workerNum)
    {
        m_throConfChange &=(~c_workerNum);
    }
    listenWorkerEvent(base);

}


bool throttleServ::masterRun()
{
      startNetworkResource();
    
      //ad thread, expire thread, poll thread
      pthread_t pth;
      pthread_t pthl;
      pthread_create(&pth, NULL, listenVastEvent, this);
      pthread_create(&pthl, NULL, recvFromWorker, this);//bucause socket pair is asyn ,if will send untill success , so need 2 pthread in case of dead while
      cout <<"==========================throttle serv start==========================="<<endl;
	  return true;
}



void* throttleServ::bindOrConnect( bool client,const char * transType,int zmqType,const char * addr,unsigned short port,int * fd)
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
 //               system("killall throttle");     
                exit(1);
           }
           cout <<"<"<<addr<<","<<port<<">"<<((client)? "connect":"bind")<<"failure"<<endl;
           sleep(1);
       }
    }while(zmqHandler == nullptr);

    cout << "<" << addr << "," << port << ">" << ((client)? "connect":"bind")<< " to this port:"<<endl;
    return zmqHandler;
}



void throttleServ::startNetworkResource()
{ 
    int hwm=3000;
    m_zmqContext = zmq_ctx_new();
    m_adRspHandler = bindOrConnect(false, "tcp", ZMQ_ROUTER, m_throttleIP.c_str(), m_throttleAdPort, &m_adFd);
    cout <<"adVast bind success:" << endl;
    
    m_publishHandler= bindOrConnect(false, "tcp", ZMQ_PUB, "*", m_publishPort, nullptr);
    int rc = zmq_setsockopt(m_publishHandler, ZMQ_RCVHWM, &hwm, sizeof(hwm));
    if(rc!=0)cerr<<"ZMQ_RCVHWM failure"<<endl;
    cout <<"bind publish service success:" << endl;
}

void throttleServ::signal_handler(int signo)
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
    }
}


void throttleServ::start_worker()
{
    eventCallBackParam *param= new eventCallBackParam;
    
    struct event_base* base = event_base_new();    
    struct event * hup_event = evsignal_new(base, SIGHUP, hupSigHandler, param);
    struct event * int_event = evsignal_new(base, SIGINT, intSigHandler, param);
    struct event * term_event = evsignal_new(base, SIGTERM, termSigHandler, param);
    struct event * usr1_event = evsignal_new(base, SIGUSR1, usr1SigHandler, param);
    struct event *clientPullEvent = event_new(base, workChannel, EV_READ|EV_PERSIST, worker_recvFrom_master_cb, param);

    param->serv = this;
    param->ev_base = base;
    param->ev_clientPullEvent = clientPullEvent;
    event_add(clientPullEvent, NULL);
    evsignal_add(hup_event, NULL);
    evsignal_add(int_event, NULL);
    evsignal_add(term_event, NULL);
    evsignal_add(usr1_event, NULL);
    event_base_dispatch(base);
}

void throttleServ::updataWorkerList(pid_t pid)
{
    workerListLock();
    auto it = m_workerList.begin();
    for(it = m_workerList.begin(); it != m_workerList.end(); it++)
    {
        BC_process_t *pro = *it;
        if(pro == NULL) continue;
        if(pro->pid == pid) 
        {
            pro->pid = 0;
            pro->status = PRO_INIT;
         
            if(needRemoveWorker())
            {
                cout <<"reduce worker"<<endl;
                delete pro;
                m_workerList.erase(it);
                write(m_listenWorkerNumFd[1],"delworker", 9);
                break;
            }
        }
    }
    workerListUnlock();

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

    m_masterPid = getpid();
    
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
                worker_exit = true;   
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

      
            if (srv_restart)
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

            if(is_child==0 &&sigusr1_recved)
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
                        workerListLock();;
				        m_workerList.push_back(pro); 
                        workerListUnlock();
                    }
                }    
            }
 
        }
        int workIndex = 1;

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
                        cout<<"+++++++++++++++++++++++++++++++++++"<<endl;
                    }
                    break;
                    case 0:
                    {     
                        close(m_eventFd[0]);
                        close(m_eventFd[1]);
                        close(m_listenWorkerNumFd[0]);
                        close(m_listenWorkerNumFd[1]);
                        close(pro->channel[0]);
                        workChannel = pro->channel[1];
                        int flags = fcntl(workChannel, F_GETFL);
                        fcntl(workChannel, F_SETFL, flags | O_NONBLOCK); 
                        is_child = 1; 
                        cout<<"worker restart"<<endl;
                        pro->pid = getpid();
                    }
                    break;
                    default:
                    {
                        cout<<"new channel[0]:"<<pro->channel[0]<<endl;
                        close(pro->channel[1]);
                        int flags = fcntl(pro->channel[0], F_GETFL);
                        fcntl(pro->channel[0], F_SETFL, flags | O_NONBLOCK); 

                        flags = fcntl(m_eventFd[0], F_GETFL);
                        fcntl(m_eventFd[0], F_SETFL, flags | O_NONBLOCK); 
                        flags = fcntl(m_listenWorkerNumFd[0], F_GETFL);
                        fcntl(m_listenWorkerNumFd[0], F_SETFL, flags | O_NONBLOCK); 
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

        if(((is_child==0)&&worker_exit)||(is_child==0 &&sigusr1_recved))
        {
            cout<<"write event to m_eventFd[1]"<<endl;
            write(m_eventFd[1],"event", 5);
            worker_exit = false;
            sigusr1_recved = 0;
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

