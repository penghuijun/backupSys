
#include"bcConfig.h"
#include <cstring>
#include<unistd.h>
#include<stdlib.h>

configureObject::configureObject(const char* configTxt):m_configName(configTxt)
{
     readConfig();
}

/*
  *auth:yanjun.xiang
  *function name : readConfig
  *return:  void 
  *function: read configure file
  */
bool configureObject::get_subString(string &src, char first, char end, string &dst)
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

void configureObject::readConfig()
{
    try
    {
        string buf;
        int symIdx =0;
        string value;
        if(m_infile.is_open()==true)//if file is open ,close the file first
        {
            m_infile.close();
        }
        
        m_infile.open(m_configName, ios::in);//open file
        if(m_infile.is_open()==false)
        {
            cerr << "sorry, open "<< m_configName << " failure"  << endl;
            exit(1);
        }
        cout << "open "<< m_configName << " success"  << endl;

        m_subKey.clear();//clear subKey ,reload subscribe key

        bool block_started = false;//block start sym;
        while(getline(m_infile, buf))
        {
            if(buf.empty() || buf.at(0)== '#' || buf.at(0)==' ') continue;// Comments or blank lines

            if(block_started)
            {
                symIdx = buf.find('}');
                if(symIdx != string::npos)
                {
                    block_started = false;
                }
                else
                {
                    symIdx = buf.find(';');
                    if(symIdx==string::npos) 
                    {
                        cerr <<"lack ;"<<endl;
                    }
                    else
                    {
                        string sub_key;
                        sub_key=buf.substr(0,symIdx);
                        m_subKey.push_back(sub_key);
                    }
                }
                continue;
            }

            if(buf.compare(0, 8, "bcServIP")==0)
            {
                if(get_subString(buf, '=', ';', m_bcIP) == false) throw 0;
            }
            else if(buf.compare(0, 15, "bcListenBidPort")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_bcListenBidderPort = atoi(value.c_str());
   
            }
            else if(buf.compare(0, 10, "throttleIP")==0)
            {
                if(get_subString(buf, '=', ';', m_throttleIP) == false) throw 0;
            }
            else if(buf.compare(0, 19, "throttlePubVastPort")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0;     
                m_throttlePubVastPort= atoi(value.c_str());
            }
            else if(buf.compare(0, 15, "redisSaveServIP")==0)
            {
                if(get_subString(buf, '=', ';', m_redisIP) == false) throw 0;
            }
            else if(buf.compare(0, 17, "redisSaveServPort")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_redisPort = atoi(value.c_str());
            }
            else if(buf.compare(0, 17, "redisSaveServTime")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_redisSaveTime = atoi(value.c_str());
            }
            else if(buf.compare(0, 15, "redisConnectNum")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_redisConnectNum = atoi(value.c_str());
            }
            else if(buf.compare(0, 12, "workerNumber")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_workNum= atoi(value.c_str());
            }
            else if(buf.compare(0, 6, "subKey")==0 && ((symIdx = buf.find('{')) != string::npos) )
            {
                block_started = true;
            }
        }//while

        m_infile.close();
        if(block_started==true)
        {
            cerr<<"error: expected '}' at end of input"<<endl;
            throw 0;
        }
        if(m_throttleIP.empty()||m_throttlePubVastPort==0|| m_bcIP.empty()|| m_bcListenBidderPort==0||m_redisIP.empty()|| m_redisPort==0) throw 0;
        if(m_workNum==0 || m_workNum>100) m_workNum = 1;                  
        if(m_redisSaveTime<=10) m_redisSaveTime=10;
    }
    catch(...)
    {
        cout << get_configFlieName() <<":config error" << endl;
        m_infile.close();
        exit(1);
    }    
}


void configureObject::display() const
{

    cout << "-----------------------------------" << endl;
    cout << "config file name: "<< get_configFlieName() << endl;
    cout << "config bc worker num: " << get_workerNum()<< endl;
    cout << "config bidder collector ip: " << get_bcIP()<< endl;
    cout << "config bc listen bidder port: " << get_bcListenBidderPort()<< endl;

    cout << "config redis ip: " << get_redisIP()<< endl; 
    cout << "config redis port: " << get_redisPort()<< endl; 
    cout << "config redis save time: " << get_redisSaveTime()<< endl; 
    cout << "config redis connect num: " <<get_redisConnectNum()<<endl;
    cout << "config throttle ip: " << get_throttleIP()<< endl;
    cout << "config throttle port: " << get_throttlePubVastPort()<< endl; 
    cout <<"bidder sub key:"<<endl;
    auto it = m_subKey.begin();
    for(it; it != m_subKey.end(); it++)
    {
        cout <<*it<<endl;
    }
    cout << "-----------------------------------" << endl;
 
}