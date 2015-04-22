/*
  *bcConfig
  *auth:yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#ifndef __ADCONFIG_H__
#define __ADCONFIG_H__
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include "spdlog/spdlog.h"
using namespace std;
using namespace spdlog;
#define BUF_SIZE 4096

extern shared_ptr<spdlog::logger> g_manager_logger;
struct exBidderInfo
{
	vector<string> subKey;
	string exBidderIP;
	unsigned short exBidderPort;
	unsigned short connectNum;
};

class throttleConfig
{
public:
	throttleConfig(){}
	throttleConfig(int id, string& ip, unsigned short adPort, unsigned short pubPort, unsigned short managerPort, unsigned short workerNum)
	{
		set_throttleID(id);
		set_throttleIP(ip);
		set_throttleAdPort(adPort);
		set_throttlePubPort(pubPort);
		set_throttleWorkerNum(workerNum);
		set_throttleManagerPort(managerPort);
	}

	void set(throttleConfig *throttle)
	{
		if(throttle)
		{
			set_throttleID(throttle->get_throttleID());
			set_throttleIP(throttle->get_throttleIP());
			set_throttleAdPort(throttle->get_throttleAdPort());
			set_throttlePubPort(throttle->get_throttlePubPort());
			set_throttleWorkerNum(throttle->get_throttleWorkerNum());	
			set_throttleManagerPort(throttle->get_throttleManagerPort());
		}
	}
	void set_throttleID(int id){m_throttleID=id;}
	void set_throttleIP(string &ip){m_throttleIP=ip;}
	void set_throttleAdPort(unsigned short port){m_throttleAdPort = port;}
	void set_throttlePubPort(unsigned short port){m_throttlePubPort = port;}
	void set_throttleManagerPort(unsigned short port){m_throttleManagerPort = port;}
	void set_throttleWorkerNum(unsigned short workerNum){m_throttleWorkerNum = workerNum;}
	int  get_throttleID(){return m_throttleID;}
	string&  get_throttleIP(){return m_throttleIP;}
	unsigned short get_throttleAdPort(){return m_throttleAdPort;}
	unsigned short get_throttlePubPort(){return m_throttlePubPort;}
	unsigned short get_throttleWorkerNum() const{return m_throttleWorkerNum;}
	unsigned short get_throttleManagerPort(){return m_throttleManagerPort;}
	void display()
	{
			g_manager_logger->info("{0:d}\t{1}\t{2:d}\t{3:d}\t{4:d}\t{5:d}",get_throttleID(), get_throttleIP(), get_throttleAdPort()
				, get_throttlePubPort(), get_throttleManagerPort(), get_throttleWorkerNum());
	}
	~throttleConfig(){}
private:
	int            m_throttleID;
	string         m_throttleIP;
	unsigned short m_throttleAdPort;
	unsigned short m_throttlePubPort;
	unsigned short m_throttleManagerPort;
	unsigned short m_throttleWorkerNum;
};

class throttleInformation
{
public:
	throttleInformation(){}
	void add_throttleConfig(int id, string& ip, unsigned short adPort, unsigned short pubPort, unsigned short managerPort, unsigned short workerNum)
	{
		throttleConfig *throttle = new throttleConfig(id, ip, adPort, pubPort, managerPort, workerNum);
		m_throttleConfigList.push_back(throttle);	
	}
	void add_throttleConfig(throttleConfig* throttle)
	{
		if(throttle) m_throttleConfigList.push_back(throttle);	
	}
	bool string_find(string& str1, const char* str2);

	void throttleInformation_erase()
	{
		for(auto it = m_throttleConfigList.begin(); it != m_throttleConfigList.end();)
		{
			throttleConfig *throttle = *it;
			delete throttle;
			it = m_throttleConfigList.erase(it);
		}

	}
	void display()
	{
		g_manager_logger->info("======throttle infomation begin======");
		g_manager_logger->info("id\tip\t\tadport\tpubport\tmanaPort\tworkerNum");

		for(auto it = m_throttleConfigList.begin(); it != m_throttleConfigList.end(); it++)
		{
			throttleConfig *throttle = *it;
			if(throttle) throttle->display();
		}
		g_manager_logger->info("======throttle infomation end======");
	}

	void set_logLevel(int logLevel)
	{
		m_logLevel = logLevel;
	}
	
	int get_logLevel()
	{
		return m_logLevel;
	}
	
	const vector<throttleConfig*>& get_throttleConfigList() const{return m_throttleConfigList;}
	~throttleInformation()
	{
		throttleInformation_erase();
	}
private:
	
	vector<throttleConfig*> m_throttleConfigList;
	int                     m_logLevel = 0;
};

class bidderConfig
{
public:
	bidderConfig(){}
	bidderConfig(int id, string& ip, unsigned short loginPort, unsigned short workerNum, unsigned short poolSize, unsigned short convence)
	{
		set_bidderID(id);
		set_bidderIP(ip);
		set_bidderLoginPort(loginPort);
		set_bidderWorkerNum(workerNum);
		set_bidderThreadPoolSize(poolSize);
		set_bidderConverceNum(convence);
	}
	void set(bidderConfig* config)
	{
		if(config)
		{
			set_bidderID(config->get_bidderID());
			set_bidderIP(config->get_bidderIP());
			set_bidderLoginPort(config->get_bidderLoginPort());
			set_bidderWorkerNum(config->get_bidderWorkerNum());
			set_bidderThreadPoolSize(config->get_bidderThreadPoolSize());
			set_bidderConverceNum(config->get_bidderConvenceNum());
		}
	}
	void set_bidderID(int id){m_bidderID=id;}
	void set_bidderIP(string &ip){m_bidderIP=ip;}
	void set_bidderLoginPort(unsigned short port){m_bidderLoginPort= port;}
	void set_bidderWorkerNum(unsigned short workerNum){m_bidderWorkerNum= workerNum;}
	void set_bidderThreadPoolSize(unsigned short size){m_bidderThreadPoolSize = size;}
	void set_bidderConverceNum(unsigned convence){m_bidderConvenceNum = convence;}
	
	int  get_bidderID(){return m_bidderID;}
	string&  get_bidderIP(){return m_bidderIP;}
	unsigned short get_bidderLoginPort(){return m_bidderLoginPort;}
	unsigned short get_bidderWorkerNum(){return m_bidderWorkerNum;}
	unsigned short get_bidderThreadPoolSize() {return m_bidderThreadPoolSize;}
	unsigned short get_bidderConvenceNum(){return m_bidderConvenceNum;}
	void display()
	{
		g_manager_logger->info("{0:d}\t{1}\t{2:d}\t{3:d}\t{4:d}\t{5:d}",get_bidderID(), get_bidderIP(), get_bidderLoginPort()
				, get_bidderWorkerNum(), get_bidderThreadPoolSize(), get_bidderConvenceNum());

	}
	~bidderConfig(){}
	
private:
	int            m_bidderID;
	string         m_bidderIP;
	unsigned short m_bidderLoginPort;
	unsigned short m_bidderWorkerNum;
	unsigned short m_bidderThreadPoolSize;
	unsigned short m_bidderConvenceNum=0;
};

class bidderInformation
{
public:
	bidderInformation(){}
	bidderInformation(string& redisIP, unsigned short redisPort)
	{
		m_redis_ip = redisIP;
		m_redis_port = redisPort;
	}
	void set_redis_ip(string& ip)
	{
		m_redis_ip = ip;
	}
	void set_redis_port(unsigned short port)
	{
		m_redis_port = port;
	}
	void set_redis(string& ip, unsigned short port)
	{
		set_redis_ip(ip);
		set_redis_port(port);
	}
	void add_bidderConfig(int id, string& ip, unsigned short loginPort, unsigned short workerNum, unsigned short poolSize, unsigned short convence)
	{
		bidderConfig *bidder = new bidderConfig(id, ip, loginPort, workerNum, poolSize, convence);
		m_bidderConfigList.push_back(bidder);	
	}
	void add_bidderConfig(bidderConfig *bidder)
	{
		m_bidderConfigList.push_back(bidder);	
	}

	void bidderInformation_erase()
	{
		for(auto it = m_bidderConfigList.begin(); it != m_bidderConfigList.end();)
		{
			bidderConfig *bidder = *it;
			delete bidder;
			it = m_bidderConfigList.erase(it);
		}
	}
	void display()
	{
	    g_manager_logger->info("======bidder infomation begin======");
	    g_manager_logger->info("id\tip\t\tloginport\tworkerNum\tthreadPoolsize\tcoverceNum");
		for(auto it = m_bidderConfigList.begin(); it != m_bidderConfigList.end(); it++)
		{
			bidderConfig *bidder = *it;
			if(bidder) bidder->display();
		}
		g_manager_logger->info("bidder target redis: {0}\t{1:d}", m_redis_ip, m_redis_port);
	    g_manager_logger->info("======bidder infomation end======");
	}

	const string &get_redis_ip() const{return m_redis_ip;}
	unsigned short get_redis_port() const{return m_redis_port;}

	int    get_logLevel(){return m_logLevel;}
	void   set_logLevel(int level){m_logLevel = level;}
	const vector<bidderConfig*>& get_bidderConfigList() const{return m_bidderConfigList;}
	~bidderInformation()
	{
		bidderInformation_erase();
	}
private:
	vector<bidderConfig*> m_bidderConfigList;
	string                m_redis_ip;
	unsigned short        m_redis_port;
	int                   m_logLevel = 0;
};


class externbidConfig
{
public:
	externbidConfig(){}
	externbidConfig(int id,const string& ip, unsigned short port, unsigned short connectCnt, const string& protocol
		, const string& format)
	{
		set_id(id);
		set_ip(ip);
		set_port(port);
		set_connectCnt(connectCnt);
		set_protocol(protocol);
		set_dataFormate(format);
	}
	void set(externbidConfig* config)
	{
		if(config)
		{
			set_id(config->get_id());
			set_ip(config->get_ip());
			set_port(config->get_port());
			set_connectCnt(config->get_connectCnt());
			set_protocol(config->get_protocol());
			set_dataFormate(config->get_dataFormat());
		}
	}
	void set_id(int id){m_id = id;}
	void set_ip(const string &ip){m_ip = ip;}
	void set_port(unsigned short port){m_port = port;}
	void set_connectCnt(unsigned short cnt){m_connectCnt = cnt;}
	void set_protocol(const string& protocol){m_protocol = protocol;}
	void set_dataFormate(const string& format){m_dataFormat = format;}
	
	int  get_id() const{return m_id;}
	const string&  get_ip() const{return m_ip;}
	unsigned short get_port()const{return m_port;}
	unsigned short get_connectCnt()const{return m_connectCnt;}
	const string& get_protocol()const {return m_protocol;}
	const string& get_dataFormat()const{return m_dataFormat;}
	void display()
	{
		g_manager_logger->info("{0:d}\t{1}\t{2:d}\t{3:d}\t{4}\t{5}",get_id(), get_ip(), get_port()
				, get_connectCnt(), get_protocol(), get_dataFormat());

	}
	~externbidConfig(){}
	
private:
	int            m_id;
	string         m_ip;
	unsigned short m_port;
	unsigned short m_connectCnt;
	string         m_protocol;
	string         m_dataFormat;
};

class externbidInformation
{
public:
	externbidInformation(){}
	void add_bidderConfig(int id,const string& ip, unsigned short port, unsigned short connectCnt, const string& protocol
		, const string& format)
	{
		externbidConfig *bidder = new externbidConfig(id, ip, port, connectCnt, protocol, format);
		m_exBidConfigList.push_back(bidder);	
	}
	void add_bidderConfig(externbidConfig *bidder)
	{
		m_exBidConfigList.push_back(bidder);	
	}

	void bidderInformation_erase()
	{
		for(auto it = m_exBidConfigList.begin(); it != m_exBidConfigList.end();)
		{
			delete *it;
			it = m_exBidConfigList.erase(it);
		}
	}
	void display()
	{
	    g_manager_logger->info("======exBid infomation begin======");
	    g_manager_logger->info("id\tip\t\tport\tconnectCnt\tprotocol\tformat");
		for(auto it = m_exBidConfigList.begin(); it != m_exBidConfigList.end(); it++)
		{
			externbidConfig *bidder = *it;
			if(bidder) bidder->display();
		}
	    g_manager_logger->info("======exBid infomation end======");
	}
	const vector<externbidConfig*>& get_exbidConfigList() const{return m_exBidConfigList;}
	~externbidInformation()
	{
		bidderInformation_erase();
	}
private:
	vector<externbidConfig*> m_exBidConfigList;
};


class connectorConfig
{
public:
	connectorConfig(){}
	connectorConfig(int id,const string& ip, unsigned short managerPort, unsigned short workerNum, unsigned short poolSize)
	{
		set(id, ip, managerPort, workerNum, poolSize);
	}

	void set(int id,const string& ip, unsigned short managerPort, unsigned short workerNum, unsigned short poolSize)
	{
		set_connectorID(id);
		set_connectorIP(ip);
		set_connectorManagerPort(managerPort);
		set_connectorWorkerNum(workerNum);
		set_connectorThreadPoolSize(poolSize);

	}
	
	void set(connectorConfig* config)
	{
		if(config)
		{
			set_connectorID(config->get_connectorID());
			set_connectorIP(config->get_connectorIP());
			set_connectorManagerPort(config->get_connectorManagerPort());
			set_connectorWorkerNum(config->get_connectorWorkerNum());
			set_connectorThreadPoolSize(config->get_connectorThreadPoolSize());
		}
	}
	void set_connectorID(int id){m_connectorID=id;}
	void set_connectorIP(const string &ip){m_connectorIP=ip;}
	void set_connectorManagerPort(unsigned short port){m_connectorManagerPort= port;}
	void set_connectorWorkerNum(unsigned short workerNum){m_connectorWorkerNum= workerNum;}
	void set_connectorThreadPoolSize(unsigned short size){m_connectorThreadPoolSize = size;}
	
	int  get_connectorID() const{return m_connectorID;}
	const string&  get_connectorIP() const{return m_connectorIP;}
	unsigned short get_connectorManagerPort() const{return m_connectorManagerPort;}
	unsigned short get_connectorWorkerNum() const{return m_connectorWorkerNum;}
	unsigned short get_connectorThreadPoolSize() const{return m_connectorThreadPoolSize;}
	void display() const
	{
		g_manager_logger->info("{0:d}\t{1}\t{2:d}\t{3:d}\t{4:d}",get_connectorID(), get_connectorIP(), get_connectorManagerPort()
				, get_connectorWorkerNum(), get_connectorThreadPoolSize());

	}
	~connectorConfig(){}
	
private:
	int            m_connectorID;
	string         m_connectorIP;
	unsigned short m_connectorManagerPort;
	unsigned short m_connectorWorkerNum;
	unsigned short m_connectorThreadPoolSize;
};

class connectorInformation
{
public:
	connectorInformation(){}
	connectorInformation(const string& redisIP, unsigned short redisPort)
	{
		m_redis_ip = redisIP;
		m_redis_port = redisPort;
	}
	void set_redis_ip(const string& ip)
	{
		m_redis_ip = ip;
	}
	void set_redis_port(unsigned short port)
	{
		m_redis_port = port;
	}
	void set_redis(const string& ip, unsigned short port)
	{
		set_redis_ip(ip);
		set_redis_port(port);
	}
	void add_connectorConfig(int id,const string& ip, unsigned short loginPort, unsigned short workerNum, unsigned short poolSize)
	{
		connectorConfig *connector = new connectorConfig(id, ip, loginPort, workerNum, poolSize);
		m_connectorConfigList.push_back(connector);	
	}
	void add_connectorConfig(connectorConfig *connector)
	{
		m_connectorConfigList.push_back(connector);	
	}

	void connectorInformation_erase()
	{
		for(auto it = m_connectorConfigList.begin(); it != m_connectorConfigList.end();)
		{
			delete *it;
			it = m_connectorConfigList.erase(it);
		}
	}
	void display() const
	{
	    g_manager_logger->info("======connector infomation begin======");
	    g_manager_logger->info("id\tip\t\tloginport\tworkerNum\tthreadPoolsize\tcoverceNum");
		for(auto it = m_connectorConfigList.begin(); it != m_connectorConfigList.end(); it++)
		{
			connectorConfig *connector = *it;
			if(connector) connector->display();
		}
		g_manager_logger->info("connector redis: {0},{1:d}", m_redis_ip, m_redis_port);
	    g_manager_logger->info("======connector infomation end======");
	}

	const string &get_redis_ip() const {return m_redis_ip;}
	unsigned short get_redis_port() const {return m_redis_port;}

	int    get_logLevel() const{return m_logLevel;}
	void   set_logLevel(int level){m_logLevel = level;}
	const vector<connectorConfig*>& get_connectorConfigList() const{return m_connectorConfigList;}
	~connectorInformation()
	{
		connectorInformation_erase();
	}
private:
	vector<connectorConfig*> m_connectorConfigList;
	string                   m_redis_ip;
	unsigned short           m_redis_port;
	int                      m_logLevel = 0;
};


class bcConfig
{
public:
	bcConfig(){}
	bcConfig(int id, string& ip, unsigned short dataPort, unsigned short managerPort, unsigned short poolSize)
	{
		set_bcID(id);
		set_bcIP(ip);
		set_bcDataPort(dataPort);
		set_bcManagerPort(managerPort);
		set_bcThreadPoolSize(poolSize);
	}
	void set_bcID(int id){m_bcID=id;}
	void set_bcIP(string &ip){m_bcIP=ip;}
	void set_bcDataPort(unsigned short port){m_bcDataPort= port;}
	void set_bcManagerPort(unsigned short port){m_bcManagerPort= port;}
	void set_bcThreadPoolSize(unsigned short size){m_bcThreadPoolSize= size;}
	void set(int id, string &ip, unsigned short dataPort, unsigned short managerPort, unsigned short threadPoolSize)
	{
		set_bcID(id);
		set_bcIP(ip);
		set_bcDataPort(dataPort);
		set_bcManagerPort(managerPort);
		set_bcThreadPoolSize(threadPoolSize);
	}
	void set(bcConfig* bc_con)
	{
		if(bc_con)
		{
			set_bcID(bc_con->get_bcID());
			set_bcIP(bc_con->get_bcIP());
			set_bcDataPort(bc_con->get_bcDataPort());
			set_bcManagerPort(bc_con->get_bcMangerPort());
			set_bcThreadPoolSize(bc_con->get_bcThreadPoolSize());
		}
	}
	int  get_bcID(){return m_bcID;}
	string&  get_bcIP(){return m_bcIP;}
	unsigned short get_bcDataPort(){return m_bcDataPort;}
	unsigned short get_bcMangerPort(){return m_bcManagerPort;}
	unsigned short get_bcThreadPoolSize(){return m_bcThreadPoolSize;}
	void display()
	{
		g_manager_logger->info("{0:d}\t{1}\t{2:d}\t{3:d}\t{4:d}",get_bcID(), get_bcIP(), get_bcDataPort()
				, get_bcMangerPort(), get_bcThreadPoolSize());

	}
	~bcConfig(){}
private:
	int m_bcID;
	string m_bcIP;
	unsigned short m_bcDataPort;
	unsigned short m_bcManagerPort;
	unsigned short m_bcThreadPoolSize;
};



class bcInformation
{
public:
	bcInformation(){}
	bcInformation(string& redisIP, unsigned short redisPort, unsigned short timeout)
	{
		m_redis_ip = redisIP;
		m_redis_port = redisPort;
		m_redis_timeout = timeout;
	}
	void set_redis_ip(string& ip)
	{
		m_redis_ip = ip;
	}
	void set_redis_port(unsigned short port)
	{
		m_redis_port = port;
	}
	void set_redis_timeout(unsigned short timeout)
	{
		m_redis_timeout = timeout;
	}

	void set_redis(string& ip, unsigned short port, unsigned short timeout)
	{
		set_redis_ip(ip);
		set_redis_port(port);
		set_redis_timeout(timeout);
	}
	void add_bcConfig(int id, string& ip, unsigned short dataPort, unsigned short managerPort, unsigned short poolSize)
	{
		bcConfig *bc = new bcConfig(id, ip, dataPort, managerPort, poolSize);
		m_bcConfigList.push_back(bc);	
	}
	void add_bcConfig(bcConfig *bc)
	{
		m_bcConfigList.push_back(bc);	
	}

	void bcInformation_erase()
	{
		for(auto it = m_bcConfigList.begin(); it != m_bcConfigList.end();)
		{
			bcConfig *bc = *it;
			delete bc;
			it = m_bcConfigList.erase(it);
		}

	}
	void display()
	{
	    g_manager_logger->info("======bc infomation begin======");
	    g_manager_logger->info("id\tip\t\tdataPort\tmanagerPort\tthreadPoolSize");
		for(auto it = m_bcConfigList.begin(); it != m_bcConfigList.end(); it++)
		{
			bcConfig *bc = *it;
			if(bc) bc->display();
		}
	    g_manager_logger->info("bc redis :{0}, {1:d}, {2:d}", m_redis_ip, m_redis_port, m_redis_timeout);

	    g_manager_logger->info("======bc infomation end======");

	}

	string& get_redis_ip(){return m_redis_ip;}
	unsigned short get_redis_port(){return m_redis_port;}
	unsigned short get_redis_timeout(){return m_redis_timeout;}
	const vector<bcConfig*>& get_bcConfigList() const{return m_bcConfigList;}

	void set_logLevel(int logLevel){m_logLevel = logLevel;}
	int  get_logLevel(){return m_logLevel;}
	~bcInformation()
	{
		bcInformation_erase();
	}
private:
	vector<bcConfig*> m_bcConfigList;
	string m_redis_ip;
	unsigned short m_redis_port;
	unsigned short m_redis_timeout;
	int    m_logLevel;
};

/*
  *configureObject
  *configure class, file operation
  */
