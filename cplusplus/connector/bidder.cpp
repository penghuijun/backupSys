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
#include "zmq.h"
#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"
#include "AdBidderResponseTemplate.pb.h"
#include "AdMobileRequest.pb.h"
#include "AdMobileResponse.pb.h"
#include "bidder.h"
#include "threadpoolmanager.h"

const char *businessCode_VAST="5.1.1";
const char *businessCode_MOBILE="5.1.2";

sig_atomic_t srv_graceful_end = 0;
sig_atomic_t srv_ungraceful_end = 0;
sig_atomic_t srv_restart = 0;
sig_atomic_t sigalrm_recved = 0;;
sig_atomic_t sigusr1_recved = 0;;
using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace rapidjson;

bool  bidderServ::RTB_mobile_http_request_json(string &request, string &business_data)
{
    MobileAdRequest mobile;
    mobile.ParseFromString(business_data);
    MobileAdRequest_Device p_mobile_dev = mobile.device();
    MobileAdRequest_User p_mobile_user = mobile.user();
    
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
    
    document.AddMember("id", mobile.id().c_str(), allocator);//bid request object id
 
    rapidjson::Value j_banObj(rapidjson::kObjectType); 
    j_banObj.AddMember("w", mobile.adspacewidth().c_str(), allocator);
    j_banObj.AddMember("h", mobile.adspaceheight().c_str(), allocator);

    rapidjson::Value j_impression_obj(rapidjson::kObjectType);
    j_impression_obj.AddMember("id", "1", allocator);
    j_impression_obj.AddMember("banner", j_banObj, allocator);
    j_impression_obj.AddMember("displaymanager", mobile.displaymanager().c_str(), allocator);
    j_impression_obj.AddMember("displaymanagerver", mobile.version().c_str(), allocator);
    rapidjson::Value j_impArr(rapidjson::kArrayType);
    j_impArr.PushBack(j_impression_obj, allocator);
    document.AddMember("imp", j_impArr, allocator); 

    rapidjson::Value j_app_obj(rapidjson::kObjectType);
    j_app_obj.AddMember("id", "1", allocator);

    rapidjson::Value j_publisherObj(rapidjson::kObjectType);
    j_publisherObj.AddMember("id", mobile.aid().c_str(), allocator);
    j_app_obj.AddMember("publisher", j_publisherObj, allocator);
    document.AddMember("app", j_app_obj, allocator);

    rapidjson::Value j_devObj(rapidjson::kObjectType);
    j_devObj.AddMember("dnt", p_mobile_dev.dnt().c_str(), allocator);
    j_devObj.AddMember("ua", mobile.ua().c_str(), allocator);
    j_devObj.AddMember("ip", mobile.dnsip().c_str(), allocator);
    rapidjson::Value j_dev_geo_obj(rapidjson::kObjectType);
    j_dev_geo_obj.AddMember("lat", mobile.latitude().c_str(), allocator);
    j_dev_geo_obj.AddMember("lon", mobile.longitude().c_str(), allocator);
    j_dev_geo_obj.AddMember("country", p_mobile_user.countrycode().c_str(), allocator);
    j_dev_geo_obj.AddMember("region", p_mobile_user.region().c_str(), allocator);   
    j_dev_geo_obj.AddMember("city", p_mobile_user.city().c_str(),allocator);
    j_devObj.AddMember("geo", j_dev_geo_obj, allocator);
    document.AddMember("dev", j_devObj, allocator);

    rapidjson::Value j_user_obj(rapidjson::kObjectType);
    rapidjson::Value j_user_geo_obj(rapidjson::kObjectType);
    j_user_obj.AddMember("buyeruid", p_mobile_user.uid().c_str(), allocator);
    MobileAdRequest_User_Gender gend = p_mobile_user.gender();
    if(gend == MobileAdRequest_User_Gender_MALE)
    {
        j_user_obj.AddMember("gender", "M", allocator);
    }
    else if(gend == MobileAdRequest_User_Gender_FEMALE)
    {
        j_user_obj.AddMember("gender", "F", allocator);
    }
    j_user_obj.AddMember("yob", p_mobile_user.age().c_str(), allocator);
    j_user_geo_obj.AddMember("lat", mobile.latitude().c_str(), allocator);
    j_user_geo_obj.AddMember("lon", mobile.longitude().c_str(), allocator);    
    j_user_geo_obj.AddMember("country", p_mobile_user.countrycode().c_str(), allocator);
    j_user_geo_obj.AddMember("region", p_mobile_user.region().c_str(), allocator);
    j_user_geo_obj.AddMember("city", p_mobile_user.city().c_str(),allocator);
    j_user_obj.AddMember("geo", j_user_geo_obj, allocator);
    document.AddMember("user", j_user_obj, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer); 
    document.Accept(writer);  
    request = buffer.GetString();     
    return true;
}


