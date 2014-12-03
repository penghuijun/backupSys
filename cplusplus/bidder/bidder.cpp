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

#include "bidder.h"
#include "threadpoolmanager.h"
#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"
#include "AdBidderResponseTemplate.pb.h"
#include "MobileAdRequest.pb.h"
#include "MobileAdResponse.pb.h"
#include "campaign.h"
using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;

sig_atomic_t srv_graceful_end = 0;
sig_atomic_t srv_ungraceful_end = 0;
sig_atomic_t srv_restart = 0;
sig_atomic_t sigalrm_recved = 0;;
sig_atomic_t sigusr1_recved = 0;;

struct timeval tm;
extern pthread_mutex_t tmMutex;
unsigned long long getLonglongTime()
{
    pthread_mutex_lock(&tmMutex);
    unsigned long long t = tm.tv_sec*1000+tm.tv_usec/1000;
    pthread_mutex_unlock(&tmMutex); 
    return t;
}

void *getTime(void *arg)
{
    //throttleServ *serv = (throttleServ*) arg;
    while(1)
    {
        usleep(1*1000);
        pthread_mutex_lock(&tmMutex);
        gettimeofday(&tm, NULL);
        pthread_mutex_unlock(&tmMutex);
    }
}

void bidderServ::readConfigFile()
{
    m_config.readConfig();
    string ip;
    unsigned short port;
    unsigned short value;
    
    m_configureChange = 0;

    
    ip = m_config.get_throttleIP();
    port  = m_config.get_throttlePort(); 
    if((ip != m_throttleIP)||(m_throttlePort!= port)) 
    {
        m_configureChange |= c_update_throttleAddr;
        m_throttleIP = ip;
        m_throttlePort = port;
    }
   

    ip = m_config.get_bidderCollectorIP();
    port  = m_config.get_bidderCollectorPort();
    if((ip != m_bidderCollectorIP)||(m_bidderCollectorPort != port)) 
    {
        m_configureChange |= c_update_bcAddr;
        m_bidderCollectorPort = port;
        m_bidderCollectorIP = ip;
    }

    m_subKey.clear();
    m_subKey = m_config.get_subKey(); 
    if(m_subKey != m_subKeying)
    {
        m_configureChange |= c_update_zmqSub;
    }

    value = m_config.get_bidderworkerNum();
    if(m_workerNum != value)
    {
        m_configureChange |= c_update_workerNum;
        m_worker_info_lock.write_lock();
        m_workerNum = value;
        m_worker_info_lock.read_write_unlock();
    } 

    value = m_config.get_threadPoolSize();
    if(m_thread_pool_size!=value)
    {
         m_configureChange |= c_update_forbid;
         m_thread_pool_size = value;
    }

    m_vastBusinessCode = m_config.get_vastBusiCode();
    m_mobileBusinessCode = m_config.get_mobileBusiCode();
    m_bidder_id = m_config.get_bidderID();
    m_target_converce_num = m_config.get_converceNum();
}

void bidderServ::calSpeed()
{
     const int printNum = 10000;
     static long long test_begTime;
     static long long test_endTime;
     static int test_count = 0;
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
        //cout<<"b-e:"<<test_begTime<<":"<<test_endTime<<endl;
        cout <<"send request num: "<<test_count<<endl;
        cout <<"cost time: " << diff<<endl;
        cout<<"sspeed: "<<speed<<endl;
     }
     else
     {
        m_test_lock.unlock();
     }
     cout<<couuut<<endl;
}
void bidderServ::updateWorker()
{
    readConfigFile();
    if((m_configureChange &c_update_forbid) == c_update_forbid)
    {
        exit(0);
    }
    if((m_configureChange &c_update_bcAddr) == c_update_bcAddr)
    {
        zmq_close(m_response_handler);
        m_response_handler = bindOrConnect(true, "tcp", ZMQ_DEALER, m_bidderCollectorIP.c_str(), m_bidderCollectorPort, NULL);
    }

    m_redis_pool_manager.redisPool_update(m_config.get_redisIP(), m_config.get_redisPort(), m_config.get_threadPoolSize()+1);
    
}

