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
#include "login.h"
using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;
using namespace com::rj::targeting::protos;
using namespace com::rj::protos::manager;

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

/*
  *get micro time from master progress
  */
unsigned long long bidderServ::getMasterMicroTime()
{
    m_mastertime_lock.lock();
    unsigned long long t = m_masterlocalTm.tv_sec*1000+m_masterlocalTm.tv_usec/1000;
    m_mastertime_lock.unlock(); 
    return t;
}

/*
  *get micro time from child progress
  */
unsigned long long bidderServ::getchildMicroTime()
{
    m_childtime_lock.lock();
    unsigned long long t = m_childlocalTm.tv_sec*1000+m_childlocalTm.tv_usec/1000;
    m_childtime_lock.unlock();
    return t;
}

/*
  *child progress update time on regular time
  */
void* bidderServ::childProcessGetTime(void *arg)
{
    bidderServ *serv = (bidderServ*) arg;
    const int getTimeInterval=1*1000;    
    while(1)
    {
        usleep(getTimeInterval);
        serv->m_childtime_lock.lock();
        gettimeofday(&serv->m_childlocalTm, NULL);
        serv->m_childtime_lock.unlock();
    }

}

/*
  *master progress update time on regular time
  */
void *bidderServ::getTime(void *arg)
{
    bidderServ *serv = (bidderServ*) arg;
    int times = 0;
    const int getTimeInterval=1*1000;    
    while(1)
    {
        usleep(getTimeInterval);
        serv->m_mastertime_lock.lock();
        gettimeofday(&serv->m_masterlocalTm, NULL);
        times++;
        serv->m_mastertime_lock.unlock();
        if((times*getTimeInterval)>(serv->m_heartInterval*1000*1000))
        {
            times = 0;
            if(serv->m_bidder_manager.dropDev())
            {
                serv->m_config.readConfig();
                serv->m_config.display();
                serv->m_bidder_manager.update_throttle(serv->m_config.get_throttle_info());
            }
        }
    }
}
#endif

void bidderServ::readConfigFile()
{
    m_config.readConfig();
}

void bidderServ::calSpeed()
{
     const int printNum = 1000;
     static long long test_begTime;
     static long long test_endTime;

     int couuut = 0;
     m_diplayLock.lock();
     test_count++;
     couuut = test_count;
     if(test_count%printNum==1)
     {
        struct timeval btime;
        gettimeofday(&btime, NULL);
        test_begTime = btime.tv_sec*1000+btime.tv_usec/1000;
        m_diplayLock.unlock();
     }
     else if(test_count%printNum==0)
     {
        struct timeval etime;
        gettimeofday(&etime, NULL);
        test_endTime = etime.tv_sec*1000+etime.tv_usec/1000;
        long long diff = test_endTime-test_begTime;
        long speed = printNum*1000/diff;
        m_diplayLock.unlock();
        g_file_logger->warn("num:{0:d}, cost:{1:d}, speed:{2:d}", couuut, diff, speed);
     }
     else
     {
        m_diplayLock.unlock();
     }
}


/*
  *changer log level
  */
bool bidderServ::logLevelChange()
{
     int logLevel = m_config.get_bidderLogLevel();
     spdlog::level::level_enum log_level;
     if((logLevel>((int)spdlog::level::off) ) || (logLevel < ((int)spdlog::level::trace))) 
     {
         log_level = spdlog::level::info;
     }
     else
     {
         log_level = (spdlog::level::level_enum) logLevel;
     }
     
     if(log_level != m_logLevel)// loglevel change
     {
         g_manager_logger->emerg("log level from level {0:d} to {1:d}", (int)m_logLevel, (int)log_level);
         m_logLevel = log_level;

         return true;
     }
     return false;
}

/*
  *update worker
  */
void bidderServ::updateWorker()
{
    readConfigFile();
    m_bidder_manager.update_bcList(m_zmq_connect, m_config.get_bc_info());//update bc list
    int poolsize = m_bidder_manager.get_bidder_config().get_bidderThreadPoolSize();
    bidderInformation &binfo = m_config.get_bidder_info();
    m_redis_pool_manager.redisPool_update(binfo.get_redis_ip(), binfo.get_redis_port(), poolsize+10);//redis pool update

    if(logLevelChange())//loglevel change
    {
        g_file_logger->set_level(m_logLevel);
        g_manager_logger->set_level(m_logLevel); 
        g_worker_logger->set_level(m_logLevel);
    }

}

