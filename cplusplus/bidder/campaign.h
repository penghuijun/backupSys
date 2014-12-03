#ifndef __CAMPAIGN_H__
#define __CAMPAIGN_H__
#include <string>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <map>
#include <set>
#include <list>
#include "bidderConfig.h"
#include "hiredis.h"
#include "zmq.h"
#include "document.h"		// rapidjson's DOM-style API
#include "prettywriter.h"	// for stringify JSON
#include "filestream.h"	// wrapper of C stream for prettywriter as output
#include "stringbuffer.h"
#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"
#include "AdBidderResponseTemplate.pb.h"
#include "MobileAdResponse.pb.h"
#include "MobileAdRequest.pb.h"
#include "redisPool.h"
#include "lock.h"

using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;

class campaign_target_json
{
public:
	campaign_target_json(){}
	void add_campaignID(int id){m_list.push_back(id);}
	void add_action(string &action){m_action=action;}
	string& get_action(){return m_action;}
	vector<int>& get_list(){return m_list;}
	int  get_list_size(){return m_list.size();}
	bool has_valid_data(){return (m_list.size()!=0);}
	void display()
	{
		cout<<"action:"<<m_action<<endl;
		int idx = 0;
		for(auto it=m_list.begin(); it != m_list.end(); it++, idx++)
		{
			cout <<*it<<"   ";
			if((idx !=0)&&(idx%20)==0) cout<<endl;
		}
		cout<<endl;
	}
	bool include_in(int id)
	{
		for(auto it = m_list.begin(); it != m_list.end(); it++)
		{
			int res =  *it;
			if(res==id)
			{
				return true;
			}
		}
		return false;
	}
	~campaign_target_json(){}
private:
	string m_action;
	vector<int> m_list;
};

class campaign_target_frequency
{
public:
	campaign_target_frequency(){}
	campaign_target_frequency(int lifeTime, int month, int week, int day, int fiveMinute)
	{
		m_lifeTime = lifeTime;
		m_month=month;
		m_week = week;
		m_day = day;
		m_fiveMinute = fiveMinute;
	}

	void set_lifetime(int lifeTime){m_lifeTime = lifeTime;}
	void set_month(int month){m_month=month;}
	void set_week(int week){m_week=week;}
	void set_day(int day){m_day = day;};
	void set_fiveMinute(int fiveMinute){m_fiveMinute=fiveMinute;}

	int  get_lifeTime(){return m_lifeTime;}
	int  get_month(){return m_month;}
	int  get_week(){return m_week;}
	int  get_day(){return m_day;}
	int  get_fiveMinute(){return m_fiveMinute;}

	void display()
	{
		cout<<"frequency:"<<endl;
		cout<<m_lifeTime<<"  "<<m_month<<"  "<<m_week<<"  "<<m_day<<"   "<<m_fiveMinute<<endl;
	}
	~campaign_target_frequency(){}
private:
	int m_lifeTime=0;
	int m_month=0;
	int m_week=0;
	int m_day=0;
	int m_fiveMinute=0;
};

class creative_structure
{
public:
	creative_structure(){}
	creative_structure(int id, string& content, string& macro)
	{
		set_data(id, content, macro);
	}
	void set_id(int id){m_id=id;}
	void set_content(string& content){m_content = content;}
	void set_macro(string& macro){m_macro = macro;}
	void set_data(int id, string& content, string& macro)
	{
		set_id(id);
		set_content(content);
		set_macro(macro);
	}
	int  get_id(){return m_id;}
	string& get_content(){return m_content;}
	string& get_macro(){return m_macro;}
	void display()
	{
		cout<<"id:"<<m_id<<endl;
		cout<<"m_content:"<<m_content<<endl;
		cout<<"m_macro:"<<m_macro<<endl;
	}
private:
	int m_id;
	string m_content;
	string m_macro;
};

