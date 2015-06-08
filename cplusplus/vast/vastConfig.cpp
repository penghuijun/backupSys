
#include"vastConfig.h"
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

bool  configureObject::string_parse(string &ori, const char* dst, string &get_str)
{
    int len = strlen(dst);
    if(ori.compare(0, len, dst)==0)
    {
        return get_subString(ori, '=', ';', get_str);
    }
    return false;
                    
} 

bool configureObject::string_find(string& str1, const char* str2)
{
    return (str1.compare(0, strlen(str2), str2)==0);
}

throttleConfig* configureObject::parse_throttle_config(string &str)
{
try
{
    size_t str_size = str.size();
    if(str.empty() || str.at(0)== '#' ) return NULL;// Comments or blank lines

    size_t startIdx = str.find('{');
    if(startIdx == string::npos) return NULL;
    size_t endIdx = str.find('}');
    if(endIdx == string::npos) return NULL;

    string dst = str.substr(startIdx+1, endIdx-startIdx-1); 
    if(dst.empty()) return NULL;

    int first_index=0;
    int second_index=0;      
    vector<string> subStrList;
    const int throttleConfig_item_num=6;
    if(parseBraces(dst, subStrList)==true)
    {
        if(subStrList.size() != throttleConfig_item_num)   return NULL;
        string& str_id = subStrList.at(0);
        string& str_ip = subStrList.at(1);
        string& str_adport = subStrList.at(2);
        string& str_pubPort = subStrList.at(3);
        string& str_managerPort = subStrList.at(4);
        string& str_workerNum = subStrList.at(5);
        if((is_number(str_id) == false)||(is_number(str_adport) == false)|| (is_number(str_pubPort) == false)
            ||(is_number(str_managerPort) == false)||(is_number(str_workerNum) == false)) 
        {
            cout<<"throttle config format error:number"<<endl;
            return NULL;
        }
        int     number_id = atoi(str_id.c_str());
        int     number_adPort = atoi(str_adport.c_str());
        int     number_pubPort = atoi(str_pubPort.c_str());
        int     number_workerNum = atoi(str_workerNum.c_str());
        int     number_managerPort = atoi(str_managerPort.c_str());
        const unsigned short short_max = 0xFFFF;
        const unsigned short worker_max = 20;
        if((number_adPort>short_max)||(number_pubPort>short_max)||(number_managerPort > short_max))
        {
            cout<<"port out of range"<<endl;
            return NULL;
        }
        if(number_workerNum>worker_max) 
        {
            cout<<"worker number out of range"<<endl;
            return NULL;
        }
        throttleConfig *throttle = new throttleConfig;
        throttle->set_throttleID(number_id);
        throttle->set_throttleIP(str_ip);
        throttle->set_throttleAdPort((unsigned short) number_adPort);
        throttle->set_throttlePubPort((unsigned short) number_pubPort);
        throttle->set_throttleWorkerNum((unsigned short) number_workerNum);
        throttle->set_throttleManagerPort((unsigned short) number_managerPort);
        return throttle;
    }
    return NULL;
}
catch(...)
{
    exit(1);
}
}


void block_over(string& line, int init_left_brace, int &res_left_brace)
{
try
{
    int left_brace = init_left_brace;
    int right_brace = 0;
    size_t size = line.size();
    for(auto i = 0; i < size; i++)
    {
        char ch = line.at(i);
        if(ch=='{') 
        {
            left_brace++;
        }
        else if(ch=='}')
        {
            left_brace--;
        }
    }
    if(left_brace < 0) throw 0;
    res_left_brace=left_brace;
}
catch(...)
{
    cout<<"block format  error"<<endl;
    exit(1);
}
}

bool configureObject::get_block(ifstream& m_infile, string &startLine, vector<string>& line_list)
{
    int left_braces = 0;
    int rignt_braces = 0;  
    if(startLine.find('{') != string::npos) //fine {
    {
        left_braces++;
    }

    string buf;
    while(getline(m_infile, buf))
    {
        if(buf.empty() || buf.at(0)== '#') continue;// Comments or blank lines
        int res_left_braces = 0;
        block_over(buf, left_braces, res_left_braces);
        left_braces = res_left_braces;
        line_list.push_back(buf);
        if(left_braces==0)
        {
            return true;
        }
    }
    return true;
}