bidderServ::bidderServ(configureObject &config):m_config(config)
{
    try
    {
        m_mastertime_lock.init();

        //set log level
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
        //bidder manager init
        m_bidder_manager.init(m_zmq_connect, m_config.get_throttle_info(), m_config.get_bidder_info(), m_config.get_bc_info());
        m_workerNum = m_bidder_manager.get_bidder_config().get_bidderWorkerNum();
        m_vastBusinessCode = m_config.get_vast_businessCode();
        m_mobileBusinessCode = m_config.get_mobile_businessCode();
        m_logRedisOn = m_config.get_logRedisState();
        m_logRedisIP = m_config.get_logRedisIP();
        m_logRedisPort = m_config.get_logRedisPort();

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
        m_diplayLock.init();
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
        g_manager_logger->emerg("bidderServ structure error");
        exit(1);
    }
}

/*
  *update m_workerList,  if m_wokerNum < workerListSize , erase the workerNode, if m_workerList update ,lock the resource
  */
void bidderServ::updataWorkerList(pid_t pid)
{
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
}

void bidderServ::signal_handler(int signo)
{
    time_t timep;
    time(&timep);
    char *timeStr = ctime(&timep);
    switch (signo)
    {
        case SIGTERM:
            g_manager_logger->trace("SIGTERM:{0}", timeStr);
            srv_ungraceful_end = 1;
            break;
        case SIGINT:
            g_manager_logger->trace("SIGINT:{0}", timeStr);
            srv_graceful_end = 1;
            break;
        case SIGHUP:
            g_manager_logger->trace("SIGHUP:{0}", timeStr);
            srv_restart = 1;
            break;
        case SIGUSR1:  
            g_manager_logger->trace("SIGUSR1:{0}", timeStr);
            sigusr1_recved = 1;
            break;
        case SIGALRM:   
            g_manager_logger->trace("SIGALARM:{0}", timeStr);
            sigalrm_recved = 1;
            break;
        default:
            g_manager_logger->trace("signo:{0}", signo);
            break;
    }
}


