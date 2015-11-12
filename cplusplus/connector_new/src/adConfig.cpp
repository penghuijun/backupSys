
#include "adConfig.h"
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <exception>

configureObject::configureObject(const char* configTxt):m_configName(configTxt)
{
     readConfig();
     readLogConfig();
}

configureObject::configureObject(string &configTxt):m_configName(configTxt)
{
     readConfig();
     readLogConfig();
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
            g_manager_logger->error("throttle config format error: configure is not igit");
            return NULL;
        }
        int     number_id = atoi(str_id.c_str());
        int     number_adPort = atoi(str_adport.c_str());
        int     number_pubPort = atoi(str_pubPort.c_str());
        int     number_workerNum = atoi(str_workerNum.c_str());
        int     number_managerPort = atoi(str_managerPort.c_str());
        const unsigned short short_max = 0xFFFF;
        const unsigned short worker_max = 0xFF;
        if((number_adPort>short_max)||(number_pubPort>short_max)||(number_managerPort > short_max))
        {
            g_manager_logger->error("port out of range");
            return NULL;
        }
        if(number_workerNum>worker_max) 
        {
            g_manager_logger->error("worker number out of range");
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

bcConfig* configureObject::parse_bc_config(string &str, int item_num)
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
        if(parseBraces(dst, subStrList)==true)
        {
            if(subStrList.size() != item_num)   return NULL;
            string& str_id = subStrList.at(0);
            string& str_ip = subStrList.at(1);
            string& str_dataport = subStrList.at(2);
            string& str_managerPort = subStrList.at(3);
            string& str_threadNum = subStrList.at(4);
            if((is_number(str_id) == false)||(is_number(str_dataport) == false)||
               (is_number(str_managerPort) == false)||(is_number(str_threadNum) == false)) 
            {
                g_manager_logger->error("throttle config format error: not ingit");
                return NULL;
            }
            int     number_id = atoi(str_id.c_str());
            int     number_dataPort = atoi(str_dataport.c_str());
            int     number_managerPort = atoi(str_managerPort.c_str());
            int     number_threadNum = atoi(str_threadNum.c_str());
            const unsigned short short_max = 0xFFFF;
            const unsigned short thread_max = 0xFF;
            if((number_dataPort>short_max)||(number_managerPort>short_max))
            {
                g_manager_logger->error("port out of range");
                return NULL;
            }
            if(number_threadNum>thread_max) 
            {
                g_manager_logger->error("thread number out of range");
                return NULL;
            }
            bcConfig *bc = new bcConfig(number_id, str_ip, (unsigned short)number_dataPort,(unsigned short)number_managerPort,(unsigned short)number_threadNum);
            return bc;
        }
        return NULL;
    }
    catch(...)
    {
        exit(1);
    }
}

bidderConfig* configureObject::parse_bidder_config(string &str, int config_item_num)
{
    try
    {
        size_t str_size = str.size();
        if(str.empty() || str.at(0)== '#' ) return NULL;// Comments or blank lines
    
        size_t startIdx = str.find('{');
        if(startIdx == string::npos) return NULL;
        size_t endIdx = str.find('}');
        if(endIdx == string::npos) return NULL;
    
        string dst = str.substr(startIdx+1, endIdx-startIdx-1); //»•µÙ¥Û¿®∫≈
        if(dst.empty()) return NULL;
    
        int first_index=0;
        int second_index=0;      
        vector<string> subStrList;
        if(parseBraces(dst, subStrList)==true)
        {
            if(subStrList.size() != config_item_num)   return NULL;
            string& str_id = subStrList.at(0);
            string& str_ip = subStrList.at(1);
            string& str_loginPort = subStrList.at(2);
            string& str_workerNum = subStrList.at(3);
            string& str_threadPoolSize = subStrList.at(4);
            string& str_convenceNum = subStrList.at(5);
            if((is_number(str_id) == false)||(is_number(str_loginPort) == false)
               ||(is_number(str_threadPoolSize) == false)||(is_number(str_workerNum) == false)
               ||(is_number(str_convenceNum) == false) )
            {
                g_manager_logger->error("throttle config format error: is not number");
                return NULL;
            }
            int     number_id = atoi(str_id.c_str());
            int     number_loginPort = atoi(str_loginPort.c_str());
            int     number_workerNum = atoi(str_workerNum.c_str());
            int     number_threadPoolSize = atoi(str_threadPoolSize.c_str());
            int     number_converceNum = atoi(str_convenceNum.c_str());

            const unsigned short worker_max = 0xFF;
            const unsigned short thread_max = 0xFF;
            const unsigned short converce_max = 0xFF;
            if((number_loginPort>short_max))
            {
                g_manager_logger->error("port out of range");
                return NULL;
            }
            if((number_workerNum>worker_max)||(number_threadPoolSize>thread_max)||(number_converceNum>converce_max)) 
            {
                g_manager_logger->error("port out of range");
                return NULL;
            }
         
            bidderConfig *bidder = new bidderConfig(number_id, str_ip, (unsigned short) number_loginPort,
                              (unsigned short)number_workerNum, (unsigned short) number_threadPoolSize,(unsigned short) number_converceNum);
            return bidder;
        }
        return NULL;
    }
    catch(...)
    {
        exit(1);
    }
 }