bool configureObject::parseBraces(string& oriStr, vector<string>& subStr_list)
{
    int first_index=0;
    int second_index=0;
    if(oriStr.empty()) return false;
    
    for(auto it = oriStr.begin(); it != oriStr.end(); it++, second_index++)
    {
       char ch = *it;
       if(ch == ',')
       {
           if(second_index>first_index)
           {
               string sub_value = oriStr.substr(first_index, second_index-first_index);
               subStr_list.push_back(sub_value);
               first_index=(second_index+1);
           }
        }
    }

    if(first_index<second_index)
    {
       string sub_value = oriStr.substr(first_index, second_index-first_index);
       subStr_list.push_back(sub_value);
    }

    //È¥µô¿Õ°××Ö·û
    for(auto it = subStr_list.begin(); it != subStr_list.end(); it++)
    {
        string &str = *it;
        size_t size = str.size();
        first_index=-1;
        second_index=-1;
        for(auto i = 0; i < size; i++)
        {
            char ch = str.at(i);
            if((ch != ' ')&&(first_index==-1))
            {
                first_index = i;
            }
            else if((ch == ' ')&&(first_index!=-1))
            {
                second_index = i;
            }
        }
        str=str.substr(first_index, second_index-first_index);
    }
    return true;
}

void configureObject::get_configBlock(const char* blockSym, vector<string>& orgin_text,  vector<string>& result_text)
{               
    int left_braces = 0; 
    bool config_start = false;
    for(auto it = orgin_text.begin(); it != orgin_text.end(); it++)
    {
        string &str = *it;
        if(str.empty() || str.at(0)== '#') continue;// Comments or blank lines

        if(config_start)
        {
            if(left_braces == 0)
            {
                if(str.find('{') != string::npos)
                {
                    left_braces++;
                }
                continue;
            }
            else 
            {              
                int res_left_braces = 0;
                block_over(str, left_braces, res_left_braces);
                left_braces = res_left_braces;
                result_text.push_back(str);
                if(left_braces==0)
                {
                    return;
                }
            }
        }
        else if(string_find(str, blockSym))
        {
            config_start = true;    
            if(str.find('{') != string::npos)
            {
                left_braces++;
            }
        }
    }
}