bool  bidderServ::RTB_video_http_request_json(string &request, string &business_data)
{
      VastRequest vast;
       vast.ParseFromString(business_data);
       VastRequest_Video p_video = vast.video();
       VastRequest_Site p_site = vast.site();
       VastRequest_Device p_dev=vast.device();
       VastRequest_User p_user = vast.user();
    
      rapidjson::Document document;
      document.SetObject();
      rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
      document.AddMember("id", vast.id().c_str(),allocator);
      document.AddMember("at", 2, allocator);
      document.AddMember("tmax", 120, allocator);
    
      rapidjson::Value j_wseatArr(rapidjson::kArrayType);
      j_wseatArr.PushBack("aaa0", allocator);
      j_wseatArr.PushBack("aaa1", allocator);
      j_wseatArr.PushBack("aaa2", allocator);
      document.AddMember("wseat", j_wseatArr, allocator);
      document.AddMember("allimps", 110, allocator);
      document.AddMember("cur", j_wseatArr, allocator);
      document.AddMember("bcat", j_wseatArr,allocator);
      document.AddMember("badv", j_wseatArr, allocator);
    
      rapidjson::Value j_vedioObj(rapidjson::kObjectType);
      j_vedioObj.AddMember("w", p_video.videowidth().c_str(), allocator);
      j_vedioObj.AddMember("h", p_video.videoheight().c_str(),allocator);
      j_vedioObj.AddMember("startdelay", p_video.startdelay().c_str(), allocator);
      j_vedioObj.AddMember("minduration", p_video.minadduration().c_str(), allocator);
      j_vedioObj.AddMember("maxduration", p_video.maxadduration().c_str(), allocator);
    
      rapidjson::Value j_mimArr(rapidjson::kArrayType);
      j_mimArr.PushBack("xxx1", allocator);
      j_mimArr.PushBack("xxx2", allocator);
      j_vedioObj.AddMember("mimes", j_mimArr, allocator);
      j_vedioObj.AddMember("linearity", 1, allocator);
      j_vedioObj.AddMember("sequence", 2, allocator);
    
      rapidjson::Value j_banObj(rapidjson::kObjectType);
      j_banObj.AddMember("w", 1, allocator);
      j_banObj.AddMember("h", 2, allocator);
      j_banObj.AddMember("id", "1234", allocator);
      
      rapidjson::Value j_banArr(rapidjson::kArrayType);
      j_banArr.PushBack(j_banObj, allocator);
    
      rapidjson::Value j_impression_obj(rapidjson::kObjectType);
      j_impression_obj.AddMember("id", "1", allocator);
      j_impression_obj.AddMember("video", j_vedioObj, allocator);
      j_impression_obj.AddMember("companionad", j_banArr, allocator);
      j_impression_obj.AddMember("displaymanager", "cctv1", allocator);
      j_impression_obj.AddMember("displaymanagerver", "cctv2", allocator);
    
      rapidjson::Value j_impArr(rapidjson::kArrayType);
      j_impArr.PushBack(j_impression_obj, allocator);
      document.AddMember("imp", j_impArr, allocator);
       
      rapidjson::Value j_siteObj(rapidjson::kObjectType);
      j_siteObj.AddMember("id", p_site.sid().c_str(), allocator);
      j_siteObj.AddMember("name", p_site.name().c_str(), allocator);
      j_siteObj.AddMember("domain", p_site.domain().c_str(), allocator);
      j_siteObj.AddMember("sitecat", p_site.sitecat().c_str(),allocator);
      j_siteObj.AddMember("page", p_site.page().c_str(), allocator);
      j_siteObj.AddMember("ref", p_site.ref().c_str(), allocator);
      rapidjson::Value j_publisherObj(rapidjson::kObjectType);
      j_publisherObj.AddMember("id", p_site.publisherid().c_str(), allocator);
      j_siteObj.AddMember("publisher", j_publisherObj, allocator);
      document.AddMember("site", j_siteObj, allocator);
     
      rapidjson::Value j_devObj(rapidjson::kObjectType);
      j_devObj.AddMember("dnt", p_dev.dnt().c_str(), allocator);
      j_devObj.AddMember("ua", p_dev.ua().c_str(), allocator);
      j_devObj.AddMember("ip", p_dev.ip().c_str(), allocator);
      j_devObj.AddMember("language", p_dev.lang().c_str(), allocator);
      j_devObj.AddMember("os", p_dev.os().c_str(), allocator);
    
      rapidjson::Value j_dev_geo_obj(rapidjson::kObjectType);
      j_dev_geo_obj.AddMember("lat", 1.23, allocator);
      j_dev_geo_obj.AddMember("lon", 2.34, allocator);
      j_dev_geo_obj.AddMember("country", "CN", allocator);
      j_devObj.AddMember("geo", j_dev_geo_obj, allocator);
      document.AddMember("dev", j_devObj, allocator);
    
      rapidjson::Value j_user_obj(rapidjson::kObjectType);
      rapidjson::Value j_user_geo_obj(rapidjson::kObjectType);
      j_user_obj.AddMember("id", p_user.uid().c_str(), allocator);
      j_user_geo_obj.AddMember("lat", p_user.latitude().c_str(), allocator);
      j_user_geo_obj.AddMember("country", p_user.countrycode().c_str(), allocator);
      j_user_geo_obj.AddMember("region", p_user.regionname().c_str(), allocator);
      j_user_geo_obj.AddMember("city", p_user.cityname().c_str(),allocator);
      j_user_obj.AddMember("geo", j_user_geo_obj, allocator);
      document.AddMember("user", j_user_obj, allocator);
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer); 
      document.Accept(writer);  
    
      request = buffer.GetString();

    return true;
}