void bidderServ::releaseConnectResource(string subKey) 
{
    for(auto it = m_handler_fd_map.begin(); it != m_handler_fd_map.end();)
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
             
             for(auto itor = m_subKeying.begin(); itor != m_subKeying.end();)
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

bidderServ::bidderServ(configureObject &config):m_config(config)
{
    try
    {
        m_bidder_id = config.get_bidderID();
        m_throttleIP = config.get_throttleIP();
        m_throttlePort = config.get_throttlePort();
        m_bidderCollectorIP = config.get_bidderCollectorIP();
        m_bidderCollectorPort = config.get_bidderCollectorPort();
        m_workerNum = config.get_bidderworkerNum();
        m_subKey = config.get_subKey();
        m_thread_pool_size = config.get_threadPoolSize();
        m_vastBusinessCode = config.get_vastBusiCode();
        m_mobileBusinessCode = config.get_mobileBusiCode();
        m_target_converce_num = config.get_converceNum();
        m_zeromq_sub_lock.init();
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
    m_worker_info_lock.read_write_unlock();
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
        serv->updateWorker();
    }
}


void display(list<int> &listC)
{
    cout<<"---------------------campaignID begin---------------------"<<endl;
    for(auto it = listC.begin(); it != listC.end(); it++)
    {
        
        cout<<*it<<"   "<<endl;
  
    }
    cout<<"---------------------campaignID   end---------------------"<<endl;
}
void display(vector<string> &listC)
{
    cout<<"---------------------campaignID begin---------------------"<<endl;
    for(auto it = listC.begin(); it != listC.end(); it++)
    {
        
        cout<<*it<<"   "<<endl;
  
    }
    cout<<"---------------------campaignID   end---------------------"<<endl;
}

/*accord to AdMobileResponse, generate mobile response protobuf
  *accord to geo, os, dev, connection, timestamp, enqiure compainID interaction, then reqire campingn according to campaignID set
  */
bool bidderServ::gen_mobile_response_protobuf(string &business_code, string &data_code, string& ttl_time,string &data_str)
{
    ostringstream os;
    MobileAdRequest mobile_request;
    mobile_request.ParseFromString(data_str);

    MobileAdRequest_Device    dev = mobile_request.device();
    MobileAdRequest_User      user = mobile_request.user();
    MobileAdRequest_GeoInfo   geo_info = mobile_request.geoinfo();
    
    string uuid = mobile_request.id();
    //geo
    string country = geo_info.country();
    string region = geo_info.region();
    string city = geo_info.city();  
    //os
    string family = dev.platform();
    string version = dev.platformversion();
    //dev
    string vendor = dev.vender();
    string model = dev.modelname();
    //signal target
    string lang  = dev.language();
    string carrier = geo_info.carrier();

    //get connecttype
    string connectType = dev.connectiontype();
    string timeStamp = mobile_request.timestamp();
    string seimps = mobile_request.session();
    string traffic = mobile_request.trafficquality();
    string app_web = mobile_request.apptype();
    string phone_tablet = dev.devicetype();

    //covert timestamp to daypart array
    time_t time_stamp = (atoll(timeStamp.c_str())/1000);
    struct tm *utc_time = gmtime(&time_stamp);
    int daypart_index = (utc_time->tm_wday-1)*24+utc_time->tm_hour;
    cerr<<"timeStamp:"<<timeStamp<<endl;
    cerr<<"tm_wday:"<<utc_time->tm_wday<<"  tm_hour:"<<utc_time->tm_hour<<endl;
    string daypart;
    intToString(daypart_index, daypart);

    string app;
    string web;

    if(app_web=="app")//"app" "web" "all" "none"
    {
        app="1";
        web="0";
    }
    else if(app_web=="web")
    {
        app="0";
        web="1";
    }
    else if(app_web=="all")
    {
        app=web="1";
    }
    else if(app_web=="none")
    {
        app=web="0";
    }
    else
    {
        app=web="1";;
    }

    string phone;
    string tablet;
    if(phone_tablet=="phone")//"phone" "tablet" "all" "none"
    {
        phone="1";
        tablet="0";
    }
    else if(phone_tablet=="tablet")
    {
        phone="0";
        tablet="1";
    }
    else if(phone_tablet=="all")
    {
        phone=tablet="1";
    }
    else if(phone_tablet=="none")
    {
        phone=tablet="0";
    }
    else
    {
        phone=tablet="1";
    }      

   string ventory = mobile_request.inventoryquality();
   int freqency_size = mobile_request.frequency_size();
   auto frequency = mobile_request.frequency();
   string ad_width = mobile_request.adspacewidth();
   string ad_height = mobile_request.adspaceheight();  
   
   os.str("");
   os<<ad_width<<"_"<<ad_height;
   string cretive_size_str = os.str();
#ifdef DEBUG
    cerr<<"ad_width_ad_height"<<cretive_size_str<<endl;;
#endif   
  // MobileAdRequest_Frequency frequency = mobile_request.  
    target_set target_obj;
    target_obj.add_target_geo("country", country);
    target_obj.add_target_geo("region", region);
    target_obj.add_target_geo("city", city);

    target_obj.add_target_os("family", family);
    target_obj.add_target_os("version", version);
    target_obj.add_target_dev("vendor", vendor);
    target_obj.add_target_dev("model", model);

    target_obj.add_target_signal("lang", lang);
    target_obj.add_target_signal("carrier", carrier);
    target_obj.add_target_signal("conn.type", connectType);
    target_obj.add_target_signal("day.part", daypart);
  //  target_obj.add_target_signal("se.imps",seimps);
    target_obj.add_target_signal("sup.m.app", app);
    target_obj.add_target_signal("sup.m.web", web);
    target_obj.add_target_signal("dev.phone", phone);
    target_obj.add_target_signal("dev.tablet", tablet);
    target_obj.add_target_signal("tra.quality", traffic);
#ifdef DEBUG
    target_obj.display();
    for(auto it = frequency.begin(); it != frequency.end();it++)
    {
       auto fre = *it;
       cerr<<"property:" <<fre.property()<<endl;
       cerr<<"id:" <<fre.id()<<endl; 
       auto value = fre.frequencyvalue();
       for(auto iter = value.begin(); iter != value.end(); iter++)
       {
           auto fre_value = *iter;
           cerr<<"frequencyType:"<<fre_value.frequencytype()<< "       times:"<<fre_value.times()<<endl;
       }
    }
#endif

    vector<string> camp_string_vec;
    if(m_redis_pool_manager.redis_get_camp_pipe(target_obj,camp_string_vec, m_target_converce_num)==false)
    {
        cerr<<"redis_get_camp_pipe error"<<endl;
        return false;
    }
#ifdef DEBUG
    //target_obj.display();
   // display(camp_string_vec);
#endif
    if(camp_string_vec.empty()) return false;

    int bidID=1;  
    bool gen_response = false;
    MobileAdResponse mobile_response;
    CommonMessage response_commMsg;
    for(auto it = camp_string_vec.begin(); it != camp_string_vec.end(); it++)
    {
        string &camp_str = *it;
        campaign_structure campaign_obj;
        if(campaign_obj.parse_campaign(camp_str)==false)
        {
            cout<<"can not parse campaign json"<<endl;
            continue;
        }
#ifdef DEBUG
//        campaign_obj.display();
#endif
        MobileAdResponse_mobileBid *mobile_bidder = NULL;
        bool  gen_mobile_bidder = false;
        campaign_creative* creatives = campaign_obj.find_creative_by_size(cretive_size_str);
        if(creatives)
        {
            auto cre_data = creatives->get_creative_data();
            for(auto cre_data_it = cre_data.begin(); cre_data_it != cre_data.end(); cre_data_it++)
            {
                creative_structure *data = *cre_data_it;
                if(data  &&(campaign_obj.campaign_valid(target_obj, ventory, frequency) == true))
                {
                    string creative_id_str;
                    intToString(data->get_id(), creative_id_str);
                    if(gen_mobile_bidder==false)
                    {
                        mobile_bidder = mobile_response.add_bidcontent();
                        gen_mobile_bidder = true;
                        gen_response = true;
                    }
                    MobileAdResponse_Creative *mobile_creative =  mobile_bidder->add_creative();
                    mobile_creative->set_creativeid(creative_id_str);
                    mobile_creative->set_admarkup(data->get_content());
                    mobile_creative->set_macro(data->get_macro());
                    mobile_creative->set_width(ad_width);
                    mobile_creative->set_height(ad_height);
               }
               else
               {              
               }
            }
        }
        
        if(mobile_bidder)
        {
            string campaign_id_str;
            string bidding_value_str;
            intToString(campaign_obj.get_id(), campaign_id_str);
            doubleToString(campaign_obj.get_biddingValue(), bidding_value_str);
            
            mobile_bidder->set_campaignid(campaign_id_str);
            mobile_bidder->set_biddingtype(campaign_obj.get_biddingType());
            mobile_bidder->set_biddingvalue(bidding_value_str);
            mobile_bidder->set_currency(campaign_obj.get_curency());

            string str_inApp;
            campaign_action cam_action = campaign_obj.get_campaign_action();
            intToString((int)(cam_action.get_inApp()), str_inApp);
            
            MobileAdResponse_Action   *mobile_action = mobile_bidder->mutable_action();
            mobile_action->set_name(cam_action.get_name()); 
            mobile_action->set_content(cam_action.get_content());
            mobile_action->set_actiontype(cam_action.get_actionTypeName());
            mobile_action->set_in_app(str_inApp);
        }
    }

    if(gen_response)
    {
    
        string bid_id_str;
        intToString(bidID, bid_id_str);
        bidID++;
        mobile_response.set_id(mobile_request.id());
        mobile_response.set_bidid(bid_id_str);
        MobileAdResponse_Bidder *bidder_info = mobile_response.mutable_bidder();
        bidder_info->set_bidderid(m_bidder_id);
        int dataSize = mobile_response.ByteSize();
        char *dataBuf = new char[dataSize];
        mobile_response.SerializeToArray(dataBuf, dataSize);  
        response_commMsg.set_businesscode(business_code);
        response_commMsg.set_datacodingtype(data_code);
        response_commMsg.set_ttl(ttl_time);
        response_commMsg.set_data(dataBuf, dataSize);

        dataSize = response_commMsg.ByteSize();
        char* comMessBuf = new char[dataSize];
        response_commMsg.SerializeToArray(comMessBuf, dataSize);
        m_worker_zeromq_lock.lock();
        int ssize = zmq_send(m_response_handler, comMessBuf, dataSize,ZMQ_NOBLOCK);
        m_worker_zeromq_lock.unlock();
        delete[] dataBuf;
        delete[] comMessBuf;
#ifdef DEBUG
        cout<<"recv valid campaign, send to BC"<<endl;
#endif
        if(ssize)
        {
            calSpeed();
        }
    }
    else
    {
#ifdef DEBUG         
       cout<<"can not find valid campaign for request"<<endl;
#endif

    }
    return true;
}

bool bidderServ::gen_vast_response_protobuf(string &business_code, string &data_code, string &data_str, CommonMessage &comm_msg)
{
    ostringstream os;
    VastRequest vast;
    vast.ParseFromString(data_str);
    string uuid = vast.id();
    VastRequest_Device dev=vast.device();
    VastRequest_User user = vast.user();
    string tlanguage = dev.lang();
    string tos = dev.os();
    string tCity = user.cityname();
    
    os.str("");
    os<<"geo_"<<tCity;
    string geo_key = os.str();
    os.str("");
    os<<"os_ver_"<<tos;
    string os_key = os.str();
    os.str("");
    os<<"lang_"<<tlanguage;
    string lang_key = os.str();
    
    vector<string> campaignKey_List;
    vector<string> camp_string_vec;
    campaignKey_List.push_back(geo_key);
    campaignKey_List.push_back(os_key);
    campaignKey_List.push_back(lang_key);

 //   if(m_redis_pool.redis_get_camp_pipe(campaignKey_List,camp_string_vec) == false)
  //  {
    //    return false;
  //  }
    
    if(jsonToProtobuf_compaign(camp_string_vec, business_code, data_code, uuid, comm_msg)==false)
    {
        return false;
    }
    //display(camp_string_vec);
    return true;
}


//the request handler
void bidderServ::handle_ad_request(void* arg)
{
    messageBuf *msg = (messageBuf*) arg;
    if(!msg) return;
    
    char *buf = msg->buf;
    int dataLen = msg->bufSize;
    bidderServ *serv = (bidderServ*) msg->serv;
    if(!buf||!serv)
    {
        delete[] msg->buf;
        delete msg;
        return;
    }

    CommonMessage request_commMsg;
    request_commMsg.ParseFromArray(buf, dataLen);     
    string tbusinessCode = request_commMsg.businesscode();
    string tdataCoding = request_commMsg.datacodingtype();
    string ttl_time = request_commMsg.ttl();
    string request_data=request_commMsg.data();// data
        
    if(tbusinessCode == serv->m_vastBusinessCode)//vast
    {
        cout<<"vast"<<endl;
      //  result = serv->gen_vast_response_protobuf(tbusinessCode, tdataCoding, request_data, response_commMsg);
    }
    else if(tbusinessCode == serv->m_mobileBusinessCode)
    {
        serv->gen_mobile_response_protobuf(tbusinessCode, tdataCoding, ttl_time, request_data);
    }
    else 
    {
        cout << "this businessCode can not analysis:" << tbusinessCode << endl;
    } 
    delete[] msg->buf;
    delete msg;
#ifdef DEBUG
    cerr<<"===========handle_ad_request end==================="<<endl;
#endif
}

/*
  *worker sync handler
  *handler: recv dataLen, then read data, throw to thread handler
  */
void bidderServ::workerBusiness_callback(int fd, short event, void *pair)
{
    bidderServ *serv = (bidderServ*) pair;
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
        return;
    }
    
    char buf[4];
    int len;
    int dataLen;
    while(1)
    {
       len = read(fd, buf, 4);//read data length
       if((len != 4))
       {
          break;
       }
       dataLen = GET_LONG(buf);  
       
       char *request_data = new char[dataLen];
       len = read(fd, request_data, dataLen);
       if(len <= 0)
       {
            delete[] request_data;
            return;
       }
       int idx = len;
       while(idx < dataLen) 
       {
           cout<<"recv data exception:"<<dataLen <<"  idx:"<<idx<<endl;
           len = read(fd, request_data+idx, dataLen-idx);
           if(len <= 0)
           {
                delete[] request_data;
                return;
           }
           idx += len;
       }

       messageBuf *msg= new messageBuf;
       msg->buf = request_data;
       msg->bufSize = len;
       msg->serv = serv;
       if(serv->m_thread_manager.Run(handle_ad_request,(void *)msg)!=0)//not return 0, failure
       {
            cout<<"thread run error"<<endl;
            delete[] request_data;
            delete msg;
       }
   }            
}


