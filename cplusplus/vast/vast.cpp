#include<iostream>
#include<sstream>
#include<fstream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<zmq.h>
#include<string.h>
#include<time.h>
#include<sys/time.h>
#include "vast.h"

#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"
#include "MobileAdRequest.pb.h"

using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace com::rj::protos::mobile;

vast::vast(configureObject &config)
{
    try
    {
        m_width = config.get_width();
        m_height = config.get_height();
        m_target = config.get_target();
        m_throttleIP = config.get_throttleIP();
        m_throttlePort= config.get_throttlePubVastPort();
        m_runTimes = config.get_runTimes();
        m_adType = config.get_adType();
        m_intervalTime = config.get_intervalTime();
        m_ttlTime = config.get_ttlTime();

        m_imps_lifetime = config.get_lifetime_imps();
        m_imps_month = config.get_month_imps();
        m_imps_week = config.get_week_imps();
        m_imps_day = config.get_day_imps();
        m_imps_fiveminute = config.get_fiveminute_imps();
        m_campaignID=config.get_campaignID();
        m_zmqContext =zmq_ctx_new();
        m_throttle = config.get_throttle();
        
    }
    catch(...)
    {   
        cerr <<"bidderServ structure error"<<endl;
        exit(1);
    }
}

int  vast::structVastFrame(string &vast)
{
    char buf[4096];
    static int sendid = 0;
    CommonMessage comm;
    VastRequest vs;
    ostringstream os;
    
    sprintf(buf, "c1fb6445-010b-45bd-9fd4-%d", sendid++);
    vs.set_id(buf);
    vs.set_player("hk");
    VastRequest_User *user = vs.mutable_user();
    VastRequest_Device *dev = vs.mutable_device();
    
    os<<"CN"<<(sendid%10);
  //  user->set_countrycode(os.str());
    user->set_countrycode("1");
    user->set_regionname("1");
    user->set_cityname("1");

    dev->set_os("1");
    dev->set_lang("1");
    int size = vs.ByteSize();
    vs.SerializeToArray(buf, size);
    
    comm.set_businesscode("1");
    comm.set_ttl(m_ttlTime.c_str());
    comm.set_data(buf, size);
    comm.SerializeToArray(buf, comm.ByteSize());
    vast.assign(buf, comm.ByteSize()); 
    return comm.ByteSize();
}


int  vast::structMobileFrame(string &vast)
{
    char buf[4096];
    static int sendid = 0;
    CommonMessage comm;
    MobileAdRequest mr;

    ostringstream os;
  
    sprintf(buf, "c1fb6445-010b-45bd-9fd4-%d", sendid++);
    mr.set_id(buf);
  //  mr.set_aid("123456");
    MobileAdRequest_User *user = mr.mutable_user();
    MobileAdRequest_Device *dev = mr.mutable_device();
    MobileAdRequest_GeoInfo *geo = mr.mutable_geoinfo();
    MobileAdRequest_Frequency *frequency = mr.add_frequency();
    geo->set_country(m_target.get_country());
    geo->set_region(m_target.get_region());
    geo->set_city(m_target.get_city());
    dev->set_platform(m_target.get_family());
    dev->set_platformversion(m_target.get_version());
    dev->set_vender(m_target.get_vendor());
    dev->set_modelname(m_target.get_model());
    dev->set_language(m_target.get_lang());
    geo->set_carrier(m_target.get_carrier());
    dev->set_connectiontype(m_target.get_conntype());


    
    time_t timestamp;
    time(&timestamp);
    os.str("");
    os<<timestamp<<"000";
    string tmptime = os.str();
    
    //mr.set_timestamp(tmptime);
    string tttime("1417510986959");
    mr.set_timestamp(tttime);
    mr.set_session(m_target.get_session());
    mr.set_apptype(m_target.get_app());
    mr.set_appid(m_target.get_appid());
    mr.set_appcategory(m_target.get_appcategory());
    mr.set_adspacewidth(m_width);
    mr.set_adspaceheight(m_height);
    dev->set_devicetype(m_target.get_phone());
    mr.set_trafficquality(m_target.get_traffic());
    mr.set_inventoryquality(m_target.get_inventory());
    frequency->set_id(m_campaignID);
    frequency->set_property("ca");
    MobileAdRequest_Frequency_FrequencyValue *frequency_value = frequency->add_frequencyvalue();
    frequency_value->set_frequencytype("f");
    frequency_value->set_times(m_imps_lifetime);
    frequency_value = frequency->add_frequencyvalue();
    frequency_value->set_frequencytype("m");
    frequency_value->set_times(m_imps_month);
    frequency_value = frequency->add_frequencyvalue();
    frequency_value->set_frequencytype("w");
    frequency_value->set_times(m_imps_week);
    frequency_value = frequency->add_frequencyvalue();
    frequency_value->set_frequencytype("d");
    frequency_value->set_times(m_imps_day);
    frequency_value = frequency->add_frequencyvalue();
    frequency_value->set_frequencytype("fm");
    frequency_value->set_times(m_imps_fiveminute);

    MobileAdRequest_AppSession *appsession = mr.add_appsession();
    appsession->set_property("ca");
    appsession->set_id(m_campaignID);
    appsession->set_times(m_imps_lifetime);

    MobileAdRequest_AdInsight *insight = mr.add_adinsight();
    insight->set_property("dln");
    insight->add_ids("1");
    insight->add_ids("224"); 
    insight = mr.add_adinsight();
    insight->set_property("dlr");
    insight->add_ids("1");
    insight->add_ids("261");  
    
    int size = mr.ByteSize();
    mr.SerializeToArray(buf, size);
    
    comm.set_businesscode("2");
    comm.set_ttl(m_ttlTime.c_str());
    comm.set_data(buf, size);
    comm.SerializeToArray(buf, comm.ByteSize());
    vast.assign(buf, comm.ByteSize());
  
    return comm.ByteSize();
}