exchangeConnector*  bidderServ::send_request_to_bidder(string businessCode, string &publishKey, string &vastStr, socket_state &sockState)
{
    exchangeConnector *conntor = m_connectorPool.get_CP_exchange_connector( publishKey, sockState);
    if(conntor)
    {
        string request;
        if(businessCode == businessCode_VAST)
        {
            RTB_video_http_request_json(request, vastStr);
        }
        if(businessCode == businessCode_MOBILE)
        {
    
           RTB_mobile_http_request_json(request, vastStr);
        }
        else
        {
           sockState = socket_invalid;
           cout << "this businessCode can not analysis:" << businessCode << endl;
           conntor->reset_recv_state();
           return NULL;
        }   

      //  cout<<request<<endl;
        if(m_connectorPool.cp_send(conntor, request.c_str(), publishKey,sockState))
        {
           return conntor;
        }
        else
        {
           sockState = socket_invalid;           
           conntor->reset_recv_state();
           return NULL;
        }   
   }
   return NULL;
}


void bidderServ::readConfigFile()
{
    m_config.readConfig();
    string ip;
    unsigned short port;
    unsigned short value;
    
    m_configureChange = 0;
    ip = m_config.get_throttleIP();
    if(ip != m_throttleIP) 
    {
        m_configureChange |= c_master_throttleIP;
        m_throttleIP = ip;
    }
   
    port  = m_config.get_throttlePort();
    if(m_throttlePort!= port)
    {
        m_configureChange |= c_master_throttlePort;
        m_throttlePort = port;
    }

    ip = m_config.get_bidderCollectorIP();
    if(ip != m_bidderCollectorIP) 
    {
        m_configureChange |= c_master_bcIP;
        m_bidderCollectorIP = ip;
    }
   
    port  = m_config.get_bidderCollectorPort();
    if(m_bidderCollectorPort != port)
    {
        m_configureChange |= c_master_bcPort;
        m_bidderCollectorPort = port;
    }

    m_subKey.clear();
    m_subKey = m_config.get_subKey(); 
    if(m_subKey != m_subKeying)
    {
        m_configureChange |= c_master_subKey;
    }

    value = m_config.get_bidderworkerNum();
    if(m_workerNum != value)
    {
        m_configureChange |= c_workerNum;
        workerList_wr_lock();
        m_workerNum = value;
        workerList_rw_unlock();
    } 
    
}


bidderServ::bidderServ(configureObject &config):m_config(config)
{
    try
    {
        m_throttleIP = config.get_throttleIP();
        m_throttlePort = config.get_throttlePort();
        m_bidderCollectorIP = config.get_bidderCollectorIP();
        m_bidderCollectorPort = config.get_bidderCollectorPort();
        m_workerNum = config.get_bidderworkerNum();
        m_subKey = config.get_subKey();
        
        pthread_mutex_init(&m_uuidListMutex, NULL);
        pthread_rwlock_init(&handler_fd_lock, NULL);
        pthread_rwlock_init(&m_workerList_lock, NULL);
        pthread_rwlock_init(&m_requestList_lock, NULL);

        auto i = 0;
    	for(i = 0; i < m_workerNum; i++)
    	{
    		BC_process_t *pro = new BC_process_t;
    		pro->pid = 0;
    		pro->status = PRO_INIT;
    		m_workerList.push_back(pro);
    	};
        m_configureChange = 0xFFFFFFFF;	

    }
    catch(...)
    {   
        cerr <<"bidderServ structure error"<<endl;
        exit(1);
    }
}