connectorConfig* configureObject::parse_connector_config(string &str, int config_item_num)
{
    try
    {
        size_t str_size = str.size();
        if(str.empty() || str.at(0)== '#' ) return NULL;// Comments or blank lines
    
        size_t startIdx = str.find('{');
        if(startIdx == string::npos) return NULL;
        size_t endIdx = str.find('}');
        if(endIdx == string::npos) return NULL;
    
        string dst = str.substr(startIdx+1, endIdx-startIdx-1); //»•µÙ¥Û¿®∫≈
        if(dst.empty()) return NULL;
    
        int first_index=0;
        int second_index=0;      
        vector<string> subStrList;
        if(parseBraces(dst, subStrList)==true)
        {
            if(subStrList.size() != config_item_num)   return NULL;
            string& str_id = subStrList.at(0);
            string& str_ip = subStrList.at(1);
            string& str_loginPort = subStrList.at(2);
            string& str_workerNum = subStrList.at(3);
            string& str_threadPoolSize = subStrList.at(4);
            if((is_number(str_id) == false)||(is_number(str_loginPort) == false)
               ||(is_number(str_threadPoolSize) == false)||(is_number(str_workerNum) == false)
               )
            {
                g_manager_logger->error("throttle config format error: is not number");
                return NULL;
            }
            int     number_id = atoi(str_id.c_str());
            int     number_loginPort = atoi(str_loginPort.c_str());
            int     number_workerNum = atoi(str_workerNum.c_str());
            int     number_threadPoolSize = atoi(str_threadPoolSize.c_str());

            const unsigned short worker_max = 0xFF;
            const unsigned short thread_max = 0xFF;
            const unsigned short converce_max = 0xFF;
            if((number_loginPort>short_max))
            {
                g_manager_logger->error("port out of range");
                return NULL;
            }
            if((number_workerNum>worker_max)||(number_threadPoolSize>thread_max)) 
            {
                g_manager_logger->error("port out of range");
                return NULL;
            }
         
            connectorConfig *connector = new connectorConfig(number_id, str_ip, (unsigned short) number_loginPort,
                              (unsigned short)number_workerNum, (unsigned short) number_threadPoolSize);
            return connector;
        }
        return NULL;
    }
    catch(...)
    {
        exit(1);
    }
 }

