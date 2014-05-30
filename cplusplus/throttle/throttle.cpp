#include <iostream>
#include <sstream>
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

throttleServ::throttleServ(throttleConfig &config)
{

    m_throttleIP = config.get_throttleIP();
    m_throttleAdPort = config.get_throttleAdPort();
    m_throttleExpirePort = config.get_throttleExpirePort();
    m_servPushPort = config.get_throttleServPushPort();
    m_servPullPort = config.get_throttleServPullPort();
    m_publishPort = config.get_throttlePubPort();

}

void *adRequestHandle(void *throttle)
{
    try
    {
	    throttleServ *serv = (throttleServ*) throttle;
        if(serv == nullptr) throw;
	    serv->adWork();
    }
    catch(...)
    {
        cout << "error: adRequestHandle param is nullptr" <<endl;
        exit(1);
    }
    return nullptr;
}



void *servRecvPollHandle(void *throttle)
{
    try
    {
	    throttleServ *serv = (throttleServ*) throttle;
        if(serv == nullptr) throw;
	    serv->servPollWork();
    }
    catch(...)
    {
        cout << "error: servRecvPollHandle param is nullptr" <<endl;
        exit(1);  
    }
    return nullptr;
}


void *expireHandle(void *throttle)
{
    try
    {
	    throttleServ *serv = (throttleServ*) throttle;
        if(serv == nullptr) throw;
	    serv->expireServWork();
    }
    catch(...)
    {
        cout << "error: expireHandle param is nullptr" <<endl;
        exit(1);  
    }
    return nullptr;
}


void *throttleServ::servPollWork()
{
     char publish[PUBLISHKEYLEN_MAX];
     char buf[BUFSIZE];   
     int size ;
     while(1)
     {
        size = zmq_recv(m_servPollHandler, buf, sizeof(buf), 0);
        memcpy(publish, buf, sizeof(publish));
        size -= PUBLISHKEYLEN_MAX;
        zmq_send(m_publishHandler, publish, strlen(publish),ZMQ_SNDMORE);
        zmq_send(m_publishHandler, &buf[0]+PUBLISHKEYLEN_MAX, size, 0);
     } 
}


static int timeoutNum = 0;
static int expireTimes = 0;
void *throttleServ::expireServWork()
{
    cout<< "expireDataServ::run" << endl;
    static int expireTimes = 0;
    ostringstream os;
    string protocol;
    
    while(1)
    {
         char buf[1024];
         int size = zmq_recv(m_zmqExpireRspHandler, buf, sizeof(buf), 0);
#ifdef DEBUG
         cout << "recv expire data:"<<++expireTimes<<endl;
#endif
         try
         {
              if(size)
              {
                  CommonMessage commMsg;
                  commMsg.ParseFromArray(buf, size);
                  string rsp = commMsg.data();
                  ExpiredMessage expireMsg;
                       
                  expireMsg.ParseFromString(rsp);
                  string uuid = expireMsg.uuid();
                  string expiredata = expireMsg.status();             
              }
         }
         catch(...)
         {
             cout << "throw error" << endl;
             exit(-1);
         }
          
     }
     return nullptr;
}

void throttleServ::workerHandler(char *buf, unsigned int size)
{    
    static int adTimes=0;

    CommonMessage commMsg;
    commMsg.ParseFromArray(buf, size);     
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
           
    if(adTimes%2==0)
    {
         publishKey = "5.1.1-CN";
  
    }
    else
    {
         publishKey = "5.1.1-US";
    
    }
              
    char *tmpBuf = new char[PUBLISHKEYLEN_MAX+size];
    memset(tmpBuf, 0x00, PUBLISHKEYLEN_MAX+size);
    memcpy(tmpBuf, publishKey.c_str(), publishKey.size());
    memcpy(tmpBuf+PUBLISHKEYLEN_MAX, buf, size);
                   
    int cSize = workerPushData(tmpBuf, PUBLISHKEYLEN_MAX+size);
    delete[] tmpBuf;
   // cout <<"cSize:"<<cSize<<endl;    
}

