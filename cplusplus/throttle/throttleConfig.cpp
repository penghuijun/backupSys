
#include"throttleConfig.h"
#include <cstring>
#include<unistd.h>
#include<stdlib.h>

throttleConfig::throttleConfig(const char* configTxt):m_configName(configTxt)
{
    try
    {
        throttleInfo tInfo; 
        static int redisElemExsit = 0;
        static int throttleAddr = 0;
        int symIdx =0;
        string value;

        string tpublishRedisIP;
        unsigned short tpublishRedisPort;
        string tbusiness;
        string tcountry;
        string tlanguage;
        string tos;
        string tbrowser;
       
        m_infile.open(configTxt, ios::in);
        if(m_infile.is_open()==false)
        {
            cerr << "sorry, open "<< m_configName << " failure"  << endl;
            exit(1);
        }
        cout << "open "<< m_configName << " success"  << endl;
        string buf;
        while(getline(m_infile, buf))
        {
            if(buf.empty() || buf.at(0)== '#' || buf.at(0)==' ') continue;               

            if(buf.compare(0, 10, "throttleIP")==0 && ((symIdx = buf.find('=')) != string::npos) )
            {
                value = buf.substr(++symIdx);
                m_throttleIP= value;
                throttleAddr = 0x01;
            }
            else if(buf.compare(0, 14, "throttleAdPort")==0 && ((symIdx = buf.find('=')) != string::npos) )
            {
                value = buf.substr(++symIdx);
                m_throttleAdPort = atoi(value.c_str());
                throttleAddr |= (0x01<<1);
            }
            else if(buf.compare(0, 18, "throttleExpirePort")==0 && ((symIdx = buf.find('=')) != string::npos) )
            {
                value = buf.substr(++symIdx);
                m_throttleExpirePort = atoi(value.c_str());
                throttleAddr |= (0x01<<2);
            }
            else if(buf.compare(0, 20, "throttleServPushPort")==0 && ((symIdx = buf.find('=')) != string::npos) )
            {
                value = buf.substr(++symIdx);
                m_servPushPort= atoi(value.c_str());
                throttleAddr |= (0x01<<3);
            }  
            else if(buf.compare(0, 20, "throttleServPullPort")==0 && ((symIdx = buf.find('=')) != string::npos) )
            {
                value = buf.substr(++symIdx);
                m_servPullPort = atoi(value.c_str());
            }
            else if(buf.compare(0, 15, "throttlePubPort")==0 && ((symIdx = buf.find('=')) != string::npos) )
            {
                value = buf.substr(++symIdx);
                m_pubPort= atoi(value.c_str());
                throttleAddr |= (0x01<<5);
            }
            else if(buf.compare(0, 15, "throttleWorkNum")==0 && ((symIdx = buf.find('=')) != string::npos) )
            {
                value = buf.substr(++symIdx);
                m_workNum= atoi(value.c_str());
            }
            else
            {
            }
        }//while

        if(m_throttleIP.empty()|| m_pubPort == 0|| m_servPullPort == 0|| m_servPushPort == 0 || m_throttleAdPort == 0|| m_throttleExpirePort==0||m_throttleExpirePort ==0) throw;
        if(m_workNum==0 || m_workNum>100) m_workNum = 1;
                     
    }
    catch(...)
    {

        cout << get_configFlieName() <<":config error" << endl;
        m_infile.close();
        exit(1);
    }
}



void throttleConfig::display() const
{

    cout << "-----------------------------------" << endl;
    cout << "config file name: "<< get_configFlieName() << endl;
    cout << "config throttle ip: " << get_throttleIP()<< endl;
    cout << "config throttle Ad port: " << get_throttleAdPort()<< endl;
    cout << "config throttle expire port: " << get_throttleExpirePort()<< endl;
    cout << "config throttle serv push port: " << get_throttleServPushPort()<< endl;
    cout << "config throttle serv pull port: " << get_throttleServPullPort()<< endl;
    cout << "config throttle pub port: " << get_throttlePubPort()<< endl;
    cout << "config throttle worker num: " << get_throttleworkerNum()<< endl;
    cout << "-----------------------------------" << endl;
 
}