/*
  *update m_workerList,  if m_wokerNum < workerListSize , erase the workerNode, if m_workerList update ,lock the resource
  */
void bidderServ::updataWorkerList(pid_t pid)
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

void bidderServ::signal_handler(int signo)
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




void bidderServ::hupSigHandler(int fd, short event, void *arg)
{
    cout <<"signal hup"<<endl;
    exit(0);
}

void bidderServ::intSigHandler(int fd, short event, void *arg)
{
   cout <<"signal int:"<<endl;
   usleep(10);
   exit(0);
}

void bidderServ::termSigHandler(int fd, short event, void *arg)
{
    cout <<"signal term"<<endl;
    exit(0);
}



void bidderServ::usr1SigHandler(int fd, short event, void *arg)
{
    cout <<"signal SIGUSR1"<<endl;
    bidderServ *serv = (bidderServ*) arg;
    if(serv != NULL)
    {
        serv->reloadWorker();
    }
}


void bidderServ::worker_parse_master_data(char *data, unsigned int dataLen)
{
    char subsribe[SUBSCRIBE_STR_LEN];
    char *probufData = NULL;
    int   probufDataLen = 0;
    string publishKey;

    memset(subsribe, 0x00, sizeof(subsribe));
    memcpy(subsribe, data, SUBSCRIBE_STR_LEN);
    publishKey = subsribe;
    probufData = data+SUBSCRIBE_STR_LEN;
    probufDataLen = dataLen - SUBSCRIBE_STR_LEN;

  //  cout<<"+++"<<endl;
    CommonMessage commMsg;
    commMsg.ParseFromArray(probufData, probufDataLen);     
    string vastStr=commMsg.data();
    string tbusinessCode = commMsg.businesscode(); 


    socket_state sockState;
    exchangeConnector* connector = send_request_to_bidder(tbusinessCode, publishKey, vastStr, sockState);
    if(sockState != socket_invalid)
    {
       uuid_subsribe_info *uinfo = new uuid_subsribe_info;
       uinfo->connector = connector;
       uinfo->publishKey = publishKey;
       uinfo->business_code = tbusinessCode;
       uinfo->data_coding_type = commMsg.datacodingtype();
       uinfo->vastStr = vastStr;
           
       if(connector==NULL)//waiting
       {
          uinfo->state = request_waiting;
       }
       else//send success
       {
         uinfo->state = request_sended;
       }
                
       requestList_wr_lock();
       m_request_info_list.push_back(uinfo);
       requestList_rw_unlock();  
    }
    else //there is not publishkey or all of the connector with the publishKey is invalid , so ignore the request;
    {
       cout<<"send http request socket_invalid"<<endl;
    }
  //  cout<<"---"<<endl;
  
}

void  handler_thread(void *arg)
{
    throttleBuf *thro =(throttleBuf *)arg;
    bidderServ *serv = (bidderServ*)thro->serv;
    if(serv)
    {
        serv->worker_parse_master_data(thro->buf, thro->bufSize);
    }
    
    delete []thro->buf;
    delete thro;
}
//recv From master
void bidderServ::workerBusiness_callback(int fd, short event, void *pair)
{

    bidderServ *serv = (bidderServ*) pair;
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
        return;
    }
    
    char buf[BUF_SIZE];
    int len;
    int dataLen;
    while(1)
    {  
       len = read(fd, buf, 4);//dataLen 
       if(len != 4) break;
       
       dataLen = GET_LONG(buf);  
       len = read(fd, buf, dataLen);
       if(len != dataLen) 
       {
           cout<<"recv data exception:"<<dataLen <<"len:"<<len<<endl;
           break;
       }
       serv->worker_parse_master_data(buf, len);
       
     /*  throttleBuf *thro = new throttleBuf;
       thro->bufSize = len;
       thro->buf = new char[len];
       thro->serv = pair;
       memcpy(thro->buf, buf, len);*/

     //   serv->calSpeed();
     //  cout<<"worker recv bytes:"<<len<<endl;
     //  serv->m_maneger.Run(handler_thread, (void *)thro);

     
  }            
}


