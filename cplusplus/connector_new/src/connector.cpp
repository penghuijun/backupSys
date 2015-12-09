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

#include "connector.h"

sig_atomic_t srv_graceful_end = 0;
sig_atomic_t srv_ungraceful_end = 0;
sig_atomic_t srv_restart = 0;
sig_atomic_t sigalrm_recved = 0;
sig_atomic_t sigusr1_recved = 0;

extern shared_ptr<spdlog::logger> g_master_logger;
extern shared_ptr<spdlog::logger> g_manager_logger;
extern shared_ptr<spdlog::logger> g_worker_logger;
extern shared_ptr<spdlog::logger> g_workerGYIN_logger;
extern shared_ptr<spdlog::logger> g_workerSMAATO_logger;
extern shared_ptr<spdlog::logger> g_workerINMOBI_logger;



vector<map<int,string>> SQL_MAP;
map<int,string> Creative_template;
map<int,string> CampaignMap;

char *stroeBuffer = new char[BUF_SIZE];
struct spliceData_t *stroeBuffer_t = new spliceData_t();

#if 0
map<string,ContentCategory> AndroidContenCategoryMap;
map<string,ContentCategory> IOSContenCategoryMap;
map<int,ConnectionType> ConnectionTypeMap;
#endif


const int CHECK_COMMMSG_TIMEOUT = 200; // 200ms
const long long TELECODEUPDATE_TIME = 24*60*60*1000; // 24 hours
const long long CLOCK_TIME = 17*60*60*1000; //Beijing time : 01:00:00  -> GMT(Greenwich mean time): 17:00:00
const long long DAWN_TIME = 16*60*60*1000; //Beijing time : 00:00:00  -> GMT(Greenwich mean time): 16:00:00

void connectorServ::versionConvert(string &Dest,const char *Src)
{
	const char *ch = Src;
	int pos = 0;
	int len = 0;
	while( (*ch != ' ')&&(*ch != '\0'))
	{
        pos++;
        ch++;
	}
	ch++;	
	while( (*ch != ' ')&&(*ch != '\0'))
	{
        len++;
        ch++;
	}		
	Dest.assign(Src+pos+1,len);
	Dest[len] = '\0';
}

void connectorServ::readConfigFile()
{
    m_config.readConfig();
}
void connectorServ::genFundaCommands(vector<string>& sql_commands)
{
    ostringstream os;
    os<<"select id,carrier_name from system_carriers order by id asc";
    sql_commands.push_back(os.str());    
    
    os.str("");
    os<<"select id,code from system_languages order by id asc";
    sql_commands.push_back(os.str());    

    os.str("");
    os<<"select id,device_vendor from system_device_vendors order by id asc";
    sql_commands.push_back(os.str());    

    os.str("");
    os<<"select id,device_model from system_device_models order by id asc";
    sql_commands.push_back(os.str());    

    os.str("");
    os<<"select id,name from system_os_families order by id asc";
    sql_commands.push_back(os.str());   

    os.str("");
    os<<"select id,os_version_name from system_os_family_versions order by id asc";
    sql_commands.push_back(os.str());   

    os.str("");
    os<<"select id,code from geo_country order by id asc";
    sql_commands.push_back(os.str());    

    os.str("");
    os<<"select id,region_name from geo_region order by id asc";
    sql_commands.push_back(os.str());
    
    os.str("");
    os<<"select id,city_name from geo_city order by id asc";
    sql_commands.push_back(os.str()); 
}
void connectorServ::genAppumpCommands(vector<string>& sql_commands)
{
    ostringstream os;
    os<<"select id,code from creative_template order by id asc";
    sql_commands.push_back(os.str());    
    
    os.str("");
    os<<"select a.network_id,b.id as campaign_id "
      <<"from app_ump.advertisers a,app_ump.advertisers_campaigns b,app_ump.network c "
      <<"where a.id = b.advertiser_id and a.network_id= c.id and c.type = 1";    
    sql_commands.push_back(os.str());    
}

int connectorServ::getDataFromMysql_Funda()
{
    //数据库配置信息  
    vector<mysqlConfig*> mysqlConfigList = m_config.get_connector_info().get_mysqlConfigList();
    const char *local_host = mysqlConfigList[0]->get_mysqlIP().c_str();  
    const char *user_name = mysqlConfigList[0]->get_mysqlUserName().c_str();  
    const char *user_pwd = mysqlConfigList[0]->get_mysqlUserPwd().c_str();  
    const char *data_base = "fundamental";  
    unsigned short data_port = mysqlConfigList[0]->get_mysqlDataPort();
    
    MYSQL mydata;//初始化mysql  
  
    //是否加载库文件  
    if(0 != mysql_library_init(0,NULL,NULL))  
    {          
        g_master_logger->error("mysql_library_init() error");
        return 0;  
    }  
  
    //MYSQL mydata;//初始化mysql  
    if(NULL == mysql_init(&mydata))  
    {          
        g_master_logger->error("mysql_init() error");
        return 0;  
    }  
  
    //设置mysql访问语言  
    if(0 != mysql_options(&mydata,MYSQL_SET_CHARSET_NAME,"gb2312"))  
    {           
        g_master_logger->error("can't set charset_name");
        return 0;  
    }  
    //连接mysql  
    if(NULL == mysql_real_connect(&mydata,local_host,user_name,user_pwd,data_base,data_port,NULL,0))  
    {         
        g_master_logger->error("connection error");
        return 0;  
    }  
  
    //sql语句 结果指针      
    MYSQL_RES *result=NULL;    
   
  
    //查询表        
    vector<string> sql_commands;
    genFundaCommands(sql_commands);
    
    for(int i=0; i<sql_commands.size(); i++)
    {
        string strsql = sql_commands[i];
        if(0==mysql_query(&mydata,strsql.c_str()))  
        {              
            result = mysql_store_result(&mydata);  
        }  
        else  
        {              
            return 0;  
        }  
        //返回记录集总数  
        int rowcount = mysql_num_rows(result);          
        
      
        //行指针 遍历行  
        MYSQL_ROW row =NULL;  
        
        //存储到map中
        map<int,string> map_item;        
        int id = 0;
        string name;
        string convertName;
        while (NULL != (row = mysql_fetch_row(result)) )  
        {             
           	id = atoi(row[0]);
           	name = row[1];           	
           	if(i == 5) //os verion name: "Android 2.2.3 Froyo" -> "2.2.3" 
           	{
                versionConvert(convertName,name.c_str());   
                map_item.insert(pair<int,string>(id,convertName)); 
           	}  
           	else
                map_item.insert(pair<int,string>(id,name));            
        }  
        SQL_MAP.push_back(map_item);    
        
    }
    
      
    //释放结果集 关闭数据库  
    mysql_free_result(result);  
    mysql_close(&mydata);  
    return 1;
}

int connectorServ::getDataFromMysql_Appump()
{
    
     //数据库配置信息  

    vector<mysqlConfig*> mysqlConfigList = m_config.get_connector_info().get_mysqlConfigList();
    const char *local_host = mysqlConfigList[0]->get_mysqlIP().c_str();  
    const char *user_name = mysqlConfigList[0]->get_mysqlUserName().c_str();  
    const char *user_pwd = mysqlConfigList[0]->get_mysqlUserPwd().c_str();  
    const char *data_base = "app_ump";  
    unsigned short data_port = mysqlConfigList[0]->get_mysqlDataPort();
    
    MYSQL mydata;//初始化mysql  
  
    //是否加载库文件  
    if(0 != mysql_library_init(0,NULL,NULL))  
    {         
        g_master_logger->error("mysql_library_init() error");
        return 0;  
    }    
    
    if(NULL == mysql_init(&mydata))  
    {          
        g_master_logger->error("mysql_init() error");
        return 0;  
    }  
  
    //设置mysql访问语言  
    if(0 != mysql_options(&mydata,MYSQL_SET_CHARSET_NAME,"gb2312"))  
    {          
        g_master_logger->error("can't set charset_name");
        return 0;  
    }  
    //连接mysql  
    if(NULL == mysql_real_connect(&mydata,local_host,user_name,user_pwd,data_base,data_port,NULL,0))  
    {          
        g_master_logger->error("connection error");
        return 0;  
    }  
    
    //sql语句 结果指针      
    MYSQL_RES *result=NULL;    
   
  
    //查询表  
    vector<string> sql_commands;
    genAppumpCommands(sql_commands);    
    for(int i=0; i<sql_commands.size(); i++)
    {
        string strsql = sql_commands[i];
        
        if(0==mysql_query(&mydata,strsql.c_str()))  
        {          
            result = mysql_store_result(&mydata);  
        }  
        else  
        {           
            return 0;  
        }  
        
        //返回记录集总数  
        int rowcount = mysql_num_rows(result);          
        //cout << "rowcount = " << rowcount << endl;
        
        //行指针 遍历行  
        MYSQL_ROW row =NULL; 
        
        //存储到map中      
        int id = 0;
        string code;
        int network_id = 0;
        string campaign_id;
        while (NULL != (row = mysql_fetch_row(result)) )  
        {  
            if(i == 0)
            {
                id = atoi(row[0]);
                code = row[1];              
                //g_master_logger->debug("id:{0:d} -> code:{1}",id, code);
                Creative_template.insert(pair<int,string>(id, code));   
            }                    
            else if(i == 1)
            {
                network_id = atoi(row[0]);
                campaign_id = row[1];    
                //cout << "network_id : " << network_id << "campaign_id : " << campaign_id << endl;
                g_master_logger->info("network_id:{0:d} -> campaign_id:{1}",network_id, campaign_id);
                CampaignMap.insert(pair<int,string>(network_id, campaign_id));
            }                    
        }      
        
    }   
    
    //释放结果集 关闭数据库  
    mysql_free_result(result);      
    mysql_close(&mydata);  
    return 1;    
    
}

#if 0
void connectorServ::mapInit()
{
    //Android
    //Primary category    
    AndroidContenCategoryMap.insert(map<string,ContentCategory>::value_type("Games",CAT_805));    
    AndroidContenCategoryMap.insert(map<string,ContentCategory>::value_type("Apps",CAT_805));

    //second category    
    AndroidContenCategoryMap.insert(map<string,ContentCategory>::value_type("Catalogues",CAT_824));
    AndroidContenCategoryMap.insert(map<string,ContentCategory>::value_type("Books&Reference",CAT_82005));
    AndroidContenCategoryMap.insert(map<string,ContentCategory>::value_type("Business",CAT_819));
    AndroidContenCategoryMap.insert(map<string,ContentCategory>::value_type("Comics",CAT_80603));
    AndroidContenCategoryMap.insert(map<string,ContentCategory>::value_type("Communication",CAT_82202));
    AndroidContenCategoryMap.insert(map<string,ContentCategory>::value_type("Education",CAT_808));    
    AndroidContenCategoryMap.insert(map<string,ContentCategory>::value_type("Enterainment",CAT_806));    
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Finance",CAT_802));    
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Health&Fitness",CAT_804));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Libraries&Demo",CAT_820));    
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Lifestyle",CAT_818));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Live Wallpaper",CAT_80604));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Media&Video",CAT_80601));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Medical",CAT_804));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Music&Audio",CAT_806));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("News&Magazines",CAT_82208));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Personalization",CAT_814));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Photography",CAT_82001));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Productivity",CAT_817));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Shopping",CAT_82204));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Social",CAT_823));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Sports",CAT_812));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Tools",CAT_82507));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Transportation",CAT_81302));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Travel&Local",CAT_813));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Weather",CAT_81307));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Widgets",CAT_82507));
    
    //GAMES
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Action",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Adventure",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Arcade",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Board",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Card",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Casino",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Education",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Family",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Music",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Puzzle",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Racing",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Role Playing",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Simulation",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Sports",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Strategy",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Trivia",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Word",CAT_805));
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("Others",CAT_805));
    
    AndroidContenCategoryMap.insert(pair<string,ContentCategory>("UNKNOWN",CAT_824));

    //IOS    
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Games",CAT_805));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Kids",CAT_803));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Education",CAT_808));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Newsstand",CAT_82204));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Photo&Video",CAT_80601));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Productivity",CAT_817));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Lifestyle",CAT_818));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Health&Fitness",CAT_804));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Travel",CAT_813));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Music",CAT_80602));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Sports",CAT_812));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Business",CAT_819));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("News",CAT_82208));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Utilities",CAT_823));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Entertainment",CAT_806));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Social Networking",CAT_822));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Food&Drink",CAT_815));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Finance",CAT_802));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Reference",CAT_825));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Navigation",CAT_82207));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Medical",CAT_804));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Books",CAT_82005));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Weather",CAT_81307));
    IOSContenCategoryMap.insert(pair<string,ContentCategory>("Catalogues",CAT_824));

    IOSContenCategoryMap.insert(pair<string,ContentCategory>("UNKNOWN",CAT_824));
    
    ConnectionTypeMap.insert(pair<int,ConnectionType>(0,UNKNOWN_TYPE));
    ConnectionTypeMap.insert(pair<int,ConnectionType>(1,ETHERNET));
    ConnectionTypeMap.insert(pair<int,ConnectionType>(2,WIFI));
    ConnectionTypeMap.insert(pair<int,ConnectionType>(3,CELLULAR_NETWORK_UNKNOWN_GENERATION));
    ConnectionTypeMap.insert(pair<int,ConnectionType>(4,CELLULAR_NETWORK_2G));
    ConnectionTypeMap.insert(pair<int,ConnectionType>(5,CELLULAR_NETWORK_3G));
    ConnectionTypeMap.insert(pair<int,ConnectionType>(6,CELLULAR_NETWORK_4G));    
   
    cout << "mapinit success" << endl;
    g_master_logger->debug("mapinit success ");
    
}
#endif
connectorServ::connectorServ(configureObject& config):m_config(config)
{
    try
    {
        m_mastertime_lock.init();     
        m_diplay_lock.init();
        
        if(getDataFromMysql_Appump() == 0)
        {
            g_master_logger->error("get Data from Mysql app_ump failed !");
            throw -1;
        }        
        g_master_logger->info("getDataFromMysql_Appump success !");
        
        if(getDataFromMysql_Funda() == 0)
        {
            g_master_logger->error("get Data from Mysql fundamental failed !");
            throw -1;
        }        
        g_master_logger->info("getDataFromMysql_Funda success !");    
        //mapInit();
        
        m_logRedisOn = m_config.get_logRedisState();
        m_logRedisIP = m_config.get_logRedisIP();
        m_logRedisPort = m_config.get_logRedisPort();
        m_vastBusinessCode = m_config.get_vast_businessCode();
        m_mobileBusinessCode = m_config.get_mobile_businessCode();
        m_heartInterval = m_config.get_heart_interval();     

        int logLevel = m_config.get_connectorLevel();
        if( (logLevel>((int)spdlog::level::off) ) || (logLevel < ((int)spdlog::level::trace))) 
        {
            m_logLevel = spdlog::level::info;
        }
        else
        {
            m_logLevel = (spdlog::level::level_enum) logLevel;
        }
        g_master_logger->set_level(m_logLevel);
        g_manager_logger->set_level(m_logLevel);
        g_manager_logger->emerg("log level:{0}", m_logLevel);

        m_throttle_manager.init(m_config.get_throttle_info());
        m_bc_manager.init(m_config.get_bc_info());
        m_connector_manager.init(m_config.get_connector_info());		
        m_dspManager.init(m_config.get_enChinaTelecom(), m_config.get_enGYIN(), m_config.get_enSmaato(), m_config.get_enInMobi());
        
        m_workerNum = m_connector_manager.get_connector_config().get_connectorWorkerNum();
        cout << "<<<<<<<< m_workerNum = " << m_workerNum << endl;
        for(int i = 0; i < m_workerNum; i++)
        {
            BC_process_t *pro = new BC_process_t;
    		pro->pid = 0;
    		pro->status = PRO_INIT;
    		m_workerList.push_back(pro);
        }
        
        m_zmq_connect.init();                
        
    }
    catch(...)
    {
        g_manager_logger->emerg("connectorServ structure error");
        exit(1);
    }
}

