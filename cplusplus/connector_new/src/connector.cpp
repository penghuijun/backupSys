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

vector<map<int,string>> SQL_MAP;
map<int,string> Creative_template;
map<int,string> CampaignMap;
#if 0
map<string,ContentCategory> AndroidContenCategoryMap;
map<string,ContentCategory> IOSContenCategoryMap;
map<int,ConnectionType> ConnectionTypeMap;
#endif


const int CHECK_COMMMSG_TIMEOUT = 100; // 100ms
const long long TELECODEUPDATE_TIME = 24*60*60*1000; // 24 hours
const long long CLOCK_TIME = 17*60*60*1000; //Beijing time : 01:00:00  -> GMT(Greenwich mean time): 17:00:00

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
    time_t timep;
    time(&timep);
    char *timeStr = ctime(&timep);
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
        g_worker_logger->debug("BidResponse.json parse success ");
        string str_id = root["id"].asString();
        //cout << "id = " << root["id"].asString() << endl;
        cmrObj = checkValidId(str_id);
        if(!cmrObj)
        {
            g_worker_logger->debug("Find BidRespnse id in commMsgRecordList fail ");
            return NULL;
        }
        g_worker_logger->debug("Find BidRespnse id in commMsgRecordList success ");
        request_commMsg = cmrObj->requestCommMsg;
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
                char str_w[16] = {0};
                char str_h[16] = {0};
                sprintf(str_w,"%d",bid[i]["w"].asInt());
                sprintf(str_h,"%d",bid[i]["h"].asInt());                
                mobile_creative->set_width(str_w);
                mobile_creative->set_height(str_h);    
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
        g_workerGYIN_logger->debug("Find BidRespnse id in commMsgRecordList fail ");
        return NULL;
    }
    g_workerGYIN_logger->debug("Find BidRespnse id in commMsgRecordList success ");
        
    request_commMsg = cmrObj->requestCommMsg;
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
        float GYIN_price = GYIN_bid.price();
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
        mobile_creative->set_adchanneltype(MobileAdResponse_AdChannelType_MOBILE_APP);

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

commMsgRecord* connectorServ::checkValidId(const string& str_id)
{   
    vector<commMsgRecord*>::iterator it = commMsgRecordList.begin();
    for( ; it != commMsgRecordList.end(); it++)
    {
        commMsgRecord* cmrObj = *it;
        CommonMessage reqCommMsg = cmrObj->requestCommMsg;
        const string& commMsg_data = reqCommMsg.data();
        MobileAdRequest mobile_request;
        mobile_request.ParseFromString(commMsg_data);

        if(!strcmp(str_id.c_str(),mobile_request.id().c_str()))
        {
            return cmrObj;            
        }        
    }
    return NULL;        
}
bool connectorServ::mutableAction(MobileAdRequest &mobile_request,MobileAdResponse_Action *mobile_action,Json::Value &action)
{
    //cout << "##### mutableAction !" << endl;
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
        
    //char *destContent = ReplaceStr(content.toStyledString().c_str(),"\"","\\\"");
    string str_content = content.toStyledString();
    replace(str_content,"\"","\\\"");
    replace(str_content,"'","\\'");
    replace(str_content,"\t"," ");
    replace(str_content,"\n","");
    replace(str_content,"\r","");
    replace(str_content,"/","\\/");
    cout << "str_content" << str_content << endl;
    mobile_action->set_content(str_content);    
    mobile_action->set_actiontype(str_acttype);
    mobile_action->set_inapp(str_inapp);       

    //printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,destContent);
    //delete [] destContent;
    //destContent = NULL;
    return true;
    
}