bool bidderServ::jsonToProtobuf_compaign(vector<string> &camp_vec, string &tbusinessCode, string &data_coding_type, string &uuid, CommonMessage &commMsg)
{
}

void bidderServ::start_worker()
{
    worker_net_init();
    m_worker_zeromq_lock.init();
    m_base = event_base_new();    
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
    
    m_thread_manager.Init(10000, m_thread_pool_size, m_thread_pool_size);
    m_redis_pool_manager.connectorPool_init(m_config.get_redisIP(), m_config.get_redisPort(), m_thread_pool_size+1);
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
handler_fd_map*  bidderServ::zmq_connect_subscribe(string &subkey)
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


//master zeromq init
void bidderServ::master_net_init()
{ 
    m_zmqContext =zmq_ctx_new(); 
    for(auto it = m_subKey.begin(); it != m_subKey.end();it++)
    {
        zmq_connect_subscribe(*it);
    }   
    cout <<"adVast bind success" <<endl;
}


void bidderServ::masterRestart(struct event_base* base)
{
 
    cout <<"masterRestart::m_throConfChange:"<<m_configureChange<<endl;
    if((m_configureChange&c_update_throttleAddr)==c_update_throttleAddr)
    {
        cout <<"c_master_throttleIP or c_master_throttlePort change"<<endl;
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
         for(auto it = m_subKey.begin(); it !=  m_subKey.end(); it++)
         {
            string &subKey = *it;
            if(zmq_connect_subscribe(subKey))
            {
                m_zeromq_sub_lock.read_lock();
                handler_fd_map *hfMap = get_master_subscribe_zmqHandler(subKey);
                struct event * pEvRead = event_new(base, hfMap->fd, EV_READ|EV_PERSIST, recvRequest_callback, this); 
                hfMap->evEvent = pEvRead;
                m_zeromq_sub_lock.read_write_unlock();
                event_add(pEvRead, NULL);
            }
         }
    }

    if((m_configureChange&c_update_zmqSub)==c_update_zmqSub)//change subsrcibe
    {
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
            m_zeromq_sub_lock.write_lock();
            releaseConnectResource(subKey);
            m_zeromq_sub_lock.read_write_unlock();
        }

        for(auto it = addVec.begin(); it != addVec.end(); it++)
        {
            string &subKey = *it;
            cout <<"add subkey:"<<subKey<<endl; 
            m_zeromq_sub_lock.write_lock();
            handler_fd_map *hfMap = zmq_connect_subscribe(subKey);
            if(hfMap)
            {
                struct event * pEvRead = event_new(base, hfMap->fd, EV_READ|EV_PERSIST, recvRequest_callback, this);  
                event_add(pEvRead, NULL);
                hfMap->evEvent = pEvRead;
            }
            m_zeromq_sub_lock.read_write_unlock();
        }
    } 
}