class configureObject
{
public:
	configureObject(const char* configTxt);
	configureObject(string &configTxt);

	bool get_block(ifstream& m_infile, string &startLine, vector<string>& line_list);

	bool is_number(string& str)
	{
		const char *id_str = str.c_str();
		size_t id_size = str.size();
		if(id_size==0) return false;
		for(int i=0; i < id_size; i++)
		{
			char ch = *(id_str+i);
			if(ch<'0'||ch>'9') return false;
		}
		return true;
	}
	bool stringToint(string& str, int& number)
	{
		if(is_number(str))
		{
			number = atoi(str.c_str());
		}
		return false;
	}
	void readConfig();
	void display() ;

	const string& get_configFlieName() const{return m_configName;}
	bool readFileToString(const char* file_name, string& fileData);
	bool string_find(string& str1, const char* str2);
	bool parseBraces(string& oriStr, vector<string>& subStr_list);
	void get_configBlock(const char* blockSym, vector<string>& orgin_text,  vector<string>& result_text);

	throttleConfig* parse_throttle_config(string &str);
	bidderConfig*   parse_bidder_config(string &str, int config_item_num);
	connectorConfig* parse_connector_config(string &str, int config_item_num);
	externbidConfig* parse_exBid_config(string &str, int config_item_num);
	bcConfig*       parse_bc_config(string &str, int item_num);
	