bool connectorServ::creativeAddEvents(MobileAdResponse_Creative  *mobile_creative,Json::Value &temp,string& nurl)
{
    //cout << "##### creativeAddEvents !" << endl;
    int temptype = temp["temtype"].asInt();                
    
    int id = 0;
    string RetCode;                            
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
                        //const char *sSrc = decodeStr.c_str();
                        //string sMatchStr = "${MY_IMAGE}";
                        //const char *sReplaceStr = creurl.c_str();
                        //char *sDest = ReplaceStr(sSrc,sMatchStr,sReplaceStr);    
                        replace(decodeStr,"${MY_IMAGE}",creurl); 
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));
                        mobile_creative->set_mediatypeid("1");
                        mobile_creative->set_mediasubtypeid("1");
                        
                        //printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,sDest);
                        //delete [] sDest;
                        //sDest = NULL;
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
                        //const char *sSrc = decodeStr.c_str();
                        //const char *sMatchStr1 = "${MY_TITLE}";
                        //const char *sMatchStr2 = "${MY_DESCRIPTION}";
                        //const char *sReplaceStr1 = title.c_str();
                        //const char *sReplaceStr2 = desc.c_str();                        
                        //char *sDest1 = ReplaceStr(sSrc,sMatchStr1,sReplaceStr1); 
                        //char *sDest2 = ReplaceStr(sDest1,sMatchStr2,sReplaceStr2); 
                        replace(decodeStr,"${MY_TITLE}",title);
                        replace(decodeStr,"${MY_DESCRIPTION}",desc);
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));  
                        mobile_creative->set_mediatypeid("1");
                        mobile_creative->set_mediasubtypeid("2");
                        //printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,sDest1);
                        //printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,sDest2);
                        //delete [] sDest1;
                        //delete [] sDest2;
                        //sDest1 = sDest2 = NULL;
                        
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
                        //const char *sSrc = decodeStr.c_str();
                        //const char *sMatchStr1 = "${MY_IMAGE}";
                        //const char *sMatchStr2 = "${MY_EXPAND_URL}";
                        //const char *sReplaceStr1 = creurl.c_str();
                        //const char *sReplaceStr2 = expurl.c_str();
                        //char *sDest1 = ReplaceStr(sSrc,sMatchStr1,sReplaceStr1); 
                        //char *sDest2 = ReplaceStr(sDest1,sMatchStr2,sReplaceStr2); 
                        replace(decodeStr,"${MY_IMAGE}",creurl);
                        replace(decodeStr,"${MY_EXPAND_URL}",expurl);
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));  
                        mobile_creative->set_mediatypeid("1");
                        mobile_creative->set_mediasubtypeid("1");
                        //printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,sDest1);
                        //printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,sDest2);
                        //delete [] sDest1;
                        //delete [] sDest2;
                        //sDest1 = sDest2 = NULL;
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
                        //const char *sSrc = decodeStr.c_str();
                        //const char *sMatchStr = "${MY_IMAGE}";
                        //const char *sReplaceStr = creurl.c_str();
                        //char *sDest = ReplaceStr(sSrc,sMatchStr,sReplaceStr);         
                        replace(decodeStr,"${MY_IMAGE}",creurl);
                        
                        mobile_creative->set_admarkup(UrlEncode(decodeStr));
                        mobile_creative->set_mediatypeid("1");
                        mobile_creative->set_mediasubtypeid("5");
                        //printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,sDest);
                        //delete [] sDest;
                        //sDest = NULL;
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
    if(temp.isMember("imurl"))
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
    
    //char *destContent = ReplaceStr(content.toStyledString().c_str(),"\"","\\\"");
    string str_content = content.toStyledString();
    replace(str_content,"\"","\\\"");
    replace(str_content,"'","\\'");
    replace(str_content,"\t"," ");
    replace(str_content,"\n","");
    replace(str_content,"\r","");
    replace(str_content,"/","\\/");
    cout << "str_content" << str_content << endl;
    
    mobile_action->set_content(str_content);
    mobile_action->set_actiontype(str_acttype);
    mobile_action->set_inapp("1");         

    //printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,destContent);
    //delete [] destContent;
    //destContent = NULL;
    return true;
}
bool connectorServ::GYIN_creativeAddEvents(MobileAdRequest &mobile_request,MobileAdResponse_Creative  *mobile_creative,Bid &GYIN_bid)
{
    MobileAdRequest_AdType type = mobile_request.type();
    int id = 0;
    string RetCode;
    AdmType GYIN_admtype = GYIN_bid.admtype();
    //string adm;
    
    switch(type)
    {
        case MobileAdRequest_AdType_BANNER:
            {
                id = 79;                
            }
            break;
        case MobileAdRequest_AdType_INTERSTITIAL:
            {
                id = 80;
            }
            break;
        default:
            break;
    }

    map<int,string>::iterator it = Creative_template.find(id);
    RetCode = it->second;                
    if(RetCode.empty() == false)
    {
        string decodeStr;    
        //bool deleteMem = false;
        
        decodeStr = UrlDecode(RetCode);                        
        //const char *sSrc = decodeStr.c_str();
        //const char *sMatchStr = "${MY_THIRD_HTML}";
        //const char *sReplaceStr = NULL;
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
                    

                    #if 0
                    char *destImg1 = ReplaceStr(img,"${SRC_URL}",src.c_str());
                    char *destImg2 = ReplaceStr(destImg1,"${W}",widthStr);
                    printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,destImg1);
                    delete [] destImg1;
                    char *destImg3 = ReplaceStr(destImg2,"${H}",heightStr);
                    printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,destImg2);
                    delete [] destImg2;
                    char *destImg4 = ReplaceStr(destImg3,"'","\\\'");
                    printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,destImg3);
                    delete [] destImg3;
                    char *destImg5 = ReplaceStr(destImg4,"/","\\/"); 
                    printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,destImg4);
                    delete [] destImg4;
                    destImg1 = destImg2 = destImg3 = destImg4 = NULL;
                   
                    sReplaceStr = destImg5;
                    cout << sReplaceStr << endl;
                    destImg5 = NULL;
                    
                    deleteMem = true;    
                    #endif
                   
                }
                break;
            default:
                break;
        }                    
        //char *sDest = ReplaceStr(sSrc,sMatchStr,sReplaceStr);  
        replace(third_html,"\"","\\\"");
        replace(third_html,"'","\\'");
        replace(third_html,"\t"," ");
        replace(third_html,"\n","");
        replace(third_html,"\r","");
        replace(third_html,"/","\\/");
        
        sReplaceStr.append("\"").append(third_html).append("\"");
        cout << sReplaceStr << endl;
                    
        replace(decodeStr,"${MY_THIRD_HTML}",sReplaceStr);
        #if 0
        if(deleteMem)
        {
            printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,sReplaceStr);
            delete [] sReplaceStr;            
            sReplaceStr = NULL;
        }            
        #endif
        mobile_creative->set_admarkup(UrlEncode(decodeStr));
        mobile_creative->set_mediatypeid("1");
        mobile_creative->set_mediasubtypeid("1");
        //printf("%s:%d:%s: delete(0x%x)\n",__FILE__,__LINE__,__func__,sDest);
        //delete [] sDest;
        //sDest = NULL;
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


