
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

bool configureObject::parseSubInfo(string& subStr, string& oriStr, vector<string>& m_subList)
{
    ostringstream os;
    int first_index=0;
    int second_index=0;
    cout<<oriStr<<endl;
    if((subStr.empty()==false)&&(oriStr.empty()==false))
    {
        for(auto it = oriStr.begin(); it != oriStr.end(); it++, second_index++)
        {
            if(*it == ',')
            {
                if(second_index>first_index)
                {
                    string sub_value = oriStr.substr(first_index, second_index-first_index);
                    first_index=(second_index+1);
                    if(sub_value.empty()==false)
                    {
                        os.str("");
                        os<<subStr;
                        os<<"_";
                        os<<sub_value;
                        os<<"SUB";
                        string sub_str = os.str();
                        m_subKey.push_back(sub_str);                  
                    }
                }
            }
        }

        if(first_index<second_index)
        {
            string sub_value = oriStr.substr(first_index, second_index-first_index);
            if(sub_value.empty()==false)
            {
                os.str("");
                os<<subStr;
                os<<"_";
                os<<sub_value;
                os<<"SUB";
                string sub_str = os.str();
                m_subKey.push_back(sub_str);                  
            }
        }
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
        ostringstream os;
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
        cerr << "open "<< m_configName << " success"  << endl;

        string subStr;
        bool block_started = false;//block start sym;
        while(getline(m_infile, buf))
        {
            if(buf.empty() || buf.at(0)== '#' || buf.at(0)==' ') continue;// Comments or blank lines
            if(block_started)
            {
                symIdx = buf.find('}');
                if(symIdx != string::npos)//find "}"
                {
                    block_started = false;
                }
                else
                {   
                    if(buf.compare(0, 6, "subKey")==0)
                    {
                        if(get_subString(buf, '=', ';', value) == false) throw 0;    
                        subStr = value;
                            
                    }
                    else if(buf.compare(0, 7, "subPipe")==0)
                    {
                        if(get_subString(buf, '=', ';', value) == false) throw 0;
                        m_subKey.clear();
                        parseSubInfo(subStr, value, m_subKey);
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
            else if(buf.compare(0, 14, "threadPoolSize")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_threadPoolSize = atoi(value.c_str());
            }
            else if(buf.compare(0, 12, "workerNumber")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_workNum= atoi(value.c_str());
            }
            else if(buf.compare(0, 13, "subscribeInfo")==0 && ((symIdx = buf.find('{')) != string::npos) )
            {
                block_started = true;
            }
            else if(buf.compare(0, 16, "vastBusinessCode")==0)
            {
                if(get_subString(buf, '=', ';', m_vastBusiCode) == false) throw 0;
            }
            else if(buf.compare(0, 18, "mobileBusinessCode")==0)
            {
                if(get_subString(buf, '=', ';', m_mobileBusiCode) == false) throw 0;
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
        cerr << get_configFlieName() <<":config error" << endl;
        m_infile.close();
        exit(1);
    }    
}


void configureObject::display() const
{

    cerr << "-----------------------------------" << endl;
    cerr << "config file name: "<< get_configFlieName() << endl;
    cerr << "config bc worker num: " << get_workerNum()<< endl;
    cerr << "config bidder collector ip: " << get_bcIP()<< endl;
    cerr << "config bc listen bidder port: " << get_bcListenBidderPort()<< endl;

    cerr << "config redis ip: " << get_redisIP()<< endl; 
    cerr << "config redis port: " << get_redisPort()<< endl; 
    cerr << "config redis save time: " << get_redisSaveTime()<< endl; 
    cerr << "config redis connect num: " <<get_threadPoolSize()<<endl;
    cerr << "config throttle ip: " << get_throttleIP()<< endl;
    cerr << "config throttle port: " << get_throttlePubVastPort()<< endl; 
    cerr << "config vast business code: " << get_vastBusiCode()<< endl;
    cerr << "config mobile business code: " << get_mobileBusiCode()<< endl;    
    cerr <<"bidder sub key:"<<endl;
    auto it = m_subKey.begin();
    for(it; it != m_subKey.end(); it++)
    {
        cerr <<*it<<endl;
    }
    cerr << "-----------------------------------" << endl;
 
}