//parse publish key
bool bidderServ::parse_publishKey(string& origin, string& bcIP, unsigned short &bcManagerPort, unsigned short &bcDataPort,
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


void bidderServ::hupSigHandler(int fd, short event, void *arg)
{
    g_manager_logger->info("signal hup");
    exit(0);
}

void bidderServ::intSigHandler(int fd, short event, void *arg)
{
   g_manager_logger->info("signal int");
   usleep(10);
   exit(0);
}

void bidderServ::termSigHandler(int fd, short event, void *arg)
{
    g_manager_logger->info("signal term");
    exit(0);
}

void bidderServ::usr1SigHandler(int fd, short event, void *arg)
{
    g_manager_logger->info("signal SIGUSR1");
    bidderServ *serv = (bidderServ*) arg;
    if(serv != NULL)
    {
        serv->updateWorker();
    }
    else
    {
        g_manager_logger->info("usr1SigHandler param excepthion");
    }
}

//get bidder target data
bool bidderServ::get_bidder_target(const MobileAdRequest& mobile_request, operationTarget &target_operation_set
                                    , verifyTarget &target_verify_set)
{
    const MobileAdRequest_Device&    dev = mobile_request.device();
    const MobileAdRequest_User&      user = mobile_request.user();
    const MobileAdRequest_GeoInfo&   geo_info = mobile_request.geoinfo();

    //get connecttype
    const string& timeStamp = mobile_request.timestamp();
    const string& app_web = mobile_request.apptype();
    const string& phone_tablet = dev.devicetype();
      //covert timestamp to daypart array
    string daypart; 
    if(timeStamp.empty()==false)
    {
        time_t time_stamp = (atoll(timeStamp.c_str())/1000);
        struct tm *utc_time = gmtime(&time_stamp);
        int daypart_index = (utc_time->tm_wday-1)*24+utc_time->tm_hour;
        intToString(daypart_index, daypart);
    }
      
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
    target_operation_set.add_target_geo("country", geo_info.country());
    target_operation_set.add_target_geo("region", geo_info.region());
    target_operation_set.add_target_geo("city", geo_info.city());
      
    target_operation_set.add_target_os("family", dev.platform());
    target_operation_set.add_target_os("version", dev.platformversion());
    target_operation_set.add_target_dev("vendor", dev.vender());
    target_operation_set.add_target_dev("model", dev.modelname());
  

    target_operation_set.add_target_signal("lang", dev.language());
    target_operation_set.add_target_signal("carrier", geo_info.carrier());
    target_operation_set.add_target_signal("conn.type", dev.connectiontype());
    target_operation_set.add_target_signal("day.part", daypart);


    target_verify_set.set_devphone(phone);
    target_verify_set.set_devtablet(tablet);
    target_verify_set.set_supmapp(app);
    target_verify_set.set_supmweb(web);
    target_verify_set.set_inventory(mobile_request.inventoryquality());
    target_verify_set.set_traffic(mobile_request.trafficquality());
    return true;
}

void bidderServ::frequency_display(shared_ptr<spdlog::logger>& file_logger,
    const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_Frequency >&frequency)
{
    ostringstream os;
    file_logger->debug("---frequency display---");
    
    for(auto it = frequency.begin(); it != frequency.end();it++)
    {
       auto fre = *it;
       os<<fre.property()<<":"<<fre.id()<<endl;
 //      file_logger->debug("property-id:{0}-{1}", fre.property(), fre.id());
       auto value = fre.frequencyvalue();
       for(auto iter = value.begin(); iter != value.end(); iter++)
       {
           auto fre_value = *iter;
           os<<fre_value.frequencytype()<<"@"<<fre_value.times()<<"     ";
        //   file_logger->debug("frequencyType:{0}    times:{1}", fre_value.frequencytype(), fre_value.times()); 
       }
       os<<endl;
    }
    file_logger->debug("{0}",  os.str());
    file_logger->debug("---frequency display---");    
    
}

void bidderServ::appsession_display(shared_ptr<spdlog::logger>& file_logger,
    const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_AppSession>& appsession)
{
    ostringstream os;
    file_logger->debug("---appsession display---");
    for(auto it = appsession.begin(); it != appsession.end();it++)
    {
       auto sess = *it;
       os<<sess.property()<<"@"<<sess.id()<<"@"<<sess.times()<<endl;
     //  file_logger->debug("appsession property-id:{0}-{1}-{2}", sess.property(), sess.id(), sess.times());
    }
    file_logger->debug("{0}",  os.str()); 
    file_logger->debug("---appsession display---");
      
}


//gen mobile response 
char* bidderServ::gen_mobile_response_protobuf(const char* pubKey, campaignInfoManager &ad_campaignInfoManager, 
 const CommonMessage& commMsg, MobileAdRequest& mobile_request, int &dataLen)
{
    bool ret = false;
    CommonMessage response_commMsg;
    MobileAdResponse mobile_response;
    auto ad_campaignList = ad_campaignInfoManager.get_campaignInfoList();
    g_file_logger->trace("cID\tcrID\ttype\tvalue\tcur\tecmp\tfre\tsess\tmtype\tmsubtype");
    for(auto it = ad_campaignList.begin(); it != ad_campaignList.end(); it++)
    {
        campaignInformation* camp_info = *it;
        if(camp_info == NULL) continue;
        MobileAdResponse_mobileBid *mobile_bidder = mobile_response.add_bidcontent();
        /**
         * set MobileAdResponse Creative Message
         */
        MobileAdResponse_Creative  *mobile_creative =  mobile_bidder->add_creative();
        mobile_creative->set_creativeid(camp_info->get_creativeID());
        mobile_creative->set_width(mobile_request.adspacewidth());
        mobile_creative->set_height(mobile_request.adspaceheight());  
        mobile_creative->set_mediatypeid(camp_info->get_mediaTypeID());
        mobile_creative->set_mediasubtypeid(camp_info->get_mediaSubTypeID());
        mobile_creative->set_ctr(camp_info->get_ctr());

        /**
         * add MobileAdResponse Creative Session
         */
        MobileAdResponse_CreativeSession *cs = mobile_creative->mutable_session();
        cs->set_sessionlimit(camp_info->get_creativeSession().sessionlimit());
        
        /**
         * add MobileAdResponse Creative UUID
         */
        MobileAdResponse_UUID *creative_uuid = mobile_creative->mutable_uuid();
        CampaignProtoEntity_UUID &cpuuid = camp_info->get_uuid();
        int cpuuid_len = cpuuid.uuidtype_size();
        for (int i = 0; i < cpuuid_len; ++i)
        {
          if (cpuuid.uuidtype(i) == CampaignProtoEntity_UuidType_FRE)
            creative_uuid->set_uuidtype(i, MobileAdResponse_UuidType_FRE);
          else if (cpuuid.uuidtype(i) == CampaignProtoEntity_UuidType_SESSION)
            creative_uuid->set_uuidtype(i, MobileAdResponse_UuidType_SESSION);

        }

        /**
         * set MobileAdResponse bidContent
         */
        mobile_bidder->set_campaignid(camp_info->get_id());
        mobile_bidder->set_biddingtype(camp_info->get_biddingType());
        mobile_bidder->set_biddingvalue(camp_info->get_biddingValue());
        mobile_bidder->set_currency(camp_info->get_curency());
        mobile_bidder->set_expectcpm(camp_info->get_expectEcmp());

        MobileAdResponse_UUID *mobileUuid = mobile_bidder->mutable_uuid();
        if(camp_info->get_camFrequecy()==true)
        {
            mobileUuid->add_uuidtype(MobileAdResponse_UuidType_FRE);
        }
        if(camp_info->get_camAppSession()==true)
        {
            mobileUuid->add_uuidtype(MobileAdResponse_UuidType_SESSION);
        }

        g_file_logger->trace("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6:d}\t{7:d}\t{8}\t{9}"
            , camp_info->get_id(), camp_info->get_creativeID(),camp_info->get_biddingType(),camp_info->get_biddingValue()
            , camp_info->get_curency(), camp_info->get_expectEcmp(), (int)camp_info->get_camFrequecy()
            , (int)camp_info->get_camAppSession(), camp_info->get_mediaTypeID(), camp_info->get_mediaSubTypeID());
        
        const campaign_action& cam_action = camp_info->get_campaign_action();  
        MobileAdResponse_Action *mobile_action = mobile_bidder->mutable_action();
        mobile_action->set_content(cam_action.get_content());
        mobile_action->set_actiontype(cam_action.get_actionTypeName());
        mobile_action->set_inapp(cam_action.get_inApp());
        ret = true;
    }
         
    if(ret)
    {
        int bidID=1;  
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
        response_commMsg.set_businesscode(commMsg.businesscode());
        response_commMsg.set_datacodingtype(commMsg.datacodingtype());
        response_commMsg.set_ttl(commMsg.ttl());
        response_commMsg.set_data(dataBuf, dataSize);
    
        dataSize = response_commMsg.ByteSize();
        char* comMessBuf = new char[dataSize];
        response_commMsg.SerializeToArray(comMessBuf, dataSize);
        delete[] dataBuf;
        dataLen = dataSize;
        return comMessBuf;
    }
    else
    {
        g_file_logger->info("can not find valid campaign for request");
        return NULL;
    }  
}

/*accord to AdMobileResponse, generate mobile response protobuf
  *accord to geo, os, dev, connection, timestamp, enqiure compainID interaction, then reqire campingn according to campaignID set
  */
bool bidderServ::mobile_request_handler(const char* pubKey, const CommonMessage& request_commMsg)
{
try
{
    unsigned long long starttime=getchildMicroTime();
    const string& request_data = request_commMsg.data();// data   
    MobileAdRequest mobile_request;
    mobile_request.ParseFromString(request_data); 
    const string& uuid = mobile_request.id(); 
    
    ostringstream os;
    if(m_logRedisOn)
    {
        os<<uuid<<"_"<<getchildMicroTime()<<"_bidder*recv";
        string logValue = os.str();
        m_log_redis_manager.redis_lpush("flow_log", logValue.c_str(), logValue.size());
    }
    operationTarget target_operation;
    verifyTarget    target_verify;
    get_bidder_target(mobile_request, target_operation, target_verify);//get target data

    os.str("");
    const string& ad_width = mobile_request.adspacewidth();
    const string& ad_height = mobile_request.adspaceheight(); 
    if((ad_width.empty()==false) ||(ad_height.empty()==false))
    {
        os.str("");
        os<<ad_width<<"_"<<ad_height;
    }
    string cretive_size = os.str();
    auto   frequency = mobile_request.frequency(); 
    auto   appsession = mobile_request.appsession();

    g_worker_logger->debug("===parse reqeust start===");
    g_worker_logger->debug("ad_width_ad_height:{0}", cretive_size);

    g_worker_logger->trace("dnsip:{0}", mobile_request.dnsip());
    g_worker_logger->trace("packagename:{0}", mobile_request.packagename());
    g_worker_logger->debug("uuid:{0}", uuid);

    target_operation.display(g_worker_logger);
    target_verify.display(g_worker_logger);
    frequency_display(g_worker_logger, frequency);
    appsession_display(g_worker_logger, appsession);
    if(mobile_request.has_aid())
    {
        auto   aidstr     = mobile_request.aid();
        g_file_logger->trace("request aid: id, networkid, publishid, appreviewed, retwork_reselling, app_reselling :{0},{1},{2},{3},{4},{5}"
            ,aidstr.id(), aidstr.networkid(), aidstr.publisher_id(), aidstr.app_reviewed(), aidstr.network_reselling(), aidstr.app_resell());
    }

    
    g_worker_logger->debug("===parse reqeust end===");


    bufferManager campaign_string_manager;
    //get campaign list with target from index redis
    if(m_redis_pool_manager.redis_get_camp_pipe(target_operation, campaign_string_manager, m_target_converce_num)==false)
    {
        g_worker_logger->debug("redis_get_camp_pipe error");
        return false;
    }

    //campaign list is null
    if(campaign_string_manager.bufferListSize()==0) 
    {
        g_worker_logger->debug("camp_string_vec.empty()");
        return false;
    }

    g_worker_logger->debug("get campaign count:{0:d}", campaign_string_manager.bufferListSize());

    auto campaignStringList = campaign_string_manager.get_bufferList();
    campaignInfoManager campaign_info_manager;
    //parse campaign list and check sum
    if(campaign_info_manager.parse_campaingnStringList(campaignStringList, target_verify, cretive_size, mobile_request)==false)
    {
        g_worker_logger->debug("have not meet the requirements of the campaign");
        return false;
    }
    int responseDataLen = 0;
    //gen response 
    char *responseDataStr = gen_mobile_response_protobuf(pubKey, campaign_info_manager, request_commMsg, mobile_request, responseDataLen);
    if(responseDataStr&&(responseDataLen>0))//if get response success
    {
        string subscribeKey(pubKey);
        string bcIP;
        unsigned short bcManagerPort;
        unsigned short bcDataPort;
        string bidderIP;
        unsigned short bidderPort;
        //parse publishkey, get dest bc address
        bool parse_result = parse_publishKey(subscribeKey, bcIP, bcManagerPort, bcDataPort,
                    bidderIP, bidderPort, subscribeKey.size());
        if(parse_result == false)
        {
            delete[] responseDataStr;
            return false;
        }
        //sent to the bc
        int ssize = m_bidder_manager.sendAdRspToBC(bcIP, bcDataPort, responseDataStr, responseDataLen, ZMQ_NOBLOCK);
        delete[] responseDataStr;

        if(ssize>0)
        {           
            g_worker_logger->debug("recv valid campaign, send to BC:{0:d},{1},{2},{3:d}"
                , ssize, uuid, bcIP, bcManagerPort);
            calSpeed();
            if(m_logRedisOn)
            {
                os.str("");
                os<<uuid<<"_"<<getchildMicroTime()<<"_bidderend*recv";
                string logValue = os.str();
                m_log_redis_manager.redis_lpush("flow_log", logValue.c_str(), logValue.size());
            }
        }
        else
        {
           g_worker_logger->error("send response to BC failure");   
        }
    }
    else
    {
        g_worker_logger->debug("can not find valid campaign for request");      
    }  
    unsigned long long endtime=getchildMicroTime();
    g_worker_logger->debug("cost time:{0:d}", endtime-starttime);       
    return true;
}
catch(...)
{
    g_worker_logger->error("mobile_request_handler exception");
}
}

//the request handler
void bidderServ::handle_ad_request(void* arg)
{
    char publishKey[PUBLISHKEYLEN_MAX];
    messageBuf *msg = (messageBuf*) arg;
    if(!msg) return;

    try
    {
        char *buf = msg->get_stringBuf().get_data();
        int dataLen = msg->get_stringBuf().get_dataLen();
        bidderServ *serv = (bidderServ*) msg->get_serv();
        memcpy(publishKey, buf, PUBLISHKEYLEN_MAX);
        if(!buf||!serv) throw;

        CommonMessage request_commMsg;
        request_commMsg.ParseFromArray(buf+PUBLISHKEYLEN_MAX, dataLen-PUBLISHKEYLEN_MAX);
        const string& tbusinessCode = request_commMsg.businesscode();
            
        if(tbusinessCode == serv->m_mobileBusinessCode)
        {
            serv->mobile_request_handler(publishKey, request_commMsg);
        }
        else 
        {
            g_worker_logger->error("this businessCode can not analysis:{0}", tbusinessCode);
        } 
        delete msg;
    }
    catch(...)
    {
        delete msg;
        g_worker_logger->error("handle_ad_request exception");
    }
}

/*
  *worker sync handler
  *handler: recv dataLen, then read data, throw to thread handler
  */
void bidderServ::workerBusiness_callback(int fd, short event, void *pair)
{
try
{
    zmq_msg_t msg;
    uint32_t events;
    size_t len;

    bidderServ *serv = (bidderServ*) pair;
    if(serv==NULL) 
    {
        g_worker_logger->emerg("workerBusiness_callback param is null");
        exit(1);
    }

    len = sizeof(events);
    void *adrsp = serv->m_workerPullHandler;
    if(adrsp == NULL) throw 0;
    int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
    if(rc == -1)
    {
        g_worker_logger->error("workerBusiness_callback zmq_getsockopt return -1");
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

            messageBuf *msg= new messageBuf(msg_data, recvLen, serv);
            g_worker_logger->debug("worker recv data length:{0:d}", recvLen);
            if(serv->m_thread_manager.Run(handle_ad_request,(void *)msg)!=0)//not return 0, failure
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

void bidderServ::start_worker()
{
try{
    srand(getpid());
    worker_net_init();
    m_diplayLock.init();
    m_childtime_lock.init();
    m_base = event_base_new(); 

    m_zmq_connect.init();
    pid_t pid = getpid();
    g_worker_logger = spdlog::rotating_logger_mt("worker", "logs/debugfile", 1048576*500, 3, true); 

    g_worker_logger->set_level(m_logLevel);  
    g_worker_logger->info("worker start:{0:d}", getpid());

    m_workerPullHandler = m_zmq_connect.establishConnect(true, "ipc", ZMQ_PULL,  "masterworker",  &m_workerPullFd);
    m_workerPushHandler = m_zmq_connect.establishConnect(true, "ipc", ZMQ_PUSH,  "workermaster",  NULL);    
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

    int poolSize = m_bidder_manager.get_bidder_config().get_bidderThreadPoolSize();
    m_thread_manager.Init(10000, poolSize, poolSize);//thread pool init
    //redis pool init
    m_redis_pool_manager.connectorPool_init(m_bidder_manager.get_redis_ip(), m_bidder_manager.get_redis_port(), poolSize+10);
    //log redis pool init
    m_log_redis_manager.connectorPool_init(m_logRedisIP, m_logRedisPort, poolSize+10);
    event_base_dispatch(m_base);
}
catch(...)
{
    g_worker_logger->emerg("start worker exception");
    exit(1);
}
}



void bidderServ::worker_net_init()
{ 
    m_zmq_connect.init();
    m_bidder_manager.init_bidder();//bidder manager init

    m_bidder_manager.dataConnectToBC(m_zmq_connect);//connect to bc
    
#ifdef TIMELOG
    pthread_t pth;
    pthread_create(&pth, NULL, childProcessGetTime, this);
#endif
}

void bidderServ::sendToWorker(char* subKey, int keyLen, char * buf,int bufLen)
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


//subscribe throttle callback
void bidderServ::recvRequest_callback(int fd, short __, void *pair)
{
    uint32_t events;
    size_t len=sizeof(events);

    bidderServ *serv = (bidderServ*)pair;
    if(serv==NULL) 
    {
        g_manager_logger->emerg("recvRequest_callback param is null");
        return;
    }

    void *handler = serv->m_bidder_manager.get_throttle_handler(fd);
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
           g_file_logger->trace("recv request from throttle:{0:d}", data_len);
           if((sub_key_len>0) &&(data_len>0))
           {
               serv->sendToWorker(subscribe_key, sub_key_len, data, data_len);
           }
           zmq_msg_close(&subscribe_key_part);
           zmq_msg_close(&data_part);
        }
    }
}

//recv login response callback
void bidderServ::recvRsponse_callback(int fd, short __, void *pair)
{
     uint32_t events;
     size_t len=sizeof(events);
     int recvLen = 0;
     ostringstream os;
     bidderServ *serv = (bidderServ*)pair;
     if(serv==NULL) 
     {
        g_manager_logger->emerg("register_throttle_response_handler param is null");
        exit(1);
     }

      void *handler = serv->m_bidder_manager.get_throttle_manager_handler(fd);
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
                const managerProtocol_messageTrans &from  =  manager_pro.messagefrom();
                const managerProtocol_messageType  &type   =  manager_pro.messagetype();
                const managerProtocol_messageValue &value =  manager_pro.messagevalue();
                managerProtocol_messageType responseType;
                serv->manager_handler(from, type, value, responseType);
            }
            zmq_msg_close(&part);
         }
     }
}


