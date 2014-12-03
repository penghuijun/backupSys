
#include"bidderConfig.h"
#include <cstring>
#include<unistd.h>
#include<stdlib.h>

configureObject::configureObject(const char* configTxt):m_configName(configTxt)
{
     readConfig();
}

bool configureObject::readFileToString(const char* file_name, string& fileData)
{


    ifstream file(file_name,  std::ifstream::binary);
   
    if(file)
    {
        // Calculate the file's size, and allocate a buffer of that size.
        file.seekg(0, file.end);
        const int file_size = file.tellg();
        char* file_buf = new char [file_size+1];
        //make sure the end tag \0 of string.

        memset(file_buf, 0, file_size+1);
       
        // Read the entire file into the buffer.
        file.seekg(0, ios::beg);
        file.read(file_buf, file_size);


        if(file)
        {
            fileData.append(file_buf);
        }
        else
        {
            std::cout << "error: only " <<  file.gcount() << " could be read";
            fileData.append(file_buf);
            return false;
        }
        file.close();
        delete []file_buf;
    }
    else
    {
        return false;
    }


    return true;
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
    //cout<<oriStr<<endl;
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
        ostringstream os;
        string buf;
        int symIdx =0;
        string value;
        if(m_infile.is_open()==true)//if file is open ,close the file first
        {
            m_infile.close();
        }

        vector<string> tmp_sub_vec;
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
        string subStr;
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
                        parseSubInfo(subStr, value, m_subKey);
                    }
                }
                continue;
            }
            if(buf.compare(0, 8, "bidderID")==0)
            {
                if(get_subString(buf, '=', ';', m_bidderID) == false) throw 0;
            }
            else if(buf.compare(0, 10, "throttleIP")==0)
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
            else if(buf.compare(0, 14, "threadPoolSize")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_threadPoolSize = atoi(value.c_str());
            }
            else if(buf.compare(0, 13, "bidderWorkNum")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_workNum= atoi(value.c_str());
            }
            else if(buf.compare(0, 14, "convergenceNum")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_converce_num = atoi(value.c_str());
            }
            else if(buf.compare(0, 16, "vastBusinessCode")==0)
            {
                if(get_subString(buf, '=', ';', m_vastBusiCode) == false) throw 0;
            }
            else if(buf.compare(0, 18, "mobileBusinessCode")==0)
            {
                if(get_subString(buf, '=', ';', m_mobileBusiCode) == false) throw 0;
            }
            else if(buf.compare(0, 13, "subscribeInfo")==0 && ((symIdx = buf.find('{')) != string::npos) )//find exbidder
            {
                block_started = true;
                subStr.clear();
                m_subKey.clear();
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
    cout << "config bidder id: "<< get_bidderID()<< endl;
    cout << "config throttle ip: " << get_throttleIP()<< endl;
    cout << "config throttle port: " << get_throttlePort()<< endl; 
    cout << "config bidder collector ip: " << get_bidderCollectorIP()<< endl;
    cout << "config bidder collector port: " << get_bidderCollectorPort()<< endl; 
    cout << "config worker num: " << get_bidderworkerNum()<< endl;
    cout << "config thread num: " << get_threadPoolSize()<<endl;
    cout << "config redis ip: " << get_redisIP()<< endl;
    cout << "config redis port: " << get_redisPort()<< endl; 
    cout << "config vast business code: " << get_vastBusiCode()<< endl; 
    cout << "config mobile business code: " << get_mobileBusiCode()<< endl; 
    cout << "config converce num: " << get_converceNum()<< endl; 
    cout <<"subscribe:"<<endl;

    for(auto it=m_subKey.begin(); it != m_subKey.end(); it++)
    {
        cout<<*it<< "   ";
    }
    cout<<endl;
    cout << "-----------------------------------" << endl;
 
}