void connectorServ::signal_handler(int signo)
{
    time_t timep;   //typedef long int time_t;
    time(&timep);   //get time put in timep
    char *timeStr = ctime(&timep);  //transform time_t -> char 
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
void connectorServ::updataWorkerList(pid_t pid)
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
            close(pro->channel[0]);
            close(pro->channel[1]);
           
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
bool connectorServ::logLevelChange()
{
     int logLevel = m_config.get_connectorLevel();
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

void connectorServ::updateConfigure()
{
    try
    {
        g_manager_logger->info("update configure......");
        m_config.readConfig();
        if(logLevelChange())
        {
            g_master_logger->set_level(m_logLevel);
            g_manager_logger->set_level(m_logLevel);  
        }

        connectorInformation &connector_info = m_config.get_connector_info();
        if(m_connector_manager.update(connector_info) == true)
        {
            int oldNum = m_workerNum;
            m_workerNum = m_connector_manager.get_connector_config().get_connectorWorkerNum(); 
            g_manager_logger->emerg("worker number from {0:d} to {1:d}", oldNum, m_workerNum);
        }
        //m_throttleManager.updateDev(m_config.get_bidder_info(), m_config.get_bc_info(), m_config.get_connector_info());
    }
    catch(...)
    {
        g_manager_logger->error("updateConfigure exception");
    }

}
void connectorServ::hupSigHandler(int fd, short event, void *arg)
{
    g_manager_logger->info("signal hup");
    exit(0);
}

void connectorServ::intSigHandler(int fd, short event, void *arg)
{
   g_manager_logger->info("signal int");
   usleep(10);
   exit(0);
}

void connectorServ::termSigHandler(int fd, short event, void *arg)
{
    g_manager_logger->info("signal term");
    exit(0);
}

void connectorServ::usr1SigHandler(int fd, short event, void *arg)
{
    g_manager_logger->info("signal SIGUSR1");
    connectorServ *serv = (connectorServ*) arg;
    if(serv != NULL)
    {
        //serv->updateWorker();
    }
    else
    {
        g_manager_logger->info("usr1SigHandler param excepthion");
    }
}
void connectorServ::updateWorker()
{
    readConfigFile();
   
    m_bc_manager.update_bcList(m_zmq_connect,m_config.get_bc_info());
    //int poolsize = m_connector_manager.get_connector_config().get_connectorThreadPoolSize();
    //connectorInformation &connectorfo = m_config.get_connector_info();
    //m_redis_pool_manager.redisPool_update(binfo.get_redis_ip(), binfo.get_redis_port(), poolsize+10);//redis pool update

    if(logLevelChange())//loglevel change
    {
        g_master_logger->set_level(m_logLevel);
        g_manager_logger->set_level(m_logLevel); 
        g_worker_logger->set_level(m_logLevel);
    }

}

char* connectorServ::convertTeleBidResponseJsonToProtobuf(char *data,int dataLen,int& ret_dataLen,string& uuid)
{    
    CommonMessage response_commMsg;
    MobileAdResponse mobile_response;

    commMsgRecord* cmrObj;
    CommonMessage request_commMsg;
    MobileAdRequest mobile_request;
    
    
    Json::Reader reader;
    Json::Value root;

    ofstream outfile;
    outfile.open("TeleBidResponse.json",ios::out | ios::binary);
    outfile.write(data,dataLen); 
    outfile.close();

    ifstream infile;
    infile.open("TeleBidResponse.json",ios::in | ios::binary);

    if(reader.parse(infile,root))
    {
        infile.close();        
        g_worker_logger->trace("BidResponse.json parse success ");
        string str_id = root["id"].asString();
        //cout << "id = " << root["id"].asString() << endl;
        cmrObj = checkValidId(str_id);
        if(!cmrObj)
        {
            g_worker_logger->debug("FIND RECLIST FAILED uuid: {0}", str_id);
            return NULL;
        }
        g_worker_logger->trace("FIND RECLIST success");

        request_commMsg.ParseFromArray(cmrObj->data, cmrObj->datalen);        
        const string& commMsg_data = request_commMsg.data();        
        mobile_request.ParseFromString(commMsg_data);
        
        bool ret = false;       
        int seatbid_size = root["seatbid"].size();        
        if(seatbid_size == 0)
        {
            g_worker_logger->debug("GEN FAILED : seatbid is null");
            return NULL;
        }
        for(int j=0; j<root["seatbid"].size(); j++)
        {            
            if(!root["seatbid"][j].isMember("bid"))
            {
                g_worker_logger->debug("GEN FAILED : no valid bid ");
                return NULL;
            }
            Json::Value bid = root["seatbid"][j]["bid"];            
            for(int i=0; i< bid.size(); i++)
            {
                MobileAdResponse_mobileBid *mobile_bidder = mobile_response.add_bidcontent();              
             
                MobileAdResponse_Creative  *mobile_creative =  mobile_bidder->add_creative();   
                if(!(bid[i].isMember("id")&&bid[i].isMember("impid")&&bid[i].isMember("price")&&bid[i].isMember("ext")))
                {
                    g_worker_logger->debug("GEN FAILED : no valid id | impid | price | ext ");
                    return NULL;
                }
                //cout << "impid : "<< bid[i]["impid"].asString() << endl;
                //mobile_creative->set_creativeid(bid[i]["impid"].asString());
                mobile_creative->set_creativeid("0");
                if(bid[i].isMember("w") && bid[i].isMember("h") && (bid[i]["w"].asInt() != 0) && (bid[i]["w"].asInt() != 0))
                {
                    char str_w[16] = {0};
                    char str_h[16] = {0};
                    sprintf(str_w,"%d",bid[i]["w"].asInt());
                    sprintf(str_h,"%d",bid[i]["h"].asInt());                
                    mobile_creative->set_width(str_w);
                    mobile_creative->set_height(str_h); 
                }
                else
                {
                    mobile_creative->set_width("null");
                    mobile_creative->set_height("null"); 
                }
                   
                mobile_creative->set_adchanneltype(MobileAdResponse_AdChannelType_MOBILE_APP);
                

                //MobileAdResponse_UUID *creative_uuid = mobile_creative->mutable_uuid();
                //creative_uuid->add_uuidtype(MobileAdResponse_UuidType_FRE);           
                
                       
                //mobile_bidder->set_campaignid(bid[i]["id"].asString());                
                mobile_bidder->set_biddingtype("CPM");      
                float price = bid[i]["price"].asDouble();
                if((price >= -EPSINON)&&(price <= EPSINON))
                {
                    g_worker_logger->debug("GEN FAILED : bid.price = {0:f} ",price);
                    return NULL;
                }
                char str_price[16] = {0};
                sprintf(str_price,"%.1f",bid[i]["price"].asDouble());                
                mobile_bidder->set_biddingvalue(str_price);
                mobile_bidder->set_expectcpm(str_price);
                if(root.isMember("cur"))
                    mobile_bidder->set_currency(root["cur"].asString());
                else
                    mobile_bidder->set_currency("CNY");
                

                //MobileAdResponse_UUID *mobileUuid = mobile_bidder->mutable_uuid();
                //mobileUuid->add_uuidtype(MobileAdResponse_UuidType_FRE);

                MobileAdResponse_Action *mobile_action = mobile_bidder->mutable_action();                  
                Json::Value ext = bid[i]["ext"];
                if(!(ext.isMember("netid")&&ext.isMember("action")&&ext.isMember("temp")))
                {
                    g_worker_logger->debug("GEN FAILED : no valid netid | action | temp ");
                    return NULL;
                }
                
                string localExtNetId = m_dspManager.getChinaTelecomObject()->getExtNetId();
                //cout << "localExtNetId : " << localExtNetId << endl;
                //cout << "netid : " << ext["netid"].asString() << endl;
                if(strcmp(ext["netid"].asString().c_str(),localExtNetId.c_str()) != 0) //netid noequal localextnetid
                {
                    g_worker_logger->debug("GEN FAILED : localExtNetId: {0} noequal json.netid: {1}", localExtNetId, ext["netid"].asString());
                    return NULL;
                }
                string intNetId = m_dspManager.getChinaTelecomObject()->getIntNetId();
                //cout << "intNetId : " << intNetId << endl;
                MobileAdResponse_Bidder *bidder_info = mobile_response.mutable_bidder();
                bidder_info->set_bidderid(intNetId);
                map<int,string>::iterator it = CampaignMap.find(atoi(intNetId.c_str()));
                if(it == CampaignMap.end())
                {
                    g_worker_logger->debug("GEN FAILED : get campainId from CampaignMap fail .  intNetId: {0}",intNetId);
                    return NULL;
                }
                string campaignId = it->second;  
                //cout << "campaignId : " << campaignId << endl;
                mobile_bidder->set_campaignid(campaignId);
                
                Json::Value action = ext["action"];
                if(!mutableAction(mobile_request,mobile_action,action))
                    return NULL;
                //cout << "mutableAction success" << endl;
                Json::Value temp = ext["temp"];
                string nurl = bid[i]["nurl"].asString();
                if(!creativeAddEvents(mobile_creative,temp,nurl))
                    return NULL;
                //cout << "creativeAddEvents success" << endl;
                ret = true;
            }
        }        

        if(ret)
        {
            if(root.isMember("id"))
                mobile_response.set_id(root["id"].asString());
            else
            {
                g_worker_logger->debug("GEN FAILED : no valid id ");
                return NULL;
            }
            uuid = root["id"].asString();
            if(root.isMember("bidid"))
                mobile_response.set_bidid(root["bidid"].asString());
            else
                mobile_response.set_bidid("null");

            //MobileAdResponse_Bidder *bidder_info = mobile_response.mutable_bidder();
            //bidder_info->set_bidderid("1");

            int dataSize = mobile_response.ByteSize();
            char *dataBuf = new char[dataSize];
            mobile_response.SerializeToArray(dataBuf, dataSize); 
            response_commMsg.set_businesscode(request_commMsg.businesscode());
            response_commMsg.set_datacodingtype(request_commMsg.datacodingtype());
            response_commMsg.set_ttl(request_commMsg.ttl());
            response_commMsg.set_data(dataBuf, dataSize);
        
            dataSize = response_commMsg.ByteSize();
            char* comMessBuf = new char[dataSize];
            response_commMsg.SerializeToArray(comMessBuf, dataSize);
            delete[] dataBuf;
            ret_dataLen = dataSize;
            g_worker_logger->debug("BidResponse.json->MobileAdResponse.proto convert success !");
            return comMessBuf;
        }
        else
        {
            g_worker_logger->debug("BidResponse.json->MobileAdResponse.proto convert failed !");
            return NULL;
        }
        
    }  
    else 
    {           
        g_worker_logger->error("BidResponse.json parse failed !");
        return NULL;
    }
}
char* connectorServ::convertGYinBidResponseProtoToProtobuf(char *data,int dataLen,int& ret_dataLen,string& uuid)
{
    
    CommonMessage response_commMsg;
    MobileAdResponse mobile_response;

    commMsgRecord* cmrObj;
    CommonMessage request_commMsg;
    MobileAdRequest mobile_request;

    BidResponse bidresponse;
    if(!bidresponse.ParseFromArray(data, dataLen))
    {
        g_workerGYIN_logger->error("GYIN BidResponse.proto Parse Fail,check required fields");
        return NULL;
    }

    string str_id = bidresponse.id();
    
    cmrObj = checkValidId(str_id);
    if(!cmrObj)
    {
        g_workerGYIN_logger->debug("FIND RECLIST FAILED uuid: {0}", str_id);
        return NULL;
    }
    //g_workerGYIN_logger->trace("FIND RECLIST success uuid: {0}", str_id);
        
    request_commMsg.ParseFromArray(cmrObj->data, cmrObj->datalen);  
    const string& commMsg_data = request_commMsg.data();        
    mobile_request.ParseFromString(commMsg_data);

    bool ret = false;
    for(int i=0; i<bidresponse.seatbid_size(); i++)
    {
        MobileAdResponse_mobileBid *mobile_bidder = mobile_response.add_bidcontent();                   
        //MobileAdResponse_Creative  *mobile_creative =  mobile_bidder->add_creative();   
        Seatbid GYIN_seatbid = bidresponse.seatbid(i);
        string localExtNetId = m_dspManager.getGuangYinObject()->getExtNetId();    
        if(strcmp(GYIN_seatbid.seat().c_str(),localExtNetId.c_str()) != 0) //netid noequal localextnetid
        {
            g_workerGYIN_logger->debug("GYIN GEN FAILED : localExtNetId: {0} noequal gyin.netid: {1}", localExtNetId, GYIN_seatbid.seat());
            return NULL;
        }
        string intNetId = m_dspManager.getGuangYinObject()->getIntNetId();    
        MobileAdResponse_Bidder *bidder_info = mobile_response.mutable_bidder();
        bidder_info->set_bidderid(intNetId);
        
        map<int,string>::iterator it = CampaignMap.find(atoi(intNetId.c_str()));
        if(it == CampaignMap.end())
        {
            g_workerGYIN_logger->debug("GYIN GEN FAILED : get campainId from CampaignMap fail .  intNetId: {0}",intNetId);
            return NULL;
        }
        string campaignId = it->second;      
        mobile_bidder->set_campaignid(campaignId);

        if(GYIN_seatbid.bid_size()==0)
        {
            g_workerGYIN_logger->debug("GYIN GEN FAILED : novalid bid, bid_size(): {0:d}",GYIN_seatbid.bid_size());
            return NULL;
        }
        Bid GYIN_bid = GYIN_seatbid.bid(0);
        mobile_bidder->set_biddingtype("CPM");   
        float GYIN_price = 10 * GYIN_bid.price();
        if((GYIN_price >= -EPSINON)&&(GYIN_price <= EPSINON))  // 0
        {
            g_worker_logger->debug("GYIN GEN FAILED : bid.price = {0:d} ",GYIN_price);
            return NULL;
        }
        char str_price[16] = {0};
        sprintf(str_price,"%.1f",GYIN_price);                
        mobile_bidder->set_biddingvalue(str_price);
        mobile_bidder->set_expectcpm(str_price);        
        
        mobile_bidder->set_currency("CNY");
        
        MobileAdResponse_Creative  *mobile_creative =  mobile_bidder->add_creative();   
        mobile_creative->set_creativeid("0");
        char str_w[16] = {0};
        char str_h[16] = {0};
        sprintf(str_w,"%.1f",GYIN_bid.w());
        sprintf(str_h,"%.1f",GYIN_bid.h());                
        mobile_creative->set_width(str_w);
        mobile_creative->set_height(str_h);

        string appType = mobile_request.apptype();
        if(!strcmp(appType.c_str(), "app"))
        {
            g_workerGYIN_logger->trace("mapp response uuid: {0}", str_id);
            mobile_creative->set_adchanneltype(MobileAdResponse_AdChannelType_MOBILE_APP);
        }
        else if(!strcmp(appType.c_str(), "mweb"))
        {
            g_workerGYIN_logger->trace("mweb response uuid: {0}", str_id);
            mobile_creative->set_adchanneltype(MobileAdResponse_AdChannelType_MOBILE_WEB);
        }
        else if(!strcmp(appType.c_str(), "pcweb"))
        {
            g_workerGYIN_logger->trace("pcweb response uuid: {0}", str_id);
            mobile_creative->set_adchanneltype(MobileAdResponse_AdChannelType_PC_WEB);
        }

        MobileAdResponse_Action *mobile_action = mobile_bidder->mutable_action();   
        if(!GYIN_mutableAction(mobile_request,mobile_action,GYIN_bid))
            return NULL;
        if(!GYIN_creativeAddEvents(mobile_request,mobile_creative,GYIN_bid))
            return NULL;
        
        ret = true;
                
    }    
    
    if(ret)
    {
        if(bidresponse.has_id())
            mobile_response.set_id(bidresponse.id());
        else
        {
            g_workerGYIN_logger->debug("GYIN GEN FAILED : no valid id ");
            return NULL;
        }
        uuid = bidresponse.id();
        if(bidresponse.has_bidid())
            mobile_response.set_bidid(bidresponse.bidid());
        else
            mobile_response.set_bidid("null");

        //MobileAdResponse_Bidder *bidder_info = mobile_response.mutable_bidder();
        //bidder_info->set_bidderid("1");

        int dataSize = mobile_response.ByteSize();
        char *dataBuf = new char[dataSize];
        mobile_response.SerializeToArray(dataBuf, dataSize); 
        response_commMsg.set_businesscode(request_commMsg.businesscode());
        response_commMsg.set_datacodingtype(request_commMsg.datacodingtype());
        response_commMsg.set_ttl(request_commMsg.ttl());
        response_commMsg.set_data(dataBuf, dataSize);
    
        dataSize = response_commMsg.ByteSize();
        char* comMessBuf = new char[dataSize];
        response_commMsg.SerializeToArray(comMessBuf, dataSize);
        delete[] dataBuf;
        ret_dataLen = dataSize;
        g_workerGYIN_logger->debug("GYIN BidResponse.proto->MobileAdResponse.proto convert success !");
        return comMessBuf;
    }
    else
    {
        g_workerGYIN_logger->debug("GYIN BidResponse.proto->MobileAdResponse.proto convert failed !");
        return NULL;
    }
}

char* connectorServ::convertSmaatoBidResponseXMLtoProtobuf(char *data,int dataLen,int& ret_dataLen,string& uuid, const CommonMessage& request_commMsg)
{
    xmlDocPtr pdoc = xmlParseMemory(data, dataLen);
    xmlNodePtr root = xmlDocGetRootElement(pdoc);
    if(root == NULL)
    {
        g_workerSMAATO_logger->error("Failed to parse xml file");
        return NULL;
    }
    xmlNodePtr cur = root;
    if(xmlStrcmp(cur->name, (const xmlChar *)"response"))
    {
        g_workerSMAATO_logger->error("The root is not response");
        return NULL;
    }

    //Node: sessionid ->req: device.hidMd5
    cur = cur->xmlChildrenNode;
    if(xmlStrcmp(cur->name, (const xmlChar *)"sessionid"))
    {
        g_workerSMAATO_logger->error("Node: sessionid miss");
        return NULL;
    }    
    xmlChar *sessionid;
    sessionid = xmlNodeGetContent(cur);
    char *ssid = new char[64];
    
    strcpy(ssid, (char *)sessionid);
    {   
        char *start = ssid;
        while(*start != '.')
            start++;
        *start = '\0';        
    }
    g_workerSMAATO_logger->debug("SESSIONID: {0}", ssid);
    xmlFree(sessionid);
    
    //Node: status
    cur = cur->next;
    if(xmlStrcmp(cur->name, (const xmlChar *)"status"))
    {
        g_workerSMAATO_logger->error("Node: status miss");
        return NULL;
    }    
    xmlChar *status;
    status = xmlNodeGetContent(cur);
    if(!strcmp((char *)status, "error"))
    {
        cur = cur->next;
        while(xmlStrcmp(cur->name, (const xmlChar *)"error"))
            cur = cur->next;
        //Node: error
        //Node: error code
        cur = cur->xmlChildrenNode;
        if(xmlStrcmp(cur->name, (const xmlChar *)"code"))
        {
            g_workerSMAATO_logger->error("Node: error code miss");
            return NULL;
        }    
        xmlChar *code;
        code = xmlNodeGetContent(cur);
        xmlFree(code);
        
        //Node: error desc
        cur = cur->next;
        if(xmlStrcmp(cur->name, (const xmlChar *)"desc"))
        {
            g_workerSMAATO_logger->error("Node: error desc miss");
            return NULL;
        }    
        xmlChar *desc;
        desc = xmlNodeGetContent(cur);
        g_workerSMAATO_logger->debug("error: {0}", (char *)desc);
        xmlFree(desc);
        xmlFree(status);
        return NULL;        
    }
    else if(!strcmp((char *)status, "success"))
    {
        g_workerSMAATO_logger->trace("STATUS success");
         xmlFree(status);
    }
    else
    {
        g_workerSMAATO_logger->error("Node: unknow status");
        xmlFree(status);
        return NULL;
    }

    //Node: user
    cur = cur->next;
    xmlNodePtr user = cur;
    //Node: user->id
    user = user->xmlChildrenNode;
    xmlChar *id = xmlNodeGetContent(user);
    xmlFree(id);

    //Node: ads
    cur = cur->next;
    if(xmlStrcmp(cur->name, (const xmlChar *)"ads"))
    {
        g_workerSMAATO_logger->error("Node: ads miss");
        return NULL;
    }    

    return xmlParseAds(cur, ret_dataLen, uuid, request_commMsg);   
    //return NULL;
    
}

char* connectorServ::convertInmobiBidResponseXMLtoProtobuf(char *data,int dataLen,int& ret_dataLen,string& uuid, const CommonMessage& request_commMsg)
{
    xmlDocPtr pdoc = xmlParseMemory(data, dataLen);
    xmlNodePtr root = xmlDocGetRootElement(pdoc);
    if(root == NULL)
    {
        g_workerINMOBI_logger->error("Failed to parse xml file");
        return NULL;
    }
    xmlNodePtr cur = root;
    if(xmlStrcmp(cur->name, (const xmlChar *)"AdResponse"))
    {
        g_workerINMOBI_logger->error("The root is not AdResponse");
        return NULL;
    }

    /*
      * xmlGetProp: get cur Node's property    xmlChar *id = xmlGetProp(cur, (const xmlChar *)"id");
      * xmlNodeGetContent: get cur Node's content    xmlChar *content = xmlNodeGetContent(cur);
      */

      
    //Node: Ads
    cur = cur->xmlChildrenNode;
    if(xmlStrcmp(cur->name, (const xmlChar *)"Ads"))
    {
        g_workerINMOBI_logger->error("Node: Ads miss");
        return NULL;
    }    
    xmlChar *ads_number = xmlGetProp(cur, (const xmlChar *)"number");
    xmlFree(ads_number);

    //Node: Ad
    cur= cur->xmlChildrenNode;
    if(xmlStrcmp(cur->name, (const xmlChar *)"Ad"))
    {
        g_workerINMOBI_logger->error("Node: Ad miss");
        return NULL;
    }
    xmlChar *type = xmlGetProp(cur, (const xmlChar *)"type");
    xmlChar *width = xmlGetProp(cur, (const xmlChar *)"width");
    xmlChar *height = xmlGetProp(cur, (const xmlChar *)"height");
    xmlChar *content = xmlNodeGetContent(cur);

    string ad_type = (char *)type;
    string ad_width = (char *)width;
    string ad_height = (char *)height;
    string ad_content = (char *)content;    

    xmlFree(type);
    xmlFree(width);
    xmlFree(height);
    xmlFree(content);

    CommonMessage response_commMsg;
    MobileAdResponse mobile_response;

    const string& commMsg_data = request_commMsg.data();
    MobileAdRequest mobile_request;
    mobile_request.ParseFromString(commMsg_data);

    MobileAdRequest_AdType requestAdtype;
    requestAdtype = mobile_request.type();

    uuid = mobile_request.id();

    mobile_response.set_id(uuid);

    string intNetId = m_dspManager.getInMobiObject()->getIntNetId();    
    MobileAdResponse_Bidder *bidder_info = mobile_response.mutable_bidder();
    bidder_info->set_bidderid(intNetId);

    mobile_response.set_bidid("null");
        
    MobileAdResponse_mobileBid *mobile_bidder = mobile_response.add_bidcontent();  
    
    //MobileAdResponse_Action *mobile_action = mobile_bidder->mutable_action();  
    MobileAdResponse_Creative  *mobile_creative =  mobile_bidder->add_creative();  

    map<int,string>::iterator it = CampaignMap.find(atoi(intNetId.c_str()));
    if(it == CampaignMap.end())
    {
        g_workerINMOBI_logger->debug("INMOBI GEN FAILED : get campainId from CampaignMap fail .  intNetId: {0}",intNetId);
        return NULL;
    }
    string campaignId = it->second;      
    mobile_bidder->set_campaignid(campaignId);

    int price = m_dspManager.getInMobiObject()->getPrice();
    stringstream ss;
    string s_price;
    ss << price;
    ss >> s_price;
    
    mobile_bidder->set_biddingtype("CPM");                   
    mobile_bidder->set_biddingvalue(s_price);
    mobile_bidder->set_expectcpm(s_price);            
    mobile_bidder->set_currency("CNY");

    mobile_creative->set_creativeid("0");    
    mobile_creative->set_adchanneltype(MobileAdResponse_AdChannelType_MOBILE_APP); 
    mobile_creative->set_width(ad_width);
    mobile_creative->set_height(ad_height);

    
    if(INMOBI_creativeAddEvents(mobile_creative, ad_type, ad_content, requestAdtype))
    {
        g_workerINMOBI_logger->debug("INMOBI_creativeAddEvents failed");
        return NULL;
    }
    

    int dataSize = mobile_response.ByteSize();
    char *dataBuf = new char[dataSize];
    mobile_response.SerializeToArray(dataBuf, dataSize); 
    response_commMsg.set_businesscode(request_commMsg.businesscode());
    response_commMsg.set_datacodingtype(request_commMsg.datacodingtype());
    response_commMsg.set_ttl(request_commMsg.ttl());
    //response_commMsg.set_businesscode("2");
    //response_commMsg.set_datacodingtype("null");
    //response_commMsg.set_ttl("250");
    response_commMsg.set_data(dataBuf, dataSize);
     
    dataSize = response_commMsg.ByteSize();
    char* comMessBuf = new char[dataSize];
    response_commMsg.SerializeToArray(comMessBuf, dataSize);
    delete[] dataBuf;
    ret_dataLen = dataSize;
    g_workerINMOBI_logger->debug("INMOBI.xml->MobileAdResponse.proto convert success !");
    return comMessBuf;
        
}


char *connectorServ::xmlParseAds(xmlNodePtr &adsNode, int& ret_dataLen, string& uuid, const CommonMessage& request_commMsg)
{
    CommonMessage response_commMsg;
    MobileAdResponse mobile_response;

    const string& commMsg_data = request_commMsg.data();
    MobileAdRequest mobile_request;
    mobile_request.ParseFromString(commMsg_data);

    MobileAdRequest_AdType requestAdtype;
    requestAdtype = mobile_request.type();

    uuid = mobile_request.id();

	//list<string>* requestUuidList = m_dspManager.getSmaatoObject()->getRequestUuidList();
    //if(requestUuidList.empty())
        //return NULL;

	//m_dspManager.getSmaatoObject()->requestUuidList_Locklock();
	//uuid = requestUuidList.front();
	//requestUuidList.pop_front();
	//m_dspManager.getSmaatoObject()->requestUuidList_Lockunlock();
        
    mobile_response.set_id(uuid); //uuid
    
    string intNetId = m_dspManager.getSmaatoObject()->getIntNetId();    
    MobileAdResponse_Bidder *bidder_info = mobile_response.mutable_bidder();
    bidder_info->set_bidderid(intNetId);

    mobile_response.set_bidid("null");
        
    MobileAdResponse_mobileBid *mobile_bidder = mobile_response.add_bidcontent();  
    
    MobileAdResponse_Action *mobile_action = mobile_bidder->mutable_action();  
    MobileAdResponse_Creative  *mobile_creative =  mobile_bidder->add_creative();  

    map<int,string>::iterator it = CampaignMap.find(atoi(intNetId.c_str()));
    if(it == CampaignMap.end())
    {
        g_workerGYIN_logger->debug("SMAATO GEN FAILED : get campainId from CampaignMap fail .  intNetId: {0}",intNetId);
        return NULL;
    }
    string campaignId = it->second;      
    mobile_bidder->set_campaignid(campaignId);

    int price = m_dspManager.getSmaatoObject()->getPrice();
    stringstream ss;
    string s_price;
    ss << price;
    ss >> s_price;
    
    mobile_bidder->set_biddingtype("CPM");                   
    mobile_bidder->set_biddingvalue(s_price);
    mobile_bidder->set_expectcpm(s_price);            
    mobile_bidder->set_currency("CNY");

    mobile_creative->set_creativeid("0");    
    mobile_creative->set_adchanneltype(MobileAdResponse_AdChannelType_MOBILE_APP);        

    xmlNodePtr cur = adsNode;

    //Node: ad
    cur= cur->xmlChildrenNode;
    if(xmlStrcmp(cur->name, (const xmlChar *)"ad"))
    {
        g_workerSMAATO_logger->error("Node: ads.ad miss");
        return NULL;
    }   

    //xmlGetProp: get cur Node's property
    xmlChar *id = xmlGetProp(cur, (const xmlChar *)"id");
    xmlChar *type = xmlGetProp(cur, (const xmlChar *)"type");

    if(!strcmp((char *)type, "TXT"))
    {
        cur = cur->xmlChildrenNode;
        SMAATO_mutableAction(mobile_action, cur, TXT);     
        if(!SMAATO_creativeAddEvents(mobile_creative, cur, TXT, requestAdtype))
        {
            g_workerSMAATO_logger->debug("TXT SMAATO_creativeAddEvents failed");
            xmlFree(id);
            xmlFree(type);
            return NULL;
        }
    }
    else if(!strcmp((char *)type, "IMG"))
    {
        g_workerSMAATO_logger->debug("CUR NO SUPPORT SMAATO IMG");
        return NULL;
        #if 0
        xmlChar *width = xmlGetProp(cur, (const xmlChar *)"width");
        xmlChar *height = xmlGetProp(cur, (const xmlChar *)"height");
                   
        mobile_creative->set_width((char *)width);
        mobile_creative->set_height((char *)height);
        
        cur = cur->xmlChildrenNode;
        SMAATO_mutableAction(mobile_action, cur, IMG);     
        if(!SMAATO_creativeAddEvents(mobile_creative, cur, IMG, requestAdtype))
        {
            g_workerSMAATO_logger->debug("IMG SMAATO_creativeAddEvents failed");
            xmlFree(id);
            xmlFree(type);
            return NULL;
        }
        #endif 
    }
    else if(!strcmp((char *)type, "RICHMEDIA"))
    {
        #if 1
        cur = cur->xmlChildrenNode;
        SMAATO_mutableAction(mobile_action, cur, RICHMEDIA);     
        if(!SMAATO_creativeAddEvents(mobile_creative, cur, RICHMEDIA, requestAdtype))
        {
            g_workerSMAATO_logger->debug("RICHMEDIA SMAATO_creativeAddEvents failed");
            xmlFree(id);
            xmlFree(type);
            return NULL;
        }
        #endif
        #if 0
        g_workerSMAATO_logger->debug("ADTYPE:RICHMEDIA not finish");
        xmlFree(id);
        xmlFree(type);
        return NULL;
        #endif
    }
    else
    {
        g_workerSMAATO_logger->error("NONSUPPORT AD TYPE");
        xmlFree(id);
        xmlFree(type);
        return NULL;
    }

    xmlFree(id);
    xmlFree(type);
    
 

     int dataSize = mobile_response.ByteSize();
     char *dataBuf = new char[dataSize];
     mobile_response.SerializeToArray(dataBuf, dataSize); 
     response_commMsg.set_businesscode(request_commMsg.businesscode());
     response_commMsg.set_datacodingtype(request_commMsg.datacodingtype());
     response_commMsg.set_ttl(request_commMsg.ttl());
     //response_commMsg.set_businesscode("2");
     //response_commMsg.set_datacodingtype("null");
     //response_commMsg.set_ttl("250");
     response_commMsg.set_data(dataBuf, dataSize);
     
     dataSize = response_commMsg.ByteSize();
     char* comMessBuf = new char[dataSize];
     response_commMsg.SerializeToArray(comMessBuf, dataSize);
     delete[] dataBuf;
     ret_dataLen = dataSize;
     g_workerSMAATO_logger->debug("SMAATO.xml->MobileAdResponse.proto convert success !");
     return comMessBuf;
     
}

commMsgRecord* connectorServ::checkValidId(const string& str_id)
{   
    auto commMsgRecordIt = commMsgRecordList.find(str_id);
    if(commMsgRecordIt != commMsgRecordList.end())
    {
        return commMsgRecordIt->second;
    }
    else
        return NULL;
}
bool connectorServ::mutableAction(MobileAdRequest &mobile_request,MobileAdResponse_Action *mobile_action,Json::Value &action)
{
    if(!action.isMember("acturl"))
    {
        g_worker_logger->debug("GEN FAILED : no valid acturl ");
        return false;
    }
    int acttype = action["acttype"].asInt();
    int inapp = action["inapp"].asInt();
    Json::Value content; 
    Json::Value download;
    const char *str_acttype = NULL;
    char str_inapp[16] = {0};
    
    sprintf(str_inapp,"%d",inapp); 

    int autoin = 1;
    if(action.isMember("autoin"))    
        autoin = action["autoin"].asInt();       
    char str_autoin[4] = {0};
    sprintf(str_autoin,"%d",autoin); 

    MobileAdRequest_Device dev = mobile_request.device();    

    /**
         *  
         * ActionType:         
         *          WEB_PAGE(1), APP_DOWNLOAD(2), WEB_PAGE(3),
         *          ENGAGMENT(4), PHONE_CALL(5), SMS(6), EMAIL(7),
         *          TWITIER(8)
         *          
         */
    
    switch(acttype)
    {
        case 1: //web page   
            {
                str_acttype = "web_page";
                if(action.isMember("page_title"))
                    content["page_title"] = action["page_title"].asString();
                if(action.isMember("domain_name"))
                    content["domain_name"] = action["domain_name"].asString();  
                content["in_app"] = inapp;
                content["web_url"] = action["acturl"].asString();    
            }
            break;
        case 2: //app download
            {
                str_acttype = "app_download";
                if(action.isMember("app_name"))
                    content["app_name"] = action["app_name"].asString();
                if(action.isMember("category"))
                    content["category"] = action["category"].asString();
                Json::Value item;
                if(strcmp("1",dev.platform().c_str()) == 0)         //andriod
                {                
                    item["platform"] = "android";                
                }
                else if(strcmp("2",dev.platform().c_str()) == 0)    //apple IOS
                {
                    item["platform"] = "ios";   
                }
                else
                {
                    g_worker_logger->debug("Get platform from adRequest fail...");
                }
                //item["in_app"] = inapp;     
                item["auto_install"] = autoin;
                item["url"] = action["acturl"].asString();
                           
                download.append(item);
                content["download"] = download;
            }
            break;
        case 3: //deeplink 
            {
                str_acttype = "deep_link";
                if(!(action.isMember("fbtype")&&action.isMember("fburl")))
                {
                    g_worker_logger->debug("GEN FAILED : no valid fbtype | fburl");
                    return false;
                }
                if(action.isMember("app_name"))
                        content["app_name"] = action["app_name"].asString();
                Json::Value urls;
                if(strcmp("1",dev.platform().c_str()) == 0)         //andriod
                {
                    Json::Value android;
                    android["scheme_url"] = action["acturl"].asString();
                    if(action.isMember("package_name"))
                        android["package_name"] = action["package_name"].asString();
                    Json::Value fallback_url;
                    fallback_url["url"] = action["fburl"].asString();
                    if(action["fbtype"].asInt() == 1)   //app installs
                    {
                        android["fallback_type"] = "app_install";
                        fallback_url["auto_install"] = autoin;
                    }
                    else if(action["fbtype"].asInt() == 2)  //website
                    {
                        android["fallback_type"] = "web_site";
                        fallback_url["in_app"] = inapp;
                    }
                    android["fallback_url"] = fallback_url;
                    urls["android"] = android;
                }
                else if(strcmp("2",dev.platform().c_str()) == 0)    //apple IOS
                {
                    Json::Value apple;
                    apple["scheme_url"] = action["acturl"].asString();
                    if(action.isMember("package_name"))
                        apple["package_name"] = action["package_name"].asString();
                    Json::Value fallback_url;
                    fallback_url["url"] = action["fburl"].asString();
                    if(action["fbtype"].asInt() == 1)   //app install
                    {
                        apple["fallback_type"] = "app_install";
                        fallback_url["auto_install"] = autoin;
                    }
                    else if(action["fbtype"].asInt() == 2)  //website
                    {
                        apple["fallback_type"] = "web_site";
                        fallback_url["in_app"] = inapp;
                    }
                    apple["fallback_url"] = fallback_url;
                    urls["android"] = apple;
                }
                
                content["urls"] = urls;
            }
            break;
        default:
            break;                
    }
               
    if(action.isMember("name"))
        mobile_action->set_name(action["name"].asString());
        
    
    string str_content = content.toStyledString();
    
    mobile_action->set_content(str_content);    
    mobile_action->set_actiontype(str_acttype);
    mobile_action->set_inapp(str_inapp);      
   
    return true;
    
}

bool connectorServ::creativeAddEvents(MobileAdResponse_Creative  *mobile_creative,Json::Value &temp,string& nurl)
{
    int temptype = temp["temtype"].asInt();                
    
    int id = 0;
    string RetCode;           

    /**
         *  MediaType: 
         *          BANNER(1), INTERSTITIAL(2), NATIVE(3)
         *
         *  MediaSubType:  
         *          NORMAL_BANNER(1), TXT_BANNER(2), EXP_BANNER(3),
         *          ICON_BANNER(1), TRD_BANNER(10), INT_FULL_PAGE(5)
         */
         
    switch(temptype)
        {
            case 1:         //normal banner with image
                {
                    id = 61;
                    if(!temp.isMember("creurl"))
                    {
                        g_worker_logger->debug("GEN FAILED : no valid creurl ");
                        return false;
                    }                        
                    string creurl = temp["creurl"].asString();
                    map<int,string>::iterator it = Creative_template.find(id);
                    RetCode = it->second;                
                    if(RetCode.empty() == false)
                    {
                        string decodeStr;                        
                        
                        decodeStr = UrlDecode(RetCode);                       
                        replace(decodeStr,"${MY_IMAGE}",creurl); 
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));
                        mobile_creative->set_mediatypeid("1");
                        mobile_creative->set_mediasubtypeid("1");                        
                    }
                }
                break;
            case 2:         //pure text banner
                {
                    id = 66;
                    if(!(temp.isMember("title")&&temp.isMember("desc")))
                    {
                        g_worker_logger->debug("GEN FAILED : no valid title | desc ");
                        return false;
                    }   
                    string title = temp["title"].asString();
                    string desc = temp["desc"].asString();
                    map<int,string>::iterator it = Creative_template.find(id);
                    RetCode = it->second;        
                    if(RetCode.empty() == false)
                    {
                        string decodeStr;                        
                        
                        decodeStr = UrlDecode(RetCode);                        
                        
                        replace(decodeStr,"${MY_TITLE}",title);
                        replace(decodeStr,"${MY_DESCRIPTION}",desc);
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));  
                        mobile_creative->set_mediatypeid("1");
                        mobile_creative->set_mediasubtypeid("2");
                    }                    
                }
                break;
            case 3:         //text banner with icon
                {
                    id = 67;
                    if(!(temp.isMember("icurl")&&temp.isMember("title")&&temp.isMember("desc")))
                    {
                        g_worker_logger->debug("GEN FAILED : no valid icurl | title | desc ");
                        return false;
                    } 
                    string icurl = temp["icurl"].asString();
                    string title = temp["title"].asString();
                    string desc = temp["desc"].asString();                    
                    map<int,string>::iterator it = Creative_template.find(id);
                    RetCode = it->second;        
                    if(RetCode.empty() == false)
                    {
                        string decodeStr;                        
                        
                        decodeStr = UrlDecode(RetCode);                    
                        
                        replace(decodeStr,"${MY_ICON}",icurl);
                        replace(decodeStr,"${MY_TITLE}",title);
                        replace(decodeStr,"${MY_DESCRIPTION}",desc);
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));    
                        mobile_creative->set_mediatypeid("1");
                        mobile_creative->set_mediasubtypeid("3");
                        
                    }                    
                }
                break;
            case 4:         //expandable banner
                {
                    id = 78;
                    if(!(temp.isMember("creurl")&&temp.isMember("expurl")))
                    {
                        g_worker_logger->debug("GEN FAILED : no valid creurl | expurl ");
                        return false;
                    } 
                    string creurl = temp["creurl"].asString();
                    string expurl = temp["expurl"].asString();
                    map<int,string>::iterator it = Creative_template.find(id);
                    RetCode = it->second;        
                    if(RetCode.empty() == false)
                    {
                        string decodeStr;                        
                        
                        decodeStr = UrlDecode(RetCode);                        
                        
                        replace(decodeStr,"${MY_IMAGE}",creurl);
                        replace(decodeStr,"${MY_EXPAND_URL}",expurl);
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));  
                        mobile_creative->set_mediatypeid("1");
                        mobile_creative->set_mediasubtypeid("1");
                    }                    
                }
                break;
            case 5:         //interstitial
                {
                    id = 59;
                    if(!temp.isMember("creurl"))
                    {
                        g_worker_logger->debug("GEN FAILED : no valid creurl ");
                        return false;
                    }     
                    string creurl = temp["creurl"].asString();
                    map<int,string>::iterator it = Creative_template.find(id);
                    RetCode = it->second;  
                    if(RetCode.empty() == false)
                    {
                        string decodeStr;                        
                        
                        decodeStr = UrlDecode(RetCode);                        
                        replace(decodeStr,"${MY_IMAGE}",creurl);
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));
                        mobile_creative->set_mediatypeid("2");
                        mobile_creative->set_mediasubtypeid("5");
                    }
                }
                break;
            default:
                break;
    }         
           
    #if 0
    if(temp.isMember("curl"))
    {
        MobileAdResponse_TrackingEvents *creative_event = mobile_creative->add_events();
        creative_event->set_event("CLICK");
        creative_event->set_trackurl(temp["curl"].asString());
    }
    #endif
    if(temp.isMember("imurl")&&(temp["imurl"].asString().empty() == false))
    {
        MobileAdResponse_TrackingEvents *creative_event = mobile_creative->add_events();
        creative_event->set_event("IMP");
        creative_event->set_trackurl(temp["imurl"].asString());
    }   
    if(nurl.empty() == false)
    {
        MobileAdResponse_TrackingEvents *creative_event = mobile_creative->add_events();
        creative_event->set_event("IMP");
        creative_event->set_trackurl(nurl);
    }  
    return true;
}
bool connectorServ::GYIN_mutableAction(MobileAdRequest &mobile_request,MobileAdResponse_Action *mobile_action,Bid &GYIN_bid)
{
    Json::Value content; 
    Json::Value download;    
    const char *str_acttype = NULL;    
    
    if((!GYIN_bid.has_action())||(GYIN_bid.action().empty())||(GYIN_bid.action() == "click")) // default : webpage
    {
        str_acttype = "web_page";   
        if(GYIN_bid.bundle().empty() == false)
            content["app_name"] = GYIN_bid.bundle();        
        //if(GYIN_bid.adomain().empty() == false)
        //    content["domain_name"] = GYIN_bid.adomain();  
        content["in_app"] = 1;
        content["web_url"] = GYIN_bid.curl();
    }
    else if(GYIN_bid.action() == "install")    // install 
    {
        str_acttype = "app_download";
        if(GYIN_bid.bundle().empty() == false)
            content["app_name"] = GYIN_bid.bundle();        
            
        MobileAdRequest_Device dev = mobile_request.device();
        Json::Value item;
        if(strcmp("1",dev.platform().c_str()) == 0)         //andriod
        {                
            item["platform"] = "android";                
        }
        else if(strcmp("2",dev.platform().c_str()) == 0)    //apple IOS
        {
            item["platform"] = "ios";   
        }
        else
        {
            g_workerGYIN_logger->debug("Get platform from adRequest fail...");
        }
        //item["in_app"] = 1;
        item["auto_install"] = 1;
        item["url"] = GYIN_bid.curl();        
        
        download.append(item);
        content["download"] = download;
    }
    
    string str_content = content.toStyledString();
    
    mobile_action->set_content(str_content);
    mobile_action->set_actiontype(str_acttype);
    mobile_action->set_inapp("1");         

    return true;
}
bool connectorServ::GYIN_creativeAddEvents(MobileAdRequest &mobile_request,MobileAdResponse_Creative  *mobile_creative,Bid &GYIN_bid)
{
    MobileAdRequest_AdType type = mobile_request.type();
    int id = 0;
    string RetCode;
    AdmType GYIN_admtype = GYIN_bid.admtype();
    //string adm;
    string mediaTypeId;
    string appType = mobile_request.apptype();

    /**
         *  MediaType: 
         *          BANNER(1), INTERSTITIAL(2), NATIVE(3)
         *
         *  MediaSubType:  
         *          NORMAL_BANNER(1), TXT_BANNER(2), EXP_BANNER(3),
         *          ICON_BANNER(1), TRD_BANNER(10), INT_FULL_PAGE(5)
         */
    
    switch(type)
    {
        case MobileAdRequest_AdType_BANNER:
            {
                if(!strcmp(appType.c_str(), "app"))
                {
                    id = 79;                
                }
                else if(!strcmp(appType.c_str(), "mweb"))
                {
                    switch(GYIN_admtype)
                    {
                        case HTML:
                            {
                                id = 103;    //third HTML code
                            }
                            break;
                        case JSON:
                            {
                                id = 86;    //image url or 102 just for url without image
                            }
                            break;
                        default:
                            break;
                    }
                }
                else if(!strcmp(appType.c_str(), "pcweb"))
                {
                    switch(GYIN_admtype)
                    {
                        case HTML:
                            {
                                id = 91;    //third HTML code
                            }
                            break;
                        case JSON:
                            {
                                id = 93;    //image url or 92 for url without image
                            }
                            break;
                        default:
                            break;
                    }
                }
                mediaTypeId = "1";
            }
            break;
        case MobileAdRequest_AdType_INTERSTITIAL:
            {
                if(!strcmp(appType.c_str(), "app"))
                {
                    id = 80;                
                }
                else if(!strcmp(appType.c_str(), "mweb"))
                {
                    switch(GYIN_admtype)
                    {
                        case HTML:
                            {
                                id = 96;    //third HTML code
                            }
                            break;
                        case JSON:
                            {
                                id = 88;    //image url or just 95 just for url without image
                            }
                            break;
                        default:
                            break;
                    }
                }
                else if(!strcmp(appType.c_str(), "pcweb"))
                {
                    g_workerGYIN_logger->error("PC WEB NO SUPPORT MobileAdRequest_AdType_INTERSTITIAL ");
                    return false;
                }
                mediaTypeId = "2";
            }
            break;
        case MobileAdRequest_AdType_NATIVE:
            {
                g_workerGYIN_logger->error("NO SUPPORT MobileAdRequest_AdType_NATIVE ");
                return false;
            }
            break;
        default:
            {
                g_workerGYIN_logger->error("NO SUPPORT ADTYPE");
                return false;
            }
            break;
    }

    map<int,string>::iterator it = Creative_template.find(id);
    if(it == Creative_template.end())
    {
        g_workerGYIN_logger->error("GYIN Creative_template find error ");
        return false;
    }
    RetCode = it->second;                
    if(RetCode.empty() == false)
    {
        string decodeStr;    
        
        decodeStr = UrlDecode(RetCode);      
        
        string sReplaceStr;
        string third_html;
        switch(GYIN_admtype)
        {
            case HTML:
                {
                    third_html = GYIN_bid.adm();                                
                }
                break;
            case JSON:
                {
                    third_html = "<img src='${SRC_URL}' width='${W}' height='${H}'></img>";
                    Json::Reader reader;
                    Json::Value root;
                    string adm = GYIN_bid.adm();
                    reader.parse(adm,root);
                    string adID = root["adID"].asString();
                    int width = root["width"].asInt();
                    int height = root["height"].asInt();
                    string src = root["src"].asString();
                    //string type = root["type"].asString();

                    char widthStr[16] = {0};
                    char heightStr[16] = {0};
                    sprintf(widthStr,"%d",width);
                    sprintf(heightStr,"%d",height);

                    replace(third_html,"${SRC_URL}",src);
                    replace(third_html,"${W}",widthStr);
                    replace(third_html,"${H}",heightStr);
                }
                break;
            default:
                break;
        }                    
        
        replace(third_html,"\"","\\\"");
        replace(third_html,"'","\\'");
        replace(third_html,"\t"," ");
        replace(third_html,"\n","");
        replace(third_html,"\r","");
        replace(third_html,"/","\\/");
        
        sReplaceStr.append("\"").append(third_html).append("\"");
        //cout << sReplaceStr << endl;
                    
        replace(decodeStr,"${MY_THIRD_HTML}",sReplaceStr);
        
        mobile_creative->set_admarkup(UrlEncode(decodeStr));
        mobile_creative->set_mediatypeid(mediaTypeId);
        mobile_creative->set_mediasubtypeid("5");
        
    }

    if(GYIN_bid.has_iurl()&&(GYIN_bid.iurl().empty() == false))
    {
        MobileAdResponse_TrackingEvents *creative_event = mobile_creative->add_events();
        creative_event->set_event("IMP");
        creative_event->set_trackurl(GYIN_bid.iurl());
    }   
    if(GYIN_bid.has_nurl()&&(GYIN_bid.nurl().empty() == false))
    {
        MobileAdResponse_TrackingEvents *creative_event = mobile_creative->add_events();
        creative_event->set_event("WIN");
        creative_event->set_trackurl(GYIN_bid.nurl());
    }      
    for(int i=0; i<GYIN_bid.extiurl_size(); i++)
    {
        MobileAdResponse_TrackingEvents *creative_event = mobile_creative->add_events();
        creative_event->set_event("IMP");
        creative_event->set_trackurl(GYIN_bid.extiurl(i));
    }
    return true;
}

