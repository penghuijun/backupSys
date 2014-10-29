
#include"bidderConfig.h"
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

        vector<string> tmpVec;
        bool getIP=false;
        string exBidderIP;
        unsigned short exBidderPort=0;
        unsigned short connectNum=0;
        while(getline(m_infile, buf))
        {
            if(buf.empty() || buf.at(0)== '#' || buf.at(0)==' ') continue;// Comments or blank lines

            if(block_started)
            {
                symIdx = buf.find('}');
                if(symIdx != string::npos)
                {
                    tmpVec.clear();
                    getIP = false;
                    block_started = false;
                }
                else
                {   
                    if(buf.compare(0, 6, "subKey")==0)
                    {
                        if(get_subString(buf, '=', ';', value) == false) throw 0;
                        tmpVec.push_back(value);
                        m_subKey.push_back(value); 
                    }
                    else if(buf.compare(0, 10, "exBidderIP")==0)
                    {
                        if(get_subString(buf, '=', ';', exBidderIP) == false) throw 0;
                        getIP = true;
                    }
                    else if(buf.compare(0, 12, "exBidderPort")==0)
                    {
                        if(get_subString(buf, '=', ';', value) == false) throw 0;   
                        exBidderPort = atoi(value.c_str());
                    }
                    else if(buf.compare(0, 10, "connectNum")==0)
                    {
                        if(get_subString(buf, '=', ';', value) == false) throw 0;   
                        connectNum = atoi(value.c_str());
                        if(!tmpVec.empty() && exBidderPort != 0 && getIP)
                        {
                            exBidderInfo *bid = new exBidderInfo;
                            bid->exBidderIP = exBidderIP;
                            bid->exBidderPort = exBidderPort;
                            bid->subKey = tmpVec;
                            bid->connectNum=connectNum;
                            if(connectNum==0)bid->connectNum=1;
                            
                            m_exBidderList.push_back(bid);
                            getIP=false;
                            tmpVec.clear();
                        }
                        else
                        {
                            throw 0;
                        }
                    }
                }
                continue;
            }

            if(buf.compare(0, 10, "throttleIP")==0)
            {
                if(get_subString(buf, '=', ';', m_throttleIP) == false) throw 0;
            }
            else if(buf.compare(0, 12, "throttlePort")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0;     
                m_throttlePort= atoi(value.c_str());
            }
            else if(buf.compare(0, 17, "bidderCollectorIP")==0)
            {
                if(get_subString(buf, '=', ';', m_bidderCollectorIP) == false) throw 0;
            }
            else if(buf.compare(0, 19, "bidderCollectorPort")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_bidderCollectorPort = atoi(value.c_str());
            }
            else if(buf.compare(0, 7, "redisIP")==0)
            {
                if(get_subString(buf, '=', ';', m_redis_ip) == false) throw 0;
            }
            else if(buf.compare(0, 9, "redisPort")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_redis_port = atoi(value.c_str());
            }
            else if(buf.compare(0, 12, "redisConnNum")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_redis_connNum = atoi(value.c_str());
            }
            else if(buf.compare(0, 9, "threadNum")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_thread_num = atoi(value.c_str());
            }
            else if(buf.compare(0, 13, "bidderWorkNum")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_workNum= atoi(value.c_str());
            }
            else if(buf.compare(0, 8, "exBidder")==0 && ((symIdx = buf.find('{')) != string::npos) )//find exbidder
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
        if(m_throttleIP.empty()|| m_throttlePort == 0|| m_bidderCollectorIP.empty()|| m_bidderCollectorPort==0) throw 0;
        if(m_workNum==0 || m_workNum>100) m_workNum = 1;                  
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
    cout << "config throttle ip: " << get_throttleIP()<< endl;
    cout << "config throttle port: " << get_throttlePort()<< endl; 
    cout << "config bidder collector ip: " << get_bidderCollectorIP()<< endl;
    cout << "config bidder collector port: " << get_bidderCollectorPort()<< endl; 
    cout << "config worker num: " << get_bidderworkerNum()<< endl;
    cout << "config thread num: " << get_threadNum()<<endl;
    cout << "config redis ip: " << get_redisIP()<< endl;
    cout << "config redis port: " << get_redisPort()<< endl; 
    cout << "config redis connect pool size: " << get_redisConnNum()<< endl; 
    cout <<"extern bidder info:"<<endl;
    auto it = m_exBidderList.begin();
    for(it; it != m_exBidderList.end(); it++)
    {
        exBidderInfo *bid = *it;
        if(bid == NULL) continue;
        cout <<"extern bidder ip:"<<bid->exBidderIP<<endl;
        cout <<"extern bidder port:"<<bid->exBidderPort<<endl;
        auto itor = bid->subKey.begin();
        for(itor = bid->subKey.begin(); itor != bid->subKey.end(); itor++)
        {
            cout<<"sub key: "<<*itor<<endl;
        }
    }
    
    cout << "-----------------------------------" << endl;
 
}