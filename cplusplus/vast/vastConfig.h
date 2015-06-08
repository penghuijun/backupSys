/*
  *bcConfig
  *auth:yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#ifndef __BCCONFIG_H__
#define __BCCONFIG_H__
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
using namespace std;

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
	unsigned short get_throttleWorkerNum(){return m_throttleWorkerNum;}
	unsigned short get_throttleManagerPort(){return m_throttleManagerPort;}
	void display()
	{
		cout<<get_throttleID()<<"\t"<<get_throttleIP()<<"\t"<<get_throttleAdPort()<<"\t"<<get_throttlePubPort()
			<<"\t"<<get_throttleManagerPort()<<"\t"<<get_throttleWorkerNum()<<endl;
	}
	~throttleConfig(){}
private:
	int m_throttleID;
	string m_throttleIP;
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
			m_throttleConfigList.erase(it);
		}

	}
	void display()
	{
		cout<<"======throttle infomation begin======"<<endl;
		cout<<"id\tip\t\tadport\tpubport\tmanaPort\tworkerNum"<<endl;
		for(auto it = m_throttleConfigList.begin(); it != m_throttleConfigList.end(); it++)
		{
			throttleConfig *throttle = *it;
			if(throttle) throttle->display();
		}
		cout<<"======throttle infomation end======"<<endl;
	}
	vector<throttleConfig*>& get_throttleConfigList(){return m_throttleConfigList;}
	~throttleInformation()
	{
		throttleInformation_erase();
	}
private:
	
	vector<throttleConfig*> m_throttleConfigList;
};


class target
{
public:
	target(){}
	bool  get_block(ifstream& m_infile, string &startLine, vector<string>& line_list);
	bool string_find(string& str1, const char* str2);
	throttleConfig* parse_throttle_config(string &str);
	bool parseBraces(string& oriStr, vector<string>& subStr_list);
	void get_configBlock(const char* blockSym, vector<string>& orgin_text,  vector<string>& result_text);
	void parse_throttle_info(vector<string>& orgin_text);

	void set_country(string &country) {m_country = country;}
	void set_region(string &region){m_region = region;}
	void set_city(string &city){m_city= city;}
	void set_family(string &family){m_family = family;}
	void set_version(string &version) {m_version= version;}
	void set_vendor(string &vendor){m_vendor= vendor;}
	void set_model(string &model){m_model= model;}
	void set_lang(string &lang){m_lang= lang;}

	void set_carrier(string &carrier) {m_carrier= carrier;}
	void set_conntype(string &value){m_connType = value;}
	void set_daypart(string &daypart){m_dayPart= daypart;}
	void set_sessionImps(string &sessionImps){m_sessionImps= sessionImps;}
	void set_app(string &app){m_sup_m_app= app;}
	void set_phone(string &phone){m_dev_phone= phone;}
	void set_tarffic(string &quality){m_traffic_quality= quality;}
	void set_inventory(string &quality){m_inventory_quality= quality;}
	void set_appid(string & id){m_appid = id;}
	void set_appcategory(string& id){m_appcategory = id;}

	string& get_country(){return m_country;}
	string& get_region(){return m_region;}
	string& get_city(){return m_city;}
	string& get_family(){return m_family;}
	string& get_version(){return m_version;}
	string& get_vendor(){return m_vendor;}
	string& get_model(){return m_model;}
	string& get_lang(){return m_lang;}
	string& get_carrier(){return m_carrier;}
	string& get_session(){return m_sessionImps;}
	string& get_conntype(){return m_connType;}
	string& get_daypart(){return m_dayPart;}
	string& get_app(){return m_sup_m_app;}
	string& get_phone(){return m_dev_phone;}
	string& get_traffic(){return m_traffic_quality;}
	string& get_inventory(){return m_inventory_quality;}
	string& get_appid(){return m_appid;}
	string& get_appcategory(){return m_appcategory;}

	void display() const
	{
		cout<<"target start:"<<endl;
		cout<<"m_country:"<<m_country<<endl;
		cout<<"m_region:"<<m_region<<endl;
		cout<<"m_city:"<<m_city<<endl;
		cout<<"m_family:"<<m_family<<endl;
		cout<<"m_version:"<<m_version<<endl;
		cout<<"m_vendor:"<<m_vendor<<endl;
		cout<<"m_model:"<<m_model<<endl;
		cout<<"m_lang:"<<m_lang<<endl;
		cout<<"m_carrier:"<<m_carrier<<endl;
		cout<<"m_connType:"<<m_connType<<endl;
		cout<<"m_dayPart:"<<m_dayPart<<endl;
		cout<<"m_sessionImps:"<<m_sessionImps<<endl;
		cout<<"web?app:"<<m_sup_m_app<<endl;
		cout<<"phone?tablet:"<<m_dev_phone<<endl;
		cout<<"m_traffic_quality:"<<m_traffic_quality<<endl;
		cout<<"m_inventory_quality:"<<m_inventory_quality<<endl;
		cout<<"m_appid:"<<m_appid<<endl;
		cout<<"m_appcategory:"<<m_appcategory<<endl;

		cout<<"target end:"<<endl;
	}
private:
	string m_country;
	string m_region;
	string m_city;
	string m_family;
	string m_version;
	string m_vendor;
	string m_model;
	string m_lang;
	string m_carrier;
	string m_connType;
	string m_dayPart;
	string m_sessionImps;
	string m_sup_m_app;
	string m_dev_phone;
	string m_traffic_quality;
	string m_inventory_quality;
	string m_appid;
	string m_appcategory;
};

/*
  *configureObject
  *configure class, file operation
  */
