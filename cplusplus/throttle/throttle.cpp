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
#include "throttle.h"
#include "hiredis.h"
#include "threadpoolmanager.h"

using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;
const char *businessCode_VAST="5.1";

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

    port = m_config.get_throttleExpirePort();
    if(m_throttleExpirePort != port)
    {
            m_throConfChange |= c_master_expPort;
            m_throttleExpirePort = port;
    }

    port = m_config.get_throttlePubPort();
    if(m_publishPort != port)
    {
            m_throConfChange |= c_master_publishVast;
            m_publishPort = port;
    }  

    port = m_config.get_throttlePubExpPort();
    if(m_publishExpirePort!= port)
    {
            m_throConfChange |= c_master_publishExp;
            m_publishExpirePort = port;
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
    m_throttleExpirePort = config.get_throttleExpirePort();
    m_publishPort = config.get_throttlePubPort();  
    m_publishExpirePort = config.get_throttlePubExpPort();
    m_workerNum = config.get_throttleworkerNum();

	auto i = 0;
	for(i = 0; i < m_workerNum; i++)
	{
		BC_process_t *pro = new BC_process_t;
		if(pro == NULL) return;
		pro->pid = 0;
		pro->status = PRO_INIT;
		m_workerList.push_back(pro);
	};
    m_throConfChange = 0xFFFFFFFF;	
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
    try
    {
        workerListLock();
        pro = m_workerList.at(workIdx%m_workerNum);
        if(pro == NULL) return;
        chanel = pro->channel[0];
        workerListUnlock();
        
        frameLen = dataLen+8;
        sendFrame = new char[frameLen];
        PUT_LONG(sendFrame, dataLen+4);
        PUT_LONG(sendFrame+4, sendID);

        memcpy(sendFrame+8, data, dataLen);
        do
        {   
            if(pro->status == PRO_KILLING)
            {
                usleep(100);
                continue;
            }
            while(chanel == 0)
            {
                workerListLock();
                pro = m_workerList.at((workIdx++)%m_workerNum);
                chanel = pro->channel[0]; 
                workerListUnlock();
                usleep(10);
            }
            int rc = write(chanel, sendFrame, frameLen);
         
            if(rc>0)
            {

                break;
            }
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

void recvAD_callback(int _, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len;
    char buf[BUFSIZE];

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

    len = sizeof(events);
    void *adrsp = serv->getAdRspHandler();
    int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {

            cout <<"adrsp is invalid" <<endl;
            return;
    }

    static int aaa=0;
    if ( events & ZMQ_POLLIN )
    {
        while (1)
        {
            int recvLen = zmq_recv(adrsp, buf, sizeof(buf), ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                break;
            }

            recvLen = zmq_recv(adrsp, buf, sizeof(buf), ZMQ_NOBLOCK);
      
            if ( recvLen == -1 )
            {
                break;
            }  
           // cout <<"sern to worker times="<<++aaa<<endl;
            serv->sendToWorker(buf, recvLen);
        }
    }
}

void recvExp_callback(int _, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len;
    char buf[BUFSIZE];

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
    void *expHandler = serv->getExpRspHandler();
    void *pubExpHandler = serv->getPublishExpHandler();

    len = sizeof(events);
    int rc = zmq_getsockopt(expHandler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        cout <<"adexp is invalid" <<endl;
        return;
    }
    
    if ( events & ZMQ_POLLIN )
    {
        while (1)
        {
            int recvLen = zmq_recv(expHandler, buf, sizeof(buf), ZMQ_NOBLOCK);
            if ( recvLen == -1 )
            {
                break;
            }
            cout << "exp recvlen:"<< recvLen<<endl;
            CommonMessage commMsg;
            commMsg.ParseFromArray(buf, recvLen);
            string rsp = commMsg.data();
            ExpiredMessage expireMsg;
                 
            expireMsg.ParseFromString(rsp);
            string uuid = expireMsg.uuid();
            string expiredata = expireMsg.status();
            string publishKey;
            
            if(serv->getPublishKey(uuid, publishKey))
            {
                  publishKey+="EXP";
                  zmq_send(pubExpHandler, publishKey.c_str(), publishKey.size(),ZMQ_SNDMORE);
                  zmq_send(pubExpHandler, buf, recvLen, 0);  
            }
        
        }
    }
}



void servPull_callback(int fd, short event, void *pair)
{
   // static int sentTbcTimes=0;
    zmq_msg_t msg;
    uint32_t events;
    int len;
    char buf[BUFSIZE];
    char publish[PUBLISHKEYLEN_MAX];
    char uuid[PUBLISHKEYLEN_MAX];
    string adUUID;

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

    //int chanel = serv->getWorkChannel();
   //     cout <<"fd:" << fd<<"channel:"<<chanel<<endl;
    int dataLen = 0;
    while(1)
    {
        len = read(fd, buf, 4);
        if(len == -1) 
        {
            break;
        }

        if(len == 0)
        {
      //      cout <<"{}{}}{}{}{}{}{}{}{}{}{}{}{}}{}{}{}{}}{}{}{}{}{}{}{}{}{}{}{}{}{}"<<endl;
            auto it = param->ev_servPullEventList.begin();
            for(it = param->ev_servPullEventList.begin(); it != param->ev_servPullEventList.end(); )
            {
                servReadWorkerEvent *workEvent = *it;
                if(workEvent==NULL)
                {
                    it++;
                    continue;
                }
                if(workEvent->fd == fd)
                {
                    event_del(workEvent->ev);
                    delete workEvent->ev;
                    delete workEvent;
                 //   close(fd);
                    it = param->ev_servPullEventList.erase(it);
                }
                else
                {
                    it++;
                }
            }
            bool rc;
            do
            {
                rc = serv->getWorkerChannelChanged();
                if(rc == false) usleep(1000);
            }while(rc==false);
         //   close(fd);
            serv->updateServWorkerEvent(pair);
            break;
        }

        if(serv->getWorkerChannelChanged()==true)
        {
            serv->updateServWorkerEvent(pair); 
        }
        if(len != 4)
        {
            cout <<"err len:" << len<<endl;
            break;
        }
        dataLen = GET_LONG(buf);

        len = read(fd, buf, dataLen);
        if(len != dataLen) 
        {
            cout <<"servPull_callback read channel exception!!!:"<<len<<":"<<dataLen<<endl;
            break;
        }
         
        unsigned int sendID = GET_LONG(buf);
        dataLen-=sizeof(int);
        memset(publish, 0, sizeof(publish));
        memcpy(publish, &buf[0]+sizeof(int), sizeof(publish));
        dataLen -= PUBLISHKEYLEN_MAX;
        memcpy(uuid, &buf[0]+PUBLISHKEYLEN_MAX+sizeof(int), sizeof(uuid));
        dataLen -= PUBLISHKEYLEN_MAX;
        adUUID = uuid;

        void *pubVastHandler = serv->getPublishVastHandler();
        zmq_send(pubVastHandler, publish, strlen(publish),0);
        zmq_send(pubVastHandler, &buf[0]+2*PUBLISHKEYLEN_MAX, dataLen, 0);   
        serv->uuidListInsert(adUUID, publish); 
      //  cout <<"send to bc:"<<dataLen<<endl;
    }
   
}
void event_pthread_cleanup(void *arg)
{
    cout <<"adRequest_cleanup begin"<<endl;

    if(arg == NULL)
    {
        return;
    }
    eventCallBackParam *param = (eventCallBackParam *) arg;
    if(param->ev_adEvent)
    {
        if(param->ev_adEvent)
        {
            event_del(param->ev_adEvent);
            delete param->ev_adEvent;
            param->ev_adEvent = NULL;
        }

        if(param->ev_expEvent)
        {
            event_del(param->ev_expEvent);
            delete param->ev_expEvent;
            param->ev_adEvent = NULL;
        }  
  
        if(param->ev_clientPullEvent)
        {
            event_del(param->ev_clientPullEvent);
            delete param->ev_clientPullEvent;
            param->ev_clientPullEvent = NULL;
        }          
    }

    if(param->ev_base)
    {
        event_base_loopexit(param->ev_base, NULL);
        event_base_free(param->ev_base);
    }

    delete param; 
    cout <<"adRequest_cleanup end"<<endl;
}

bool throttleServ::updateServWorkerEvent(void *pair)
{
    bool result = false;
    eventCallBackParam *param = (eventCallBackParam*)  pair;    
    workerListLock();
       auto workIt = m_workerList.begin();
       for( workIt = m_workerList.begin(); workIt != m_workerList.end(); workIt++)
       {
            BC_process_t *pro = *workIt;
            if(pro == NULL) continue;
          
            int readChannel = pro->channel[0];
            auto servIt = param->ev_servPullEventList.begin();
            for(servIt = param->ev_servPullEventList.begin(); servIt != param->ev_servPullEventList.end(); servIt++)
            {
                servReadWorkerEvent *rEvent =  *servIt;
                if(rEvent == NULL) continue;
                if(rEvent->fd == readChannel)
                {
                    break;
                }
            }

            if(servIt == param->ev_servPullEventList.end())//not listen readChannel
            {
                struct event * pServRead = event_new(param->ev_base, readChannel, EV_READ|EV_PERSIST, servPull_callback, pair);
                servReadWorkerEvent *eventR = new servReadWorkerEvent;
                eventR->fd = readChannel;
                eventR->ev = pServRead;
                param->ev_servPullEventList.push_back(eventR);
                event_add(pServRead, NULL);
                result = true;
            }
       }
    workerListUnlock();
    return result;
}

void *servRecvPollHandle(void *throttle)
{
    throttleServ *serv = (throttleServ*) throttle;
    eventCallBackParam *param= new eventCallBackParam;
    memset(param, 0x00, sizeof(eventCallBackParam));
    pthread_cleanup_push(event_pthread_cleanup, param);
    struct event_base* base = event_base_new();   

    vector<BC_process_t*> proVec = serv->getWorkerProList();

    auto it = proVec.begin();
    for(it = proVec.begin(); it != proVec.end();it++)
    {
        BC_process_t *pro = *it;
        if(pro == NULL) continue;
        struct event * pEvRead = event_new(base, pro->channel[0], EV_READ|EV_PERSIST, servPull_callback, param);
        servReadWorkerEvent *event = new servReadWorkerEvent;
        event->fd = pro->channel[0];
        cout <<"evnet pro channel:"<< pro->channel[0]<<endl;
        event->ev = pEvRead;
        param->ev_servPullEventList.push_back(event);
        event_add(pEvRead, NULL);
    }
    param->serv = throttle;
    param->ev_base = base;
    event_base_dispatch(base);
    pthread_cleanup_pop(0); 
	return NULL;
}


void *adRequestHandle(void *throttle)
{

      throttleServ *serv = (throttleServ*) throttle;
      int fd = serv->getAdfd();

      eventCallBackParam *param= NULL;
      param = new eventCallBackParam;
      memset(param, 0x00, sizeof(eventCallBackParam));

      pthread_cleanup_push(event_pthread_cleanup, param);
      struct event_base* base = event_base_new();    
      struct event * pEvRead = event_new(base, fd, EV_READ|EV_PERSIST, recvAD_callback, param);    

      param->serv = throttle;
      param->ev_base = base;
      param->ev_adEvent = pEvRead;
  
      event_add(pEvRead, NULL);
      event_base_dispatch(base);
      pthread_cleanup_pop(0); 
      return NULL;  
}
void *expireHandle(void *throttle)
{
    throttleServ *serv = (throttleServ*) throttle;
    int fd = serv->getExpFd();

    eventCallBackParam *param= new eventCallBackParam;
    memset(param, 0x00, sizeof(eventCallBackParam));
    
    pthread_cleanup_push(event_pthread_cleanup, param);
    struct event_base* base = event_base_new();       
    struct event * pExpRead = event_new(base, fd, EV_READ|EV_PERSIST, recvExp_callback, param);
    param->serv = throttle;
    param->ev_base = base;
    param->ev_expEvent = pExpRead;
    event_add(pExpRead, NULL);
    event_base_dispatch(base);
    pthread_cleanup_pop(0); 
	return NULL;
}
    
static int acbd = 0;
void clientPull_callback(int fd, short event, void *pair)
{
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
       char buf[1024];
       int len;
       int dataLen;
    while(1)
    {
    
       len = read(fd, buf, 4);
       if(len == -1)
       {
         // cout<<"+++++++++++++++++++++++++++++++++"<<endl;
          break;
       }
       if(len !=4) break;
       dataLen = GET_LONG(buf);      
       len = read(fd, buf, dataLen);
       if(len != dataLen) 
       {
           cout<<"recv data exception:"<<dataLen <<"len:"<<len<<endl;
           break;
       }
       
 //     cout <<"client pid:"<<getpid()<<"  cnt:"<<++acbd<<endl;
      static int adTimes=0;
      CommonMessage commMsg;
      commMsg.ParseFromArray(&buf[4], dataLen-4);     
      string vastStr=commMsg.data();
      string tbusinessCode = commMsg.businesscode();
      string tcountry;
      string tlanguage;
      string tos;
      string tbrowser;
      string uuid;
      string publishKey;
                   
                                         
      if(tbusinessCode == businessCode_VAST)
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
      else
      {
         cout << "this businessCode can not analysis:" << tbusinessCode << endl;
         return;
      }
                               
#ifdef DEBUG
                  //  cout << "bus:" << tbusinessCode << " ad uuid:" << uuid << "  times:" <<++adTimes<< endl;
                  //  cout <<"recv info::" << tbusinessCode << "  " << tcountry << "  " << tlanguage << "  " << tos << "  " << tbrowser << endl;
#endif
                      
      if(adTimes++%2==0)
      {
         publishKey = "5.1.1-CN";
      }
      else
      {
         publishKey = "5.1.1-US";  
      }

      int tmpBufLen = 2*PUBLISHKEYLEN_MAX+dataLen+sizeof(int);
      char *tmpBuf = new char[tmpBufLen];
      memset(tmpBuf, 0x00, tmpBufLen);
      PUT_LONG(&tmpBuf[0], tmpBufLen-4);
      memcpy(tmpBuf+sizeof(int), &buf[0], sizeof(int));
      memcpy(tmpBuf+2*sizeof(int), publishKey.c_str(), publishKey.size());
      memcpy(tmpBuf+PUBLISHKEYLEN_MAX+2*sizeof(int), uuid.c_str(), uuid.size());
      memcpy(tmpBuf+2*PUBLISHKEYLEN_MAX+2*sizeof(int), &buf[4], dataLen-sizeof(int));  
      int rc = 0;
      do
      {
            rc = write(fd, tmpBuf, tmpBufLen); 
             if(rc > 0 ) break;
                        
      }while(1);
      delete[] tmpBuf; 
  }            
}


static void
hupSigHandler(int fd, short event, void *arg)
{
    cout <<"signal hup"<<endl;
    exit(0);
}


static void
intSigHandler(int fd, short event, void *arg)
{
    cout <<"signal int:"<<endl;
 //   eventCallBackParam *param = (eventCallBackParam *) arg;
 //   throttleServ *serv = (throttleServ*) param->serv;
 //   if(serv)
  //  {
  //      sleep(1);
  //      serv->stopWorkerData(param);
  //  }
   usleep(100);
   exit(0);
}

static void
termSigHandler(int fd, short event, void *arg)
{
    cout <<"signal term"<<endl;
    exit(0);
}



static void
usr1SigHandler(int fd, short event, void *arg)
{
    cout <<"signal SIGUSR1"<<endl;
/*
    eventCallBackParam *param = (eventCallBackParam *) arg;
    throttleServ *serv = (throttleServ*) param->serv;
    if(serv)
    {
        serv->workerRestart(param);
    }*/
}


void throttleServ::workerHandler()
{  
    eventCallBackParam *param= new eventCallBackParam;
    
    struct event_base* base = event_base_new();    
    struct event * hup_event = evsignal_new(base, SIGHUP, hupSigHandler, param);
    struct event * int_event = evsignal_new(base, SIGINT, intSigHandler, param);
    struct event * term_event = evsignal_new(base, SIGTERM, termSigHandler, param);
    struct event * usr1_event = evsignal_new(base, SIGUSR1, usr1SigHandler, param);
    struct event *clientPullEvent = event_new(base, workChannel, EV_READ|EV_PERSIST, clientPull_callback, param);

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



void* throttleServ::establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd)
{
    ostringstream os;
    string pro;
    int rc;
    void *handler = nullptr;
    size_t size;
    int time = 0;

    int sockfd;
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
        sockfd = 0;
        int cnt = 1000;
        char buf[1024];
        
        zmq_setsockopt (handler, ZMQ_RCVHWM, &cnt, sizeof(cnt));
        zmq_setsockopt (handler, ZMQ_SNDHWM, &cnt, sizeof(cnt));
        
        int recvLen = zmq_recv(handler, buf, sizeof(buf), ZMQ_NOBLOCK);
       
        rc = zmq_getsockopt(handler, ZMQ_FD, &sockfd, &size);
        if(rc != 0 )
        {
            cout<< "<" << addr << "," << port << ">::" <<"ZMQ_FD faliure::" <<zmq_strerror(zmq_errno())<<"::"<<sockfd<<endl;
            zmq_close(handler);
            return nullptr; 
        }
        *fd = sockfd;
    }

    zmq_setsockopt (handler, ZMQ_LINGER, &time, sizeof(time));
    return handler;
}

void throttleServ::stopWorkerData(eventCallBackParam *param)
{
        close(workChannel);   
}

void throttleServ::workerRestart(eventCallBackParam *param)
{
    if(param == NULL) return;
    
    m_throConfChange = 0;
    readConfigFile();
    
    cout <<"workerRestart::m_throConfChange:"<<m_throConfChange<<endl;
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

void throttleServ::masterRestart()
{
    void *handler = NULL;
        int connectTimes = 0;
    const int connectMax=3;
    cout <<"masterRestart::m_throConfChange:"<<m_throConfChange<<endl;
    if((m_throConfChange&c_master_adPort)==c_master_adPort)
    {   
        int rc = zmq_close(m_adRspHandler); 
        cout <<"c_master_adPort:"<<rc<<endl;
        m_adRspHandler = NULL;  
        pthread_cancel(m_adVastPthread);
        connectTimes = 0;
       // handler = bindOrConnect(c_master_adPort, false, "tcp", ZMQ_ROUTER, m_throttleIP.c_str(), m_throttleAdPort, &m_adFd);
       // if(handler != NULL) m_adRspHandler = handler;
        do
        {
            m_adRspHandler = establishConnect(false, "tcp", ZMQ_ROUTER, m_throttleIP.c_str(), m_throttleAdPort, &m_adFd);
            if(m_adRspHandler == nullptr)
            {
                readConfigFile();
                sleep(1);
                if(connectTimes++ >= connectMax)
                {
                    cout <<"retry to connect to MAX"<<endl;
                    system("killall throttle");
                    exit(1);
                }
            }

        }while(m_adRspHandler == nullptr);
        cout <<"advast reconnect to throttle success"<<endl;
        pthread_create(&m_adVastPthread, NULL, adRequestHandle, this);
    }
       
    if((m_throConfChange&c_master_expPort)==c_master_expPort)
    {
        zmq_close(m_zmqExpireRspHandler); 
        cout <<"c_master_expPort"<<endl;
        m_zmqExpireRspHandler = NULL;

        pthread_cancel(m_adExpPthread);
        handler = bindOrConnect(c_master_expPort, false , "tcp", ZMQ_DEALER, m_throttleIP.c_str(), m_throttleExpirePort,&m_expFd);
        if(handler != NULL) m_zmqExpireRspHandler = handler;       
        cout <<"adExp reconnect to throttle success"<<endl;
        pthread_create(&m_adExpPthread, NULL, expireHandle, this);    
    }
    
    if((m_throConfChange&c_master_publishVast)==c_master_publishVast)
    {
        zmq_close(m_publishHandler);    
        m_publishHandler = NULL;  
       // handler = bindOrConnect(c_master_publishVast, false, "tcp", ZMQ_PUB, "*", m_publishPort, nullptr);
       // if(handler != NULL) m_publishHandler = handler;    
        connectTimes = 0;
        do
        {
           m_publishHandler = establishConnect(false, "tcp", ZMQ_PUB, "*", m_publishPort, nullptr);
           if(m_publishHandler == nullptr) 
           {
               readConfigFile();
               sleep(1);
           }
           if(connectTimes++ >= connectMax)
           {
                cout <<"retry to connect to MAX"<<endl;
                system("killall throttle");
                exit(1);
           }
        }while(m_publishHandler == nullptr);
        cout <<"publish vast reconnect to throttle success"<<endl;
    }

    if((m_throConfChange&c_master_publishExp)==c_master_publishExp)
    {
         zmq_close(m_publishExpireHandler);  
        // handler = bindOrConnect(c_master_publishExp, false, "tcp", ZMQ_PUB, "*", m_publishExpirePort, nullptr);
        // if(handler != NULL) m_publishExpireHandler = handler;
        connectTimes = 0;
         do
         {
      
            m_publishExpireHandler= establishConnect(false, "tcp", ZMQ_PUB, "*", m_publishExpirePort, nullptr);
            if(m_publishExpireHandler == nullptr)
            {
               readConfigFile();
               sleep(1);   
            }
            if(connectTimes++ >= connectMax)
            {
                cout <<"retry to connect to MAX"<<endl;
                system("killall throttle");
                exit(1);
            }
         }while(m_publishExpireHandler == nullptr);
         cout <<"publish expire reconnect to throttle success"<<endl;
    }

}


bool throttleServ::masterRun()
{
        int *ret;
        pthread_t worker;
    // pthread_create(&worker, NULL, getTime, this);
    // usleep(20);
        startNetworkResource();
    
      //ad thread, expire thread, poll thread
      pthread_create(&m_adVastPthread, NULL, adRequestHandle, this);
      pthread_create(&m_adExpPthread, NULL, expireHandle, this);
      pthread_create(&m_servPullPthread, NULL, servRecvPollHandle, this);
      cout <<"==========================throttle serv start==========================="<<endl;
      
   //   pthread_join(m_adVastPthread, (void **)&ret);
    //  pthread_join(m_adExpPthread, (void **)&ret);
    //  pthread_join(m_servPullPthread, (void **)&ret);
    //  pthread_join(worker, (void **)&ret);
	  return true;
}

void* throttleServ::bindOrConnect(unsigned int configure, bool client,const char * transType,int zmqType,const char * addr,unsigned short port,int * fd)
{
    int connectTimes = 0;
    const int connectMax=3;

    void *zmqHandler = NULL;
    if(m_throConfChange&configure==configure)
    {
        do
        {
            zmqHandler = establishConnect(client, transType, zmqType, addr, port, fd);
            if(zmqHandler == nullptr)
            {
                readConfigFile();
                sleep(1);
            }
         }while(zmqHandler == nullptr);
         if(connectTimes++ >= connectMax)
         {
              cout <<"try to connect to MAX"<<endl;
              system("killall throttle");
              
              exit(1);
         }
     }
    cout << "<" << addr << "," << port << ">" << ((client)? "connect":"bind")<< " to this port:"<<endl;
    return zmqHandler;
}

void throttleServ::startNetworkResource()
{ 
    m_zmqContext = zmq_ctx_new();
    void *handler;
    int connectTimes = 0;
    const int connectMax=3;
    cout <<"m_throConfChange="<<m_throConfChange<<endl;

    if(m_throConfChange&c_master_adPort==c_master_adPort)
    {
        connectTimes = 0;
        do
        {
            m_adRspHandler = establishConnect(false, "tcp", ZMQ_ROUTER, m_throttleIP.c_str(), m_throttleAdPort, &m_adFd);
            if(m_adRspHandler == nullptr)
            {
                readConfigFile();
                sleep(1);
            }

            if(connectTimes++ >= connectMax)
            {
                cout <<"try to connect to MAX"<<endl;
                system("killall throttle");
                exit(1);
            }
         }while(m_adRspHandler == nullptr);
     }
    cout <<"adVast bind success:" << endl;

        //PUBLISH KEY
     if(m_throConfChange&c_master_publishVast==c_master_publishVast)
     {
         connectTimes = 0;
         do
         {
             m_publishHandler = establishConnect(false, "tcp", ZMQ_PUB, "*", m_publishPort, nullptr);
             if(m_publishHandler == nullptr) 
             {
                 readConfigFile();
                sleep(1);
             }        
             if(connectTimes++ >= 2*connectMax)
             {
                 cout <<"try to connect to MAX"<<endl;
                 system("killall throttle");
                 exit(1);
             }

         }while(m_publishHandler == nullptr);
     }
     cout <<"bind publish service success:" << endl;

        //recv expire msg
     if(m_throConfChange&c_master_expPort == c_master_expPort)
     {
         connectTimes = 0;
         do
         {
             m_zmqExpireRspHandler = establishConnect(false, "tcp", ZMQ_DEALER, m_throttleIP.c_str(), m_throttleExpirePort,&m_expFd );
             if(m_zmqExpireRspHandler == nullptr)
             {
                 readConfigFile();
                sleep(1);
             }
             
             if(connectTimes++ >= connectMax)
             {
                 cout <<"try to connect to MAX"<<endl;
                 system("killall throttle");
                 exit(1);
             }

         }while(m_zmqExpireRspHandler == nullptr);
     }
     cout <<"expire bind success:" << endl;

//PUBLISH KEY
     if(m_throConfChange&c_master_publishExp== c_master_publishExp) 
     {
         connectTimes = 0;
         do
         {
      
            m_publishExpireHandler= establishConnect(false, "tcp", ZMQ_PUB, "*", m_publishExpirePort, nullptr);
            if(m_publishExpireHandler == nullptr)
            {
               readConfigFile();
               sleep(1);   
            }

            if(connectTimes++ >= connectMax)
            {
                cout <<"try to connect to MAX"<<endl;
                system("killall throttle");
                exit(1);
            }

         }while(m_publishExpireHandler == nullptr);
     }
     cout <<"bind publish expire service success:" << endl;
     cout <<"all connection of the master  is success:" << endl;
}

void signal_handler(int signo)
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


void throttleServ::workerLoop()
{
    try
    {
        while(1)
        {
            workerHandler();
        }
    }
    catch(...)
    {
        cout <<"workerLoop exit:"<<getpid()<<endl;
    }
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
                break;
            }
        }
    }
    workerListUnlock();

}