char* memstr(char* full_data, int full_data_len, const char* substr)  
{  
    if (full_data == NULL || full_data_len <= 0 || substr == NULL) {  
        return NULL;  
    }  
  
    if (*substr == '\0') {  
        return NULL;  
    }  
  
    int sublen = strlen(substr);  
  
    int i;  
    char* cur = full_data;  
    int last_possible = full_data_len - sublen + 1;  
    for (i = 0; i < last_possible; i++) {  
        if (*cur == *substr) {  
            //assert(full_data_len - i >= sublen);  
            if (memcmp(cur, substr, sublen) == 0) {  
                //found  
                return cur;  
            }  
        }  
        cur++;  
    }  
  
    return NULL;  
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

int connectorServ::getHttpRspData(char *Dest,char *Src)
{   
    string temp = Src;
    if(temp.empty())
    {        
        g_worker_logger->debug("HTTP RSP DATA is empty");
        g_workerGYIN_logger->debug("HTTP RSP DATA is empty");
        return 0;
    }
    if(temp.find("chunked") != temp.npos)   //China Telecom : JSON
    {
        char *Src_end = Src+strlen(Src); //avoid point overflow
        char *chunked_start = memstr(Src,strlen(Src),"\r\n\r\n");    
        if(chunked_start == NULL)
        {
            return 0;
        }
        chunked_start += 4; 
        if(chunked_start >= Src_end) //overflow
            return 0;
        int partLen;
        int len = getPartLen(chunked_start,partLen);   
        if(len == -1)
        {
            g_worker_logger->debug("TELE HTTP RSP : chunked len novalid");
            return 0;
        }
        chunked_start = chunked_start+len+2;  
        if(chunked_start >= Src_end) //overflow
            return 0;
        int tempLen = 0;
        while(partLen)
        {
            tempLen = getPartData(Dest,chunked_start);
            if(tempLen == -1)
            {
                g_worker_logger->debug("TELE HTTP RSP : jsonData novalid");
                break;
            }
            chunked_start = chunked_start+tempLen+2;
            if(chunked_start >= Src_end) //overflow
                break;
            len = getPartLen(chunked_start,partLen);
            if(len == -1)
                break;
            chunked_start = chunked_start+len+2;     
            if(chunked_start >= Src_end) //overflow
                break;
        }
        return strlen(Dest);
    }
    else if((temp.find("200 OK") != temp.npos) && (temp.find("Content-Length") != temp.npos))  //GYIN : protobuf
    {
        int pos = temp.find("Content-Length");
    	if(!(pos>=0))
        {        
            g_workerGYIN_logger->debug("GYIN HTTP RSP : Find proto data Failed !");
            return 0;
        }   
    	char *ch = Src+pos+15;
    	int len = 0;
    	while((*ch != '\r')&&(*ch != '\0'))
    	{
    		ch++;
    		len++;		    		
    	}
    	char *con_len_str = new char[len+1];
    	strncpy(con_len_str,Src+pos+15,len);
        con_len_str[len] = '\0';    	
    	int content_len = atoi(con_len_str);       	
        delete [] con_len_str;	    	
    	char *dataStart = memstr(Src,strlen(Src),"\r\n\r\n");
    	memcpy(Dest,dataStart+4,content_len); 
        Dest[content_len] = '\0';
        g_workerGYIN_logger->debug("GYIN HTTP RSP: 200 OK");
        g_workerGYIN_logger->debug("Content-Length: {0:d}",content_len);
        return content_len;
    }
    else if(temp.find("204 No Content") != temp.npos)   //GYIN
    {
        g_workerGYIN_logger->debug("GYIN HTTP RSP: 204 No Content");
        return 0;
    }
    
    
    #if 0
    int pos = temp.find("content-length");
	if(!(pos>=0))
    {        
        g_worker_logger->debug("httpRecvData : Find json data Failed !");
        return NULL;
    }   
	char *ch = Src+pos+15;
	int len = 0;
	while(*ch != '\r')
	{
		ch++;
		len++;		
	}
	char *con_len_str = new char[len+1];
	strncpy(con_len_str,Src+pos+15,len);
    con_len_str[len] = '\0';
	//cout << con_len_str << endl;
	int content_len = atoi(con_len_str);
	//cout << content_len << endl;
    delete [] con_len_str;	
	len = strlen(Src);
	strncpy(Src,Src+len-content_len,content_len); 
    Src[content_len] = '\0';
    #endif
    //return Src;
}

void connectorServ::hashGetBCinfo(string& uuid,string& bcIP,unsigned short& bcDataPort)
{
    m_bc_manager.hashGetBCinfo(uuid,bcIP,bcDataPort);
}

void connectorServ::handle_BidResponseFromDSP(dataType type,char *data,int dataLen)
{
    int responseDataLen = 0;    
    string uuid;    //for hash get bc info
    char *responseDataStr;
    switch(type)
    {
        case DATA_JSON:
            responseDataStr = convertTeleBidResponseJsonToProtobuf(data,dataLen,responseDataLen,uuid);
            break;
        case DATA_PROTOBUF:
            responseDataStr = convertGYinBidResponseProtoToProtobuf(data,dataLen,responseDataLen,uuid);
            break;
        default:
            break;
    }    
    
    if(responseDataStr && (responseDataLen > 0))
    {
        switch(type)
        {
            case DATA_JSON:      
                if(m_config.get_logTeleRsp())
                {
                    displayCommonMsgResponse(g_worker_logger,responseDataStr,responseDataLen);   
                }                
                break;
            case DATA_PROTOBUF:         
                if(m_config.get_logGYINRsp())
                {
                    displayCommonMsgResponse(g_workerGYIN_logger,responseDataStr,responseDataLen); 
                }                  
                break;
            default:
                break; 
        }
        string bcIP;
        unsigned short bcDataPort;
        hashGetBCinfo(uuid,bcIP,bcDataPort);   
        #if 0
        int ssize = m_bc_manager.sendAdRspToBC(bcIP, bcDataPort, responseDataStr, responseDataLen, ZMQ_NOBLOCK);
        if(ssize > 0)
        {
            g_worker_logger->debug("send AdResponse to BC: {0:d}, {1}, {2}, {3:d} success \r\n",ssize, uuid, bcIP, bcDataPort);
        }
        else
        {
            g_worker_logger->error("send AdResponse to BC:{0}, {1:d} failed \r\n",bcIP, bcDataPort);
        }
        #endif
        delete [] responseDataStr;
    }
    else
    {
        g_worker_logger->debug("no valid BidResponse generate \r\n");   
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
        g_worker_logger->debug("get string from SQLMAP success!");
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
    g_worker_logger->debug("MobileAdRequest.proto->TeleBidRequest.json success");
    
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
bool connectorServ::GYin_AdReqProtoMutableDev(Device *device,const MobileAdRequest& mobile_request)
{
    const MobileAdRequest_Device& request_dev = mobile_request.device();     
    const MobileAdRequest_GeoInfo& request_geo = mobile_request.geoinfo();        
    
    vector<string> str_buf;
    if(getStringFromSQLMAP(str_buf,request_dev,request_geo))
    {
        g_workerGYIN_logger->debug("get string from SQLMAP success!");
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
    banner->set_w(atoi(mobile_request.adspacewidth().c_str()));
    banner->set_h(atoi(mobile_request.adspaceheight().c_str()));
    
    //btype : not support adtype
    banner->add_btype(IFRAME); 
    banner->add_btype(JS);

    //App
    App *app;    
    app = bidRequest.mutable_app();
    if(!GYin_AdReqProtoMutableApp(app,mobile_request))
        return false;

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
    g_workerGYIN_logger->debug("MobileAdRequest.proto->GYinBidRequest.protobuf success , Length : {0:d}",length);

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
        
        /*
                *send to CHINA TELECOM
                */                
        if(m_config.get_enChinaTelecom()) 
        {
            string reqTeleJsonData;
            if(convertProtoToTeleJson(reqTeleJsonData, mobile_request))
            {
                 //callback func: handle_recvAdResponseTele active by socket EV_READ event                
                if(!m_dspManager.sendAdRequestToChinaTelecomDSP(m_base, reqTeleJsonData.c_str(), strlen(reqTeleJsonData.c_str()), m_config.get_logTeleReq(),handle_recvAdResponseTele, this))
                {
                    g_worker_logger->debug("POST TO TELE fail uuid : {0}",uuid);            
                }
                g_worker_logger->debug("POST TO TELE success uuid : {0} \r\n",uuid);  
            }
            else
                 g_worker_logger->debug("convertProtoToTeleJson Failed ");  
        }
        
        
        /*
                *send to GYIN
                */
        if(m_config.get_enGYIN())
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
                if(!m_dspManager.sendAdRequestToGuangYinDSP(m_base,buf,length,handle_recvAdResponseGYin,this))
                {
                    g_workerGYIN_logger->debug("POST TO GYIN fail uuid : {0}",uuid); 
                }
                else
                    g_workerGYIN_logger->debug("POST TO GYIN success uuid : {0} \r\n",uuid); 
                delete [] buf;
            }     
            else
                g_workerGYIN_logger->debug("convertProtoToGYinProto Failed ");  
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
        timeval tv;
        memset(&tv,0,sizeof(struct timeval));
        gettimeofday(&tv,NULL);
        struct commMsgRecord* obj = new commMsgRecord();
        obj->requestCommMsg = request_commMsg;
        obj->tv = tv;
        serv->commMsgRecordList_lock.lock();
        serv->commMsgRecordList.push_back(obj);
        serv->commMsgRecordList_lock.unlock();
        //serv->global_requestCommMsg.ParseFromArray(buf+PUBLISHKEYLEN_MAX, dataLen-PUBLISHKEYLEN_MAX);
        const string& tbusinessCode = request_commMsg.businesscode();
            
        if(tbusinessCode == serv->m_mobileBusinessCode)
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
        if(managerProtocol_messageType_LOGIN_RSP == rspType)
            g_manager_logger->info("[login rsp][connector -> throttle]:{0}, {1:d}", connectorIP, connectorPort);
        else if(managerProtocol_messageType_HEART_RSP == rspType)
            g_manager_logger->info("[heart rsp][connector -> throttle]:{0}, {1:d}", connectorIP, connectorPort);
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
           g_master_logger->debug("recv AdRequest from throttle:{0:d}", data_len);
           if((sub_key_len>0) &&(data_len>0))
           {
               serv->sendToWorker(subscribe_key, sub_key_len, data, data_len);
           }
           zmq_msg_close(&subscribe_key_part);
           zmq_msg_close(&data_part);
        }
    }
}
void connectorServ::handle_recvAdResponseTele(int sock,short event,void *arg)
{   
    connectorServ *serv = (connectorServ*)arg;
    if(serv==NULL) 
    {
        g_worker_logger->emerg("handle_recvAdResponseTele param is null");
        return;
    }
   
    struct event *listenEvent = serv->m_dspManager.getChinaTelecomObject()->findListenObject(sock)->_event;
    char *recv_str = new char[4096];
    memset(recv_str,0,4096*sizeof(char));    
    
    int ret = recv(sock, recv_str, 4096*sizeof(char), 0);
    if (ret == 0)
    {
        g_worker_logger->debug("server TELE CLOSE_WAIT ...");
        serv->m_dspManager.getChinaTelecomObject()->eraseListenObject(sock);
        close(sock);
        event_del(listenEvent);
        delete [] recv_str;
        return;
    }
    if (ret == -1)
    {
        g_worker_logger->emerg("recv AdResponse fail !");
        serv->m_dspManager.getChinaTelecomObject()->eraseListenObject(sock);
        close(sock);
        event_del(listenEvent);
        delete [] recv_str;
        return;
    }
    
    g_worker_logger->debug("RECV TELE HTTP RSP by PID: {0:d}",getpid());
    if(serv->m_config.get_logTeleHttpRsp())
    {
        g_worker_logger->debug("\r\n{0}",recv_str);	
    }        
    char * jsonData = new char[4048];
    memset(jsonData,0,4048*sizeof(char));
    int dataLen = serv->getHttpRspData(jsonData, recv_str);       
    delete [] recv_str;
	if(dataLen == 0)
    {
        g_worker_logger->error("Get json data from HTTP response failed !");
        delete [] jsonData;
        return;
    }   
    g_worker_logger->debug("BidRsponse : {0}",jsonData);    
    serv->handle_BidResponseFromDSP(DATA_JSON,jsonData,dataLen);   
    delete [] jsonData;
    
}

void connectorServ::handle_recvAdResponseGYin(int sock,short event,void *arg)
{   
    connectorServ *serv = (connectorServ*)arg;
    if(serv==NULL) 
    {
        g_workerGYIN_logger->emerg("handle_recvAdResponseGYin param is null");
        return;
    }
   
    struct event *listenEvent = serv->m_dspManager.getGuangYinObject()->findListenObject(sock)->_event;
    char *recv_str = new char[4096];
    memset(recv_str,0,4096*sizeof(char));    
    
    int ret = recv(sock, recv_str, 4096*sizeof(char), 0);
    if (ret == 0)
    {
        g_workerGYIN_logger->debug("server GYIN CLOSE_WAIT ...");
        serv->m_dspManager.getGuangYinObject()->eraseListenObject(sock);
        close(sock);
        event_del(listenEvent);
        delete [] recv_str;
        return;
    }
    if (ret == -1)
    {
        g_workerGYIN_logger->emerg("recv AdResponse fail !");
        serv->m_dspManager.getGuangYinObject()->eraseListenObject(sock);
        close(sock);
        event_del(listenEvent);
        delete [] recv_str;
        return;
    }
    
    g_workerGYIN_logger->debug("RECV GYIN HTTP RSP by PID: {0:d}",getpid());    
    char * protoData = new char[4048];
    memset(protoData,0,4048*sizeof(char));
    int dataLen = serv->getHttpRspData(protoData, recv_str);        
    delete [] recv_str;
	if(dataLen == 0)
    {        
        delete [] protoData;
        return;
    }   
    if(serv->m_config.get_logGYINHttpRsp())
    {
        serv->displayGYinBidResponse(protoData, dataLen);
    }    
    serv->handle_BidResponseFromDSP(DATA_PROTOBUF,protoData,dataLen);   
    delete [] protoData;    
    
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
        if((pre_timeMs-CLOCK_TIME)%TELECODEUPDATE_TIME == 0) // update telecom passwd at 01:00:00 (Beijing TIME)everyday
        {
            if(!serv->m_dspManager.getCeritifyCodeFromChinaTelecomDSP())
            {
                g_worker_logger->error("UPDATE DSP PASSWORD FAIL ... ");                
            }  
        }
        
        serv->commMsgRecordList_lock.lock();
        vector<commMsgRecord*>::iterator it = serv->commMsgRecordList.begin();
        for(; it != serv->commMsgRecordList.end(); )
        {            
            commMsgRecord* cmrObj = *it;
            long long record_timeMs = cmrObj->tv.tv_sec*1000 + cmrObj->tv.tv_usec/1000;
            if(cur_timeMs - record_timeMs >= CHECK_COMMMSG_TIMEOUT)
            {
                delete cmrObj;
                cmrObj = NULL;
                it = serv->commMsgRecordList.erase(it);
            } 
            else
                it++;               
        }        
        serv->commMsgRecordList_lock.unlock();
        sleep(1);
    }
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
    g_worker_logger = spdlog::rotating_logger_mt("worker", "logs/debugfile", 1048576*500, 3, true); 
    g_worker_logger->set_level(m_logLevel);  
    g_workerGYIN_logger = spdlog::rotating_logger_mt("GYIN", "logs/GYINdebugfile", 1048576*500, 3, true); 
    g_workerGYIN_logger->set_level(m_logLevel);
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

    m_dspManager.init();
    
    m_bc_manager.connectToBCListDataPort(m_zmq_connect);

        
    commMsgRecordList_lock.init();
    pthread_t pth;
    pthread_create(&pth,NULL,checkTimeOutCommMsg,this);       
    
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
    sigprocmask(SIG_SETMASK, &set, NULL);
                  
    struct itimerval timer;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 10;
    timer.it_interval.tv_usec = 0;          
    setitimer(ITIMER_REAL, &timer, NULL);
    
    while(true)
    {
        pid_t pid;
        if(num_children != 0)
        {  
            pid = wait(NULL);
            if(pid != -1)
            {
                num_children--;
                g_manager_logger->info("chil process exit:{0}", pid); 
                updataWorkerList(pid);
            }

            if (srv_graceful_end || srv_ungraceful_end)//CTRL+C ,killall throttle
            {
                cout << "<<<<<<planing to excute kill command !" << endl;
                g_manager_logger->info("srv_graceful_end"); 
                if (num_children == 0)
                {
                    break;    //get out of while(true)
                }

                auto it = m_workerList.begin();
                for (it = m_workerList.begin(); it != m_workerList.end(); it++)
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
                    case 0:
                    {   
                        close(pro->channel[0]);
                        is_child = 1; 
                        pro->pid = getpid();
                    }
                    break;
                    default:
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