class configureObject
{
public:
	
	configureObject(const char* configTxt);
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
	
	void get_configBlock(const char* blockSym, vector<string>& orgin_text,  vector<string>& result_text);

	void readConfig();
	void display();
	const string& get_configFlieName() const{return m_configName;}
	bool  string_parse(string &ori, const char* dst, string &get_str);
	bool string_find(string& str1, const char* str2);
	void parse_throttle_info(vector<string>& orgin_text);

	const string& get_throttleIP() const {return m_throttleIP;}
	unsigned short get_throttlePubVastPort() const{return m_throttlePubVastPort;}
	throttleConfig* parse_throttle_config(string &str);
	bool get_block(ifstream& m_infile, string &startLine, vector<string>& line_list);
	bool parseBraces(string& oriStr, vector<string>& subStr_list);

	int get_adType() const{return m_adType;}
	int get_runTimes() const{return runTimes;}
	int get_intervalTime() const{return m_interverTime;}
	const string& get_ttlTime() const{return m_ttlTime;}

	const string& get_lifetime_imps()const{return m_lifetime_imps;}
	const string& get_month_imps()const{return m_month_imps;}
	const string& get_week_imps()const{return m_week_imps;}
	const string& get_day_imps()const{return m_day_imps;}
	const string& get_fiveminute_imps()const{return m_fiveminute_imps;}
	const string &get_campaignID()const{return m_campagnID;}
	const string& get_width()const{return m_width;}
	const string &get_height()const{return m_hight;}

	const target& get_target()const{ return m_target;}
	throttleInformation& get_throttle(){return m_throttle_info;}

	~configureObject()
	{
		m_infile.close();
	}
private:
	bool get_subString(string &src, char first, char end, string &dst);
	ifstream m_infile;
	string m_configName;
	
	string m_throttleIP;
	unsigned short m_throttlePubVastPort=0;

	int m_adType = 0;
	int runTimes=1;
	string m_ttlTime;//ms
	int m_interverTime=100;//ms


	string m_lifetime_imps;
	string m_month_imps;
	string m_week_imps;
	string m_day_imps;
	string m_fiveminute_imps;

	target m_target;

	string m_width;
	string m_hight;
	string m_campagnID;
	throttleInformation m_throttle_info;

};

#endif