mysqlConfig* configureObject::parse_mysql_config(string &str, int config_item_num)
{
    try
    {        
        size_t str_size = str.size();
        if(str.empty() || str.at(0)== '#' ) return NULL;// Comments or blank lines
    
        size_t startIdx = str.find('{');
        if(startIdx == string::npos) return NULL;
        size_t endIdx = str.find('}');
        if(endIdx == string::npos) return NULL;
    
        string dst = str.substr(startIdx+1, endIdx-startIdx-1); //»•µÙ¥Û¿®∫≈
        if(dst.empty()) return NULL;
    
        int first_index=0;
        int second_index=0;      
        vector<string> subStrList;
        if(parseBraces(dst, subStrList)==true)
        {
            if(subStrList.size() != config_item_num)   return NULL;
            string& str_id = subStrList.at(0);
            string& str_ip = subStrList.at(1);
            string& str_dataPort = subStrList.at(2);
            string& str_userName = subStrList.at(3);
            string& str_userPwd = subStrList.at(4);
            if((is_number(str_id) == false)||(is_number(str_dataPort) == false))
            {
                g_manager_logger->error("mysql config format error: is not number");
                return NULL;
            }
            int     number_id = atoi(str_id.c_str());
            int     number_dataPort = atoi(str_dataPort.c_str());            
            
            if((number_dataPort>short_max))
            {
                g_manager_logger->error("port out of range");
                return NULL;
            }            
         
            mysqlConfig *mysql = new mysqlConfig(number_id, str_ip, (unsigned short) number_dataPort,
                              str_userName, str_userPwd);
            return mysql;
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
    g_manager_logger->emerg("block format error: {0}", line);
    exit(1);
}
}
bool configureObject::get_block(ifstream& m_infile, string &startLine, vector<string>& line_list)
{
try
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
catch(...)
{
    g_manager_logger->emerg("get block exception");
    exit(1);
}
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

    //»•µÙø’∞◊◊÷∑˚
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
    string value;
    vector<string> config_block;
    get_configBlock("throttleConfig", orgin_text, config_block);
    for(auto it = config_block.begin(); it != config_block.end(); it++)
    {
        string &str = *it;
        throttleConfig *throttle = parse_throttle_config(str);
        if(throttle)
        {
            m_throttle_info.add_throttleConfig(throttle);
        }
    }

    for(auto it = orgin_text.begin(); it != orgin_text.end();it++)
    {
        string &str = *it;
        if(string_find(str, "logLevel"))
        {
            string redisLine;
            if(get_subString(str, '=', ';', value) == false) throw 0;  
            int level = atoi(value.c_str());
            m_throttle_info.set_logLevel(level);
        }
    }  
}

void configureObject::parse_bidder_info(vector<string> &orgin_text)
{
try
{
    vector<string> config_block;
    get_configBlock("bidderConfig", orgin_text, config_block);
    const int item_num = 6;
    for(auto it = config_block.begin(); it != config_block.end(); it++)
    {
        string &str = *it;
        bidderConfig *bidder = parse_bidder_config(str, item_num);
        if(bidder)
        {
            m_bidder_info.add_bidderConfig(bidder);
        }
    }

    for(auto it = orgin_text.begin(); it != orgin_text.end();it++)
    {
        string &str = *it;
        if(string_find(str, "redisConfig"))
        {
            string redisLine;
            if(get_subString(str, '=', ';', redisLine) == false) throw 0;
            vector<string> subStringList;
            if(parseBraces(redisLine, subStringList)==true)
            {
                if(subStringList.size()!=2) throw 0;
                if(is_number(subStringList.at(1))==false) throw 0;
                int port = atoi(subStringList.at(1).c_str());
                if(port>short_max)throw 0;
                m_bidder_info.set_redis(subStringList.at(0), (unsigned short)port);
                break;
            }
        }
    } 

    string value;
    for(auto it = orgin_text.begin(); it != orgin_text.end();it++)
    {
        string &str = *it;
        if(string_find(str, "logLevel"))
        {
            string redisLine;
            if(get_subString(str, '=', ';', value) == false) throw 0;  
            int level = atoi(value.c_str());
            m_bidder_info.set_logLevel(level);
        }
    }  
}
catch(...)
{
    g_manager_logger->emerg("redis config error");
    exit(1);
}
}

void configureObject::parse_connector_info(vector<string> &orgin_text)
{
    try
    {
        vector<string> config_block;
        get_configBlock("connectorConfig", orgin_text, config_block);
        const int item_num = 5;
        for(auto it = config_block.begin(); it != config_block.end(); it++)
        {
            string &str = *it;
            connectorConfig *connector = parse_connector_config(str, item_num);
            if(connector)
            {
                m_connector_info.add_connectorConfig(connector);
            }
        }

        vector<string> mysql_block;
        get_configBlock("mysqlConfig", orgin_text, mysql_block);
        for(auto it = mysql_block.begin(); it != mysql_block.end(); it++)
        {
            string &str = *it;
            mysqlConfig *mysql = parse_mysql_config(str,item_num);
            if(mysql)
            {
                m_connector_info.add_mysqlConfig(mysql);
            }            
        }              
        
        for(auto it = orgin_text.begin(); it != orgin_text.end();it++)
        {
            string &str = *it;
            if(string_find(str, "redisConfig"))
            {
                string redisLine;
                if(get_subString(str, '=', ';', redisLine) == false) throw 0;
                vector<string> subStringList;
                if(parseBraces(redisLine, subStringList)==true)
                {
                    if(subStringList.size()!=2) throw 0;
                    if(is_number(subStringList.at(1))==false) throw 0;
                    int port = atoi(subStringList.at(1).c_str());
                    if(port>short_max)throw 0;
                    m_connector_info.set_redis(subStringList.at(0), (unsigned short)port);
                    break;
                }
            }
        } 
    
        string value;
        for(auto it = orgin_text.begin(); it != orgin_text.end();it++)
        {
            string &str = *it;
            if(string_find(str, "logLevel"))
            {
                string redisLine;
                if(get_subString(str, '=', ';', value) == false) throw 0;  
                int level = atoi(value.c_str());
                m_connector_info.set_logLevel(level);
            }
        }  
    }
    catch(...)
    {
        g_manager_logger->emerg("redis config error");
        exit(1);
    }
}



void configureObject::parse_bc_info(vector<string> &orgin_text)
{
try
{
    vector<string> config_block;
    get_configBlock("bcConfig", orgin_text, config_block);
    const int item_num = 5;
    for(auto it = config_block.begin(); it != config_block.end(); it++)
    {
        string &str = *it;
        bcConfig *bc = parse_bc_config(str, item_num);
        if(bc)
        {
            m_bc_info.add_bcConfig(bc);
        }
    }
    for(auto it = orgin_text.begin(); it != orgin_text.end();it++)
    {
        string &str = *it;
        if(string_find(str, "redisConfig"))
        {
            string redisLine;
            if(get_subString(str, '=', ';', redisLine) == false) throw 0;
            vector<string> subStringList;
            if(parseBraces(redisLine, subStringList)==true)
            {
                if(subStringList.size()!=3) throw 0;
                if((is_number(subStringList.at(1))==false)||(is_number(subStringList.at(2))==false)) throw 0;
                int port = atoi(subStringList.at(1).c_str());
                int timeout = atoi(subStringList.at(2).c_str());
                if((port>short_max)||(timeout>short_max))throw 0;
                m_bc_info.set_redis(subStringList.at(0), (unsigned short)port, (unsigned short)timeout);
                break;
            }
        }
    } 
    
    string value;
    for(auto it = orgin_text.begin(); it != orgin_text.end();it++)
    {
        string &str = *it;
        if(string_find(str, "logLevel"))
        {
            string redisLine;
            if(get_subString(str, '=', ';', value) == false) throw 0;  
            int level = atoi(value.c_str());
            m_bc_info.set_logLevel(level);
        }
    }      
}
catch(...)
{
    g_manager_logger->emerg("bc redis config error");
    exit(1);
}
}

//read configure file
void configureObject::readConfig()
{
    try
    {
        clear();
        string buf;
        if(m_infile.is_open()==true)//if file is open ,close the file first
        {
            m_infile.close();
        }
        
        m_infile.open(m_configName, ios::in);//open file
        if(m_infile.is_open()==false)
        {
            g_manager_logger->emerg("sorry, open {0} failure", m_configName);
            exit(1);
        }
        g_manager_logger->info("open {0} success", m_configName);

        while(getline(m_infile, buf))
        {
            if(buf.empty() || buf.at(0)== '#' || buf.at(0)==' ') continue;// Comments or blank lines

            if(string_find(buf, "throttleInfo"))
            {
                vector<string> throttle_info;
                if(get_block(m_infile, buf, throttle_info) == false) throw 0;
                parse_throttle_info(throttle_info);
            }
            else if(string_find(buf, "bidderInfo"))
            {
                vector<string> bidder_info;
                if(get_block(m_infile, buf, bidder_info) == false) throw 0;
                parse_bidder_info(bidder_info);
            }
            else if(string_find(buf, "connectorInfo"))
            {
                vector<string> connector_info;
                if(get_block(m_infile, buf, connector_info) == false) throw 0;
                parse_connector_info(connector_info);
            }
            else if(string_find(buf, "bcInfo"))
            {
                vector<string> bc_info;
                if(get_block(m_infile, buf, bc_info) == false) throw 0;
                parse_bc_info(bc_info);
            }
            else if(string_find(buf, "vastBusinessCode"))
            {
                if(get_subString(buf, '=', ';', m_vastBusiCode) == false) throw 0;
            }
            else if(string_find(buf, "mobileBusinessCode"))
            {
                if(get_subString(buf, '=', ';', m_mobileBusiCode) == false) throw 0;
            }
            else if(string_find(buf, "logRedisOn"))
            {
                string value;
                if(get_subString(buf, '=', ';', value) == false) throw 0;
                if(atoi(value.c_str())==0)
                {
                    m_logRedisOn = false;
                }
                else
                {
                    m_logRedisOn = true;
                }
            }
            else if(string_find(buf, "logRedisIP"))
            {
                if(get_subString(buf, '=', ';', m_logRedisIP) == false) throw 0;
            }
            else if(string_find(buf, "logRedisPort"))
            {
                string value;
                if(get_subString(buf, '=', ';', value) == false) throw 0;
                 m_logRedisPort = atoi(value.c_str());               
            }
            else if(string_find(buf, "heartInterval"))
            {
                string value;
                if(get_subString(buf, '=', ';', value) == false) throw 0;
                m_heartInterval = atoi(value.c_str());
            }

        }//while       

        m_infile.close();
    }
    catch(...)
    {
        g_manager_logger->emerg("configure {0} famat error", get_configFlieName());
        m_infile.close();
        exit(1);
    }    
}

void configureObject::readLogConfig()
{
    ifstream ifile; 
    ifile.open("./conf/runConfig.json",ios::in);
    if(ifile.is_open() == false)
    {               
        g_manager_logger->error("Open runConfig.json failure...");
        exit(1);    
    }   
    
    Json::Reader reader;
    Json::Value root;
    
    if(reader.parse(ifile, root))
    {
        enChinaTelecom = root["enChinaTelecom"].asBool();
        logTeleReq = root["logTeleReq"].asBool();
        logTeleHttpRsp = root["logTeleHttpRsp"].asBool();
        logTeleRsp = root["logTeleRsp"].asBool();

        enGYIN = root["enGYIN"].asBool();
        logGYINReq = root["logGYINReq"].asBool();
        logGYINHttpRsp = root["logGYINHttpRsp"].asBool();
        logGYINRsp = root["logGYINRsp"].asBool();

        enSmaato = root["enSmaato"].asBool();
        logSmaatoReq = root["logSmaatoReq"].asBool();
        logSmaatoHttpRsp = root["logSmaatoHttpRsp"].asBool();
        logSmaatoRsp = root["logSmaatoRsp"].asBool();
        

        enInMobi = root["enInMobi"].asBool();
        logInMobiReq = root["logInMobiReq"].asBool();
        logInMobiHttpRsp = root["logInMobiHttpRsp"].asBool();
        logInMobiRsp = root["logInMobiRsp"].asBool();
        
    }
    else
    {
        g_manager_logger->error("Parse runConfig.json failure...");
        exit(1);
    }
}

extern shared_ptr<spdlog::logger> g_manager_logger;

void configureObject::display() 
{
    g_manager_logger->info("-----------------------------------");   
    m_throttle_info.display(g_manager_logger);
    m_bidder_info.display(g_manager_logger);
    m_bc_info.display(g_manager_logger);
    g_manager_logger->info("vast business code:{0}", m_vastBusiCode);
    g_manager_logger->info("mobile business code:{0}", m_mobileBusiCode);
    g_manager_logger->info("heart interval:{0:d}", m_heartInterval);
    g_manager_logger->info("log redis:{0:d}, {1}, {2:d}",(int) m_logRedisOn, m_logRedisIP, m_logRedisPort);  
    g_manager_logger->info("-----------------------------------");  
}