void bidderServ::recvFromExBidderHandlerint(int fd, short event, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len=sizeof(int);
    char buf[BUFSIZE*10];
    ostringstream os;

    bidderServ *serv = (bidderServ*)pair;
    
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
        return;
    }

    int idx =0;
    int recvSize;
    string recvStr;
    string recvByteStr;
    int httpChunkSize=0;
    char *sendHttpRsp = NULL;
    int  sendHttpRspLen = 0;

 //  cout<<"%%%%:"<<event<<":"<< EV_TIMEOUT<<":"<<fd<<endl;
 //recv header

    exchangeConnector *conn = serv->get_exchangeConnect(fd);
    if(conn==NULL)
    {
        cout<<"can not find the fd--------------------------- "<<endl;
        close(fd);
        return;
    }
    
    if(event==EV_TIMEOUT)
    {
        conn->release();
        serv->earse_request_info(fd);
        conn->connect_async(serv->m_base, recvFromExBidderHandlerint, pair);
        return;    
    }

    while(1)
    {
      //  memset(buf, 0x00, sizeof(buf));
        recvSize = read(fd, buf, sizeof(buf));
        if(recvSize == 0)
        {
            cout <<"remote serv close connect:"<<fd<<endl;
            conn->release();
            serv->earse_request_info(fd);
            conn->connect_async(serv->m_base, recvFromExBidderHandlerint, pair);
            return;
        }
        else if(recvSize < 0)
        {
            return;
        }
        else
        {
            buf[recvSize] = '\0';
            sendHttpRsp = conn->recv_async(buf, recvSize, &sendHttpRspLen);//parse recv data     
            if(sendHttpRsp) 
            {
                break;
            }
        }
    }

    if((sendHttpRsp != NULL)&& sendHttpRspLen)
    {
   //     cout <<sendHttpRsp<<endl;
        rapidjson::Document document;
        document.Parse<0>(sendHttpRsp);

        if(document.HasParseError())
        {
            cout<< "rapidjson parse error:"<<document.GetParseError()<<endl;
            return;
        }

        //cout<<document["id"].GetString()<<"*"<<fd<<endl;
        uuid_subsribe_info* httpInfo = serv->get_request_info(fd);
        if(httpInfo==NULL)
        {
            cout<<"can not find httpInfo"<<endl;
            return;
        }
        CommonMessage commMsg;
        if(serv->transJsonToProtoBuf(document, httpInfo->business_code, httpInfo->data_coding_type, httpInfo->vastStr, commMsg) == false)
        {
            cout<<"transJsonToProtoBuf failure"<<endl;
            return;        
        }
        serv->earse_request_info(fd);
        conn->reset_recv_state();
        delete sendHttpRsp;
        
        if(conn->get_disconnect_sym())
        {
            serv->dis_connector_fd(fd);
        }
    
        char comBuf[BUF_SIZE];
        commMsg.SerializeToArray(comBuf, sizeof(comBuf));
        int ssize = zmq_send(serv->m_response_handler, comBuf, commMsg.ByteSize(),ZMQ_NOBLOCK);
     //   cout<<"send to bc::"<<fd<<endl;
         serv->sendTo_externBid_next();         
         serv->calSpeed();
    }


}