void bidderServ::worker_net_init()
{ 
    m_zmqContext =zmq_ctx_new();
    m_response_handler = bindOrConnect(true, "tcp", ZMQ_DEALER, m_bidderCollectorIP.c_str(), m_bidderCollectorPort, NULL);  
    cout <<"worker net init success:" <<endl;
}

void bidderServ::addSendUnitToList(void * data,unsigned int dataLen,int channel)
{
    BC_process_t* pro = NULL;
    int frameLen;
    int rc;
    int idx=0;
    int channel_fd = channel;
    const int try_send_times_max=1024;
    try
    {   
        std::unique_ptr<char[]> sendFrame_ptr;
        frameLen = dataLen+4;
        sendFrame_ptr.reset(new char[frameLen]);
        char *sendFrame = sendFrame_ptr.get();
        PUT_LONG(sendFrame, dataLen);
        memcpy(sendFrame+4, data, dataLen);
        do
        {   
            rc = write(channel_fd, sendFrame, frameLen);
            if(rc>0)
            { 
             //   calSpeed();
                break;
            }
            
            m_worker_info_lock.read_lock();
            pro = m_workerList.at((idx++)%m_workerNum);
            channel_fd = pro->channel[0]; 
            if(idx>try_send_times_max)//尝试很多次都不能发送给worker，那么丢弃
            {
                m_worker_info_lock.read_write_unlock();
                break;
            }
            m_worker_info_lock.read_write_unlock();  
       }while(1);
    }
    catch(std::out_of_range &err)
    {
        cerr<<err.what()<<"LINE:"<<__LINE__  <<" FILE:"<< __FILE__<<endl;
        addSendUnitToList(data, dataLen, 0);
    }
    catch(...)
    {
        cout <<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&"<<endl;
        throw;
    }
}

