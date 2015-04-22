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
#include "adConfig.h"
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
#include "campaign_proto.pb.h"
#include "redisPool.h"
#include "lock.h"
#include "spdlog/spdlog.h"

using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace com::rj::targeting::protos;
using namespace std;

extern shared_ptr<spdlog::logger> g_file_logger;
extern shared_ptr<spdlog::logger> g_manager_logger;
extern shared_ptr<spdlog::logger> g_worker_logger;

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
		g_worker_logger->trace("frequency:");
		g_worker_logger->trace("{0:d},{1:d},{2:d},{3:d},{4:d}", m_lifeTime, m_month, m_week, m_day, m_fiveMinute);
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
		g_worker_logger->trace("creative id:{0:d}", m_id);
		g_worker_logger->trace("creative content:{0}", m_content);
		g_worker_logger->trace("creative macro:{0}", m_macro);
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
	campaign_action(string& actionTypeName, string& inApp, string& content)
	{
		set(actionTypeName, inApp, content);
	}
	void set(const string& actionTypeName,const string& inApp,const string& content)
	{
		m_actionTypeName = actionTypeName;
		m_inApp = inApp;
		m_content = content;
	}
	
	void set_actionTypeName(string &actionTypeName)
	{
		m_actionTypeName = actionTypeName;
	}
	void set_inApp(string& inApp)
	{
		m_inApp = inApp;
	}
	void set_content(string &content)
	{
		m_content = content;
	}

	const string &get_actionTypeName() const{return m_actionTypeName;}
	const string &get_inApp() const{return m_inApp;}
	const string &get_content() const{return m_content;}
	~campaign_action(){}
private:
	string m_actionTypeName;
	string m_inApp;
	string m_content;
};

class verifyTarget
{
public:
	verifyTarget(){}
	bool is_number(const string& str)
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

	bool appCriteria_valid(const CampaignProtoEntity_Targeting_AppCriteria& appcriteria, const string& request_appid);

	void set_supmapp(const string &res)
	{
		m_target_supmapp = res;
	}
	void set_supmweb(const string& res)
	{
		m_target_supmweb = res;
	}
	void set_devphone(const string &res)
	{
		m_target_devphone = res;
	}
	void set_devtablet(const string &res)
	{
		m_target_devtablet = res;
	}
	void set_traffic(const string& res)         
	{
		m_target_traffic = res;
	}
	void set_inventory(const string &res) 
	{
		m_target_inventory = res;
	}
	void set_appid(const string &res)
	{
		if(is_number(res))m_target_appid = res;
	}
	void set_appcategory(const string &res)
	{
		if(is_number(res)) m_target_appcategory = res;
	}

	string& get_supmapp(){return m_target_supmapp;}
	string& get_supmweb(){return m_target_supmweb;}
	string& get_devphone(){return m_target_devphone;}
	string& get_devTablet(){return m_target_devtablet;}
	string& get_traffic(){return m_target_traffic;}
	string& get_inventory(){return m_target_inventory;}
	string& get_appid(){return m_target_appid;}
	string& get_appcategory(){return m_target_appcategory;}

	bool target_valid(const CampaignProtoEntity_Targeting &camp_target);
	void display(shared_ptr<spdlog::logger>& file_logger);
	~verifyTarget(){}
private:
	string m_target_supmapp;
	string m_target_supmweb;
	string m_target_devphone;
	string m_target_devtablet;
	string m_target_traffic;
	string m_target_inventory;
	string m_target_appid;
	string m_target_appcategory;
};


class campaignInformation
{
public:
	campaignInformation()
	{
	}
	bool parse_action(const rapidjson::Value &tmp_obj);
	bool parse_campaign( char *data, int dataLen , verifyTarget& cam_target, string& creativeSize, MobileAdRequest &mobile_request);

	bool frequency_valid(	const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_Frequency >& frequency
        ,const CampaignProtoEntity_Targeting_Frequency& camp_frequency);
    bool expectecpm(MobileAdRequest &mobile_request, CampaignProtoEntity &campaign_proto, string& invetory);
    bool set_appSession(MobileAdRequest &mobile_request, const CampaignProtoEntity_Targeting& camp_targeting);


	bool parseCreativeSize(const string &creativeSize, int &width, int &height);
	string &get_id(){return m_id;}
	string &get_state(){return m_state;}
	string &get_biddingType(){return m_biddingType;}
	string &get_biddingValue(){return m_biddingValue;}
	string &get_curency(){return m_curency;}
	string &get_advertiserID(){return m_advertiserID;}
	string &get_creativeID(){return m_creativeID;}
	string &get_categoryID(){return m_caterogyID;}
	string &get_mediaTypeID(){return m_mediaTypeID;}
	string &get_mediaSubTypeID(){return m_mediaSubTypeID;}
	string &get_expectEcmp(){return m_expectEcmp;}
	const campaign_action& get_campaign_action() const {return m_action;}

	void set_camFrequecy(bool res){m_camFrequecy = res;}
	bool get_camFrequecy(){return m_camFrequecy;}
	bool get_camAppSession(){return m_camAppSession;}

	void display();	
	~campaignInformation()
	{
	}
private:
	string   m_id;
	string   m_curency;
	string   m_state;
	string   m_biddingType;
	string   m_biddingValue;
	string   m_advertiserID;
	campaign_action m_action;
	string   m_creativeID;
	string   m_caterogyID;
	string   m_mediaTypeID;
	string   m_mediaSubTypeID;
	bool     m_camAppSession = false;
	bool     m_camFrequecy = false;
	string   m_expectEcmp;
};


class campaignInfoManager
{
public:
	campaignInfoManager()
	{
	}

	void add_campaign(campaignInformation* campaign)
	{
		m_campaignInfoList.push_back(campaign);
	}

	campaignInformation* get_campaign(int idx)
	{
		return m_campaignInfoList.at(idx);
	}

	vector<campaignInformation*> &get_campaignInfoList(){return m_campaignInfoList;}

	bool parse_campaingnStringList( vector<stringBuffer*>& campaignStringList, verifyTarget& target_verify_set
		, string &creativeSize, MobileAdRequest &mobile_request)
	{
		for(auto it = campaignStringList.begin(); it != campaignStringList.end(); it++)
		{
			stringBuffer* campaignStr = *it;
			if(campaignStr == NULL) continue; 
			campaignInformation *campaignInfoPtr = new campaignInformation;
			if(campaignInfoPtr->parse_campaign( campaignStr->get_data(), campaignStr->get_dataLen()
				, target_verify_set, creativeSize, mobile_request)==false)
			{
				delete campaignInfoPtr;
				continue;
			}
			m_campaignInfoList.push_back(campaignInfoPtr);;
		}

		return ((m_campaignInfoList.size() != 0)? true:false);
	}

	
	void display()
	{
		for(auto it = m_campaignInfoList.begin(); it != m_campaignInfoList.end();)
		{
			campaignInformation *campaign = *it;
			if(campaign) campaign->display();
		}	
	}
	~campaignInfoManager()
	{
		for(auto it = m_campaignInfoList.begin(); it != m_campaignInfoList.end();)
		{
			delete *it;
			it = m_campaignInfoList.erase(it);
		}
	}
private:
	vector<campaignInformation*> m_campaignInfoList;
};


#endif