bool connectorServ::SMAATO_mutableAction(MobileAdResponse_Action *mobile_action, xmlNodePtr &adNode, smRspType adType)
{
    xmlNodePtr cur = adNode;
    switch(adType)
    {
        case TXT:
            {
                Json::Value content; 
                content["app_name"] = "test";       
                content["in_app"] = 1;
                
                while(cur != NULL)
                {
                    //ads action
                    if(!xmlStrcmp(cur->name, (const xmlChar *)"action"))
                    {
                        xmlChar *actUrl = xmlGetProp(cur, (const xmlChar *)"target");    //      
                        content["web_url"] = (char *)actUrl;
                        xmlFree(actUrl);
                    }
                    cur = cur->next;
                }
                
                string str_content = content.toStyledString();
                mobile_action->set_content(str_content);
                mobile_action->set_actiontype("web_page");
                mobile_action->set_inapp("1"); 
            }
            break;
        case IMG:
            {
                Json::Value content; 
                content["app_name"] = "test";       
                content["in_app"] = 1;
                
                while(cur != NULL)
                {
                    //ads action
                    if(!xmlStrcmp(cur->name, (const xmlChar *)"action"))
                    {
                        xmlChar *actUrl = xmlGetProp(cur, (const xmlChar *)"target");    //      
                        content["web_url"] = (char *)actUrl;
                        xmlFree(actUrl);
                    }
                    cur = cur->next;
                }
                
                string str_content = content.toStyledString();
                mobile_action->set_content(str_content);
                mobile_action->set_actiontype("web_page");
                mobile_action->set_inapp("1"); 
            }
            break;
        case RICHMEDIA:
            {

            }
            break;
        default:
            break;
    }
    

    return true;
}

char *getTarMediadata(char *data)
{
    char *start = data;
    char *pos_start = memstr(start,strlen(start),"<![CDATA[");
    if(pos_start == NULL)
        return NULL;
    pos_start += 9;
    char *pos_end = memstr(start, strlen(start), "]]>");
    if(pos_end == NULL)
        return NULL;
    *pos_end = '\0';
    return pos_start;
}