void bidderServ::sendToWorker(char * buf,int bufLen)
{
    static int workerID = 0;
    try
    {
        m_worker_info_lock.read_lock();
        BC_process_t* pro = m_workerList.at((++workerID)%m_workerNum);
        if(pro) 
        {
            int chanel = pro->channel[0];
            m_worker_info_lock.read_write_unlock();
            addSendUnitToList(buf, bufLen, chanel);
        }
        else
        {
            m_worker_info_lock.read_write_unlock();
        }
    }
    catch(...)
    {
        m_worker_info_lock.read_write_unlock();
        addSendUnitToList(buf, bufLen, 0);
        cout <<"sendToWorker exception!"<<endl;
        throw;
    }
}

void *bidderServ::get_master_subscribe_zmqHandler(int fd)
{   
    m_zeromq_sub_lock.read_lock();
    for(auto it = m_handler_fd_map.begin(); it != m_handler_fd_map.end(); it++)
    {
        handler_fd_map *hfMap = *it;
        if(hfMap &&(hfMap->fd == fd))
        {
            void *handler = hfMap->handler;
            m_zeromq_sub_lock.read_write_unlock();
            return handler;
        }
    }
    m_zeromq_sub_lock.read_write_unlock();
    return NULL;
}

handler_fd_map *bidderServ::get_master_subscribe_zmqHandler(string subKey)
{   
    void *handler;
    m_zeromq_sub_lock.read_lock();
    for(auto it = m_handler_fd_map.begin(); it != m_handler_fd_map.end(); it++)
    {
        handler_fd_map *hfMap = *it;
        if(hfMap &&(hfMap->subKey == subKey))
        {
            m_zeromq_sub_lock.read_write_unlock();
            return hfMap;
        }
    }
    m_zeromq_sub_lock.read_write_unlock();
    return NULL;
}
int bidderServ::zmq_get_message(void* socket, zmq_msg_t &part, int flags)
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