void bidderServ::sendTo_externBid_next()
{
    requestList_wr_lock();

    auto it = m_request_info_list.begin();
    for(it = m_request_info_list.begin(); it != m_request_info_list.end();)
    {
        uuid_subsribe_info *uinfo = *it;
        if(uinfo && (uinfo->state== request_waiting))
        {
            socket_state sockState;
            exchangeConnector* connector = send_request_to_bidder(uinfo->business_code, uinfo->publishKey, uinfo->vastStr, sockState);
            if(sockState == socket_invalid)
            {
                delete uinfo;
                m_request_info_list.erase(it++);
                requestList_rw_unlock();
            }
            else if(sockState == socket_valid)
            {
                uinfo->connector = connector;
                uinfo->state = request_sended;
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
            it++;
        }
    }
    requestList_rw_unlock();
}

bool bidderServ::transJsonToProtoBuf( rapidjson::Document &document , string &tbusinessCode, string &data_coding_type, string &vastStr, CommonMessage &commMsg)
{
   string uuid; 
    if(tbusinessCode == businessCode_VAST)
    {
       VastRequest vast;
       vast.ParseFromString(vastStr);
       uuid = vast.id();
       
       BidderResponse t_bidder_response;
       const  rapidjson::Value &j_seatbid = document["seatBid"];
       if(j_seatbid.IsArray())
       {
            for(size_t i = 0; i < j_seatbid.Size(); i++)
            {
                const rapidjson::Value &j_seat_bid = j_seatbid[i];
    
                const rapidjson::Value &j_bid_arr = j_seat_bid["bid"];
                if(j_bid_arr.IsArray())
                {
                    for(size_t j = 0; j < j_bid_arr.Size(); j++)
                    {
                        const rapidjson::Value &j_bid = j_bid_arr[j];
         
                       char buf[BUF_SIZE];
                       char comBuf[BUF_SIZE];      
                       t_bidder_response.set_id(uuid);
                       t_bidder_response.set_bidderid(document["bidid"].GetString());
                       BidderResponse_Ad *ad = t_bidder_response.add_ads();
                       ad->set_price(j_bid["price"].GetString());
                       ad->set_adid(j_bid["adid"].GetString());
                       ad->set_campaignid(j_bid["cid"].GetString());
                       t_bidder_response.SerializeToArray(buf, sizeof(buf));
                       int rspSize = t_bidder_response.ByteSize();
                       commMsg.set_businesscode(tbusinessCode);
                       commMsg.set_datacodingtype(data_coding_type);
                       commMsg.set_data(buf, rspSize);
                       break;
                    }
                }
            }
       }
       else
       {
             cout<<"seatbid is not array" << endl; 
       }
    }
    else if(tbusinessCode == businessCode_MOBILE)
    {
        MobileAdRequest mobile;
        mobile.ParseFromString(vastStr);
        uuid = mobile.id();

        MobileAdResponse mobile_resp;
        const  rapidjson::Value &j_seatbid = document["seatbid"];
        const char *j_id = document["id"].GetString();
        const char *j_cur = document["cur"].GetString();
    //    cout<<"response id:"<<j_id<<endl;
     //   cout<<"response cur:"<<j_cur<<endl;
        if((j_seatbid.IsNull()==false)&&j_seatbid.IsArray())
        {
             for(size_t i = 0; i < j_seatbid.Size(); i++)
             {
                 const rapidjson::Value &j_seat_bid = j_seatbid[i];
        
                 const rapidjson::Value &j_bid_arr = j_seat_bid["bid"];
                 if((j_bid_arr.IsNull()==false)&&j_bid_arr.IsArray())
                 {
                     for(size_t j = 0; j < j_bid_arr.Size(); j++)
                     {
                        const rapidjson::Value &j_bid = j_bid_arr[j];
          
                        char buf[BUF_SIZE];
                        char comBuf[BUF_SIZE];   
                        mobile_resp.set_id(uuid);
                        mobile_resp.set_bidderid(document["bidid"].GetString());
/*
                        const char *adm = j_bid["adm"].GetString();
                        cout<<"adm:"<<adm<<endl;
                        const char *impid = j_bid["impid"].GetString();
                        cout<<"impid:"<<impid<<endl;
                        const char *nurl = j_bid["nurl"].GetString();
                        cout<<"nurl:"<<nurl<<endl;

                        
                        const rapidjson::Value &j_adomain = j_bid["adomain"];
                        if((j_adomain.IsNull()==false)&&j_adomain.IsArray())
                        {
                            for(size_t j = 0; j < j_adomain.Size(); j++)
                            {
                                const char *adomain = j_adomain[j].GetString();
                                cout<<"adomain:"<< adomain<<endl;
                            }
                        }*/
                        /*BidderResponse_Ad *ad = t_bidder_response.add_ads();
                        ad->set_price(j_bid["price"].GetString());
                        ad->set_adid(j_bid["adid"].GetString());
                        ad->set_campaignid(j_bid["cid"].GetString());*/
                        mobile_resp.SerializeToArray(buf, sizeof(buf));
                        int rspSize = mobile_resp.ByteSize();
                        commMsg.set_businesscode(tbusinessCode);
                        commMsg.set_datacodingtype(data_coding_type);
                        commMsg.set_data(buf, rspSize);
                        break;
                     }
                 }
                 else
                 {
                    cout<<"bid is not array"<<endl;
                 }
             }
        }
        else
        {
              cout<<"seatbid is not array" << endl; 
        }

    }
        

    return true;
}  

void *bidderServ::check_connector(void *arg)
{ 
    bidderServ *serv = (bidderServ*)arg;
    if(serv)
    {
        while(1)
        {
            sleep(10);
            if(serv->connectorPool_update())
            {
                write(serv->m_connector_eventFd[1],"addConnEvent", 12);
            }
        }
    }
}

void bidderServ::connector_maintain(int fd, short __, void *pair)
{
    bidderServ *serv = (bidderServ*)pair;
    if(serv)
    {
        char buf[1024]={0};
        int rc = read(fd, buf, sizeof(buf));
        if(memcmp(buf,"addConnEvent", 12) == 0)
        {
            cout <<"connector_maintain"<<endl;
            serv->connector_libevent_add(serv->m_base, recvFromExBidderHandlerint,(void *) serv);
        }  
    }
}

void bidderServ::start_worker()
{

    worker_net_init();
   // m_maneger.Init(10000, 100,100);
    m_base = event_base_new();  
    
    int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, m_connector_eventFd);
    struct event * pEvRead = event_new(m_base, m_connector_eventFd[0], EV_READ|EV_PERSIST, connector_maintain, this); 
    event_add(pEvRead, NULL);

    struct event * hup_event = evsignal_new(m_base, SIGHUP, hupSigHandler, this);
    struct event * int_event = evsignal_new(m_base, SIGINT, intSigHandler, this);
    struct event * term_event = evsignal_new(m_base, SIGTERM, termSigHandler, this);
    struct event * usr1_event = evsignal_new(m_base, SIGUSR1, usr1SigHandler, this);
    struct event *clientPullEvent = event_new(m_base, m_workerChannel, EV_READ|EV_PERSIST, workerBusiness_callback, this);

    event_add(clientPullEvent, NULL);
    evsignal_add(hup_event, NULL);
    evsignal_add(int_event, NULL);
    evsignal_add(term_event, NULL);
    evsignal_add(usr1_event, NULL);

    cout<<"starttttttttttttttttttttttttttttttttttttttttttttt"<<endl;
    //init connector pool
    const vector<exBidderInfo*>& listVec = m_config.get_exBidderList();
    m_connectorPool.connector_pool_init(listVec, m_base, recvFromExBidderHandlerint , (void *)this);
    display_connectPool();
    cout<<"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeend"<<endl; 

    pthread_t pth; 
    pthread_create(&pth, NULL, check_connector, this); 
    event_base_dispatch(m_base);
}