	void parse_throttle_info(vector<string>& textList);
	void parse_bidder_info(vector<string>& textList);
	void parse_exBid_info(vector<string> &textList);
	void parse_connector_info(vector<string> &textList);
	void parse_bc_info(vector<string>& textList);

 	const throttleInformation& get_throttle_info() const{return m_throttle_info;}
 	const bidderInformation&   get_bidder_info() const{return m_bidder_info;}
	const bcInformation&       get_bc_info() const{return m_bc_info;}
	const connectorInformation& get_connector_info() const{ return m_connector_info;}
	const externbidInformation& get_externBid_info()const{return m_exbid_info;}
	int                  get_throttleLogLevel(){return m_throttle_info.get_logLevel();}
	int                  get_bidderLogLevel(){return m_bidder_info.get_logLevel();}
	int                  get_bcLogLevel(){return m_bc_info.get_logLevel();}
	string&              get_vast_businessCode(){return m_vastBusiCode;}
	string&              get_mobile_businessCode(){return m_mobileBusiCode;}
	unsigned short       get_heart_interval(){return m_heartInterval;}

	void clear()
	{
		m_throttle_info.throttleInformation_erase();
		m_bidder_info.bidderInformation_erase();
		m_bc_info.bcInformation_erase();
	}
	~configureObject()
	{
		m_infile.close();
	}
private:
	bool get_subString(string &src, char first, char end, string &dst);
	bool parseSubInfo(string& subStr, string& oriStr, vector<string>& m_subList);

	ifstream             m_infile;
	string               m_configName;
	throttleInformation  m_throttle_info;
	bidderInformation    m_bidder_info;
	externbidInformation m_exbid_info;
	connectorInformation m_connector_info;
	bcInformation        m_bc_info;
	string 				 m_vastBusiCode;
	string 				 m_mobileBusiCode;
	int                  m_heartInterval=1;
    const unsigned short short_max = 0xFFFF;	
};

#endif