bool connectorServ::SMAATO_creativeAddEvents(MobileAdResponse_Creative  *mobile_creative, xmlNodePtr &adNode, smRspType adType, MobileAdRequest_AdType& requestAdtype)
{
    xmlNodePtr cur = adNode;
    int id = 0;
    string RetCode;           
    string mediaTypeId;

    /**
         *  MediaType: 
         *          BANNER(1), INTERSTITIAL(2), NATIVE(3)
         *
         *  MediaSubType:  
         *          NORMAL_BANNER(1), TXT_BANNER(2), EXP_BANNER(3),
         *          ICON_BANNER(1), TRD_BANNER(10), INT_FULL_PAGE(5)
         */
         
    switch(adType)
        {            
            case TXT:         //pure text banner
                {
                    switch(requestAdtype)
                    {
                        case MobileAdRequest_AdType_BANNER:
                            id = 66;
                            mediaTypeId = "1";
                            break;
                        case MobileAdRequest_AdType_INTERSTITIAL:
                            break;
                        default:
                            break;
                    }
                    if(id == 0)
                        return false;
                    
                    string title;
                    while(cur != NULL)
                    {
                        //adtext
                        if(!xmlStrcmp(cur->name, (const xmlChar *)"adtext"))
                        {
                            xmlChar *adtext = xmlNodeGetContent(cur);    //   
                            title = (char *)adtext;
                            xmlFree(adtext);
                        }
                        cur = cur->next;
                    }
                    
                    map<int,string>::iterator it = Creative_template.find(id);
                    RetCode = it->second;        
                    if(RetCode.empty() == false)
                    {
                        string decodeStr;                        
                        
                        decodeStr = UrlDecode(RetCode);                        
                        
                        replace(decodeStr,"${MY_TITLE}",title);
                        replace(decodeStr,"${MY_DESCRIPTION}","null");
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));  
                        mobile_creative->set_mediatypeid("1");
                        mobile_creative->set_mediasubtypeid("2");
                    }                    
                }
                break;                
            case IMG:         //normal banner with image
                {
                    switch(requestAdtype)
                    {
                        case MobileAdRequest_AdType_BANNER:
                            id = 61;
                            mediaTypeId = "1";
                            break;
                        case MobileAdRequest_AdType_INTERSTITIAL:
                            id = 59;
                            mediaTypeId = "2";
                            break;
                        default:
                            break;
                    }
                    if(id == 0)
                        return false;
                    
                    string creurl;
                    while(cur != NULL)
                    {
                        //link
                        if(!xmlStrcmp(cur->name, (const xmlChar *)"link"))
                        {
                            xmlChar *imgLink = xmlNodeGetContent(cur);    //   
                            creurl = (char *)imgLink;
                            xmlFree(imgLink);
                        }
                        cur = cur->next;
                    }
                    
                    map<int,string>::iterator it = Creative_template.find(id);
                    RetCode = it->second;  
                    if(RetCode.empty() == false)
                    {
                        string decodeStr;                        
                        
                        decodeStr = UrlDecode(RetCode);                        
                        replace(decodeStr,"${MY_IMAGE}",creurl);
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));
                        mobile_creative->set_mediatypeid("2");
                        mobile_creative->set_mediasubtypeid("5");
                    }
                }
                break;
            case RICHMEDIA:
                {
                    switch(requestAdtype)
                    {
                        case MobileAdRequest_AdType_BANNER:
                            id = 101;    //MY_THIRD_HTML
                            mediaTypeId = "1";
                            break;
                        case MobileAdRequest_AdType_INTERSTITIAL:
                            id = 97;    //MY_THIRD_HTML
                            mediaTypeId = "2";
                            break;
                        default:
                            break;
                    }
                    if(id == 0)
                        return false;
                        
                    map<int,string>::iterator it = Creative_template.find(id);
                    if(it == Creative_template.end())
                    {
                        g_workerGYIN_logger->error("GYIN Creative_template find error ");
                        return false;
                    }
                    RetCode = it->second;                
                    if(RetCode.empty() == false)
                    {
                        string decodeStr;    
                        
                        decodeStr = UrlDecode(RetCode);      
                        
                        string sReplaceStr;
                        string third_html;

                        while(cur != NULL)
                        {
                            //mediadata
                            if(!xmlStrcmp(cur->name, (const xmlChar *)"mediadata"))
                            {
                                xmlChar *mediadata = xmlNodeGetContent(cur);    //   
                                //third_html = getTarMediadata((char *)mediadata);
                                third_html = (char *)mediadata;
                                xmlFree(mediadata);
                            }
                            cur = cur->next;
                        }                       

                        replace(third_html,"<![CDATA[","");
                        replace(third_html,"]]>","");
						
                        replace(third_html,"\"","\\\"");
                        replace(third_html,"'","\\'");
                        replace(third_html,"\t"," ");
                        replace(third_html,"\n","");
                        replace(third_html,"\r","");
                        replace(third_html,"/","\\/");

                        
                        sReplaceStr.append("\"").append(third_html).append("\"");
                        g_workerSMAATO_logger->debug("THIRD_HTML:\r\n{0}", third_html);
                                    
                        replace(decodeStr,"${MY_THIRD_HTML}",sReplaceStr);
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));
                        mobile_creative->set_mediatypeid(mediaTypeId);
                        mobile_creative->set_mediasubtypeid("5");
                        
                    }                    
                    
                }
                break;
            default:
                break;
    }         
           
    
    cur = adNode;
    while(cur != NULL)
    {
        //beacons
        if(!xmlStrcmp(cur->name, (const xmlChar *)"beacons"))
        {
            xmlNodePtr beaconNode = cur->xmlChildrenNode;
            while(beaconNode != NULL)
            {
                if(!xmlStrcmp(beaconNode->name, (const xmlChar *)"beacon"))
                {
                    xmlChar *beaconChar = xmlNodeGetContent(beaconNode);    //   
                    MobileAdResponse_TrackingEvents *creative_event = mobile_creative->add_events();
                    creative_event->set_event("IMP");
                    creative_event->set_trackurl((char *)beaconChar);
                    xmlFree(beaconChar);
                }
                beaconNode = beaconNode->next;
            }            
        }
        cur = cur->next;
    }
        
    return true;
}

bool connectorServ::INMOBI_creativeAddEvents(MobileAdResponse_Creative  *mobile_creative, string &ad_type, string &ad_content, MobileAdRequest_AdType& requestAdtype)
{
    int id = 0;
    string RetCode;
    string mediaTypeId;

     /**
         *  MediaType: 
         *          BANNER(1), INTERSTITIAL(2), NATIVE(3)
         *
         *  MediaSubType:  
         *          NORMAL_BANNER(1), TXT_BANNER(2), EXP_BANNER(3),
         *          ICON_BANNER(1), TRD_BANNER(10), INT_FULL_PAGE(5)
         */
         
    
    if(!strcmp(ad_type.c_str(), "text"))
    {
        switch(requestAdtype)
        {
            case MobileAdRequest_AdType_BANNER:
                //id = 66;
                mediaTypeId = "1";
                break;
            case MobileAdRequest_AdType_INTERSTITIAL:
                break;
            default:
                break;
        }
        if(id == 0)
            return false;
    }
    else if(!strcmp(ad_type.c_str(), "banner"))
    {
        switch(requestAdtype)
        {
            case MobileAdRequest_AdType_BANNER:
                //id = 61;
                mediaTypeId = "1";
                break;
            case MobileAdRequest_AdType_INTERSTITIAL:
                //id = 59;
                mediaTypeId = "2";
                break;
            default:
                break;
        }
        
        id = 79;            //MY_THIRD_HTML
        if(id == 0)
            return false;
                        
    }
    else if(!strcmp(ad_type.c_str(), "rm"))   //"rm"
    {
        switch(requestAdtype)
        {
            case MobileAdRequest_AdType_BANNER:
                id = 101;    //MY_THIRD_HTML
                mediaTypeId = "1";
                break;
            case MobileAdRequest_AdType_INTERSTITIAL:
                id = 97;    //MY_THIRD_HTML
                mediaTypeId = "2";
                break;
            default:
                break;
        }
        if(id == 0)
            return false;
    }

    map<int,string>::iterator it = Creative_template.find(id);
    if(it == Creative_template.end())
    {
        g_workerINMOBI_logger->error("INMOBI Creative_template find error ");
        return false;
    }
    RetCode = it->second;                
    if(RetCode.empty() == false)
    {
        string decodeStr;    
        
        decodeStr = UrlDecode(RetCode);      
        
        string sReplaceStr;
        string third_html;
        
        third_html = ad_content;                 

        replace(third_html,"<![CDATA[","");
        replace(third_html,"]]>","");
        
        replace(third_html,"\"","\\\"");
        replace(third_html,"'","\\'");
        replace(third_html,"\t"," ");
        replace(third_html,"\n","");
        replace(third_html,"\r","");
        replace(third_html,"/","\\/");
        
        sReplaceStr.append("\"").append(third_html).append("\"");
        //cout << sReplaceStr << endl;
                    
        replace(decodeStr,"${MY_THIRD_HTML}",sReplaceStr);
        
        mobile_creative->set_admarkup(UrlEncode(decodeStr));
        mobile_creative->set_mediatypeid(mediaTypeId);
        mobile_creative->set_mediasubtypeid("5");
        
    }

    return true;
}

void connectorServ::displayCommonMsgResponse(shared_ptr<spdlog::logger> &logger,char *data,int dataLen)
{
    CommonMessage response_commMsg;
    if(!response_commMsg.ParseFromArray(data,dataLen))
    {
        logger->error("display CommonMessage.proto Parse Fail,check required fields");
        return ;
    }
    
    MobileAdResponse mobile_response;
    const string& commMsg_data = response_commMsg.data();        
    if(!mobile_response.ParseFromString(commMsg_data))
    {
        logger->error("display MobileAdResponse.proto Parse Fail,check required fields");
        return ;
    }

    logger->debug("**********************display commonMsgResponseProtobuf**************");
    logger->debug("businessCode : {0}",response_commMsg.businesscode());
    logger->debug("dataCodingType : {0}",response_commMsg.datacodingtype());
    logger->debug("ttl : {0}",response_commMsg.ttl());

    logger->debug("MobileAdResponse:{");
    logger->debug("   id : {0}",mobile_response.id());
    
    MobileAdResponse_Bidder bidder = mobile_response.bidder();
    logger->debug("   Bidder {");
    logger->debug("       bidderId : {0}",bidder.bidderid());
    logger->debug("   }");
    
    logger->debug("   bidId : {0}",mobile_response.bidid());

    

    int bidcontentCnt = mobile_response.bidcontent_size();
    for(int n=0; n<bidcontentCnt; n++)
    {
        logger->debug("   mobileBid {");
        MobileAdResponse_mobileBid bidContent = mobile_response.bidcontent(n);
        logger->debug("       campaignId : {0}",bidContent.campaignid());
        logger->debug("       biddingType : {0}",bidContent.biddingtype());
        logger->debug("       biddingvalue : {0}",bidContent.biddingvalue());
        logger->debug("       currency : {0}",bidContent.currency());
        logger->debug("       expectCpm : {0}",bidContent.expectcpm());

        int creativeCnt = bidContent.creative_size();
        for(int i=0; i<creativeCnt; i++)
        {
            logger->debug("       Creative : [");
            MobileAdResponse_Creative creative = bidContent.creative(i);
            logger->debug("           creativeid : {0}",creative.creativeid());
            logger->debug("           admarkup : {0}",creative.admarkup());
            logger->debug("           width : {0}",creative.width());
            logger->debug("           height : {0}",creative.height());
            logger->debug("           macro : {0}",creative.macro());
            logger->debug("           mediaTypeId : {0}",creative.mediatypeid());
            logger->debug("           mediasubtypeid : {0}",creative.mediasubtypeid());
            logger->debug("           CTR : {0}",creative.ctr());
            logger->debug("           adChannelType: {0}",creative.adchanneltype());

            int eventCnt = creative.events_size();
            for(int j=0; j<eventCnt; j++)
            {
                logger->debug("           TrackingEvents : [");
                MobileAdResponse_TrackingEvents event = creative.events(j);
                logger->debug("               event : {0}",event.event());
                logger->debug("               trackUrl : {0}",event.trackurl());
                logger->debug("           ]");// TrackingEvents end
            }

            MobileAdResponse_CreativeSession session = creative.session();
            logger->debug("           CreativeSession {");
            logger->debug("               sessionLimit : {0}",session.sessionlimit());
            logger->debug("           }");//CreativeSession endl

            MobileAdResponse_UUID uuid = creative.uuid();
            int uuidtypeCnt = uuid.uuidtype_size();
            for(int z=0; z<uuidtypeCnt; z++)
            {
                MobileAdResponse_UuidType uuidtpye = uuid.uuidtype(z);
                switch(uuidtpye)
                {
                    case MobileAdResponse_UuidType_FRE:
                        logger->debug("           uuidType : MobileAdResponse_UuidType_FRE");
                        break;
                    case MobileAdResponse_UuidType_SESSION:
                        logger->debug("           uuidType : MobileAdResponse_UuidType_SESSION");
                        break;
                    default:
                        break;
                }            
            }
            logger->debug("       ]"); // creative end
        }
        
        MobileAdResponse_Action action = bidContent.action();
        logger->debug("       action :{");
        logger->debug("           name : {0}",action.name());
        logger->debug("           inApp : {0}",action.inapp());
        logger->debug("           content : {0}",action.content());
        logger->debug("           actionType : {0}",action.actiontype());
        logger->debug("       }");

        MobileAdResponse_UUID uuid = bidContent.uuid();
        int uuidtypeCnt = uuid.uuidtype_size();
        for(int z=0; z<uuidtypeCnt; z++)
        {
            MobileAdResponse_UuidType uuidtpye = uuid.uuidtype(z);
            switch(uuidtpye)
            {
                case MobileAdResponse_UuidType_FRE:
                    logger->debug("       uuidType : MobileAdResponse_UuidType_FRE");
                    break;
                case MobileAdResponse_UuidType_SESSION:
                    logger->debug("       uuidType : MobileAdResponse_UuidType_SESSION");
                    break;
                default:
                    break;
            }            
        }         
        logger->debug("   }"); //mobileBid end
    }
    logger->debug("}"); //     MobileAdResponse end
    
    logger->debug("**********************display end************************************");

}
void connectorServ::displayGYinBidRequest(const char *data,int dataLen)
{
    BidRequest bidrequest;    
    if(!bidrequest.ParseFromArray(data,dataLen))
    {
        g_workerGYIN_logger->error("display GYIN BidRequest.proto Parse Fail,check required fields ");
        return ;
    }
    g_workerGYIN_logger->debug("BidRequest {");
    g_workerGYIN_logger->debug("    id: {0}",bidrequest.id());

    Imp imp;
    g_workerGYIN_logger->debug("    Imp {");
    for(int i=0; i<bidrequest.imp_size(); i++)
    {
        imp = bidrequest.imp(i);
        g_workerGYIN_logger->debug("        id: {0}",imp.id());
        Banner banner = imp.banner();
        g_workerGYIN_logger->debug("        Banner {");
        g_workerGYIN_logger->debug("            id: {0}",banner.id());
        g_workerGYIN_logger->debug("            w: {0:d}",banner.w());
        g_workerGYIN_logger->debug("            h: {0:d}",banner.h());
        for(int j=0; j<banner.btype_size(); j++)
        {
            if(banner.btype(j) == IFRAME)
                g_workerGYIN_logger->debug("            btype{0:d}: IFRAME ",j);
            else if(banner.btype(j) == JS)
                g_workerGYIN_logger->debug("            btype{0:d}: JS",j);
        }
        g_workerGYIN_logger->debug("        }");//banner end
        g_workerGYIN_logger->debug("        bidfloor: {0:f}",imp.bidfloor());
        
    }
    g_workerGYIN_logger->debug("    }");//imp end

    if(bidrequest.has_app())
    {
        App app = bidrequest.app();
        g_workerGYIN_logger->debug("    App {");
        g_workerGYIN_logger->debug("        id: {0}",app.id());
        g_workerGYIN_logger->debug("        name: {0}",app.name());
        g_workerGYIN_logger->debug("        bundle: {0}",app.bundle());
        //g_workerGYIN_logger->debug("        domain: {0}",app.domain());
        g_workerGYIN_logger->debug("        storeurl: {0}",app.storeurl());
        g_workerGYIN_logger->debug("        cat: {0}",app.cat());
        g_workerGYIN_logger->debug("        paid: {0:d}",app.paid());
        g_workerGYIN_logger->debug("        Publisher {");
        Publisher pub = app.publisher();
        g_workerGYIN_logger->debug("            id: {0}",pub.id());
        g_workerGYIN_logger->debug("            domain: {0}",pub.domain());
        g_workerGYIN_logger->debug("        }");//publisher end
        for(int i=0; i<app.keywords_size(); i++)
        {
            g_workerGYIN_logger->debug("        keywords{0:d}: {1}",i,app.keywords(i));
        }
        g_workerGYIN_logger->debug("    }");//app end
    }

    if(bidrequest.has_site())
    {
        Site webSite = bidrequest.site();
        g_workerGYIN_logger->debug("");
        g_workerGYIN_logger->debug("    Site {");
        g_workerGYIN_logger->debug("        id: {0}",webSite.id());
        g_workerGYIN_logger->debug("        name: {0}",webSite.name());
        g_workerGYIN_logger->debug("        domain: {0}",webSite.domain());
        g_workerGYIN_logger->debug("        page: {0}",webSite.page());
        g_workerGYIN_logger->debug("        ref: {0}",webSite.ref());
        g_workerGYIN_logger->debug("        mobile: {0:d}",webSite.mobile());
        g_workerGYIN_logger->debug("        Publisher {");
        Publisher pub = webSite.publisher();
        g_workerGYIN_logger->debug("            id: {0}",pub.id());
        g_workerGYIN_logger->debug("            domain: {0}",pub.domain());
        g_workerGYIN_logger->debug("        }");//publisher end
        for(int i=0; i<webSite.cat_size(); i++)
        {
            g_workerGYIN_logger->debug("        cat{0:d}: {1}",i,webSite.cat(i));
        }
        for(int i=0; i<webSite.search_size(); i++)
        {
            g_workerGYIN_logger->debug("        search{0:d}: {1}",i,webSite.search(i));
        }
        for(int i=0; i<webSite.keywords_size(); i++)
        {
            g_workerGYIN_logger->debug("        keywords{0:d}: {1}",i,webSite.keywords(i));
        }
        g_workerGYIN_logger->debug("    }");//webSite end
    }


    User user = bidrequest.user();
    g_workerGYIN_logger->debug("    User {");
    g_workerGYIN_logger->debug("        id: {0}",user.id());
    g_workerGYIN_logger->debug("    }");//user end

    Device dev = bidrequest.device();
    g_workerGYIN_logger->debug("    Device {");
    g_workerGYIN_logger->debug("        ua: {0}",dev.ua());
    Geo geo = dev.geo();
    g_workerGYIN_logger->debug("        Geo {");
    g_workerGYIN_logger->debug("            lat: {0:f}",geo.lat());
    g_workerGYIN_logger->debug("            lon: {0:f}",geo.lon());
    g_workerGYIN_logger->debug("            type: {0}",geo.type());
    g_workerGYIN_logger->debug("            country: {0}",geo.country());
    g_workerGYIN_logger->debug("            province: {0}",geo.province());
    g_workerGYIN_logger->debug("            city: {0}",geo.city());
    g_workerGYIN_logger->debug("        }");//geo end
    g_workerGYIN_logger->debug("        ip: {0}",dev.ip());
    g_workerGYIN_logger->debug("        devicetype: {0}",dev.devicetype());
    g_workerGYIN_logger->debug("        make: {0}",dev.make());
    g_workerGYIN_logger->debug("        model: {0}",dev.model());
    g_workerGYIN_logger->debug("        os: {0}",dev.os());
    g_workerGYIN_logger->debug("        osv: {0}",dev.osv());
    g_workerGYIN_logger->debug("        w: {0}",dev.w());
    g_workerGYIN_logger->debug("        h: {0}",dev.h());
    g_workerGYIN_logger->debug("        language: {0}",dev.language());
    g_workerGYIN_logger->debug("        connectiontype: {0}",dev.connectiontype());
    g_workerGYIN_logger->debug("        imei: {0}",dev.imei());
    g_workerGYIN_logger->debug("        idfa: {0}",dev.idfa());
    g_workerGYIN_logger->debug("    }");//device end

    g_workerGYIN_logger->debug("    test: {0:d}",bidrequest.test());
    g_workerGYIN_logger->debug("    tmax: {0:d}",bidrequest.tmax());
    g_workerGYIN_logger->debug("    at: {0:d}",bidrequest.at());
    
    Scenario scenario = bidrequest.scenario();
    g_workerGYIN_logger->debug("    Scenario {");
    g_workerGYIN_logger->debug("        type: {0}",scenario.type());
    g_workerGYIN_logger->debug("    }");//Scenario end
    
    g_workerGYIN_logger->debug("}");//bidrequest end
}
void connectorServ::displayGYinBidResponse(const char *data,int dataLen)
{
    BidResponse bidresponse;
    if(!bidresponse.ParseFromArray(data, dataLen))
    {
        g_workerGYIN_logger->error("display GYIN BidResponse.proto Parse Fail,check required fields");
        return ;
    }

    g_workerGYIN_logger->debug("GYin BidResponse {");
    g_workerGYIN_logger->debug("    id: {0}",bidresponse.id());
    Seatbid seatbid;
    for(int i=0; i<bidresponse.seatbid_size(); i++)
    {
        g_workerGYIN_logger->debug("    seatbid [");
        seatbid = bidresponse.seatbid(i);
        Bid bid;
        for(int j=0; j<seatbid.bid_size(); j++)
        {
            g_workerGYIN_logger->debug("        Bid [");
            bid = seatbid.bid(j);
            g_workerGYIN_logger->debug("            id: {0}",bid.id());
            g_workerGYIN_logger->debug("            impid: {0}",bid.impid());
            g_workerGYIN_logger->debug("            price: {0:f}",bid.price());
            g_workerGYIN_logger->debug("            adm: {0}",bid.adm());
            g_workerGYIN_logger->debug("            adomain: {0}",bid.adomain());
            g_workerGYIN_logger->debug("            bundle: {0}",bid.bundle());
            g_workerGYIN_logger->debug("            iurl: {0}",bid.iurl());
            g_workerGYIN_logger->debug("            w: {0:f}",bid.w());
            g_workerGYIN_logger->debug("            h: {0:f}",bid.h());
            g_workerGYIN_logger->debug("            adid: {0}",bid.adid());
            g_workerGYIN_logger->debug("            nurl: {0}",bid.nurl());
            g_workerGYIN_logger->debug("            cid: {0}",bid.cid());
            g_workerGYIN_logger->debug("            crid: {0}",bid.crid());
            g_workerGYIN_logger->debug("            cat: {0}",bid.cat());
            g_workerGYIN_logger->debug("            attr: {0}",bid.attr());
            g_workerGYIN_logger->debug("            curl: {0}",bid.curl());
            g_workerGYIN_logger->debug("            type: {0}",bid.type());
            for(int z=0; z<bid.extiurl_size(); z++)
            {
                g_workerGYIN_logger->debug("            extiurl: {0}",bid.extiurl(z));
            }
            g_workerGYIN_logger->debug("            action: {0}",bid.action());
            g_workerGYIN_logger->debug("            admtype: {0}",bid.admtype());
            g_workerGYIN_logger->debug("        ]"); //bid end
        }
        g_workerGYIN_logger->debug("        seat: {0}",seatbid.seat());
        g_workerGYIN_logger->debug("    ]"); //seatbid end
    }
    g_workerGYIN_logger->debug("    bidid: {0}",bidresponse.bidid());
    g_workerGYIN_logger->debug("    nobidreasoncodes: {0}",bidresponse.nbr());
    g_workerGYIN_logger->debug("    process_time: {0:d}",bidresponse.process_time());
    g_workerGYIN_logger->debug("}"); //bidresponse end
}

int getPartLen(char *Src,int& partLen)
{    
    char *ch = memstr(Src,strlen(Src),"\r\n");
    if(ch == NULL)
        return -1;
    int len = ch - Src;
    char partLenStr[5] = {};
    strncpy(partLenStr,Src,len);    
    partLen = strtol(partLenStr, (char**)NULL, 16);
    return len;
}
int getPartData(char *Dest,char *Src)
{
    char *ch = memstr(Src,strlen(Src),"\r\n");
    if(ch == NULL)
        return -1;
    int len = ch - Src;
    strncat(Dest, Src, len);    
    return len;
}

void connectorServ::hashGetBCinfo(string& uuid,string& bcIP,unsigned short& bcDataPort)
{
    m_bc_manager.hashGetBCinfo(uuid,bcIP,bcDataPort);
}