void *bidderServ::throttle_request_handler(void *arg)
{ 
      bidderServ *serv = (bidderServ*) arg;
      if(!serv) return NULL;
      struct event_base* base = event_base_new(); 
      serv->m_bidder_manager.subscribe_throttle_event_new(serv->m_zmq_connect, base, recvRequest_callback, arg);
      event_base_dispatch(base);
      return NULL;  
}

//request message handler
void bidderServ::bidderManagerMsg_handler(int fd, short __, void *pair)
{
    uint32_t events;
    size_t len=sizeof(events);
    int recvLen = 0;

    eventArgment *argment = (eventArgment*) pair;
    bidderServ *serv = (bidderServ*)argment->get_serv();
    
    if(serv==NULL) 
    {
        g_manager_logger->emerg("serv is nullptr");
        exit(1);
    }

    void *handler = serv->m_bidder_manager.get_bidderLoginHandler();
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
                serv->manager_handler(handler, identify, from, type, value, argment->get_base(), serv);           
           }            
           zmq_msg_close(&part);
        }
    }
}


void *bidderServ::bidderManagerHandler(void *arg)
{ 
      bidderServ *serv = (bidderServ*) arg;
      if(!serv) return NULL;
      struct event_base* base = event_base_new(); 
      eventArgment *event_arg = new eventArgment(base, arg);
  
      serv->m_bidder_manager.subscribe_throttle_event_new(serv->m_zmq_connect, base, recvRequest_callback, arg);   
      serv->m_bidder_manager.login_event_new(serv->m_zmq_connect, base, bidderManagerMsg_handler, event_arg);
      serv->m_bidder_manager.connectAll(serv->m_zmq_connect, base, recvRsponse_callback, arg);
      event_base_dispatch(base);
      return NULL;  
}