class campaign_action
{
public:
	campaign_action(){}
	campaign_action(string& actionTypeName, bool inApp, string& content, string &name)
	{
		set(actionTypeName, inApp, content, name);
	}
	void set(string& actionTypeName, bool inApp, string& content, string &name)
	{
		m_actionTypeName = actionTypeName;
		m_inApp = inApp;
		m_content = content;
		m_name = name;
	}
	
	void set_name(string &name)
	{
		m_name = name;
	}

	void set_actionTypeName(string &actionTypeName)
	{
		m_actionTypeName = actionTypeName;
	}
	void set_inApp(bool inApp)
	{
		m_inApp = inApp;
	}
	void set_content(string &content)
	{
		m_content = content;
	}

	string &get_name(){return m_name;}
	string &get_actionTypeName(){return m_actionTypeName;}
	bool    get_inApp(){return m_inApp;}
	string &get_content(){return m_content;}
	~campaign_action(){}
private:
	string m_name;
	string m_actionTypeName;
	bool   m_inApp;
	string m_content;
};

class campaign_creative
{
public:
	campaign_creative(){}
	campaign_creative(string& size)
	{
		set_size(size);
	}
	void set_size(string& size){m_size=size;}
	string& get_size(){return m_size;}
	vector<creative_structure*>& get_creative_data(){return m_createData;}
	void add_creative_data(int id, string& content, string& macro)
	{
		creative_structure* data=new creative_structure(id, content, macro);
		m_createData.push_back(data);
		
	}
	void add_creative_data(creative_structure* data)
	{
		m_createData.push_back(data);	
	}
	void display()
	{
		cout<<"size:"<<m_size<<endl;
		for(auto it = m_createData.begin(); it != m_createData.end(); it++)
		{
			creative_structure* data=*it;
			data->display();
		}
	}
	~campaign_creative()
	{
		for(auto it=m_createData.begin(); it != m_createData.end();)
		{
			delete *it;
			it=m_createData.erase(it);
		}
	}
private:
	string m_size;
	vector<creative_structure*> m_createData;
};



class campaign_target
{
public:
	campaign_target(){}
	bool parseCampaignTarget(const rapidjson::Value &tmp_obj, campaign_target_json& target_obj);
	bool parseCampaignTarget_array(const rapidjson::Value &tmp_obj, vector<int>& array);
	bool parseCampaignTarget_int(const rapidjson::Value & tmp_obj, int &imps);
	bool parseCampaignTarget_string(const rapidjson::Value & tmp_obj, string &imps);
	bool parseCampaignTarget_bool(const rapidjson::Value & tmp_obj, bool &result);
	bool parseCampaignTarget_frequency(const rapidjson::Value & tmp_obj,campaign_target_frequency& target_obj);
	bool parse(const rapidjson::Value &targeting_obj);
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
	

	void display(vector<int>& array);
    bool target_valid(target_set &target_obj, string &ventory, const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_Frequency >& frequency,int camp_id);
	void display();
	~campaign_target(){}
private:
	campaign_target_json m_target_country;
	campaign_target_json m_target_region;
	campaign_target_json m_target_city;
	campaign_target_json m_target_family;
	campaign_target_json m_target_version;
	campaign_target_json m_target_vendor;
	campaign_target_json m_target_model;
	campaign_target_json m_target_lang;
	campaign_target_json m_target_carrier;
	campaign_target_json m_target_conntype;
	vector<int> m_target_daypart;
	campaign_target_frequency m_target_frequency;
	int m_target_seimps=0;
	bool m_target_supmapp;
	bool m_target_supmweb;
	bool m_target_devphone;
	bool m_target_devtablet;
	int  m_target_traffic;
	string m_target_inventory;
};