void connectorServ::handle_BidResponseFromDSP(dspType type,char *data,int dataLen, const CommonMessage& request_commMsg)
{
    int responseDataLen = 0;    
    string uuid;    //for hash get bc info
    char *responseDataStr = NULL;

    shared_ptr<spdlog::logger> g_logger = NULL;
    string dspName;
    bool flag_displayCommonMsgResponse = false; 
    
    switch(type)
    {
        case TELE:
            responseDataStr = convertTeleBidResponseJsonToProtobuf(data,dataLen,responseDataLen,uuid);
            g_logger = g_worker_logger;
            dspName = "TELE";
            flag_displayCommonMsgResponse = m_config.get_logTeleRsp();
            break;
        case GYIN:
            responseDataStr = convertGYinBidResponseProtoToProtobuf(data,dataLen,responseDataLen,uuid);
            g_logger = g_workerGYIN_logger;
            dspName = "GYIN";
            flag_displayCommonMsgResponse = m_config.get_logGYINRsp();
            break;
	case SMAATO:
	     responseDataStr = convertSmaatoBidResponseXMLtoProtobuf(data,dataLen,responseDataLen,uuid, request_commMsg);
	     g_logger = g_workerSMAATO_logger;
	     dspName = "SMAATO";
	     flag_displayCommonMsgResponse = m_config.get_logSmaatoRsp();
	     break;
	case INMOBI:
            responseDataStr = convertInmobiBidResponseXMLtoProtobuf(data,dataLen,responseDataLen,uuid, request_commMsg);
            g_logger = g_workerINMOBI_logger;
            dspName = "INMOBI";
            flag_displayCommonMsgResponse = m_config.get_logInMobiRsp();
            break;
        default:
            break;
    }    
    
    if(responseDataStr && (responseDataLen > 0))
    {
        if(flag_displayCommonMsgResponse)
            displayCommonMsgResponse(g_logger, responseDataStr, responseDataLen);  

        
        #if 0
        switch(type)
        {
            case TELE:      
                if(m_config.get_logTeleRsp())
                {
                    displayCommonMsgResponse(g_worker_logger,responseDataStr,responseDataLen);   
                }                
                break;
            case GYIN:         
                if(m_config.get_logGYINRsp())
                {
                    displayCommonMsgResponse(g_workerGYIN_logger,responseDataStr,responseDataLen); 
                }                  
                break;
            default:
                break; 
        }
        #endif
        
        string bcIP;
        unsigned short bcDataPort;
        hashGetBCinfo(uuid,bcIP,bcDataPort);   
        
        int ssize = m_bc_manager.sendAdRspToBC(bcIP, bcDataPort, responseDataStr, responseDataLen, ZMQ_NOBLOCK);
        if(ssize > 0)
        {
            g_logger->debug("{0} send AdResponse to BC success: {1} \r\n", dspName, uuid);
            #if 0
            switch(type)
            {
                 case TELE: 
                    g_worker_logger->debug("TELE send AdResponse to BC success: {0} \r\n", uuid);
                    break;
                 case GYIN:
                    g_workerGYIN_logger->debug("GYIN send AdResponse to BC success: {0} \r\n", uuid);
                    break;
                 default:
                    break;
            }            
            #endif
        }
        else
        {
            g_logger->debug("{0} send AdResponse to BC failed: {1} \r\n", dspName, uuid);
            #if 0
            switch(type)
            {
                 case TELE: 
                    g_worker_logger->debug("TELE send AdResponse to BC fail: {0} \r\n", uuid);
                    break;
                 case GYIN:
                    g_workerGYIN_logger->debug("GYIN send AdResponse to BC fail: {0} \r\n", uuid);
                    break;
                 default:
                    break;
            }
            #endif
        }
        
        delete [] responseDataStr;
    }
    else
    {
        g_logger->debug("no valid BidResponse generate \r\n");
    }    
}
bool connectorServ::getStringFromSQLMAP(vector<string>& str_buf,const MobileAdRequest_Device& request_dev,const MobileAdRequest_GeoInfo& request_geo)
{    
    vector<int> sql_id;
    if(request_geo.has_carrier())
        sql_id.push_back(atoi(request_geo.carrier().c_str()));
    else 
        sql_id.push_back(0);
    if(request_dev.has_language())
        sql_id.push_back(atoi(request_dev.language().c_str()));
    else 
        sql_id.push_back(0);
    if(request_dev.has_vender())
        sql_id.push_back(atoi(request_dev.vender().c_str()));
    else
        sql_id.push_back(0);
    if(request_dev.has_modelname())
        sql_id.push_back(atoi(request_dev.modelname().c_str()));
    else
        sql_id.push_back(0);
    if(request_dev.has_platform())
        sql_id.push_back(atoi(request_dev.platform().c_str()));
    else
        sql_id.push_back(0);
    if(request_dev.has_platformversion())
        sql_id.push_back(atoi(request_dev.platformversion().c_str()));
    else
        sql_id.push_back(0);
    if(request_geo.has_country())
        sql_id.push_back(atoi(request_geo.country().c_str()));
    else
        sql_id.push_back(0);
    if(request_geo.has_region())
        sql_id.push_back(atoi(request_geo.region().c_str()));
    else
        sql_id.push_back(0);
    if(request_geo.has_city())
        sql_id.push_back(atoi(request_geo.city().c_str()));
    else 
        sql_id.push_back(0);
    
    for(int i=0; i<SQL_MAP.size(); i++)
    {
        if(sql_id[i] == 0)
            str_buf.push_back("");
        else
        {
            map<int,string>::iterator it = SQL_MAP[i].find(sql_id[i]);
            if(it == SQL_MAP[i].end())
            {
                return false;
            }
            else
                str_buf.push_back(it->second);
        }        
    }
    return true;    
}

void connectorServ::Tele_AdReqJsonAddApp(Json::Value &app, const MobileAdRequest& mobile_request)
{
    const MobileAdRequest_Aid& request_app = mobile_request.aid();   
    
    Json::Value publisher;
    Json::Value catArray;   
    Json::Value sectioncatArray;
    
    app["id"]       = request_app.id();
    app["name"]         = request_app.appname();
    app["bundle"]       = request_app.apppackagename();
    
    catArray.append(request_app.appcategory());
    app["cat"]          = catArray;

    sectioncatArray.append(mobile_request.section());
    app["sectioncat"]   = sectioncatArray;
    
    app["keywords"]     = request_app.appkeywords(0);
    
    publisher["id"]     = request_app.publisherid();
    app["publisher"]    = publisher;    

    app["storeurl"]     = request_app.appstoreurl(0);
    
}
bool connectorServ::Tele_AdReqJsonAddDevice(Json::Value &device,const MobileAdRequest& mobile_request)
{
    const MobileAdRequest_Device& request_dev = mobile_request.device();     
    const MobileAdRequest_GeoInfo& request_geo = mobile_request.geoinfo();        
    
    vector<string> str_buf;
    if(getStringFromSQLMAP(str_buf,request_dev,request_geo))
    {
        g_worker_logger->trace("get string from SQLMAP success!");
    }
    else
    {
        g_worker_logger->error("get string from SQLMAP fail,AdRequest abort!!!!!");
        return false;
    }
        
    Json::Value dev_geo;
    Json::Value ext;

    int size = mobile_request.paramter_size();
    int i=0;
    for(; i<size; i++)
    {
        MobileAdRequest_Paramter paramter = mobile_request.paramter(i);
        if(strcmp(paramter.key().c_str(),"imei") == 0)
        {
            ext["imei"]     = paramter.value();
            break;
        }            
    }
    if(i == size)
        ext["imei"]     = "";      
    
    device["ua"]    = request_dev.ua();
    device["ip"]    = mobile_request.dnsip();
    device["uuid"]  = request_dev.udid();
    device["carrier"]   = str_buf.at(0);
    device["language"]  = str_buf.at(1);
    device["make"]      = str_buf.at(2);
    device["model"]     = str_buf.at(3);
    device["os"]        = str_buf.at(4);
    device["osv"]       = str_buf.at(5);
    device["connectiontype"] = atoi(request_dev.connectiontype().c_str());
    device["devicetype"]        = request_dev.devicetype();
    dev_geo["lat"]  = atof(request_geo.latitude().c_str());
    dev_geo["lon"]  = atof(request_geo.longitude().c_str());
    dev_geo["country"] = str_buf.at(6);
    dev_geo["region"]  = str_buf.at(7);
    dev_geo["city"]    = str_buf.at(8);        
    dev_geo["type"]    = atoi(request_geo.usagetype().c_str());
    device["geo"]      = dev_geo;     

    device["dpidmd5"]  = request_dev.hidmd5();
    device["dpidsha1"] = request_dev.hidsha1();
    device["h"]        = atoi(request_dev.screenheight().c_str());
    device["w"]        = atoi(request_dev.screenwidth().c_str());
    device["ppi"]      = atoi(request_dev.density().c_str());
    return true;
    
}
void connectorServ::Tele_AdReqJsonAddImp(Json::Value &impArray,const MobileAdRequest &mobile_request)
{
    Json::Value imp;
    Json::Value banner;    
    Json::Value apiArray; 
    Json::Value mimesArray;

    string adWidth = mobile_request.adspacewidth();
    string adHeith = mobile_request.adspaceheight();
    
    imp["id"]      = "1";
    banner["w"] = atoi(adWidth.c_str());
    banner["h"] = atoi(adHeith.c_str());        
    apiArray.append(5);
    banner["api"]  = apiArray;
    mimesArray.append("image/jpg");
    mimesArray.append("image/gif");
    mimesArray.append("image/png");
    banner["mimes"] = mimesArray;
    banner["pos"]   = 0;
    imp["banner"]  = banner;   
    imp["bidfloor"] = 5.0;
    imp["bidfloorcur"] = "CNY";
    if(mobile_request.type() == MobileAdRequest_AdType_INTERSTITIAL)
        imp["instl"] = 1;
    else
        imp["instl"] = 0;
    imp["secure"] = 1;
    impArray.append(imp);
}
bool connectorServ::convertProtoToTeleJson(string &reqTeleJsonData,const MobileAdRequest& mobile_request)
{
    string id = mobile_request.id();                           
        
    Json::Value root;            
    Json::Value app;        
    Json::Value device;
    Json::Value impArray;     
    Json::Value curArray;
    if(!Tele_AdReqJsonAddDevice(device,mobile_request))
        return false;
    Tele_AdReqJsonAddApp(app,mobile_request);
    Tele_AdReqJsonAddImp(impArray,mobile_request);

    curArray.append("CNY");
    curArray.append("EUR");
    curArray.append("HKD");
    curArray.append("CHF");
    curArray.append("USD");
    curArray.append("CAD");
    curArray.append("GBP");
    curArray.append("JPY");
    curArray.append("AUD");
    curArray.append("TWD");

    root["at"]      = 1;
    root["id"]      = id;
    root["imp"]     = impArray;
    root["app"]     = app;
    root["device"]  = device;
    root["allimps"] = 1;
    root["tmax"]    = 80;
    root["cur"]     = curArray;

    root.toStyledString();
    reqTeleJsonData = root.toStyledString();
    g_worker_logger->trace("MobileAdRequest.proto->TeleBidRequest.json success");
    
    ofstream outfile;
    outfile.open("TeleBidRequest.json",ios::out | ios::binary);        
    outfile.write(reqTeleJsonData.c_str(),strlen(reqTeleJsonData.c_str()));
    outfile.close();
    
    if(m_dspManager.isChinaTelecomObjectCeritifyCodeEmpty())
    {
        if(!m_dspManager.getCeritifyCodeFromChinaTelecomDSP())
        {
            g_worker_logger->error("GET DSP PASSWORD FAIL ... ");   
            return false;
        }
    }
    return true;
    
}
bool connectorServ::GYin_AdReqProtoMutableApp(App *app,const MobileAdRequest& mobile_request)
{
    MobileAdRequest_Aid aid = mobile_request.aid();
    app->set_id(aid.id());
    //g_workerGYIN_logger->debug("aid.id : {0}",aid.id());
    app->set_name(aid.appname());
    app->set_bundle(aid.apppackagename());
    app->set_storeurl(aid.appstoreurl(0));
    app->set_paid(0);
    
    MobileAdRequest_Device dev = mobile_request.device();
    string category = aid.appcategory();
    enum ContentCategory targetCat;
    if(strcmp("1",dev.platform().c_str()) == 0)         //andriod
    {   
        targetCat = GYin_AndroidgetTargetCat(category);
        #if 0
        map<string,ContentCategory>::iterator it = AndroidContenCategoryMap.find(category);
        if(it == AndroidContenCategoryMap.end())
        {
            g_worker_logger->debug("CONVERT GYIN FAIL : novlid android category");
            return false;
        }
        app->set_cat(it->second);
        #endif
    }
    else if(strcmp("2",dev.platform().c_str()) == 0)    //IOS
    {
        targetCat = GYin_IOSgetTargetCat(category);
        #if 0
        map<string,ContentCategory>::iterator it = IOSContenCategoryMap.find(category);
        if(it == IOSContenCategoryMap.end())
        {
            g_worker_logger->debug("CONVERT GYIN FAIL : novlid IOS category");
            return false;
        }
        app->set_cat(it->second);
        #endif
    }
    else                                                                //no platform found in request,it's must,if not process will exit
    {
        targetCat = CAT_805;
    }
    app->set_cat(targetCat);
    
    Publisher *publisher;
    string pubID = m_dspManager.getGuangYinObject()->getPublisherID();
    publisher = app->mutable_publisher();
    publisher->set_id(pubID);
    publisher->set_domain("www.reachjunction.com");
    
    for(int i=0; i<aid.appkeywords_size(); i++)
    {
        app->add_keywords(aid.appkeywords(i));
    }
    return true;
}

bool connectorServ::GYin_AdReqProtoMutableWebsite(Site *webSite, const MobileAdRequest& mobile_request)
{
    MobileAdRequest_Wid wid = mobile_request.wid();
    webSite->set_id(wid.id());
    webSite->set_name(wid.sitename());
    webSite->set_domain(wid.domain());
    if(mobile_request.has_page()&&(mobile_request.page().empty() == false))
        webSite->set_page(mobile_request.page());
    else
    {
        g_workerGYIN_logger->debug("CONVERT GYIN FAIL: website novlid page");
        return false;
    }

    MobileAdRequest_Device dev = mobile_request.device();
    string category = wid.category();
    enum ContentCategory targetCat;
    if(strcmp("1",dev.platform().c_str()) == 0)         //andriod
    {   
        targetCat = GYin_AndroidgetTargetCat(category);
        #if 0
        map<string,ContentCategory>::iterator it = AndroidContenCategoryMap.find(category);
        if(it == AndroidContenCategoryMap.end())
        {
            g_worker_logger->debug("CONVERT GYIN FAIL : novlid android category");
            return false;
        }
        app->set_cat(it->second);
        #endif
    }
    else if(strcmp("2",dev.platform().c_str()) == 0)    //IOS
    {
        targetCat = GYin_IOSgetTargetCat(category);
        #if 0
        map<string,ContentCategory>::iterator it = IOSContenCategoryMap.find(category);
        if(it == IOSContenCategoryMap.end())
        {
            g_worker_logger->debug("CONVERT GYIN FAIL : novlid IOS category");
            return false;
        }
        app->set_cat(it->second);
        #endif
    }
    else                                                                //no platform found in request,it's must,if not process will exit
    {
        targetCat = CAT_805;
    }
    webSite->add_cat(targetCat);

    webSite->set_mobile(0);
    Publisher *publisher;
    string pubID = m_dspManager.getGuangYinObject()->getPublisherID();
    publisher = webSite->mutable_publisher();
    publisher->set_id(pubID);
    publisher->set_domain("www.reachjunction.com");

    for(int i=0; i<wid.keywords_size(); i++)
    {
        webSite->add_keywords(wid.keywords(i));
    }
    return true;
}

bool connectorServ::GYin_AdReqProtoMutableDev(Device *device,const MobileAdRequest& mobile_request)
{
    const MobileAdRequest_Device& request_dev = mobile_request.device();     
    const MobileAdRequest_GeoInfo& request_geo = mobile_request.geoinfo();        
    
    vector<string> str_buf;
    if(getStringFromSQLMAP(str_buf,request_dev,request_geo))
    {
        g_workerGYIN_logger->trace("get string from SQLMAP success!");
    }
    else
    {
        g_workerGYIN_logger->error("get string from SQLMAP fail,AdRequest abort!!!!!");
        return false;
    }           
    
    int size = mobile_request.paramter_size();
    int i=0;
    for(; i<size; i++)
    {
        MobileAdRequest_Paramter paramter = mobile_request.paramter(i);
        if(strcmp(paramter.key().c_str(),"imei") == 0)
        {
            device->set_imei(paramter.value());
            break;
        }            
    }
    if(i == size)
        device->set_imei("");   
   
    
    int connectionTypeID = atoi(request_dev.connectiontype().c_str());
    ConnectionType conType = GYin_getConnectionType(connectionTypeID);
    device->set_connectiontype(conType);
    #if 0
    map<int,ConnectionType>::iterator it = ConnectionTypeMap.find(connectionTypeID);
    if(it == ConnectionTypeMap.end())
    {
        g_worker_logger->debug("CONVERT GYIN FAIL : novlid connectionType");
        return false;
    }
    device->set_connectiontype(it->second);    
    #endif

    device->set_ua(request_dev.ua());
    Geo *geo;
    MobileAdRequest_GeoInfo geoinfo = mobile_request.geoinfo();
    geo = device->mutable_geo();
    
    if(geoinfo.latitude().empty() == false)
        geo->set_lat(atof(geoinfo.latitude().c_str()));    
    if(geoinfo.longitude().empty() == false)
        geo->set_lon(atof(geoinfo.longitude().c_str()));
    
    geo->set_country(str_buf.at(6));
    geo->set_province(str_buf.at(7));
    geo->set_city(str_buf.at(8));
    geo->set_type(IP_ADDRESS);

    device->set_ip(mobile_request.dnsip());
    device->set_devicetype(MOBILE);
    device->set_language(str_buf.at(1));
    device->set_make(str_buf.at(2));
    device->set_model(str_buf.at(3));
    device->set_os(str_buf.at(4));
    device->set_osv(str_buf.at(5));
    device->set_w(atoi(request_dev.screenwidth().c_str()));
    device->set_h(atoi(request_dev.screenheight().c_str()));
    if(request_dev.has_udid() && (request_dev.udid().empty() == false))
        device->set_idfa(request_dev.udid());

    return true;
    
}

bool connectorServ::convertProtoToGYinProto(BidRequest& bidRequest,const MobileAdRequest& mobile_request)
{
    MobileAdRequest_Device dev = mobile_request.device();
    
    //BidRequest bidRequest;
    bidRequest.set_id(mobile_request.id());
    bidRequest.set_tmax(100);    //100ms
    bidRequest.set_at(1);
    
    bool test = m_dspManager.getGuangYinObject()->getTestValue();
    if(test)
        bidRequest.set_test(1);
    else
        bidRequest.set_test(0);

    //Imp
    Imp *imp;
    imp = bidRequest.add_imp();
    imp->set_id("1");
    imp->set_bidfloor(5.0); 
    
    Banner *banner = imp->mutable_banner();
    banner->set_id("1");

    //banner->set_w(atoi(mobile_request.adspacewidth().c_str()));
    //banner->set_h(atoi(mobile_request.adspaceheight().c_str()));
    
    MobileAdRequest_AdType adtype = mobile_request.type();
    switch(adtype)
    {
        case MobileAdRequest_AdType_BANNER:
        {
            banner->set_w(320);
            banner->set_h(50);
        }
        break;
        case MobileAdRequest_AdType_INTERSTITIAL:
        {
            banner->set_w(360);
            banner->set_h(300);
        }
        break;
        default:
        {
            g_workerGYIN_logger->error("GYIN NO SUPPORT ADTYPE");
            return false;
        }
        break;    
    }    
    
    //btype : not support adtype
    banner->add_btype(IFRAME); 
    banner->add_btype(JS);

    string appType = mobile_request.apptype();
    if(!strcmp(appType.c_str(), "app"))
    {
        //App
        App *app;    
        app = bidRequest.mutable_app();
        if(!GYin_AdReqProtoMutableApp(app, mobile_request))
            return false;
    }
    else if((!strcmp(appType.c_str(), "pcweb"))||(!strcmp(appType.c_str(), "mweb")))
    {
        //web site
        if(!strcmp(appType.c_str(), "mweb"))
            g_workerGYIN_logger->trace("mweb request uuid: {0}", mobile_request.id());
        else if(!strcmp(appType.c_str(), "pcweb"))
            g_workerGYIN_logger->trace("pcweb request uuid: {0}", mobile_request.id());
        Site *webSite;
        webSite = bidRequest.mutable_site();
        if(!GYin_AdReqProtoMutableWebsite(webSite, mobile_request))
            return false;
    }
    else
    {
        g_workerGYIN_logger->debug("CONVERT GYIN FAIL: novlid appType: {0}", appType);
        return false;
    }

    //User
    User *user;
    user = bidRequest.mutable_user();
    if((dev.has_udid())&&(dev.udid().empty() == false))
    {
        user->set_id(dev.udid());
    }
    else 
    {
        string tempid = dev.hidmd5() + "-" + dev.hidsha1();
        user->set_id(tempid);
    }   
    
    //Device
    Device *device;
    device = bidRequest.mutable_device();
    if(!GYin_AdReqProtoMutableDev(device,mobile_request)) 
        return false;

    //Scenario
    Scenario *scenario;
    scenario = bidRequest.mutable_scenario();
    scenario->set_type(APP);

    int length = bidRequest.ByteSize();    
    g_workerGYIN_logger->trace("MobileAdRequest.proto->GYinBidRequest.protobuf success , Length : {0:d}",length);

    return true;    
}