void* bidderServ::establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd)
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

void* bidderServ::bindOrConnect( bool client,const char * transType,int zmqType,const char * addr,unsigned short port,int * fd)
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

/*
  *function name: zmq_connect_subscribe
  *param: subsribe key
  *return:handler_fd_map*
  *function: establish subscribe connection
  *all right reserved
  */
handler_fd_map*  bidderServ::zmq_connect_subscribe(string subkey)
{
    int fd;
    int rc;
    size_t size = sizeof(int);
    int hwm = 3000;
    
    void *handler = bindOrConnect(true, "tcp", ZMQ_SUB, m_throttleIP.c_str(), m_throttlePort, &fd);//client sub 
    rc = zmq_setsockopt(handler, ZMQ_RCVHWM, &hwm, sizeof(hwm));
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

void bidderServ::master_net_init()
{ 
    m_zmqContext =zmq_ctx_new();
    auto it = m_subKey.begin();  
    for(it = m_subKey.begin(); it != m_subKey.end();it++)
    {
        zmq_connect_subscribe(*it);
    }   
    cout <<"adVast bind success" <<endl;
}


void bidderServ::masterRestart(struct event_base* base)
{
 
    cout <<"masterRestart::m_throConfChange:"<<m_configureChange<<endl;
    if(((m_configureChange&c_master_throttleIP)==c_master_throttleIP)|| ((m_configureChange&c_master_throttlePort)==c_master_throttlePort))
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
}

void bidderServ::worker_net_init()
{ 
    m_zmqContext =zmq_ctx_new();
    m_response_handler = bindOrConnect(true, "tcp", ZMQ_DEALER, m_bidderCollectorIP.c_str(), m_bidderCollectorPort, NULL);  
    cout <<"worker net init success:" <<endl;
}


//send to worker sendToWorker
void bidderServ::write_worker_channel(void * data,unsigned int dataLen,int chanelFd, string &subsribe)
{
    BC_process_t* pro = NULL;
    char *sendFrame=NULL;
    int frameLen;
    int chanel=chanelFd;   
    int rc;
    int workIdx = 0;
    
    try
    {
        frameLen = dataLen+4+SUBSCRIBE_STR_LEN;//dataLen  + dataLen len+subscribe len
        sendFrame = new char[frameLen];
        memset(sendFrame, 0x00, frameLen);
        PUT_LONG(sendFrame, frameLen-4);

        memcpy(sendFrame+4, subsribe.c_str(), subsribe.size());
        memcpy(sendFrame+4+SUBSCRIBE_STR_LEN, data, dataLen);
        do
        {   
            rc = write(chanel, sendFrame, frameLen);
            
            if(rc>0)
            {
            //    calSpeed();
                break;
            }
            else
            {
                workerList_rd_lock();
                pro = m_workerList.at((workIdx++)%m_workerNum);
                if(pro==NULL)continue;
                chanel = pro->channel[0]; 
                workerList_rw_unlock();
            }
       }while(1);
       delete[] sendFrame;
    }
    catch(std::out_of_range &err)
    {
        cerr<<err.what()<<"LINE:"<<__LINE__  <<" FILE:"<< __FILE__<<endl;
        delete[] sendFrame;
        write_worker_channel(data, dataLen, 0, subsribe);
    }
    catch(...)
    {
        delete[] sendFrame;
        cout <<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&"<<endl;
        throw;
    }
}

//master send data to worker
void bidderServ::sendToWorker(char * buf,int bufLen, string &subsribe)
{
    static int workerID = 0;
    BC_process_t* pro = NULL;
    int channel = 0;
    try
    {
        workerList_rd_lock();
        pro = m_workerList.at((workerID++)%m_workerNum);
        if(pro)
        {
            channel = pro->channel[0]; 
            write_worker_channel(buf, bufLen, channel, subsribe);
        }
        workerList_rw_unlock();
    }
    catch(...)
    {
        workerList_rd_lock();
        workerID=0;
        pro = m_workerList.at((workerID++)%m_workerNum);
        if(pro)
        {
            channel = pro->channel[0]; 
            write_worker_channel(buf, bufLen, channel, subsribe);
        }
        workerList_rw_unlock();
        cout <<"sendToWorker exception!"<<endl;
    }
}


//handler the data of recv from throttle 
void bidderServ::recvRequest_callback(int fd, short __, void *pair)
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len=sizeof(events);
    char buf[BUFSIZE];
    string subsribe;
    bidderServ *serv = (bidderServ*)pair;
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
        return;
    }

    //find the handler corresponse to fd
    void *handler = serv->get_request_handler(fd);
    if(!handler)
    {
        cout<<"recvRequest_callback can not find zmq handler!"<<endl;
        return;
    }
    
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        cout <<"recvRequest_callback zmq_getsockopt failure!" <<endl;
        return;
    }

    
    if(events & ZMQ_POLLIN)// can read 
    {
        while (1)
        {
            //recv subscribe key
            int recvLen = zmq_recv(handler, buf, sizeof(buf), ZMQ_NOBLOCK);
            if(recvLen == -1 )
            {
                break;
            }
            buf[recvLen] = '\0';
            subsribe = buf;
            //recv data
            recvLen = zmq_recv(handler, buf, sizeof(buf), ZMQ_NOBLOCK);
            if(recvLen == -1 )
            {
                break;
            }  
            buf[recvLen] = '\0';
          //  cout<<"recv from throttle:"<<recvLen<<endl;
            serv->sendToWorker(buf, recvLen, subsribe);
        }
    }
}