class campaign_structure
{
public:
	campaign_structure(){}
	//void set_id(string &id){m_id=id;}
	bool parse_target(const rapidjson::Value &targeting_obj)
	{
		return m_target.parse(targeting_obj);
	}
	bool parse_action(const rapidjson::Value &tmp_obj);
	bool parse_campaign(string &campaign_json);
	void display();
    bool campaign_valid(target_set &target_obj, string &ventory, const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_Frequency >& frequency)
    {
    	return m_target.target_valid(target_obj, ventory, frequency, m_id);
    }

	int get_id(){return m_id;}
	string &get_name(){return m_name;}
	string &get_code(){return m_code;}
	string &get_state(){return m_state;}
	string &get_deliveryChanel(){return m_deliveryChannel;}
	string &get_biddingType(){return m_biddingType;}
	double get_biddingValue(){return m_biddingValue;}
	string &get_curency(){return m_curency;}
	int    get_biddingAgent(){return m_biddingAgent;}
	uint64_t get_startDate(){return m_starttDate;}
	uint64_t get_endDate(){return m_endDate;}
	string &get_budgetType(){return m_budgetType;}
	int     get_impressionLifetimeBudget(){return m_impressionLifetimeBudget;}
	int     get_impressionDailyBudget(){return m_impressionDailyBudget;}
	string &get_impressionDailyPacing(){return m_impressionDailyPacing;}
	int     get_mediacostLifetimeBudget(){return m_mediacostLifetimeBudget;}
	int     get_mediacostDailyBudget(){return m_mediacostDailyBudget;}
	string &get_mediacostDailyPacing(){return m_mediacostDailyPacing;}
	int     get_advertiserID(){return m_advertiserID;}
	int     get_dealID(){return m_dealID;}
	uint64_t get_createAt(){return m_createAt;}
	uint64_t get_updateAt(){return m_updateAt;}
	int     get_frequencyLifetime(){return m_frequencyLifetime;}
	int     get_frequencyDay(){return m_frequencyDay;}
	int     get_frequencyMinute(){return m_frequencyMinute;}
	int     get_frequencyMinuteType(){return m_frequencyMinuteType;}
	int     get_daypartTimezone(){return m_daypartTimezone;}
	
	vector<campaign_creative*>& get_creatives(){return m_creatives;}
	campaign_target& get_campaign_target(){return m_target;}	
	campaign_action& get_campaign_action(){return m_action;}

	campaign_creative* find_creative_by_size(string &size)
	{
		for(auto it = m_creatives.begin(); it != m_creatives.end(); it++)
		{
			campaign_creative* cre = *it;
			if(cre)
			{
				string creSize = cre->get_size();
				if(size == creSize) return cre;
			}
		}
		return NULL;
	}
	
	~campaign_structure()
	{
		for(auto it= m_creatives.begin(); it != m_creatives.end();)
		{
			campaign_creative *data=*it;
			delete data;
			it=m_creatives.erase(it);
		}
	}
private:
	int      m_id=0;
	string   m_name;
	string   m_code;
	string   m_state;
	string   m_deliveryChannel;
	string   m_biddingType;
	string   m_curency;
	double   m_biddingValue=0;
	int      m_biddingAgent=0;
	uint64_t m_starttDate=0;
	uint64_t m_endDate=0;
	string   m_budgetType;
	int m_impressionLifetimeBudget=0;
	int m_impressionDailyBudget=0;
	string   m_impressionDailyPacing;
	int m_mediacostLifetimeBudget=0;
	int m_mediacostDailyBudget=0;
	string   m_mediacostDailyPacing;
	int m_advertiserID=0;
	int m_dealID=0;
	uint64_t m_createAt=0;
	uint64_t m_updateAt=0;
	int m_frequencyLifetime=0;
	int m_frequencyDay=0;
	int m_frequencyMinute=0;
	int m_sessionImps=0;
	int m_frequencyMinuteType=0;
	int m_daypartTimezone=0;
	vector<campaign_creative*> m_creatives;
	campaign_target m_target;
	campaign_action m_action;
};

#endif