void* vast::establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd)
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
        cout <<pro<<endl;
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

void vast::run(int speed, int cnt)
{
    vector<void *>handlerList;
    
    auto throtConfigList = m_throttle.get_throttleConfigList();
    for(auto it = throtConfigList.begin(); it != throtConfigList.end(); it++)
    {
        throttleConfig* config = *it;
        if(!config) continue;
        void *handler = establishConnect(true, "tcp", ZMQ_DEALER, config->get_throttleIP().c_str(), config->get_throttleAdPort(), NULL);
        if(handler == NULL) 
        {
            cerr <<"vast connect throttle failure"<<endl;
        }
        else
        {
            cerr <<"vast connect throttle success"<<endl;    
            handlerList.push_back(handler);
        }
    }

    int handlerListSize = handlerList.size();
    if(handlerListSize==0)
    {
        cout<<"handlerListSize==0"<<endl;
        return;
    }
  /*  m_pubHandler = establishConnect(true, "tcp",ZMQ_DEALER, m_throttleIP.c_str(), m_throttlePort, NULL);
    if(m_pubHandler == NULL) 
    {
        cout <<"vast connect throttle failure"<<endl;
    }
    else
    {
        cout <<"vast connect throttle success"<<endl;    
    }*/
    
    string adRequest;
    int size = 0;
    int sendT = 0;
    srand(time(0));
    const int numPerMicroSec = 15;
    if(cnt) m_runTimes = cnt;
    while(1)
    {
        adRequest.clear();
        if(m_adType == 0)
        {
            size = structVastFrame(adRequest);
        }
        else
        {
            size = structMobileFrame(adRequest);
        }

        int randNum = rand()%(handlerListSize);
       // cout<<"randNum = "<<randNum<<endl;
        void *handler = handlerList.at(randNum);
        if(zmq_send(handler, adRequest.c_str(), size, 0) >0)
        {
          //  cout<<siii<<endl;
            ++sendT;
          if(speed == 0)
          {
            if(m_intervalTime) usleep(m_intervalTime);
          }
          else
          {
                if(sendT%speed==0)
                {
                    usleep(1000);
                }
          }

          calSpeed();
          if(sendT >= m_runTimes)
          {
             cout<<"send over:"<<sendT<<endl;
             break;
          }            
       }

       // if(m_intervalTime)  usleep(m_intervalTime);
    }
    sleep(2);
    exit(1);
    
}

void vast::calSpeed()
{
    static long long test_begTime;
	static long long test_endTime;
	static int test_count = 0;
	test_count++;
	if(test_count%10000==1)
	{
		struct timeval btime;
		gettimeofday(&btime, NULL);
		test_begTime = btime.tv_sec*1000+btime.tv_usec/1000;
	}
    else if(test_count%10000==0)
    {
		struct timeval etime;
		gettimeofday(&etime, NULL);
		test_endTime = etime.tv_sec*1000+etime.tv_usec/1000;
		cout <<"send request num: "<<test_count<<endl;
		cout <<"cost time: " << test_endTime-test_begTime<<endl;
		cout<<"sspeed: "<<10000*1000/(test_endTime-test_begTime)<<endl;
	}
	else
    {
    }
}

