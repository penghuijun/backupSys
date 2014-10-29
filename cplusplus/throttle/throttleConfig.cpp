
#include"throttleConfig.h"
#include <cstring>
#include<unistd.h>
#include<stdlib.h>

throttleConfig::throttleConfig(const char* configTxt):m_configName(configTxt)
{
     readConfig();
}

bool throttleConfig::get_subString(string &src, char first, char end, string &dst)
{
    int idx1 = src.find(first);
    if(idx1 == string::npos) return false;
    int idx2 = src.find(end);
    if(idx2 == string::npos) return false;
    idx1++;
    if(idx1<=idx2)
    {
        dst = src.substr(idx1, idx2-idx1);
    }
    else
    {
        dst = src.substr(idx2, idx1-idx2);        
    }
    return true;
}

void throttleConfig::readConfig()
{
    try
    {
        string buf;
        static int throttleAddr = 0;
        int symIdx =0;
        string value;
        if(m_infile.is_open()==true)
        {
            m_infile.close();
        }
        
        m_infile.open(m_configName, ios::in);
        if(m_infile.is_open()==false)
        {
            cerr << "sorry, open "<< m_configName << " failure"  << endl;
            exit(1);
        }
        cout << "open "<< m_configName << " success"  << endl;
      
        while(getline(m_infile, buf))
        {
            if(buf.empty() || buf.at(0)== '#' || buf.at(0)==' ') continue;               

            if(buf.compare(0, 10, "throttleIP")==0)
            {
                if(get_subString(buf, '=', ';', m_throttleIP) == false) throw 0;
            }
            else if(buf.compare(0, 14, "throttleAdPort")==0)
            {
               if(get_subString(buf, '=', ';', value) == false) throw 0;     
                m_throttleAdPort= atoi(value.c_str());
            }
            else if(buf.compare(0, 15, "throttlePubPort")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0;     
                m_pubPort= atoi(value.c_str());  
            }
            else if(buf.compare(0, 15, "throttleWorkNum")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0;     
                m_workNum= atoi(value.c_str()); 
            }
            else
            {
            }
        }//while
        m_infile.close();
        if(m_throttleIP.empty()|| m_pubPort == 0 || m_throttleAdPort == 0) throw 0;
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
    cout << "config throttle pub port: " << get_throttlePubPort()<< endl;
    cout << "config throttle worker num: " << get_throttleworkerNum()<< endl;
    cout << "-----------------------------------" << endl;
 
}