void bidderServ::func(int fd, short __, void *pair)
{
    eventParam *param = (eventParam *) pair;
    if(!param) return;
    bidderServ *serv = (bidderServ*)param->serv;
    if(!serv) return;
    
    char buf[1024]={0};
    int rc = read(fd, buf, sizeof(buf));
    cout<<buf<<endl;
    if(memcmp(buf,"event", 5) == 0)
    {
        cout <<"update connect:"<<getpid()<<endl;
        serv->masterRestart(param->base);
    }    
}

void *bidderServ::throttle_request_handler(void *arg)
{ 
      bidderServ *serv = (bidderServ*) arg;
      struct event_base* base = event_base_new(); 

      eventParam *param = new eventParam;
      param->base = base;
      param->serv = serv;
      int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, serv->m_eventFd);
      struct event * pEvRead = event_new(base, serv->m_eventFd[0], EV_READ|EV_PERSIST, func, param); 
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


bool bidderServ::masterRun()
{
      int *ret;
      pthread_t pth;   
      master_net_init();
      pthread_create(&pth, NULL, throttle_request_handler, this);   
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
void bidderServ::run()
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
                        workerList_wr_lock();
				        m_workerList.push_back(pro); 
                        workerList_rw_unlock();
                    }
                }    
            }
 
        }

        if(is_child==0 &&sigusr1_recved)//fathrer process and recv sig usr1
        {
            write(m_eventFd[1],"event", 5);
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
                        close(m_eventFd[0]);
                        close(m_eventFd[1]);
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