void configureObject::parse_throttle_info(vector<string>& orgin_text)
{
    vector<string> config_block;
    get_configBlock("vastConfig", orgin_text, config_block);
    for(auto it = config_block.begin(); it != config_block.end(); it++)
    {
        string &str = *it;
        throttleConfig *throttle = parse_throttle_config(str);
        if(throttle)
        {
            m_throttle_info.add_throttleConfig(throttle);
        }
    }
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

        bool block_started = false;//block start sym;
        while(getline(m_infile, buf))
        {
            if(buf.empty() || buf.at(0)== '#' || buf.at(0)==' ') continue;// Comments or blank lines
            if(block_started)
            {
                if(buf.at(0)=='}')
                {
                    block_started = false;
                    continue;
                }
                string target_value;

                if(string_parse(buf, "country", target_value))
                {
                    m_target.set_country(target_value);
                }
                else if(string_parse(buf, "region", target_value))
                {
                    m_target.set_region(target_value);
                }
                else if(string_parse(buf, "city", target_value))
                {
                    m_target.set_city(target_value);
                }
                else if(string_parse(buf, "family", target_value))
                {
                    m_target.set_family(target_value);
                }
                else if(string_parse(buf, "version", target_value))
                {
                    m_target.set_version(target_value);
                }
                else if(string_parse(buf, "vendor", target_value))
                {
                    m_target.set_vendor(target_value);
                }
                else if(string_parse(buf, "model", target_value))
                {
                    m_target.set_model(target_value);
                }    
                else if(string_parse(buf, "lang", target_value))
                {
                    m_target.set_lang(target_value);
                }
                else if(string_parse(buf, "carrier", target_value))
                {
                    m_target.set_carrier(target_value);
                }
                else if(string_parse(buf, "conn_type", target_value))
                {
                    m_target.set_conntype(target_value);
                }
                else if(string_parse(buf, "dayPart", target_value))
                {
                    m_target.set_daypart(target_value);
                }
                else if(string_parse(buf, "session_imps", target_value))
                {
                    m_target.set_sessionImps(target_value);
                }
                else if(string_parse(buf, "app_web", target_value))
                {
                    m_target.set_app(target_value);
                }  
                else if(string_parse(buf, "phone_tablet", target_value))
                {
                    m_target.set_phone(target_value);
                }
                else if(string_parse(buf, "tarffic_quality", target_value))
                {
                    m_target.set_tarffic(target_value);
                }
                else if(string_parse(buf, "inventory_quality", target_value))
                {
                    m_target.set_inventory(target_value);
                } 
                else if(string_parse(buf, "app.instance", target_value))
                {
                    m_target.set_appid(target_value);
                }   
                else if(string_parse(buf, "app.category", target_value))
                {
                    m_target.set_appcategory(target_value);
                }   
            }
            if(string_find(buf, "vastInfo"))
            {
                cout<<"============================-0-0-0-0-0-0"<<endl;
                vector<string> throttle_info;
                bool ret = get_block(m_infile, buf, throttle_info);
                cout<<"ret = "<<ret<<endl;
                parse_throttle_info(throttle_info);
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
            else if(buf.compare(0, 8, "runTimes")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                runTimes = atoi(value.c_str());
            }
            else if(buf.compare(0, 6, "adtype")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_adType = atoi(value.c_str());
            }
            else if(buf.compare(0, 12, "intervalTime")==0)
            {
                if(get_subString(buf, '=', ';', value) == false) throw 0; 
                m_interverTime = atoi(value.c_str());
            }
            else if(buf.compare(0, 13, "lifetime_imps")==0)
            {
                if(get_subString(buf, '=', ';', m_lifetime_imps) == false) throw 0; 
            }
            else if(buf.compare(0, strlen("month_imps"), "month_imps")==0)
            {
                if(get_subString(buf, '=', ';', m_month_imps) == false) throw 0; 
            }
            else if(buf.compare(0, strlen("week_imps"), "week_imps")==0)
            {
                if(get_subString(buf, '=', ';', m_week_imps) == false) throw 0; 
            }
            else if(buf.compare(0, strlen("day_imps"), "day_imps")==0)
            {
                if(get_subString(buf, '=', ';', m_day_imps) == false) throw 0; 
            }
            else if(buf.compare(0, strlen("fiveMinutes_imps"), "fiveMinutes_imps")==0)
            {
                if(get_subString(buf, '=', ';', m_fiveminute_imps) == false) throw 0; 
            }
            else if(buf.compare(0, 9, "campagnID")==0)
            {
                if(get_subString(buf, '=', ';', m_campagnID) == false) throw 0; 
            }
            else if(buf.compare(0, 7, "ttlTime")==0)
            {
                if(get_subString(buf, '=', ';', m_ttlTime) == false) throw 0; 
            }
            else if(string_parse(buf, "ad_width", value))
            {
                m_width = value;
            }
            else if(string_parse(buf, "ad_hight", value))
            {
                m_hight = value;
            }
            
            else if(buf.compare(0, 7, "target{")==0)
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
    }
    catch(...)
    {

        cout << get_configFlieName() <<":config error" << endl;
        m_infile.close();
        exit(1);
    }    
}


void configureObject::display() 
{

    cout << "-----------------------------------" << endl;
    cout << "config file name: "<< get_configFlieName() << endl;
    cout << "config runtims: " << get_runTimes()<< endl;

    cout << "config throttle ip: " << get_throttleIP()<< endl;
    cout << "config throttle port: " << get_throttlePubVastPort()<< endl; 

    cout << "config ad type: " << get_adType()<< endl; 
    cout << "config ttl time:"<<get_ttlTime()<<endl;
    cout << "config interval time:"<<get_intervalTime()<<endl;
    cout << "config campaignID:"<<get_campaignID()<<endl;
    cout << "config ad width:"<<get_width()<<endl;
    cout << "config height:"<<get_height()<<endl;
    m_target.display();
    cout << "config frequency:"<<endl;
    cout<<get_lifetime_imps()<< "---"<<get_month_imps()<<"---"<<get_week_imps()<<"---"<<get_day_imps()<<"---"<<get_fiveminute_imps()<<endl;

    cout <<"throttle list:"<<endl;
    m_throttle_info.display();
    cout << "-----------------------------------" << endl;
 
}