bool connectorServ::convertProtoToHttpGETArg(char *buf, const MobileAdRequest& mobile_request)
{
    char *strbuf = buf;
    if((!mobile_request.has_device()) || (!mobile_request.has_geoinfo()) ||(!mobile_request.has_aid()))
    {
        g_workerSMAATO_logger->error("GEN HTTP ARG error: invalid device |geoinfo |aid ");
        return false;
    }
    
    MobileAdRequest_Device dev = mobile_request.device();    
    MobileAdRequest_GeoInfo geo = mobile_request.geoinfo();    
    MobileAdRequest_Aid aid = mobile_request.aid();
    
    vector<string> query_buf;
    if(getStringFromSQLMAP(query_buf,dev,geo))
    {
        g_workerSMAATO_logger->trace("get string from SQLMAP success!");
    }
    else
    {
        g_workerSMAATO_logger->error("get string from SQLMAP fail,AdRequest abort!!!!!");
        return false;
    }         
    
    string apiver = "apiver=501";
    strcat(strbuf, apiver.c_str());
    strcat(strbuf, "&");

    string& adspaceid = m_dspManager.getSmaatoObject()->getAdSpaceId();
    //string adspace = "adspace=130061846";
    string adspace = "adspace=" + adspaceid;
    strcat(strbuf, adspace.c_str());
    strcat(strbuf, "&");

    string& pubId = m_dspManager.getSmaatoObject()->getPublisherId();
    //string pub = "pub=1100014094";
    string pub="pub=" + pubId;
    strcat(strbuf, pub.c_str());
    strcat(strbuf, "&");   

    string beacon = "beacon=true";
    strcat(strbuf, beacon.c_str());
    strcat(strbuf, "&");

    if(mobile_request.has_dnsip())
    {
        string devip = "devip=" + mobile_request.dnsip();
        strcat(strbuf, devip.c_str());
        strcat(strbuf, "&");
    }
    else
    {        
        g_workerSMAATO_logger->error("GEN HTTP ARG error: invalid devip");
        return false;
    }
    
    if(dev.has_ua())
    {
        string device = "device=" + UrlEncode(dev.ua());
        strcat(strbuf, device.c_str());
        strcat(strbuf, "&");
    }
    else 
    {
        g_workerSMAATO_logger->error("GEN HTTP ARG error: invalid device");
        return false;
    }
    

    string format = "format=all";
    strcat(strbuf, format.c_str());
    strcat(strbuf, "&");

    string googleadid = "googleadid=";
    if((dev.has_udid())&&(dev.udid().empty() == false))
    {
        googleadid += dev.udid();
    }
    else if(dev.has_hidmd5() && dev.has_hidsha1())
    {
        string tempid = dev.hidmd5() + "-" + dev.hidsha1();
        googleadid += tempid;
    }   
    else
    {
        g_workerSMAATO_logger->error("GEN HTTP ARG error: invalid googleadid");
        return false;
    }
    strcat(strbuf, googleadid.c_str());
    strcat(strbuf, "&");

    string googlednt = "googlednt=true";
    strcat(strbuf, googlednt.c_str());    
    strcat(strbuf, "&");

    string response = "response=XML";
    strcat(strbuf, response.c_str());
    strcat(strbuf, "&");

    string coppa = "coppa=0";
    strcat(strbuf, coppa.c_str());
    strcat(strbuf, "&");

    if(mobile_request.has_adspaceheight())
    {
        string height = "height=" + mobile_request.adspaceheight();
        strcat(strbuf, height.c_str());
        strcat(strbuf, "&");
    }
    else
    {
        g_workerSMAATO_logger->debug("GEN HTTP ARG debug: invalid height");
    }
    
    if(mobile_request.has_adspacewidth())
    {
        string width = "width=" + mobile_request.adspacewidth();
        strcat(strbuf, width.c_str());
        strcat(strbuf, "&");
    }
    else
    {
        g_workerSMAATO_logger->debug("GEN HTTP ARG debug: invalid width");
    }
    
    if(dev.has_hidmd5())
    {
        g_workerSMAATO_logger->debug("MD5: {0}", dev.hidmd5());
        string session = "session=" + dev.hidmd5();
        strcat(strbuf, session.c_str());
        strcat(strbuf, "&");
    }
    else
    {
        g_workerSMAATO_logger->debug("GEN HTTP ARG debug: invalid session");
    }
    
    string bundle = "bundle=reach.junction.funnystory";
    strcat(strbuf, bundle.c_str());
    strcat(strbuf, "&");
    
    
    if(aid.has_appname()&&(!query_buf.at(4).empty())&& mobile_request.has_adspacewidth()&&mobile_request.has_adspaceheight())
    {
        string appname = aid.appname();
        string OSname = query_buf.at(4);
        string AdspaceDimension = mobile_request.adspacewidth() + "x" + mobile_request.adspaceheight();    

        string adspacename = "adspacename=" + appname + "_" + OSname + "_" + AdspaceDimension;
        strcat(strbuf, adspacename.c_str());
        strcat(strbuf, "&");      
    }
    else
    {        
        g_workerSMAATO_logger->debug("GEN HTTP ARG debug: invalid adspacename");
    } 

    string kws = "kws=news";
    strcat(strbuf, kws.c_str());
    strcat(strbuf, "&");

    string age = "age=25";
    strcat(strbuf, age.c_str());    
    strcat(strbuf, "&");

    string gender = "gender=m";
    strcat(strbuf, gender.c_str());    
    strcat(strbuf, "&");

    if(geo.has_latitude()&&geo.has_longitude()&&(!geo.latitude().empty())&&(!geo.longitude().empty()))
    {
        string gps = "gps=" + geo.latitude() + "," + geo.longitude();
        strcat(strbuf, gps.c_str());    
        strcat(strbuf, "&");
    }
    else
    {
        g_workerSMAATO_logger->debug("GEN HTTP ARG debug: invalid gps");
    }
    
    if(!query_buf.at(7).empty())
    {
        string region = "region=" + query_buf.at(7);
        strcat(strbuf, region.c_str());    
        strcat(strbuf, "&");
    }
    else
    {
        g_workerSMAATO_logger->debug("GEN HTTP ARG debug: invalid region");
    }
    
    if(!query_buf.at(3).empty())
    {
        string devicemodel = "devicemodel=" + query_buf.at(3);
        strcat(strbuf, devicemodel.c_str());    
        strcat(strbuf, "&");
    }
    else
    {        
        g_workerSMAATO_logger->debug("GEN HTTP ARG debug: invalid devicemodel");
    }
    
    if(!query_buf.at(2).empty())
    {
        string devicemake = "devicemake=" + query_buf.at(2);
        strcat(strbuf, devicemake.c_str());    
    }
    else
    {        
        g_workerSMAATO_logger->debug("GEN HTTP ARG debug: invalid devicemake");
    }

    int len = strlen(strbuf);
    if(strbuf[len-1] == '&')
    {
        strbuf[len-1] = '\0';
    }

    return true;
    
}
bool connectorServ::InMobi_AdReqJsonAddImp(Json::Value &impArray, const MobileAdRequest & mobile_request)
{
    Json::Value imp;

    imp["ads"] = 1;

    MobileAdRequest_AdType request_adType = mobile_request.type();
    if(request_adType != MobileAdRequest_AdType_INTERSTITIAL)
        return false;
    imp["adtype"] = "int";                   //Set value as int to designate an interstitial request.
    imp["displaymanager"] = "s_mediatorname";
    imp["displaymanagerver"] = "1.2.0";

    Json::Value banner;
    banner["adsize"] = 14;                  //Interstitial 320*480
    banner["pos"] = "top";
    
    imp["banner"] = banner;
    impArray.append(imp);

    return true;
}
bool connectorServ::InMobi_AdReqJsonAddSite(Json::Value &site, const MobileAdRequest & mobile_request)
{
    site["id"] = m_dspManager.getInMobiObject()->getSiteId();
    //site["id"] = "713b1884a3a54eec9aaf81646c5433ab";
    return true;
}
bool connectorServ::InMobi_AdReqJsonAddDevice(Json::Value &device, const MobileAdRequest & mobile_request)
{
    MobileAdRequest_Device dev = mobile_request.device();
    if(dev.has_udid() && (!dev.udid().empty()))
    {
        device["gpid"] = dev.udid();
    }

    device["o1"] = dev.hidsha1();             //SHA1 of Android_ID
    device["um5"] = dev.hidmd5();           //MD5 of Android_ID
    //device["sid"] = mobile_request.session();
    device["ip"] = mobile_request.dnsip();
    device["ua"] = dev.ua();
    device["locale"] = "zh_CN";

    switch(atoi(dev.connectiontype().c_str()))
    {
        case 0:
            device["connectiontype"] = "unknown";
            break;
        case 1:
            device["connectiontype"] = "ethernet";
            break;
        case 2:
            device["connectiontype"] = "wifi";
            break;
        case 3:
            device["connectiontype"] = "unknown";
            break;
        case 4:
            device["connectiontype"] = "2G";
            break;
        case 5:
            device["connectiontype"] = "3G";
            break;
        case 6:
            device["connectiontype"] = "4G";
            break;
        default:
            device["connectiontype"] = "unknown";
            break;
    }

    if(mobile_request.has_orientation())
    {
        MobileAdRequest_Orientation ori = mobile_request.orientation();    
        switch(ori)
        {
            case MobileAdRequest_Orientation_LANDSCAPE:
                device["orientation"] = 3;
                break;
            case MobileAdRequest_Orientation_PORTRAIT:
                device["orientation"] = 1;
                break;
            default:
                break;
        }        
    }
    else
        device["orientation"] = 1;

    MobileAdRequest_GeoInfo geoinfo = mobile_request.geoinfo();
    Json::Value geo;
    geo["lat"] = geoinfo.latitude();
    geo["lon"] = geoinfo.longitude();
    geo["accu"] = "0";                      //Accuracy of the lat, lon values. Set accuracy to 0 if unavailable.

    device["geo"] = geo;
    
    return true;
}
bool connectorServ::InMobi_AdReqJsonAddUser(Json::Value &user, const MobileAdRequest & mobile_request)
{
    MobileAdRequest_User requestUser = mobile_request.user();
    user["yob"] = "1990";
    user["gender"] = "M";

    Json::Value data;
    data["id"] = "0";
    data["name"] = "user";

    Json::Value segment;
    segment["name"] = "income";
    segment["value"] = "10000";
    
    Json::Value segmentArray;
    segmentArray.append(segment);
    data["segment"] = segmentArray;

    Json::Value dataArray;
    dataArray.append(data);

    user["data"] = dataArray;

    return true;
}
bool connectorServ::convertProtoToInMobiJson(string & reqTeleJsonData,const MobileAdRequest & mobile_request)
{
    string id = mobile_request.id();                           
        
    Json::Value root;         
    root["responseformat"] = "axml";

    Json::Value impArray;
    Json::Value site;
    Json::Value device;
    Json::Value user;

    if(!InMobi_AdReqJsonAddImp(impArray, mobile_request))       //filter request adtype: INTERSTITIAL
        return false;
    InMobi_AdReqJsonAddSite(site, mobile_request);
    InMobi_AdReqJsonAddDevice(device, mobile_request);
    InMobi_AdReqJsonAddUser(user, mobile_request);
    
    root["imp"] = impArray;
    root["site"] = site;
    root["device"] = device;
    root["user"] = user;
    
    root.toStyledString();
    reqTeleJsonData = root.toStyledString();
    g_worker_logger->trace("MobileAdRequest.proto->TeleBidRequest.json success");
    return true;
}
void connectorServ::mobile_AdRequestHandler(const char *pubKey,const CommonMessage& request_commMsg)
{
    try
    {    
        const string& commMsg_data = request_commMsg.data();
        MobileAdRequest mobile_request;
        mobile_request.ParseFromString(commMsg_data);

        string uuid = mobile_request.id();

        MobileAdRequest_Device dev = mobile_request.device();
        string ua = dev.ua();
        
        /*
                *send to CHINA TELECOM
                */                
        if(m_config.get_enChinaTelecom()) 
        {
            int TELE_maxFlowLimit = m_dspManager.getChinaTelecomObject()->getMaxFlowLimit();
            int TELE_curFlowCount = m_dspManager.getChinaTelecomObject()->getCurFlowCount();
            if((TELE_curFlowCount < TELE_maxFlowLimit)||(TELE_maxFlowLimit == 0))
            {
                    string reqTeleJsonData;
                    if(convertProtoToTeleJson(reqTeleJsonData, mobile_request))
                    {
                        //callback func: handle_recvAdResponseTele active by socket EV_READ event                
                        if(!m_dspManager.sendAdRequestToChinaTelecomDSP(reqTeleJsonData.c_str(), strlen(reqTeleJsonData.c_str()), m_config.get_logTeleReq(), ua))
                        {
                            g_worker_logger->debug("POST TO TELE fail uuid : {0}",uuid);            
                        }
                        else
                        {
                            if(TELE_maxFlowLimit != 0)
                                m_dspManager.getChinaTelecomObject()->curFlowCountIncrease();
                            g_worker_logger->debug("POST TO TELE success uuid : {0} \r\n",uuid);  
                        }
                    }
                    else
                        g_worker_logger->debug("convertProtoToTeleJson Failed ");  
            }
            else if(TELE_curFlowCount == TELE_maxFlowLimit)
            {
                m_dspManager.getChinaTelecomObject()->curFlowCountIncrease();
                g_worker_logger->debug("FLOW LIMITED...");
            }
        }
        
        
        /*
                *send to GYIN
                */
        if(m_config.get_enGYIN())
        {
            int GYIN_maxFlowLimit = m_dspManager.getGuangYinObject()->getMaxFlowLimit();
            int GYIN_curFlowCount = m_dspManager.getGuangYinObject()->getCurFlowCount();
            if((GYIN_curFlowCount < GYIN_maxFlowLimit)||(GYIN_maxFlowLimit == 0))
            {
                BidRequest bidRequest;        
                if(convertProtoToGYinProto(bidRequest,mobile_request))
                {            
                    int length = bidRequest.ByteSize();
                    char* buf = new char[length];
                    bidRequest.SerializeToArray(buf,length);        
                    if(m_config.get_logGYINReq())
                    {                    
                        displayGYinBidRequest(buf,length);                
                    }
                    if(!m_dspManager.sendAdRequestToGuangYinDSP(buf, length, ua))
                    {
                        g_workerGYIN_logger->debug("POST TO GYIN fail uuid: {0}",uuid); 
                    }
                    else
                    {
                    	    if(GYIN_maxFlowLimit != 0)
                        	m_dspManager.getGuangYinObject()->curFlowCountIncrease();
                        g_workerGYIN_logger->debug("POST TO GYIN success uuid: {0} \r\n",uuid); 
                    }
                    delete [] buf;
                }     
                else
                    g_workerGYIN_logger->debug("convertProtoToGYinProto Failed ");  
            }   
            else if(GYIN_curFlowCount == GYIN_maxFlowLimit)
            {
                m_dspManager.getGuangYinObject()->curFlowCountIncrease();
                g_workerGYIN_logger->debug("FLOW LIMITED...");
            }
        }

        /*
                *send to SMAATO
                */
        if(m_config.get_enSmaato())
        {
            int SMAATO_maxFlowLimit = m_dspManager.getSmaatoObject()->getMaxFlowLimit();
            int SMAATO_curFlowCount = m_dspManager.getSmaatoObject()->getCurFlowCount();
            if((SMAATO_curFlowCount < SMAATO_maxFlowLimit)||(SMAATO_maxFlowLimit == 0))
            {
		     char *http_getArg = new char[4096];
	            memset(http_getArg, 0 ,4096*sizeof(char));
	            if(!convertProtoToHttpGETArg(http_getArg, mobile_request))
	                return ;
	            int sock = 0;
	            if((sock = m_dspManager.sendAdRequestToSmaatoDSP(http_getArg, strlen(http_getArg), uuid, ua)) <= 0)
	            {
	                g_workerSMAATO_logger->debug("POST TO SMAATO fail uuid: {0}", uuid);
	            }
	            else
	            {
	                if(SMAATO_maxFlowLimit != 0)
	                    m_dspManager.getSmaatoObject()->curFlowCountIncrease();
	                g_workerSMAATO_logger->debug("POST TO SMAATO success uuid: {0}", uuid);
	                

                        char *fullData = new char[BUF_SIZE];
                        memset(fullData, 0, BUF_SIZE*sizeof(char));

                        struct spliceData_t *fullData_t = new spliceData_t();
                        fullData_t->data = fullData;
                        fullData_t->curLen = 0;

                        if(m_dspManager.recvBidResponseFromSmaatoDsp(sock, fullData_t))
                        {
                            int dataLen = 0;    
                           char * bodyData = new char[BUF_SIZE];
                            memset(bodyData, 0, BUF_SIZE*sizeof(char));

                            struct spliceData_t *httpBodyData_t = new spliceData_t();
                            httpBodyData_t->data = bodyData;
                            httpBodyData_t->curLen = 0;

                            string dspName = "SMAATO";
                            switch(httpBodyTypeParse(fullData_t->data, fullData_t->curLen))
                            {
                                case HTTP_204_NO_CONTENT:
                                    g_workerSMAATO_logger->debug("{0} HTTP RSP: 204 No Content\r\n", dspName);
                                    break;
                                case HTTP_CONTENT_LENGTH:
                                    g_workerSMAATO_logger->debug("{0} HTTP RSP 200 OK: Content-Length", dspName);
                                    dataLen = httpContentLengthParse(httpBodyData_t, fullData_t->data, fullData_t->curLen);
                                    break;
                                case HTTP_CHUNKED:
                                    g_workerSMAATO_logger->debug("{0} HTTP RSP 200 OK: Chunked", dspName);
                                    dataLen = httpChunkedParse(httpBodyData_t, fullData_t->data, fullData_t->curLen);                    
                                    g_workerSMAATO_logger->trace("\r\nCHUNKED:\r\n{0}", httpBodyData_t->data);
                                    break;
                                case HTTP_UNKNOW_TYPE:
                                    g_workerSMAATO_logger->debug("{0} HTTP RSP: HTTP UNKNOW TYPE", dspName);
                                    break;
                                 default:
                                    break;                    
                            }

                            delete [] fullData;
                            delete [] fullData_t;            
                            
                            if(dataLen == 0)
                            {
                                ; //already printf "GYIN HTTP RSP: 204 No Content"  
                            }   
                            else if(dataLen == -1)
                            {
                                g_workerSMAATO_logger->error("HTTP BODY DATA INCOMPLETE ");
                            }
                            else
                            {
                                g_workerSMAATO_logger->debug("SmaatoRsponse: \r\n{0}", bodyData);                          
                                handle_BidResponseFromDSP(SMAATO, bodyData, dataLen, request_commMsg);   
                            }        
                            delete [] bodyData;
                            delete httpBodyData_t;
                        }
  
					//if(requestUuidList.size() <= 100)
						//requestUuidList.push_back(uuid);	

					#if 0
					list<string>* m_uuidList = m_dspManager.getSmaatoObject()->getRequestUuidList();					
				  	if(m_uuidList->size() <= 100)
				  	{
						m_dspManager.getSmaatoObject()->requestUuidList_Locklock();
						m_uuidList->push_back(uuid);
						m_dspManager.getSmaatoObject()->requestUuidList_Lockunlock();
					}	
					#endif
	            }
	            delete [] http_getArg;
		}
		else if(SMAATO_curFlowCount == SMAATO_maxFlowLimit)
              {
                   m_dspManager.getSmaatoObject()->curFlowCountIncrease();
                   g_workerGYIN_logger->debug("FLOW LIMITED...");
              }
        }      

         /*
                *send to INMOBI
                */                
        if(m_config.get_enInMobi()) 
        {
            int INMOBI_maxFlowLimit = m_dspManager.getInMobiObject()->getMaxFlowLimit();
            int INMOBI_curFlowCount = m_dspManager.getInMobiObject()->getCurFlowCount();
            if((INMOBI_curFlowCount < INMOBI_maxFlowLimit)||(INMOBI_curFlowCount == 0))
            {
		    string reqTeleJsonData;
		    #if 0
		    {
                        Json::Reader reader;
                        Json::Value root;

                        ifstream infile;
                        infile.open("inmobitest.json",ios::in | ios::binary);

                        if(reader.parse(infile,root))
                        {
                            infile.close();        
                            root.toStyledString();
                            reqTeleJsonData = root.toStyledString();
                        }
		    }
		    #endif
	            if(convertProtoToInMobiJson(reqTeleJsonData, mobile_request))
	            {
                        //callback func: handle_recvAdResponseTele active by socket EV_READ event     
	                 int sock = 0;
	                if((sock = m_dspManager.sendAdRequestToInMobiDSP(reqTeleJsonData.c_str(), strlen(reqTeleJsonData.c_str()), m_config.get_logInMobiReq(), ua)) <= 0)
	                {
	                    g_workerINMOBI_logger->debug("POST TO INMOBI fail uuid : {0}",uuid);            
	                }
	                else
	                {
	                	if(INMOBI_curFlowCount != 0)
                        	    m_dspManager.getInMobiObject()->curFlowCountIncrease();
				g_workerINMOBI_logger->debug("POST TO INMOBI success uuid : {0} \r\n",uuid);  

				char *fullData = new char[BUF_SIZE];
                            memset(fullData, 0, BUF_SIZE*sizeof(char));

                            struct spliceData_t *fullData_t = new spliceData_t();
                            fullData_t->data = fullData;
                            fullData_t->curLen = 0;

                            if(m_dspManager.recvBidResponseFromSmaatoDsp(sock, fullData_t))
                            {
                                int dataLen = 0;    
                               char * bodyData = new char[BUF_SIZE];
                                memset(bodyData, 0, BUF_SIZE*sizeof(char));

                                struct spliceData_t *httpBodyData_t = new spliceData_t();
                                httpBodyData_t->data = bodyData;
                                httpBodyData_t->curLen = 0;

                                string dspName = "INMOBI";
                                switch(httpBodyTypeParse(fullData_t->data, fullData_t->curLen))
                                {
                                    case HTTP_204_NO_CONTENT:
                                        g_workerINMOBI_logger->debug("{0} HTTP RSP: 204 No Content\r\n", dspName);
                                        break;
                                    case HTTP_CONTENT_LENGTH:
                                        g_workerINMOBI_logger->debug("{0} HTTP RSP 200 OK: Content-Length", dspName);
                                        dataLen = httpContentLengthParse(httpBodyData_t, fullData_t->data, fullData_t->curLen);
                                        break;
                                    case HTTP_CHUNKED:
                                        g_workerINMOBI_logger->debug("{0} HTTP RSP 200 OK: Chunked", dspName);
                                        dataLen = httpChunkedParse(httpBodyData_t, fullData_t->data, fullData_t->curLen);                    
                                        g_workerINMOBI_logger->trace("\r\nCHUNKED:\r\n{0}", httpBodyData_t->data);
                                        break;
                                    case HTTP_UNKNOW_TYPE:
                                        g_workerINMOBI_logger->debug("{0} HTTP RSP: HTTP UNKNOW TYPE", dspName);
                                        break;
                                     default:
                                        break;                    
                                }

                                delete [] fullData;
                                delete [] fullData_t;            
                                
                                if(dataLen == 0)
                                {
                                    ; //already printf "GYIN HTTP RSP: 204 No Content"  
                                }   
                                else if(dataLen == -1)
                                {
                                    g_workerINMOBI_logger->error("HTTP BODY DATA INCOMPLETE ");
                                }
                                else
                                {
                                    g_workerINMOBI_logger->debug("InmobiRsponse: \r\n{0}", bodyData);                          
                                    handle_BidResponseFromDSP(INMOBI, bodyData, dataLen, request_commMsg);   
                                }        
                                delete [] bodyData;
                                delete httpBodyData_t;
                            }
			   }
	            }
	            else
	                 g_workerINMOBI_logger->debug("convertProtoToInMobiJson Failed ");  
	     }
	     else if(INMOBI_curFlowCount == INMOBI_maxFlowLimit)
            {
                m_dspManager.getInMobiObject()->curFlowCountIncrease();
                g_workerINMOBI_logger->debug("FLOW LIMITED...");
            }
            
        }
             
    }
    catch(...)
    {
        g_worker_logger->error("mobile_AdRequestHandler exception \r\n");
    }
}
void connectorServ::thread_handleAdRequest(void *arg)
{    
    char publishKey[PUBLISHKEYLEN_MAX];
    messageBuf *msg = (messageBuf*) arg;
    if(!msg) return;

    try
    {
        char *buf = msg->get_stringBuf().get_data();
        int dataLen = msg->get_stringBuf().get_dataLen();
        connectorServ *serv = (connectorServ*) msg->get_serv();
        memcpy(publishKey, buf, PUBLISHKEYLEN_MAX);     //uuid
        if(!buf||!serv) throw;

        CommonMessage request_commMsg;
        if(!request_commMsg.ParseFromArray(buf+PUBLISHKEYLEN_MAX, dataLen-PUBLISHKEYLEN_MAX))
        {
            g_worker_logger->error("adREQ CommonMessage.proto Parse Fail,check required fields");
            throw -1;
        }
        
        
        const string& tbusinessCode = request_commMsg.businesscode();

        const string& commMsg_data = request_commMsg.data();
        MobileAdRequest mobile_request;
        mobile_request.ParseFromString(commMsg_data);

        string uuid = mobile_request.id();

        timeval tv;
        memset(&tv,0,sizeof(struct timeval));
        gettimeofday(&tv,NULL);

        char *commMsgData = new char[dataLen-PUBLISHKEYLEN_MAX];
        memcpy(commMsgData, buf+PUBLISHKEYLEN_MAX, dataLen-PUBLISHKEYLEN_MAX);
        
        struct commMsgRecord* obj = new commMsgRecord();
        obj->data = commMsgData;
        obj->datalen = dataLen-PUBLISHKEYLEN_MAX;
        obj->tv = tv;
        if(request_commMsg.has_ttl()&&(!request_commMsg.ttl().empty()))
        {
            obj->ttl = atoi(request_commMsg.ttl().c_str());            
        }
        else
        {
            obj->ttl = CHECK_COMMMSG_TIMEOUT;
        }
        
        
        serv->commMsgRecordList_lock.lock();
        serv->commMsgRecordList.insert(pair<string, commMsgRecord*>(uuid, obj));
        g_worker_logger->trace("LIST INSERT uuid: {0}", uuid);
        serv->commMsgRecordList_lock.unlock();

        
            
        if(tbusinessCode == serv->m_mobileBusinessCode)     //"2"
        {
            serv->mobile_AdRequestHandler(publishKey,request_commMsg);
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
        g_worker_logger->error("thread_handleAdRequest exception");
    }  
}
void connectorServ::workerHandle_callback(int fd, short event, void *pair)
{
    try
    {
        zmq_msg_t msg;
        uint32_t events;
        size_t len;
        
        g_worker_logger->debug("##### workerHandle_callback");
        connectorServ *serv = (connectorServ*) pair;
        if(serv==NULL) 
        {
            g_worker_logger->emerg("workerHandle_callback param is null");
            exit(1);
        }

        len = sizeof(events);
        void *adrsp = serv->m_workerPullHandler;
        if(adrsp == NULL) throw 0;
        int rc = zmq_getsockopt(adrsp, ZMQ_EVENTS, &events, &len);
        if(rc == -1)
        {
            g_worker_logger->error("workerHandle_callback zmq_getsockopt return -1");
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
                g_worker_logger->debug("worker {0:d} recv data length:{1:d}",getpid(), recvLen);                
                if(serv->m_thread_manager.Run(thread_handleAdRequest,(void *)msg)!=0)//not return 0, failure
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
        g_worker_logger->emerg("workerHandle_callback exception");
        exit(1);
    }
}
int connectorServ::zmq_get_message(void* socket, zmq_msg_t &part, int flags)
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
void *connectorServ::get_sendLoginHeartToOtherHandler(int fd)
{
    void *handler = m_throttle_manager.get_sendLoginHeartToThrottleHandler(fd);
    if(handler) return handler;
    handler = m_bc_manager.get_sendLoginHeartToBCHandler(fd);
    return handler;
}
bool connectorServ::manager_from_BC_handler(const managerProtocol_messageType &type
  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg)
{
    const string& bc_ip = value.ip();
    if(value.port_size() != 2) return false;
    unsigned short bc_mangerPort = value.port(0);
    unsigned short bc_dataPort = value.port(1);
    bool ret = false;
    switch(type)
    {
        case managerProtocol_messageType_LOGIN_REQ:
        {
            //add_bc_to_bidder_request
            g_manager_logger->info("[login req][connector <- BC]:{0},{1:d},{2:d}", bc_ip, bc_mangerPort, bc_dataPort);     
            //cout << "[login req][connector <- BC]: " << bc_ip << endl;
            string& connectorIdentify = m_connector_manager.get_connectorIdentify();
            m_bc_manager.add_bc(bc_ip,bc_dataPort,bc_mangerPort,m_zmq_connect,connectorIdentify,pth1_base,handle_recvLoginHeartRsp,arg);  
            
            const string& connectorIP = m_connector_manager.get_connector_config().get_connectorIP();
            unsigned short conManagerPort = m_connector_manager.get_connector_config().get_connectorManagerPort();       
            
            string& key = m_zmqSubKey_manager.add_subKey(connectorIP,conManagerPort,bc_ip, bc_mangerPort, bc_dataPort);                
            if(key.empty() == false)
            {
                m_throttle_manager.add_throSubKey(key);
            }
            
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
            g_manager_logger->info("[login rsp][connector <- BC]:{0},{1:d}", bc_ip, bc_mangerPort);
            
            const string& connectorIP = m_connector_manager.get_connector_config().get_connectorIP();
            unsigned short conManagerPort = m_connector_manager.get_connector_config().get_connectorManagerPort();       
            
            string& key = m_zmqSubKey_manager.add_subKey(connectorIP,conManagerPort,bc_ip, bc_mangerPort, bc_dataPort);                
            if(key.empty() == false)
            {
                m_throttle_manager.add_throSubKey(key);
            }
            
            m_bc_manager.logined(bc_ip,bc_mangerPort);
            break;
        }        
        case managerProtocol_messageType_HEART_RSP:
        {
            g_manager_logger->info("[heart rsp][connector <- BC]:{0},{1:d}", bc_ip, bc_mangerPort);
            m_bc_manager.recvHeartRsp(bc_ip,bc_mangerPort);
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
bool connectorServ::manager_from_throttle_handler(const managerProtocol_messageType &type
  , const managerProtocol_messageValue &value, managerProtocol_messageType &rspType, struct event_base * base, void * arg)
{
    const string& thro_ip = value.ip();
    if(value.port_size() != 2) return false;
    unsigned short thro_managerPort = value.port(0);
    unsigned short thro_dataPort = value.port(1);
    bool ret = false;
    switch(type)
    {
        case managerProtocol_messageType_LOGIN_REQ:
        {            
            g_manager_logger->info("[login req][connector <- throttle]:{0},{1:d},{2:d}", thro_ip, thro_managerPort, thro_dataPort);    
            string& connectorIdentify = m_connector_manager.get_connectorIdentify();
            m_throttle_manager.add_throttle(thro_ip,thro_dataPort,thro_managerPort,m_zmq_connect,connectorIdentify,pth1_base,handle_recvLoginHeartRsp,arg);            
            ret = true;
            rspType = managerProtocol_messageType_LOGIN_RSP;
            break;
        }
        case managerProtocol_messageType_LOGIN_RSP:
        {            
            g_manager_logger->info("[login rsp][connector <- throttle]:{0},{1:d},{2:d}", thro_ip, thro_managerPort, thro_dataPort);
            m_throttle_manager.logined(thro_ip,thro_managerPort);
            break;
        }
        case managerProtocol_messageType_HEART_REQ:
        {           
            g_manager_logger->info("[heart req][connector <- throttle]:{0}, {1:d}", thro_ip, thro_managerPort);
            ret = m_throttle_manager.recvHeartReq(thro_ip,thro_managerPort);
            rspType = managerProtocol_messageType_HEART_RSP;
            break;
        }
        case managerProtocol_messageType_REGISTER_RSP:
        {            
            const string& key = value.key();
            g_manager_logger->info("[register response][connector <- throttle]:{0},{1:d},{2:d}", thro_ip, thro_managerPort, thro_dataPort);
            m_throttle_manager.set_throSubKeyRegisted(thro_ip,thro_managerPort,key);
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
bool connectorServ::manager_handler(const managerProtocol_messageTrans &from
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
bool connectorServ::manager_handler(void *handler, string& identify,  const managerProtocol_messageTrans &from
            ,const managerProtocol_messageType &type, const managerProtocol_messageValue &value,struct event_base * base, void * arg)
{
    managerProtocol_messageType rspType;
    bool ret = manager_handler(from, type, value, rspType, base, arg);
    if(ret)
    {
        connectorConfig configure = m_connector_manager.get_connector_config();
        const string& connectorIP = configure.get_connectorIP();
        unsigned short connectorPort = configure.get_connectorManagerPort();
        if(managerProtocol_messageTrans_BC == from)
        {
            if(managerProtocol_messageType_LOGIN_RSP == rspType)
                g_manager_logger->info("[login rsp][connector -> BC]:{0}, {1:d}", connectorIP, connectorPort);
            else if(managerProtocol_messageType_HEART_RSP == rspType)
                g_manager_logger->info("[heart rsp][connector -> BC]:{0}, {1:d}", connectorIP, connectorPort);
        }
        else if(managerProtocol_messageTrans_THROTTLE == from)
        {
            if(managerProtocol_messageType_LOGIN_RSP == rspType)
                g_manager_logger->info("[login rsp][connector -> throttle]:{0}, {1:d}", connectorIP, connectorPort);
            else if(managerProtocol_messageType_HEART_RSP == rspType)
                g_manager_logger->info("[heart rsp][connector -> throttle]:{0}, {1:d}", connectorIP, connectorPort);
        }
        
        int sndsize = managerProPackage::send_response(handler, identify, managerProtocol_messageTrans_CONNECTOR, rspType
            , connectorIP, connectorPort);
    }
    return ret;
}
void connectorServ::sendToWorker(char* subKey, int keyLen, char * buf,int bufLen)
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
void connectorServ::handle_recvLoginHeartReq(int fd,short event,void *arg)
{
    //cout << "<<<<<< recvLoginHeart Event acting !!! fd = "<< fd << endl;    
    uint32_t events;
    size_t len=sizeof(events);
    int recvLen = 0;

    eventArgment *argment = (eventArgment*) arg;
    connectorServ *serv = (connectorServ*)argment->get_serv();
    
    if(serv==NULL) 
    {
        g_manager_logger->emerg("serv is nullptr");
        exit(1);
    }

    void *handler = serv->get_recvLoginHeartReqHandler();
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
                if(!manager_pro.ParseFromArray(msg_data, recvLen))
                {
                    g_manager_logger->error("adREQ managerProtocol.proto Parse Fail,check required fields");                    
                }
                else
                {
                    const managerProtocol_messageTrans &from = manager_pro.messagefrom();
                    const managerProtocol_messageType &type = manager_pro.messagetype();
                    const managerProtocol_messageValue &value = manager_pro.messagevalue();
                    serv->manager_handler(handler, identify, from, type, value, argment->get_base(), serv);    
                }
                       
           }            
           zmq_msg_close(&part);
        }
    }    
}
void connectorServ::handle_recvLoginHeartRsp(int fd,short event,void *arg)
{    
    uint32_t events;
    size_t len=sizeof(events);
    int recvLen = 0;
    ostringstream os;
    connectorServ *serv = (connectorServ*)arg;
    if(serv==NULL) 
    {
       g_manager_logger->emerg("register_throttle_response_handler param is null");
       exit(1);
    }

     void *handler = serv->get_sendLoginHeartToOtherHandler(fd);
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
                //cout << "<<<<< recv login rsp !!" << endl;
               managerProtocol manager_pro;
               if(!manager_pro.ParseFromArray(msg_data, recvLen))  
               {
                    g_manager_logger->error("RSP managerProtocol.proto Parse Fail,check required fields");
               }
               else
               {
                    const managerProtocol_messageTrans &from  =  manager_pro.messagefrom();
                    const managerProtocol_messageType  &type   =  manager_pro.messagetype();
                    const managerProtocol_messageValue &value =  manager_pro.messagevalue();
                    managerProtocol_messageType responseType;
                    serv->manager_handler(from, type, value, responseType);
               }
               
           }
           zmq_msg_close(&part);
        }
    }
}
void connectorServ::handle_recvAdRequest(int fd,short event,void * arg)
{
    uint32_t events;
    size_t len=sizeof(events);

    connectorServ *serv = (connectorServ*)arg;
    if(serv==NULL) 
    {
        g_manager_logger->emerg("recvAdRequest_callback param is null");
        return;
    }

    void *handler = serv->m_throttle_manager.get_recvAdReqFromThrottleHandler(fd);
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
           //why call zmq_get_message twice ?
           //because: throttle zmq_send twice, 1)subscribe_key  2)data
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

            {
                CommonMessage request_commMsg;
                if(!request_commMsg.ParseFromArray(data, data_len))
                {
                    g_worker_logger->error("adREQ CommonMessage.proto Parse Fail,check required fields");
                    throw -1;
                }

                const string& commMsg_data = request_commMsg.data();
                MobileAdRequest mobile_request;
                mobile_request.ParseFromString(commMsg_data);

                string uuid = mobile_request.id();
                g_master_logger->debug("recv AdRequest from throttle uuid:{0} ,len: {1:d}", uuid, data_len);
            }
   

		   
           if((sub_key_len>0) &&(data_len>0))
           {
               serv->sendToWorker(subscribe_key, sub_key_len, data, data_len);
           }
           zmq_msg_close(&subscribe_key_part);
           zmq_msg_close(&data_part);
        }
    }
}
void connectorServ::handle_recvAdResponse(int sock, short event, void *arg, dspType type)
{
    connectorServ *serv = (connectorServ*)arg;
    shared_ptr<spdlog::logger> g_logger = NULL;
    string dspName;
    struct event *listenEvent = NULL;
    bool flag_displayBodyData = false; 
    
    switch(type)
    {
        case TELE:
            g_logger = g_worker_logger;
            dspName = "TELE";
            flag_displayBodyData = serv->m_config.get_logTeleHttpRsp();
            break;
        case GYIN:
            g_logger = g_workerGYIN_logger;
            dspName = "GYIN";
            flag_displayBodyData = serv->m_config.get_logGYINHttpRsp();
            break;
	 case SMAATO:
	     g_logger = g_workerSMAATO_logger;
	     dspName = "SMAATO";
	     flag_displayBodyData = serv->m_config.get_logSmaatoHttpRsp();
	     break;
	 case INMOBI:
	    g_logger = g_workerINMOBI_logger;
	    dspName = "INMOBI";
	    flag_displayBodyData = serv->m_config.get_logInMobiHttpRsp();
        default:
            break;          
    }    
    
    char *recv_str = new char[BUF_SIZE];
    memset(recv_str, 0, BUF_SIZE*sizeof(char));    
    int recv_bytes = 0;

    char *fullData = new char[BUF_SIZE];
    memset(fullData, 0, BUF_SIZE*sizeof(char));

    struct spliceData_t *fullData_t = new spliceData_t();
    fullData_t->data = fullData;
    fullData_t->curLen = 0;
    
    g_logger->debug("RECV {0} HTTP RSP by PID: {1:d}", dspName, getpid());
    

    int dataLen = 0;    
    int temp = 0;
    
    while(1)
    {
        memset(recv_str,0,BUF_SIZE*sizeof(char));    
        recv_bytes = recv(sock, recv_str, BUF_SIZE*sizeof(char), 0);
        if (recv_bytes == 0)    //connect abort
        {
            g_logger->debug("server {0} CLOSE_WAIT ... \r\n", dspName);
            switch(type)
            {
                case TELE:	
			{
                        struct listenObject *obj = serv->m_dspManager.getChinaTelecomObject()->findListenObject(sock);
                        if(obj != NULL)
                        {
                            listenEvent = obj->_event;
                            serv->m_dspManager.getChinaTelecomObject()->eraseListenObject(sock);
                            event_del(listenEvent);
                        }
                        else 
                        {
                            g_logger->error("FIND LISTEN OBJ FAILED");		 
                        }
                    }
                    break;
                case GYIN:					
                    {
                        struct listenObject *obj = serv->m_dspManager.getGuangYinObject()->findListenObject(sock);
                        if(obj != NULL)
                        {
                            listenEvent = obj->_event;
                            serv->m_dspManager.getGuangYinObject()->eraseListenObject(sock);
                            event_del(listenEvent);
                        }
                        else 
                        {
                            g_logger->error("FIND LISTEN OBJ FAILED");		 
                        }
                    }
                    break;
		  case SMAATO:
		      {
			   struct listenObject *obj = serv->m_dspManager.getSmaatoObject()->findListenObject(sock);
                        if(obj != NULL)
                        {
                            listenEvent = obj->_event;
                            serv->m_dspManager.getSmaatoObject()->eraseListenObject(sock);
                            event_del(listenEvent);
                        }
                        else 
                        {
                            g_logger->error("FIND LISTEN OBJ FAILED");		 
                        }	
			}		  
		  break;
		case INMOBI:					
                    {
                        struct listenObject *obj = serv->m_dspManager.getInMobiObject()->findListenObject(sock);
                        if(obj != NULL)
                        {
                            listenEvent = obj->_event;
                            serv->m_dspManager.getInMobiObject()->eraseListenObject(sock);
                            event_del(listenEvent);
                        }
                        else 
                        {
                            g_logger->error("FIND LISTEN OBJ FAILED");		 
                        }
                    }
                    break;
                default:
                    break;                    
            }
            close(sock);
            delete [] recv_str;
            delete [] fullData;
            delete [] fullData_t;
            return;
        }
        else if (recv_bytes < 0)  //SOCKET_ERROR
        {
            //socket type: O_NONBLOCK
            if(errno == EAGAIN)     //EAGAIN mean no data in recv_buf world be read, loop break
            {
                g_logger->trace("ERRNO EAGAIN: RECV END");
                break;
            }
            else if(errno == EINTR) //function was interrupted by a signal that was caught, before any data was available.need recv again
            {
                g_logger->trace("ERRNO EINTR: RECV AGAIN");
                continue;
            }
        }
        else    //normal success
        {
            if(temp)
                g_logger->trace("SPLICE HAPPEN");
            g_logger->trace("\r\n{0}", recv_str);            
            int full_expectLen = fullData_t->curLen + recv_bytes;
            if(full_expectLen > BUF_SIZE)
            {
                g_logger->error("RECV BYTES:{0:d} > BUF_SIZE[{1:d}], THROW AWAY", full_expectLen, BUF_SIZE);
                delete [] recv_str;
                delete [] fullData;
                delete [] fullData_t;
                return ;
            }
            char *curPos = fullData_t->data + fullData_t->curLen;
            memcpy(curPos, recv_str, recv_bytes);
            fullData_t->curLen += recv_bytes;
            temp++;            
            usleep(1000);    // 1ms
        }
    }

    delete [] recv_str;

    char * bodyData = new char[BUF_SIZE];
    memset(bodyData, 0, BUF_SIZE*sizeof(char));

    struct spliceData_t *httpBodyData_t = new spliceData_t();
    httpBodyData_t->data = bodyData;
    httpBodyData_t->curLen = 0;
               
    switch(httpBodyTypeParse(fullData_t->data, fullData_t->curLen))
    {        
        case HTTP_204_NO_CONTENT:
            g_logger->debug("{0} HTTP RSP: 204 No Content\r\n", dspName);
            break;
        case HTTP_CONTENT_LENGTH:
            g_logger->debug("{0} HTTP RSP 200 OK: Content-Length", dspName);
            dataLen = httpContentLengthParse(httpBodyData_t, fullData_t->data, fullData_t->curLen);
            break;
        case HTTP_CHUNKED:
            g_logger->debug("{0} HTTP RSP 200 OK: Chunked", dspName);
            dataLen = httpChunkedParse(httpBodyData_t, fullData_t->data, fullData_t->curLen);                    
            //g_logger->trace("\r\nCHUNKED:\r\n{0}", httpBodyData_t->data);
            break;
        case HTTP_UNKNOW_TYPE:
        {
            g_logger->debug("{0} HTTP RSP: HTTP UNKNOW TYPE", dspName);
            g_logger->trace("STORE DATA:\r\n {0} \r\n LEN:\r\n {1:d}", stroeBuffer_t->data, stroeBuffer_t->curLen);
            char *curPos = stroeBuffer_t->data + stroeBuffer_t->curLen;
            memcpy(curPos, fullData_t->data, fullData_t->curLen);
            stroeBuffer_t->curLen += fullData_t->curLen;
            g_logger->trace("STORE DATA:\r\n {0} \r\n LEN:\r\n {1:d}", stroeBuffer_t->data, stroeBuffer_t->curLen);
            dataLen = httpChunkedParse(httpBodyData_t, stroeBuffer_t->data, stroeBuffer_t->curLen);   
            g_logger->debug("DATALEN: {0:d}", dataLen);
            memset(stroeBuffer, 0, BUF_SIZE*sizeof(char));
            stroeBuffer_t->curLen = 0;
        }
            break;
        case HTTP_400_BAD_REQUEST:
            g_logger->debug("{0} HTTP RSP: 400 Bad Request", dspName);
            break;
         default:
            break;                    
    }

          
    if(dataLen == 0)
    {
        ; //already printf "GYIN HTTP RSP: 204 No Content"  
    }   
    else if(dataLen == -1)
    {
        g_logger->error("HTTP BODY DATA INCOMPLETE ");
        
        memset(stroeBuffer, 0, BUF_SIZE*sizeof(char));
        stroeBuffer_t->data = stroeBuffer;
        stroeBuffer_t->curLen = 0;
        
        memcpy(stroeBuffer, fullData_t->data, fullData_t->curLen);        
        stroeBuffer_t->curLen += fullData_t->curLen;
    }
    else
    {
        if(flag_displayBodyData)
        {
            switch(type)
            {
                case TELE:
                    g_logger->debug("TeleRsponse: \r\n{0}", bodyData);   //json
                    break;
                case GYIN:
                    serv->displayGYinBidResponse(bodyData, dataLen);    //protobuf
                    break;
		  case SMAATO:
		      g_logger->debug("SmaatoRsponse: \r\n{0}", bodyData);  //xml
		      break;
		  case INMOBI:
		      g_logger->debug("InMobi: \r\n{0}", bodyData);     //json
		      break;
                default:
                    break;                              
            }
        }
        CommonMessage request_commMsg; 
        serv->handle_BidResponseFromDSP(type, bodyData, dataLen, request_commMsg);   
    }        
    delete [] fullData;
    delete fullData_t;     
    delete [] bodyData;
    delete httpBodyData_t;
    
}
void connectorServ::handle_recvAdResponseTele(int sock,short event,void *arg)
{   
    connectorServ *serv = (connectorServ*)arg;
    if(serv==NULL) 
    {
        g_worker_logger->emerg("handle_recvAdResponseTele param is null");
        return;
    }
    
    serv->handle_recvAdResponse(sock, event, arg, TELE);

    #if 0
    if(serv==NULL) 
    {
        g_worker_logger->emerg("handle_recvAdResponseTele param is null");
        return;
    }
   
    struct event *listenEvent = serv->m_dspManager.getChinaTelecomObject()->findListenObject(sock)->_event;
    
    char *recv_str = new char[4096];
    memset(recv_str, 0, 4096*sizeof(char));    
    int recv_bytes = 0;

    char *fullData = new char[4096];
    memset(fullData, 0, 4096*sizeof(char));

    struct spliceData_t *fullData_t = new spliceData_t();
    fullData_t->data = fullData;
    fullData_t->curLen = 0;
    
    g_worker_logger->debug("RECV TELE HTTP RSP by PID: {0:d}",getpid());
    char * jsonData = new char[4048];
    memset(jsonData, 0, 4048*sizeof(char));

    struct spliceData_t *httpBodyData_t = new spliceData_t();
    httpBodyData_t->data = jsonData;
    httpBodyData_t->curLen = 0;

    int dataLen = 0;    
    int temp = 0;
    
    while(1)
    {
        memset(recv_str,0,4096*sizeof(char));    
        recv_bytes = recv(sock, recv_str, 4096*sizeof(char), 0);
        if (recv_bytes == 0)    //connect abort
        {
            g_worker_logger->debug("server TELE CLOSE_WAIT ...");
            serv->m_dspManager.getChinaTelecomObject()->eraseListenObject(sock);
            close(sock);
            event_del(listenEvent);
            delete [] recv_str;
            return;
        }
        else if (recv_bytes < 0)  //SOCKET_ERROR
        {
            //socket type: O_NONBLOCK
            if(errno == EAGAIN)     //EAGAIN mean no data in recv_buf world be read, loop break
            {
                g_worker_logger->debug("ERRNO EAGAIN: RECV END");
                break;
            }
            else if(errno == EINTR) //function was interrupted by a signal that was caught, before any data was available.need recv again
            {
                g_worker_logger->debug("ERRNO EINTR: RECV AGAIN");
                continue;
            }
        }
        else    //normal success
        {
            if(temp)
                g_worker_logger->debug("SPLICE HAPPEN");
            g_worker_logger->debug("\r\n{0}", recv_str);
            char *curPos = fullData_t->data + fullData_t->curLen;
            memcpy(curPos, recv_str, recv_bytes);
            fullData_t->curLen += recv_bytes;
            temp++;            
        }
    }

    if(serv->m_config.get_logTeleHttpRsp())
    {
        g_worker_logger->debug("\r\n{0}",fullData_t->data);	
    }                    
    switch(httpBodyTypeParse(fullData_t->data, fullData_t->curLen))
    {
        case HTTP_204_NO_CONTENT:
            g_worker_logger->debug("TELE HTTP RSP: 204 No Content");
            break;
        case HTTP_CONTENT_LENGTH:
            g_worker_logger->debug("TELE HTTP RSP: Content-Length");
            dataLen = httpContentLengthParse(httpBodyData_t, fullData_t->data, fullData_t->curLen);
            break;
        case HTTP_CHUNKED:
            g_worker_logger->debug("TELE HTTP RSP: Chunked");
            dataLen = httpChunkedParse(httpBodyData_t, fullData_t->data, fullData_t->curLen);                    
            g_worker_logger->debug("\r\nCHUNKED:\r\n{0}", httpBodyData_t->data);
            break;
        case HTTP_UNKNOW_TYPE:
            g_worker_logger->debug("TELE HTTP RSP: HTTP UNKNOW TYPE");
            break;
         default:
            break;                    
    }

    delete [] fullData;
    delete [] fullData_t;
            
    //delete [] recv_str;
    if(dataLen == 0)
    {
        ; //already printf "GYIN HTTP RSP: 204 No Content"  
    }   
    else if(dataLen == -1)
    {
        g_worker_logger->error("HTTP BODY DATA INCOMPLETE ");
    }
    else
    {
        g_worker_logger->debug("BidRsponse : {0}",jsonData);    
        serv->handle_BidResponseFromDSP(TELE, jsonData, dataLen);   
    }        
    delete [] jsonData;
    delete httpBodyData_t;
    #endif
}

void connectorServ::handle_recvAdResponseGYin(int sock,short event,void *arg)
{   
    connectorServ *serv = (connectorServ*)arg;
    if(serv==NULL) 
    {
        g_workerGYIN_logger->emerg("handle_recvAdResponseGYin param is null");
        return;
    }
    
    serv->handle_recvAdResponse(sock, event, arg, GYIN);

    #if 0
    if(serv==NULL) 
    {
        g_workerGYIN_logger->emerg("handle_recvAdResponseGYin param is null");
        return;
    }
   
    char *recv_str = new char[4096];
    memset(recv_str,0,4096*sizeof(char));    
    int recv_bytes = 0;

    
    char *fullData = new char[4096];
    memset(fullData, 0, 4096*sizeof(char));

    struct spliceData_t *fullData_t = new spliceData_t();
    fullData_t->data = fullData;
    fullData_t->curLen = 0;

    g_workerGYIN_logger->debug("RECV GYIN HTTP RSP by PID: {0:d}",getpid());    
    char * protoData = new char[4048];
    memset(protoData,0,4048*sizeof(char));

    struct spliceData_t *httpBodyData_t = new spliceData_t();
    httpBodyData_t->data = protoData;
    httpBodyData_t->curLen = 0;

    int dataLen = 0;    
    int temp = 0;

    while(1)
    {
        memset(recv_str,0,4096*sizeof(char));    
        recv_bytes = recv(sock, recv_str, 4096*sizeof(char), 0);
        if (recv_bytes == 0)
        {
            g_workerGYIN_logger->debug("server GYIN CLOSE_WAIT ...");        
            struct event *listenEvent = serv->m_dspManager.getGuangYinObject()->findListenObject(sock)->_event;
            serv->m_dspManager.getGuangYinObject()->eraseListenObject(sock);
            close(sock);
            event_del(listenEvent);
            delete [] recv_str;
            return;
        }
       else if (recv_bytes < 0)  //SOCKET_ERROR
        {
            //socket type: O_NONBLOCK
            if(errno == EAGAIN)     //EAGAIN mean no data in recv_buf world be read, loop break
            {
                g_workerGYIN_logger->debug("ERRNO EAGAIN: RECV END");
                break;
            }
            else if(errno == EINTR) //function was interrupted by a signal that was caught, before any data was available.need recv again
            {
                g_workerGYIN_logger->debug("ERRNO EINTR: RECV AGAIN");
                continue;
            }
        }
        else
        {
            if(temp)
                g_workerGYIN_logger->debug("SPLICE HAPPEN");
            g_workerGYIN_logger->debug("\r\n{0}", recv_str);
            char *curPos = fullData_t->data + fullData_t->curLen;
            memcpy(curPos, recv_str, recv_bytes);
            fullData_t->curLen += recv_bytes;
            temp++;               
        }
    }

    switch(httpBodyTypeParse(fullData_t->data, fullData_t->curLen))
    {
        case HTTP_204_NO_CONTENT:
            g_workerGYIN_logger->debug("GYIN HTTP RSP: 204 No Content");
            break;
        case HTTP_CONTENT_LENGTH:
            g_workerGYIN_logger->debug("GYIN HTTP RSP: Content-Length");
            dataLen = httpContentLengthParse(httpBodyData_t, fullData_t->data, fullData_t->curLen);
            break;
        case HTTP_CHUNKED:
            g_workerGYIN_logger->debug("GYIN HTTP RSP: Chunked");
            dataLen = httpChunkedParse(httpBodyData_t, fullData_t->data, fullData_t->curLen);
            g_workerGYIN_logger->debug("\r\nCHUNKED:\r\n{0}", httpBodyData_t->data);
            break;
        case HTTP_UNKNOW_TYPE:
            g_workerGYIN_logger->debug("GYIN HTTP RSP: HTTP UNKNOW TYPE");
            break;
         default:
            break;                    
    }

    delete [] fullData;
    delete [] fullData_t;
    
    //int dataLen = serv->getHttpRspData(protoData, recv_str);        
    delete [] recv_str;
    if(dataLen == 0)
    {        
        ;  //already printf "GYIN HTTP RSP: 204 No Content"
    }   
    else if(dataLen == -1)
    {
        g_workerGYIN_logger->error("HTTP BODY DATA INCOMPLETE");
    }
    else
    {
        if(serv->m_config.get_logGYINHttpRsp())
        {
            serv->displayGYinBidResponse(protoData, dataLen);
        }    
        serv->handle_BidResponseFromDSP(GYIN,protoData,dataLen);     
    }      
    delete [] protoData;    
    delete httpBodyData_t;
    #endif
}

void connectorServ::handle_recvAdResponseSmaato(int sock,short event,void *arg)
{   
    connectorServ *serv = (connectorServ*)arg;
    if(serv==NULL) 
    {
        g_workerGYIN_logger->emerg("handle_recvAdResponseSmaato param is null");
        return;
    }
    
    serv->handle_recvAdResponse(sock, event, arg, SMAATO);
}

void connectorServ::handle_recvAdResponseInMobi(int sock,short event,void *arg)
{   
    connectorServ *serv = (connectorServ*)arg;
    if(serv==NULL) 
    {
        g_workerINMOBI_logger->emerg("handle_recvAdResponseInMobi param is null");
        return;
    }
    
    serv->handle_recvAdResponse(sock, event, arg, INMOBI);
}


void *connectorServ::connectToOther(void *arg)
{
    connectorServ *serv = (connectorServ*) arg;    
    serv->pth1_base = event_base_new();

    connectorManager& cManager = serv->m_connector_manager;
    string& connectorIdentify = cManager.get_connectorIdentify();
    serv->m_throttle_manager.connectToThrottleManagerPortList(serv->m_zmq_connect,connectorIdentify,serv->pth1_base,handle_recvLoginHeartRsp,arg);
    serv->m_bc_manager.connectToBCList(serv->m_zmq_connect,connectorIdentify,serv->pth1_base,handle_recvLoginHeartRsp,arg);    
    
    event_base_dispatch(serv->pth1_base);   
}
void *connectorServ::sendLoginHeartToOther(void *arg)
{
    connectorServ *serv = (connectorServ*) arg;
    while(1)
    {        
        const string& ip = serv->m_connector_manager.get_connector_config().get_connectorIP();
        unsigned short manager_port = serv->m_connector_manager.get_connector_config().get_connectorManagerPort();
        serv->m_throttle_manager.sendLoginRegisterToThrottleList(ip,manager_port);
        serv->m_bc_manager.sendLoginHeartToBCList(ip,manager_port);
        sleep(serv->m_heartInterval);
    }
    
}
void *connectorServ::recvLoginHeartFromOther(void *arg)
{
    connectorServ *serv = (connectorServ*) arg;
    int hwm = 30000;
    struct event_base* 			pth3_base;
    pth3_base = event_base_new();
    eventArgment *event_arg = new eventArgment(pth3_base,arg);

    connectorManager& cManager = serv->m_connector_manager;
    const string& ip = cManager.get_connector_config().get_connectorIP();
    unsigned short managerPort = cManager.get_connector_config().get_connectorManagerPort();    
    serv->m_recvLoginHeartReqHandler = serv->m_zmq_connect.establishConnect(false,"tcp",ZMQ_ROUTER,ip.c_str(),managerPort,&serv->m_recvLoginHeartReqFd);
    zmq_setsockopt(serv->m_recvLoginHeartReqHandler,ZMQ_RCVHWM,&hwm,sizeof(hwm));

    struct event *recvLoginHeartEvent = event_new(pth3_base,serv->m_recvLoginHeartReqFd,EV_READ|EV_PERSIST,handle_recvLoginHeartReq,event_arg);
    event_add(recvLoginHeartEvent,NULL);
    event_base_dispatch(pth3_base);
    
}
void *connectorServ::recvAdRequestFromThrottle(void *arg)
{
    connectorServ *serv = (connectorServ*) arg;
    if(!serv) return NULL;
    struct event_base *pth4_base = event_base_new();

    serv->m_throttle_manager.connectToThrottlePubPortList(serv->m_zmq_connect,pth4_base,handle_recvAdRequest,arg);

    event_base_dispatch(pth4_base);
    
}
void *connectorServ::getTime(void *arg)
{
    connectorServ *serv = (connectorServ*) arg;    
    while(1)
    {
        sleep(3);
        if(serv->m_throttle_manager.dropDev())
        {
            serv->m_config.readConfig();
            //serv->m_config.display();            
            serv->m_throttle_manager.update_throttleList(serv->m_config.get_throttle_info().get_throttleConfigList());
        }
        vector<string> unsubKeyList;
        const string& connectorIP = serv->m_connector_manager.get_connector_config().get_connectorIP();
        unsigned short conManagerPort = serv->m_connector_manager.get_connector_config().get_connectorManagerPort();
        if(serv->m_bc_manager.dropDev(connectorIP,conManagerPort,unsubKeyList))
        {
            for(auto it= unsubKeyList.begin();it != unsubKeyList.end();it++)
            {
                string& key = *it;                
                serv->m_zmqSubKey_manager.erase_subKey(key);
                serv->m_throttle_manager.erase_throSubKey(key);
            }            
            serv->m_config.readConfig();
            serv->m_bc_manager.update_bcList(serv->m_config.get_bc_info().get_bcConfigList());
        }
    }
}
void *connectorServ::checkTimeOutCommMsg(void *arg)
{
    connectorServ *serv = (connectorServ*) arg;    
    while(1)
    {
        timeval tv;
        memset(&tv,0,sizeof(struct timeval));
        gettimeofday(&tv,NULL);
        long long cur_timeMs = tv.tv_sec*1000 + tv.tv_usec/1000;
        long long pre_timeMs = tv.tv_sec*1000;        
        if(serv->m_config.get_enChinaTelecom())
        {
            if((pre_timeMs-CLOCK_TIME)%TELECODEUPDATE_TIME == 0) // update telecom passwd at 01:00:00 (Beijing TIME)everyday
            {
                if(!serv->m_dspManager.getCeritifyCodeFromChinaTelecomDSP())
                {
                    g_worker_logger->error("UPDATE DSP PASSWORD FAIL ... ");                
                }  
            }
        }
		
		if((pre_timeMs-DAWN_TIME)%TELECODEUPDATE_TIME == 0) // update curFlowCount  at 00:00:00 (Beijing TIME)everyday
		{
			if(serv->m_config.get_enChinaTelecom())
				serv->m_dspManager.getChinaTelecomObject()->curFlowCountClean();
			if(serv->m_config.get_enGYIN())
				serv->m_dspManager.getGuangYinObject()->curFlowCountClean();
			if(serv->m_config.get_enSmaato())
				serv->m_dspManager.getSmaatoObject()->curFlowCountClean();
		       if(serv->m_config.get_enInMobi())
			       serv->m_dspManager.getInMobiObject()->curFlowCountClean();
		}
           
        serv->commMsgRecordList_lock.lock();        
        for(auto it = serv->commMsgRecordList.begin(); it != serv->commMsgRecordList.end(); )
        {            
            commMsgRecord* cmrObj = it->second;
            long long record_timeMs = cmrObj->tv.tv_sec*1000 + cmrObj->tv.tv_usec/1000;
            if(cur_timeMs - record_timeMs >= cmrObj->ttl)
            {
                g_worker_logger->trace("TIMEOUT DEL uuid: {0}", it->first);
                delete [] cmrObj->data;
                delete cmrObj;
                cmrObj = NULL;
                it = serv->commMsgRecordList.erase(it);                
            } 
            else
                it++;               
        }        
        serv->commMsgRecordList_lock.unlock();
        usleep(1000);   //1ms
    }
}
void *connectorServ::checkConnectNum(void *arg)
{
    connectorServ *serv = (connectorServ*) arg;   
    int TELE_maxConnectNum = 0;
    int GYIN_maxConnectNum = 0;
    int SMAATO_maxConnectNum = 0;
    int INMOBI_maxConnectNum = 0;
    struct connectDsp_t * con_t = (struct connectDsp_t *)malloc(sizeof(struct connectDsp_t));
    con_t->base = serv->m_base;
    con_t->arg = serv;
    if(serv->m_config.get_enChinaTelecom())
    {
        con_t->fn = handle_recvAdResponseTele;
        con_t->dspObj = serv->m_dspManager.getChinaTelecomObject();
        TELE_maxConnectNum = serv->m_dspManager.getChinaTelecomObject()->getMaxConnectNum(); 
    }
    if(serv->m_config.get_enGYIN())
    {        
        con_t->fn = handle_recvAdResponseGYin;
        con_t->dspObj = serv->m_dspManager.getGuangYinObject();
        GYIN_maxConnectNum = serv->m_dspManager.getGuangYinObject()->getMaxConnectNum();    
    }
    if(serv->m_config.get_enSmaato())
    {
        SMAATO_maxConnectNum = serv->m_dspManager.getSmaatoObject()->getMaxConnectNum();
    }
    if(serv->m_config.get_enInMobi())
    {
        con_t->fn = handle_recvAdResponseInMobi;
        con_t->dspObj = serv->m_dspManager.getInMobiObject();
        INMOBI_maxConnectNum = serv->m_dspManager.getInMobiObject()->getMaxConnectNum();
    }
    while(1)
    {        
        if(serv->m_config.get_enChinaTelecom())
        {
            int TELE_curConnectNum = serv->m_dspManager.getChinaTelecomObject()->getCurConnectNum();
            if(TELE_curConnectNum < TELE_maxConnectNum)
            {
                if(!serv->m_tConnect_manager.Run(serv->m_dspManager.getChinaTelecomObject()->addConnectToDSP, con_t))   //add task in taskPool success
                    serv->m_dspManager.getChinaTelecomObject()->connectNumIncrease();
                else
                    g_worker_logger->error("ADD TASK IN TASKPOOL FAIL");
            }
        }
        if(serv->m_config.get_enGYIN())
        {
            int GYIN_curConnectNum = serv->m_dspManager.getGuangYinObject()->getCurConnectNum();        
            if(GYIN_curConnectNum < GYIN_maxConnectNum)
            {            
                if(!serv->m_tConnect_manager.Run(serv->m_dspManager.getGuangYinObject()->addConnectToDSP, con_t))   //add task in taskPool success
                    serv->m_dspManager.getGuangYinObject()->connectNumIncrease();
                else
                    g_workerGYIN_logger->error("ADD TASK IN TASKPOOL FAIL");
            }        
        }        
        if(serv->m_config.get_enSmaato())
        {
            int SMAATO_curConnectNum = serv->m_dspManager.getSmaatoObject()->getCurConnectNum();        
            if(SMAATO_curConnectNum < SMAATO_maxConnectNum)
            {         
            	if(!serv->m_tConnect_manager.Run(serv->m_dspManager.getSmaatoObject()->smaatoAddConnectToDSP, serv->m_dspManager.getSmaatoObject()))    //add task in taskPool success
            	    serv->m_dspManager.getSmaatoObject()->connectNumIncrease();
            	else
            	    g_workerSMAATO_logger->error("ADD TASK IN TASKPOOL FAIL");
            }      
        }
        if(serv->m_config.get_enInMobi())
        {
            int INMOBI_curConnectNum = serv->m_dspManager.getInMobiObject()->getCurConnectNum();
            if(INMOBI_curConnectNum < INMOBI_maxConnectNum)
            {
                if(!serv->m_tConnect_manager.Run(serv->m_dspManager.getInMobiObject()->inmobiAddConnectToDSP, serv->m_dspManager.getInMobiObject()))     //add task in taskPool success
                    serv->m_dspManager.getInMobiObject()->connectNumIncrease();
                else
                    g_workerINMOBI_logger->error("ADD TASK IN TASKPOOL FAIL");
            }
        }
        usleep(1000);     //1ms
    }
    free(con_t);
}
void connectorServ::masterRun()
{       
    m_masterPushHandler = m_zmq_connect.establishConnect(false, "ipc", ZMQ_PUSH, "masterworker" , NULL);
    m_masterPullHandler = m_zmq_connect.establishConnect(false, "ipc", ZMQ_PULL, "workermaster" , &m_masterPullFd);    
    if((m_masterPushHandler == NULL)||(m_masterPullHandler == NULL))
    {
        g_manager_logger->emerg("[master push or pull exception]");
        exit(1);
    }
    g_manager_logger->info("[master push and pull success]");    

    pthread_t pth1;
    pthread_t pth2;
    pthread_t pth3;
    pthread_t pth4;
    pthread_t pth5;    

    pthread_create(&pth1,NULL,connectToOther,this);
    pthread_create(&pth2,NULL,sendLoginHeartToOther,this);
    pthread_create(&pth3,NULL,recvLoginHeartFromOther,this);
    pthread_create(&pth4,NULL,recvAdRequestFromThrottle,this); 
    pthread_create(&pth5,NULL,getTime,this);
    
}

void connectorServ::workerRun()
{
    pid_t pid = getpid();
    g_worker_logger = spdlog::daily_logger_mt("worker", "logs/debugfile", true); 
    g_worker_logger->set_level(m_logLevel);  
    g_workerGYIN_logger = spdlog::daily_logger_mt("GYIN", "logs/GYINdebugfile", true); 
    g_workerGYIN_logger->set_level(m_logLevel);
    g_workerSMAATO_logger = spdlog::daily_logger_mt("SMAATO", "logs/SMAATOdebugfile", true);
    g_workerSMAATO_logger->set_level(m_logLevel);
    g_workerINMOBI_logger= spdlog::daily_logger_mt("INMOBI", "logs/INMOBIdebugfile", true);
    g_workerINMOBI_logger->set_level(m_logLevel);
    
    g_worker_logger->info("worker start:{0:d}", getpid());
    
    m_zmq_connect.init();
    
    m_base = event_base_new();
    struct event * hup_event = evsignal_new(m_base, SIGHUP, hupSigHandler, this);
    struct event * int_event = evsignal_new(m_base, SIGINT, intSigHandler, this);
    struct event * term_event = evsignal_new(m_base, SIGTERM, termSigHandler, this);
    struct event * usr1_event = evsignal_new(m_base, SIGUSR1, usr1SigHandler, this);

    evsignal_add(hup_event, NULL);
    evsignal_add(int_event, NULL);
    evsignal_add(term_event, NULL);
    evsignal_add(usr1_event, NULL);

    m_workerPullHandler = m_zmq_connect.establishConnect(true, "ipc", ZMQ_PULL,  "masterworker",  &m_workerPullFd);
    m_workerPushHandler = m_zmq_connect.establishConnect(true, "ipc", ZMQ_PUSH,  "workermaster",  NULL);    
    if((m_workerPullHandler == NULL)||(m_workerPushHandler == NULL))
    {
        g_worker_logger->info("worker push or pull exception");
        exit(1); 
    }
    g_worker_logger->info("worker {0:d} push or pull success", pid);

    struct event *clientPullEvent = event_new(m_base, m_workerPullFd, EV_READ|EV_PERSIST, workerHandle_callback, this);
    event_add(clientPullEvent, NULL);

    int poolSize = m_connector_manager.get_connector_config().get_connectorThreadPoolSize();
    m_thread_manager.Init(10000, poolSize, poolSize);//thread pool init
    m_tConnect_manager.Init(10000, 50, 50);//connect pool init
    
    m_bc_manager.connectToBCListDataPort(m_zmq_connect);

        
    commMsgRecordList_lock.init();
    pthread_t pth1;
    pthread_t pth2;
    pthread_create(&pth1,NULL,checkTimeOutCommMsg,this);   
    pthread_create(&pth2,NULL,checkConnectNum,this);
    
    event_base_dispatch(m_base);    
    
}
void connectorServ::run()
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
    sigprocmask(SIG_SETMASK, &set, NULL);   //将set里面的信号阻塞掉，也就是只使能以上5个信号
                  
    struct itimerval timer;         //定时器
    timer.it_value.tv_sec = 1;      // 1 秒后将启动定时器
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 10;  // 启动定时器后每隔10秒将执行相应函数
    timer.it_interval.tv_usec = 0;          
    setitimer(ITIMER_REAL, &timer, NULL);   //ITIMER_REAL: 以系统真实的时间来计算，它送出SIGALRM信号
    
    while(true)
    {
        pid_t pid;
        if(num_children != 0)
        {  
            pid = wait(NULL);       //wait函数被SIGALRM信号中断返回-1
            if(pid != -1)
            {
                num_children--;
                g_manager_logger->info("child process exit:{0}", pid); 
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
                for (; it != m_workerList.end(); it++)
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
                    case 0:     //child
                    {   
                        close(pro->channel[0]);
                        is_child = 1; 
                        pro->pid = getpid();
                    }
                    break;
                    default:    //parent
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
    workerRun();
}