void bidderServ::recvRequest_callback(int fd, short __, void *pair)
{
    uint32_t events;
    size_t len=sizeof(events);
    int recvLen = 0;

    bidderServ *serv = (bidderServ*)pair;
    if(serv==NULL) 
    {
        cout<<"serv is nullptr"<<endl;
        return;
    }

    void *handler = serv->get_master_subscribe_zmqHandler(fd);
    int rc = zmq_getsockopt(handler, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        cout <<"subscribe fd: " <<fd <<" error!"<<endl;
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
                serv->sendToWorker(msg_data, recvLen);
            }
            zmq_msg_close(&part);
        }
    }
}

void bidderServ::func(int fd, short __, void *pair)
{
    eventParam *param = (eventParam *) pair;
    bidderServ *serv = (bidderServ*)param->serv;
    char buf[1024]={0};
    int rc = read(fd, buf, sizeof(buf));
    cout<<buf<<endl;
    if(memcmp(buf,"event", 5) == 0)
    {
        cout <<"update connect:"<<getpid()<<endl;
        serv->masterRestart(param->base);
    }    
}

void bidderServ::master_subscribe_async_event_register(struct event_base* base, event_callback_fn func, void *arg)
{ 
    m_zeromq_sub_lock.read_lock();
    for(auto it = m_handler_fd_map.begin(); it != m_handler_fd_map.end(); it++)
    {
       handler_fd_map *hfMap = *it;
       if(hfMap)
       {
          struct event * pEvRead = event_new(base, hfMap->fd, EV_READ|EV_PERSIST, func, arg);  
          event_add(pEvRead, NULL);
          hfMap->evEvent = pEvRead;
       }
    }
    m_zeromq_sub_lock.read_write_unlock();
}


void *bidderServ::throttle_request_handler(void *arg)
{ 
      bidderServ *serv = (bidderServ*) arg;
      struct event_base* base = event_base_new(); 

      eventParam *param = new eventParam;
      param->base = base;
      param->serv = serv;

      //update config event 
      int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, serv->m_eventFd);
      struct event * pEvRead = event_new(base, serv->m_eventFd[0], EV_READ|EV_PERSIST, func, param); 
      event_add(pEvRead, NULL);

      serv->master_subscribe_async_event_register(base,recvRequest_callback , arg);
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
                        m_worker_info_lock.write_lock();
				        m_workerList.push_back(pro); 
                        m_worker_info_lock.read_write_unlock();
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
//#ifdef DEBUG

//#endif