void* throttleServ::establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port)
{
    ostringstream os;
    string pro;
    int rc;
    void *handler = nullptr;
    
    os.str("");
    handler = zmq_socket (m_zmqContext, zmqType);
    os << transType << "://" << addr << ":" << port;
    pro = os.str();
    if(client)
    {
        rc = zmq_connect(handler, pro.c_str());
    }
    else
    {
        rc = zmq_bind (handler, pro.c_str());
    }
    if(rc!=0) return nullptr; 
    return handler;
}

void throttleServ::workerStart()
{
    ostringstream os;
    string pro;
    int rc;
    try
    {
        m_zmqContext = zmq_ctx_new();
        m_clientPullHandler = establishConnect(true, "tcp", ZMQ_PULL , "localhost", m_servPushPort);
        if(m_clientPullHandler == nullptr) throw m_servPushPort;
        cout << "pull connect push serv success:" << pro << endl;

        m_clientPushHandler = establishConnect(true, "tcp", ZMQ_PUSH , "localhost", m_servPullPort);
        if(m_clientPushHandler == nullptr) throw m_servPullPort;
        cout << "push connect pull serv success:" << endl;
    }
    catch(unsigned short &port)
    {
        cout <<"worker error:" << port << "can not connect"<< endl;
        exit(1);
    }
    catch(...)
    {
        cout << "connect error" << endl;   
        exit(1);
    }
}

void *throttleServ::adWork()
{
    char buf[BUFSIZE]={0};
    int size = 0;
 
    while(1)
    {
        try
        {
    		size = zmq_recv(m_adRspHandler, buf, sizeof(buf), 0);
    		if(size <= 0)
    		{
    			continue;
    		}
        
            size = zmq_recv(m_adRspHandler, buf, sizeof(buf), 0);
    		if(size <= 0)
    		{
    			continue;
    		}

            zmq_send(m_servPushHandler, buf, size, 0);
        }
        catch(...)
        {
           cout<< "error:adWork exception" << endl;
        }
	}
	return NULL;
}

bool throttleServ::masterRun()
{
    int rc;
    int *ret;
    ostringstream os;
    string protocol;
    pthread_t worker;
    int thread_nbr;
    void *adWorks = nullptr;
    try
    {
        m_zmqContext = zmq_ctx_new();
        //RECV AD REQUEST
        m_adRspHandler = establishConnect(false, "tcp", ZMQ_ROUTER, m_throttleIP.c_str(), m_throttleAdPort);
        if(m_adRspHandler == nullptr) throw m_throttleAdPort;
        cout<< "throttleServ start, ADvast bind success:"<< endl;
        
         //PUSH SERV
        m_servPushHandler = establishConnect(false, "tcp", ZMQ_PUSH, "*", m_servPushPort);
        if(m_servPushHandler == nullptr) throw m_servPushPort;
        cout <<"bind router push success:" << endl;

        //MASTER POOL SERV
        m_servPollHandler = establishConnect(false, "tcp", ZMQ_PULL, "*", m_servPullPort);
        if(m_servPollHandler == nullptr) throw m_servPullPort;
        cout <<"bind route pool service success:" << endl;

        //PUBLISH KEY
        m_publishHandler = establishConnect(false, "tcp", ZMQ_PUB, "*", m_publishPort);
        if(m_publishHandler == nullptr) throw m_publishPort;
        cout <<"bind publish service success:" << endl;

        //recv expire msg
        m_zmqExpireRspHandler = establishConnect(false, "tcp", ZMQ_DEALER, m_throttleIP.c_str(), m_throttleExpirePort);
        if(m_zmqExpireRspHandler == nullptr) throw m_throttleExpirePort;
        cout <<"expire bind success:" << endl;

//ad thread, expire thread, poll thread
        pthread_create(&worker, NULL, adRequestHandle, this);
        pthread_create(&worker, NULL, expireHandle, this);
        pthread_create(&worker, NULL, servRecvPollHandle, this);
    	pthread_join(worker, (void **)&ret);
    }
    catch(unsigned short &port)
    {
        cout << "<" << m_throttleIP << "," << port << ">" << "can not bind this port" << "@"<<protocol<< endl;
        exit(-1);
    }
    catch(...)
    {
        cout << "unknow error" << endl;
        exit(-1);
    }
	return true;
}