//send login or heart to bc, and register key to throttle
void *bidderServ::sendHeartToBC(void *bidder)
{
    bidderServ *serv = (bidderServ*) bidder;
    sleep(1);
    while(1)
    {
        serv->m_bidder_manager.loginOrHeartReqToBc(serv->m_config);
        serv->m_bidder_manager.registerBCToThrottle();
        sleep(serv->m_heartInterval);
    }
}

//handler message from bc
bool bidderServ::manager_from_BC_handler(const managerProtocol_messageType &type
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
            g_manager_logger->info("[login req][bidder <- bc]:{0},{1:d},{2:d}", ip, mangerPort, dataPort);    
            m_bidder_manager.add_bc(m_zmq_connect, base, recvRsponse_callback, arg, ip, mangerPort, dataPort); 
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
            ret = true;
            rspType = managerProtocol_messageType_LOGIN_RSP;
            break;
        }
        case managerProtocol_messageType_LOGIN_RSP:
        {
            g_manager_logger->info("[login rsp][bidder <- bc]:{0},{1:d}", ip, mangerPort);
            m_bidder_manager.loginedBC(ip, mangerPort); 
            break;
        }
        case managerProtocol_messageType_HEART_RSP:
        {
            g_manager_logger->info("[heart rsp][bidder <- bc]:{0},{1:d}", ip, mangerPort);
            m_bidder_manager.recv_BCHeartBeatRsp(ip, mangerPort);
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


//handler message from throttle
bool bidderServ::manager_from_throttle_handler(const managerProtocol_messageType &type
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
            g_manager_logger->info("[login req][bidder <- throttle]:{0},{1:d},{2:d}", ip, mangerPort, dataPort);
            m_bidder_manager.add_throttle(m_zmq_connect, base, recvRsponse_callback
                                        , recvRequest_callback,arg, ip, mangerPort, dataPort);
            ret = true;
            rspType = managerProtocol_messageType_LOGIN_RSP;
            break;
        }
        case managerProtocol_messageType_LOGIN_RSP:
        {
            //add_bidder_to_throttle_response
            g_manager_logger->info("[login rsp][bidder <- throttle]:{0},{1:d},{2:d}", ip, mangerPort, dataPort);
            m_bidder_manager.loginedThrottle(ip, mangerPort);
            break;
        }
        case managerProtocol_messageType_HEART_REQ:
        {
            //throttle_heart_to_bidder_requset:
            g_manager_logger->info("[heart req][bidder <- throttle]:{0}, {1:d}", ip, mangerPort);
            ret = m_bidder_manager.recv_throttleHeartReq(ip, mangerPort);
            rspType = managerProtocol_messageType_HEART_RSP;
            break;
        }
        case managerProtocol_messageType_REGISTER_RSP:
        {
            //bidder_register_to_throttle_response
            const string& key = value.key();
            g_manager_logger->info("[register response][bidder <- throttle]:{0},{1:d},{2:d}", ip, mangerPort, dataPort);
            m_bidder_manager.set_publishKey_registed(ip, mangerPort, key);
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

bool bidderServ::manager_handler(const managerProtocol_messageTrans &from
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

bool bidderServ::manager_handler(void *handler, string& identify,  const managerProtocol_messageTrans &from
            ,const managerProtocol_messageType &type, const managerProtocol_messageValue &value,struct event_base * base, void * arg)
{
    managerProtocol_messageType rspType;
    bool ret = manager_handler(from, type, value, rspType, base, arg);
    if(ret)
    {
        bidderConfig& configure = m_bidder_manager.get_bidder_config();
        int sndsize = managerProPackage::send_response(handler, identify, managerProtocol_messageTrans_BIDDER, rspType
            , configure.get_bidderIP(), configure.get_bidderLoginPort());
    }
    return ret;
}

bool bidderServ::masterRun()
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

void bidderServ::updateConfigure()
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

        bidderInformation &bidder_info = m_config.get_bidder_info();
        if(m_bidder_manager.update(bidder_info) == true)
        {
            int oldNum = m_workerNum;
            m_workerNum = m_bidder_manager.get_bidder_config().get_bidderWorkerNum(); 
            g_manager_logger->emerg("worker number from {0:d} to {1:d}", oldNum, m_workerNum);
        }
        //m_throttleManager.updateDev(m_config.get_bidder_info(), m_config.get_bc_info(), m_config.get_connector_info());
    }
    catch(...)
    {
        g_manager_logger->error("updateConfigure exception");
    }

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
                updateConfigure();//read configur file
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
				        m_workerList.push_back(pro); 
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