void throttleServ::run()
{
    int num_children = 0;
    int restart_finished = 1;
    int usr1_finished = 1;
    int is_child = 0;
    int i;
    static int proIdx = 0;
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
                        cout <<"kill:"<< pro->pid<<endl;
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
                int ndx;
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
                    kill(pro->pid, SIGHUP);
                    //child_info[ndx].state = 3;  重启中
                 
                    //还是为了尽量避免连续的SIGHUP的不良操作带来的颠簸
                    //,所以决定取消(3重启中)这个状态
                    //并不是说连续SIGHUP会让程序出错,只是不断的挂掉新进程很愚蠢
                }
            }

            if(is_child==0 &&sigusr1_recved)
            {    
                cout <<"sigusr1_recved"<<endl;
                auto count = 0;
                
                workerListLock();
                reloadConfig();
                int workerSize = m_workerList.size();
                for (count=0; count < m_workerNum; count++)
                {
                    if(count >= workerSize)
                    {
				        BC_process_t *pro = new BC_process_t;
		
				        if(pro == NULL) return;
				        pro->pid = 0;
				        pro->status = PRO_INIT;
				        m_workerList.push_back(pro);   
                    }
                 }    
                

                auto it = m_workerList.begin();
                count = 0;
                for (it = m_workerList.begin(); it != m_workerList.end(); it++, count++)
                {
                      BC_process_t *pro = *it;
                      if(pro == NULL) continue;
                      if(pro->pid == 0) 
                      {
                          continue;
                      }
               
                      cout <<"pro->pid:" << pro->pid<<"::"<< getpid()<<endl;
                      kill(pro->pid, SIGINT);  // zmq_close() has no effect after fork()
                        //  close(pro->channel[0]);
                      pro->status = PRO_KILLING;
                
                 }
                 workerListUnlock();
            }
 
        }

        if(is_child==0 &&sigusr1_recved)
        {
            sigusr1_recved = 0;
            cout <<"master restart"<<endl;
            masterRestart();
        }

        bool proInit = false;
        auto it = m_workerList.begin();
        for (it = m_workerList.begin(); it != m_workerList.end(); it++)
        {
         
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
                        closeZmqHandlerResource();
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

        
        if(is_child==0 && m_masterStart==false)
        {
             //master serv start    
             m_masterStart = true;  
             pthread_t worker;
             masterRun();   
        }
        else if(is_child == 0&& proInit == true)
        {   
            setWorkerChannelChanged(true);
        }

        if (!pid) break; 
    }
 
    if (!is_child)    
    {
        cout << "master exit"<<endl;
        exit(0);    
    }

    workerLoop();
}

