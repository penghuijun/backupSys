#include "campaign.h"

bool campaign_target::parseCampaignTarget(const rapidjson::Value &tmp_obj, campaign_target_json& target_obj)
	{
	    if(tmp_obj.IsNull() || (tmp_obj.IsObject()==false)) return false;

	    if(tmp_obj["action"].IsNull() || (tmp_obj["action"].IsString()==false)) return false;
	    string value = tmp_obj["action"].GetString();
	    target_obj.add_action(value);

	    if(tmp_obj["list"].IsNull() || (tmp_obj["list"].IsArray()==false)) return false;
	    const rapidjson::Value &tmp_obj1= tmp_obj["list"];
	    if(tmp_obj1.IsNull()||(tmp_obj1.IsArray()==false)) return false;
	    for(size_t i = 0; i < tmp_obj1.Size(); i++)
	    {
	        if((tmp_obj1[i].IsNull()==false) && tmp_obj1[i].IsInt())
	        {
	            int value = tmp_obj1[i].GetInt();
	            target_obj.add_campaignID(value);
	        }  
	    }
	    return true;
	}

	bool campaign_target::parseCampaignTarget_array(const rapidjson::Value &tmp_obj, vector<int>& array)
	{
	    if(tmp_obj.IsNull() || (tmp_obj.IsArray()==false))
		{
			return false;
	    }
	    for(size_t i = 0; i < tmp_obj.Size(); i++)
	    {
	        if((tmp_obj[i].IsNull()==false) && tmp_obj[i].IsInt())
	        {
	            int value = tmp_obj[i].GetInt();
	            array.push_back(value);
	        } 
	    }
	    return true;
	}

	bool campaign_target::parseCampaignTarget_int(const rapidjson::Value & tmp_obj, int &imps)
	{
		imps = 0;
		if(tmp_obj.IsNull() || (tmp_obj.IsInt()==false)) return false;
		imps=tmp_obj.GetInt();
	    return true;
	}

	bool campaign_target::parseCampaignTarget_string(const rapidjson::Value & tmp_obj, string &imps)
	{
		if(tmp_obj.IsNull() || (tmp_obj.IsString()==false)) return false;
		imps=tmp_obj.GetString();
	    return true;
	}
	bool campaign_target::parseCampaignTarget_bool(const rapidjson::Value & tmp_obj, bool &result)
	{
		if(tmp_obj.IsNull() || (tmp_obj.IsBool()==false)) return false;
		result=tmp_obj.GetBool();
	    return true;
	}

	bool campaign_target::parseCampaignTarget_frequency(const rapidjson::Value & tmp_obj,campaign_target_frequency& target_obj)
	{
	    if(tmp_obj.IsNull() || (tmp_obj.IsObject()==false)) return false;

	    if((tmp_obj["lifetime"].IsNull() == false) || tmp_obj["lifetime"].IsInt())
	    {
	    	int value = tmp_obj["lifetime"].GetInt();
			target_obj.set_lifetime(value);
	   	}
	    if((tmp_obj["month"].IsNull() == false) || tmp_obj["month"].IsInt())
	    {
	    	int value = tmp_obj["month"].GetInt();
			target_obj.set_month(value);
	   	}
	    if((tmp_obj["week"].IsNull() == false) || tmp_obj["week"].IsInt())
	    {
	    	int value = tmp_obj["week"].GetInt();
			target_obj.set_week(value);
	   	}
	    if((tmp_obj["day"].IsNull() == false) || tmp_obj["day"].IsInt())
	    {
	    	int value = tmp_obj["day"].GetInt();
			target_obj.set_day(value);
	   	}
	    if((tmp_obj["fiveMinutes"].IsNull() == false) || tmp_obj["fiveMinutes"].IsInt())
	    {
	    	int value = tmp_obj["fiveMinutes"].GetInt();
			target_obj.set_fiveMinute(value);
	   	}

		return true;
	}
	bool campaign_target::parse(const rapidjson::Value &targeting_obj)
	{
	    if(targeting_obj.IsNull() || (targeting_obj.IsObject()==false))
		{
			cerr<<"targeting_obj is invalid!"<<endl;
			return false;
	    }
		parseCampaignTarget(targeting_obj["geoCountry"], m_target_country);
		parseCampaignTarget(targeting_obj["geoRegion"], m_target_region);
		parseCampaignTarget(targeting_obj["geoCity"], m_target_city);
		parseCampaignTarget(targeting_obj["osFamilies"], m_target_family);
		parseCampaignTarget(targeting_obj["osFamilyVersions"], m_target_version);
		parseCampaignTarget(targeting_obj["deviceVendors"], m_target_vendor);
		parseCampaignTarget(targeting_obj["deviceModels"], m_target_model);
		parseCampaignTarget(targeting_obj["languages"], m_target_lang);
		parseCampaignTarget(targeting_obj["carriers"], m_target_carrier);
		parseCampaignTarget(targeting_obj["connectionTypes"], m_target_conntype);
		parseCampaignTarget_array(targeting_obj["dayPart"], m_target_daypart);
		parseCampaignTarget_frequency(targeting_obj["frequency"],m_target_frequency);
		parseCampaignTarget_int(targeting_obj["sessionImps"], m_target_seimps);
		parseCampaignTarget_bool(targeting_obj["supplyTypeMobileApp"], m_target_supmapp);
		parseCampaignTarget_bool(targeting_obj["supplyTypeMobileWeb"], m_target_supmweb);
		parseCampaignTarget_bool(targeting_obj["deviceTypePhone"], m_target_devphone);
		parseCampaignTarget_bool(targeting_obj["deviceTypeTablet"], m_target_devtablet);
		parseCampaignTarget_int(targeting_obj["trafficQuality"], m_target_traffic);
		parseCampaignTarget_string(targeting_obj["inventoryQuality"], m_target_inventory);
		return true;
	}

	void campaign_target::display(vector<int>& array)
	{
		int idx = 1;
		cout<<"vector:"<<endl;
		for(auto it=array.begin(); it != array.end(); it++, idx++)
		{
			cout <<*it<<"	";
			if((idx%20)==0) cout<<endl;
		}
		cout<<endl;
	}
    bool campaign_target::target_valid(target_set &target_obj, string &ventory, const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_Frequency >& frequency,int camp_id)
    {
 		auto geo = target_obj.get_target_geo();
		auto os  = target_obj.get_target_os();
		auto dev = target_obj.get_target_dev();
		auto signal = target_obj.get_target_signal();
		bool geo_include =false;
		bool os_include =false;
		bool dev_include = false;
		for(auto it=geo.begin(); it != geo.end(); it++)
		{
			target_infomation *info = *it;
			if(info)
			{
				string name= info->get_name();
				string nameid = info->get_id();
                if(is_number(nameid)==false)continue;
				int nameid_dec = atoi(nameid.c_str());
				if(name=="country")
				{
					if(m_target_country.include_in(nameid_dec)==true) geo_include = true;
				}
				else if(name=="region")
				{
					if(m_target_region.include_in(nameid_dec)==true) geo_include = true;
				}
				else if(name=="city")
				{
					if(m_target_city.include_in(nameid_dec)==true) geo_include = true;
				}
				else
				{
				    cout<<"invalid geo info"<<endl;
					return false;
				}
			}
		}

		if(geo.size()&&geo_include==false) return false;
		for(auto it=os.begin(); it != os.end(); it++)
		{
			target_infomation *info=*it;
			if(info)
			{
				string name= info->get_name();
				string nameid = info->get_id();
				int nameid_dec = atoi(nameid.c_str());
				if(name=="family")
				{
					if(m_target_family.include_in(nameid_dec)==true) os_include = true;
				}
				else if(name=="version")
				{
					if(m_target_version.include_in(nameid_dec)==true) os_include = true;	
				}
				else
				{
				    cout<<"invalid os info"<<endl;
					return false;
				}
			}
		}

		if(os.size()&&os_include==false) return false;
		
		for(auto it=dev.begin(); it != dev.end(); it++)
		{
			target_infomation *info=*it;
			if(info)
			{
				string name= info->get_name();
				string nameid = info->get_id();
				int nameid_dec = atoi(nameid.c_str());
				if(name=="vendor")
				{
					if(m_target_vendor.include_in(nameid_dec)==true) dev_include=true;
				}
				else if(name=="model")
				{
					if(m_target_model.include_in(nameid_dec)==true) dev_include=true;	
				}
				else
				{
				    cout<<"invalid dev info"<<endl;
					return false;
				}
			}		
		}
		if(dev.size()&&dev_include==false) return false;

		for(auto it = signal.begin(); it != signal.end();it++)
		{
			target_infomation *info=*it;
			if(info)
			{
				string name= info->get_name();
				string nameid = info->get_id();
                if(nameid.empty()) continue;
				int nameid_dec = atoi(nameid.c_str());
				if(name=="lang")
				{
					if(m_target_lang.include_in(nameid_dec)==false)  return false;
				}
				else if(name=="carrier")
				{
					if(m_target_carrier.include_in(nameid_dec)==false) return false;	
				}
				else if(name=="conn.type")
				{
					if(m_target_conntype.include_in(nameid_dec)==false) return false;	
				}
				else if(name=="day.part")
				{
					if(m_target_daypart.size())
					{
						int hour_index = m_target_daypart.at(nameid_dec-1);
						if(hour_index!=1)
						{
							return false;	
						}
					}
				}
				else if(name=="se.imps")
				{
					if(nameid_dec != m_target_seimps)
					{
						return false;	
					}
				}
				else if(name=="sup.m.app")
				{
					if(nameid_dec != (int)m_target_supmapp)
					{
						return false;
					}
				}
				else if(name=="sup.m.web")
				{
					if(nameid_dec != (int)m_target_supmweb)
					{
						return false;
					}
				}
				else if(name=="dev.phone")
				{
					if(nameid_dec != (int)m_target_devphone)
					{
						return false;	
					}
				}
				else if(name=="dev.tablet")
				{
					if(nameid_dec != (int)m_target_devtablet)
					{
						return false;
					}
				}
				else if(name=="tra.quality")
				{
					if(nameid_dec != m_target_traffic)
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}		
		}

		if(m_target_inventory!=ventory)
		{
	//		cout<<"invenroy error"<<endl;
			return false;
		}
		int lifeTime_imps_max=m_target_frequency.get_lifeTime();
        int week_imps_max = m_target_frequency.get_week();
        int month_imps_max = m_target_frequency.get_month();
		int day_imps_max=m_target_frequency.get_day();
        int fiveMinutes_imps_max = m_target_frequency.get_fiveMinute();

        int MaxTimes_frequency = 0;
		for(auto it = frequency.begin(); it != frequency.end(); it++)
		{
			auto& fre = *it;
			string camp_str = fre.id();
			int campID = atoi(camp_str.c_str());
#ifdef DEBUG
 //           cerr<<"campID:"<<campID<<endl;
#endif

			if(camp_id==campID)
			{
				int size = fre.frequencyvalue_size();

				for(auto k = 0; k < size; k++)
				{
					MobileAdRequest_Frequency_FrequencyValue value = fre.frequencyvalue(k);
					string fre_type = value.frequencytype();
					int imps_times = atoi(value.times().c_str());
#ifdef DEBUG
   //                 cerr<<fre_type<<":"<<imps_times<<endl;               
#endif
					if(fre_type=="fe")
					{			    
						if((lifeTime_imps_max)&&(imps_times>=lifeTime_imps_max))
						{
				//			cout<<"more than lifetimes imps"<<endl;
							return false;
						}
					}
					else if(fre_type=="m")//month
					{
						if((month_imps_max)&&(imps_times>=month_imps_max))
						{
					//		cout<<"more than month imps"<<endl;
							return false;
						}					
					}
					else if(fre_type == "w")
					{
						if((week_imps_max)&&(imps_times>=week_imps_max))
						{
						//	cout<<"more than week imps"<<endl;
							return false;
						}					
					}
					else if(fre_type=="d")
					{
						if(day_imps_max &&(imps_times>=day_imps_max))
						{
						//	cout<<"more than day imps"<<endl;
							return false;
						}
					}
					else if(fre_type=="fm")
					{
						if((fiveMinutes_imps_max)&&(imps_times>=fiveMinutes_imps_max))
						{
						//	cout<<"more than fiveminutes imps"<<endl;
							return false;
						}
					}
					else
					{
						break;
					}
				}
			}
		}
		//cout<<"return true"<<endl;
		return true;
    }

	void campaign_target::display()
	{
		m_target_country.display();
	 	m_target_region.display();
	 	m_target_city.display();
		m_target_family.display();
 		m_target_version.display();
		m_target_vendor.display();
		m_target_model.display();
	 	m_target_lang.display();
 		m_target_carrier.display();
 		m_target_conntype.display();
		display(m_target_daypart);
 		m_target_frequency.display();
		cerr<<"m_target_seimps:"<<m_target_seimps<<endl;
		cerr<<"m_target_supmapp:"<<m_target_supmapp<<endl;
		cerr<<"m_target_supmweb:"<<m_target_supmweb<<endl;
		cerr<<"m_target_devphone:"<<m_target_devphone<<endl;
		cerr<<"m_target_devtablet:"<<m_target_devtablet<<endl;
		cerr<<"m_target_traffic:"<<m_target_traffic<<endl;
		cerr<<"m_target_inventory:"<<m_target_inventory<<endl;
	}
    
	bool campaign_structure::parse_action(const rapidjson::Value &tmp_obj)
    {
	    if((tmp_obj.IsNull()==false) && (tmp_obj.IsObject()))
        {   
    	    if((tmp_obj["actionType"].IsNull()==false) && (tmp_obj["actionType"].IsString()))
            {
    	        string value = tmp_obj["actionType"].GetString();
    	        m_action.set_actionTypeName(value);
            }
    	    if((tmp_obj["inApp"].IsNull()==false) &&(tmp_obj["inApp"].IsBool()))
            {
    	        bool value = tmp_obj["inApp"].GetBool();
    	        m_action.set_inApp(value);
            }
    	    if((tmp_obj["content"].IsNull()==false)&& (tmp_obj["content"].IsString()))
            {
    	        string value = tmp_obj["content"].GetString();
    	        m_action.set_content(value);
            }
     	    if((tmp_obj["name"].IsNull()==false)&& (tmp_obj["name"].IsString()))
            {
    	        string value = tmp_obj["name"].GetString();
    	        m_action.set_name(value);
            }
        }
	    return true;
    }   
    bool campaign_structure::parse_campaign(string &campaign_json)
	{
	    ostringstream os;

	    rapidjson::Document document;   
	    document.Parse<0>(campaign_json.c_str());
	    if(document.IsObject()==false)
        {
            cerr<<"campaign is not correct json format"<<endl;
            return false;
        }   
	    if((document["id"].IsNull()==false) && document["id"].IsInt())
	    {
	        m_id = document["id"].GetInt();
	    }
	    if((document["name"].IsNull()==false) && document["name"].IsString())
	    {
	        m_name = document["name"].GetString();
	    }
	    if((document["code"].IsNull()==false) && document["code"].IsString())
	    {
	        m_code = document["code"].GetString();
	    }
	    if((document["state"].IsNull()==false) && document["state"].IsString())
	    {
	        m_state = document["state"].GetString();
	    }
	    if((document["deliveryChannel"].IsNull()==false) && document["deliveryChannel"].IsString())
	    {
	        m_deliveryChannel= document["deliveryChannel"].GetString();
	    }
	    if((document["biddingType"].IsNull()==false) && document["biddingType"].IsString())
	    {
	        m_biddingType = document["biddingType"].GetString();
	    }
	    if((document["biddingValue"].IsNull()==false) && document["biddingValue"].IsDouble())
	    {
	        m_biddingValue = document["biddingValue"].GetDouble();
	    }
	    if((document["currency"].IsNull()==false) && document["currency"].IsString())
	    {
	        m_curency = document["currency"].GetString();
	    }
		if((document["biddingAgent"].IsNull()==false) && document["biddingAgent"].IsUint64())
	    {
	        m_biddingAgent= document["biddingAgent"].GetUint64();
	    }
	    if((document["startDate"].IsNull()==false) && document["startDate"].IsUint64())
	    {
	        m_starttDate = document["startDate"].GetUint64();
	    }
		if((document["endDate"].IsNull()==false) && document["endDate"].IsUint64())
	    {
	        m_endDate = document["endDate"].GetUint64();
	    }
	    if((document["budgetType"].IsNull()==false) && document["budgetType"].IsString())
	    {
	        m_budgetType= document["budgetType"].GetString();
	    }
	    if((document["impressionLifetimeBudget"].IsNull()==false) && document["impressionLifetimeBudget"].IsInt())
	    {
	        m_impressionLifetimeBudget = document["impressionLifetimeBudget"].GetInt();
	    }
		if((document["impressionDailyBudget"].IsNull()==false) && document["impressionDailyBudget"].IsInt())
	    {
	        m_impressionDailyBudget= document["impressionDailyBudget"].GetInt();
	    }
	    if((document["impressionDailyPacing"].IsNull()==false) && document["impressionDailyPacing"].IsString())
	    {
	        m_impressionDailyPacing = document["impressionDailyPacing"].GetString();
	    }
	    if((document["mediacostLifetimeBudget"].IsNull()==false) && document["mediacostLifetimeBudget"].IsInt())
	    {
	        m_mediacostLifetimeBudget = document["mediacostLifetimeBudget"].GetInt();
	    }
		if((document["mediacostDailyBudget"].IsNull()==false) && document["mediacostDailyBudget"].IsInt())
	    {
	        m_mediacostDailyBudget= document["mediacostDailyBudget"].GetInt();
	    }
	    if((document["mediacostDailyPacing"].IsNull()==false) && document["mediacostDailyPacing"].IsString())
	    {
	        m_mediacostDailyPacing = document["mediacostDailyPacing"].GetString();
	    }
	    if((document["advertiserId"].IsNull()==false) && document["advertiserId"].IsInt())
	    {
	        m_advertiserID = document["advertiserId"].GetInt();
	    }
		if((document["dealId"].IsNull()==false) && document["dealId"].IsInt())
	    {
	        m_dealID = document["dealId"].GetInt();
	    }
	    if((document["createdAt"].IsNull()==false) && document["createdAt"].IsUint64())
	    {
	        m_createAt = document["createdAt"].GetUint64();
	    }
		if((document["updatedAt"].IsNull()==false) && document["updatedAt"].IsUint64())
	    {
	        m_updateAt = document["updatedAt"].GetUint64();
	    }
	    if((document["frequencyLifetime"].IsNull()==false) && document["frequencyLifetime"].IsInt())
	    {
	        m_frequencyLifetime = document["frequencyLifetime"].GetInt();
	    }
		if((document["frequencyDay"].IsNull()==false) && document["frequencyDay"].IsInt())
	    {
	        m_frequencyDay = document["frequencyDay"].GetInt();
	    }
	    if((document["frequencyMinute"].IsNull()==false) && document["frequencyMinute"].IsInt())
	    {
	        m_frequencyMinute = document["frequencyMinute"].GetInt();
	    }
		if((document["frequencyMinuteType"].IsNull()==false) && document["frequencyMinuteType"].IsInt())
	    {
	        m_frequencyMinuteType = document["frequencyMinuteType"].GetInt();
	    }
	    if((document["sessionImps"].IsNull()==false) && document["sessionImps"].IsInt())
	    {
	        m_sessionImps = document["sessionImps"].GetInt();
	    }
		if((document["daypartTimezone"].IsNull()==false) && document["daypartTimezone"].IsInt())
	    {
	        m_daypartTimezone = document["daypartTimezone"].GetInt();
	    }
      	if((document["action"].IsNull()==false) && document["action"].IsObject())
	    {
	     //	const rapidjson::Value &value = document["action"];
	        parse_action(document["action"]);
	    }  
		if((document["creatives"].IsNull()==false) && document["creatives"].IsArray())
	    {
	    	const rapidjson::Value &value = document["creatives"];
			int value_size = value.Size();

			for(size_t i = 0; i < value_size; i++)
	    	{
	    		const rapidjson::Value &creativeJson=value[i];
                if(creativeJson.IsObject()==false) continue;
				campaign_creative *creative = NULL;
	        	if((creativeJson["size"].IsNull()==false) && creativeJson["size"].IsString())
	        	{
	            	string value = creativeJson["size"].GetString();
					creative = new campaign_creative(value);
					m_creatives.push_back(creative);
				//	cout<<"sssize:"<<value<<endl;
	        	}	
				
	        	if((creativeJson["datas"].IsNull()==false) &&creativeJson["datas"].IsArray())
	        	{
	        		const rapidjson::Value &value_tt=creativeJson["datas"];
                    int v_size = value_tt.Size();
					for(size_t i = 0; i < v_size; i++)
	    			{
						const rapidjson::Value &value_1=value_tt[i];
                        if(value_1.IsObject()== false) continue;	        
						int id=0;
						string macro;
                        string content;
	    				if((value_1["id"].IsNull()==false) && value_1["id"].IsInt())
	    				{
	        				id = value_1["id"].GetInt();
	    				}	
                        if((value_1["content"].IsNull()==false) && value_1["content"].IsString())
	    				{
	        				content = value_1["content"].GetString();
	    				}
                        if((value_1["macro"].IsNull()==false) && value_1["macro"].IsString())
	    				{
	        				macro = value_1["macro"].GetString();
	    				}
						if(creative)creative->add_creative_data(id, content, macro);
					}
	        	 }
			 }
	    }

	    bool result = parse_target(document["targeting"]); 
	    return result;
	}

	void campaign_structure::display()
	{
		cout<<"m_id:"<<m_id<<endl;
		cout<<"m_name:"<<m_name<<endl;
		cout<<"m_code:"<<m_code<<endl;
		cout<<"m_state:"<<m_state<<endl;
		cout<<"m_deliveryChannel:"<<m_deliveryChannel<<endl;
		cout<<"m_biddingType:"<<m_biddingType<<endl;
		cout<<"m_biddingValue:"<<m_biddingValue<<endl;
		cout<<"m_curency:"<<m_curency<<endl;
		cout<<"m_biddingAgent:"<<m_biddingAgent<<endl;
		cout<<"m_starttDate:"<<m_starttDate<<endl;
		cout<<"m_endDate:"<<m_endDate<<endl;
		cout<<"m_budgetType:"<<m_budgetType<<endl;
		cout<<"m_impressionLifetimeBudget:"<<m_impressionLifetimeBudget<<endl;
		cout<<"m_impressionDailyBudget:"<<m_impressionDailyBudget<<endl;
		cout<<"m_impressionDailyPacing:"<<m_impressionDailyPacing<<endl;
		cout<<"m_mediacostLifetimeBudget:"<<m_mediacostLifetimeBudget<<endl;
		cout<<"m_mediacostDailyBudget:"<<m_mediacostDailyBudget<<endl;
		cout<<"m_mediacostDailyPacing:"<<m_mediacostDailyPacing<<endl;
		cout<<"m_advertiserID:"<<m_advertiserID<<endl;
		cout<<"m_dealID:"<<m_dealID<<endl;
		cout<<"m_createAt:"<<m_createAt<<endl;
		cout<<"m_updateAt:"<<m_updateAt<<endl;
		cout<<"m_frequencyLifetime:"<<m_frequencyLifetime<<endl;
		cout<<"m_frequencyDay:"<<m_frequencyDay<<endl;
		cout<<"m_frequencyMinute:"<<m_frequencyMinute<<endl;
		cout<<"m_sessionImps:"<<m_sessionImps<<endl;
		cout<<"m_frequencyMinuteType:"<<m_frequencyMinuteType<<endl;
		cout<<"m_daypartTimezone:"<<m_daypartTimezone<<endl;
        cout<<"actions-name:"<<m_action.get_name()<<endl;
        cout<<"actions-inApp:"<<m_action.get_inApp()<<endl;
        cout<<"actions-content:"<<m_action.get_content()<<endl;
        cout<<"actions-type:"<<m_action.get_actionTypeName()<<endl;
		cout<<"m_creatives:"<<endl;
		for(auto it = m_creatives.begin(); it != m_creatives.end();it++)
		{
			campaign_creative* creative = *it;
			creative->display();
		}
		cout<<endl;
		m_target.display();
	}

