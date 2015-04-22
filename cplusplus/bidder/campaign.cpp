#include "campaign.h"
using namespace com::rj::targeting::protos;

bool verifyTarget::appCriteria_valid(const CampaignProtoEntity_Targeting_AppCriteria& appcriteria, const string& request_appid)
{
    bool appAllow = appcriteria.in();
    if(request_appid.empty()) return true;
    int numappid = atoi(request_appid.c_str());
    if(appAllow)
    {
       bool inner = false;
       auto idsCnt = appcriteria.ids_size();
       for(auto i = 0; i < idsCnt; i++)
       {
          int num = appcriteria.ids(i);
          g_file_logger->debug("in:{0:d}", num);
          if(num == numappid)
          {
              inner = true;
              break;
          }
       }
       if(inner == false) return false;
   }
   else
   {
      auto idsCnt = appcriteria.ids_size();
      for(auto i = 0; i < idsCnt; i++)
      {
          int num = appcriteria.ids(i);
          g_file_logger->debug("not in:{0:d}", num);
          if(num == numappid)
          {
              return false;
          }
      }
   }
   return true;
}

bool verifyTarget::target_valid(const CampaignProtoEntity_Targeting &camp_target)
{
    const string& supmapp = camp_target.supplytypemobileapp();
    const string& supmweb = camp_target.supplytypemobileweb();
    const string& devphone = camp_target.devicetypephone();
    const string& devtablet = camp_target.devicetypetablet();
//    const string& ventory = camp_target.inventoryquality();
    int tranffic = camp_target.trafficquality();
    int request_tranffic = atoi(m_target_traffic.c_str());

    const CampaignProtoEntity_Targeting_AppCriteria& appid = camp_target.appid();
    const CampaignProtoEntity_Targeting_AppCriteria& appcategory = camp_target.appcategory();

    if((m_target_appid.empty() == false)&&(camp_target.has_appid())&&(appCriteria_valid(appid, m_target_appid) == false))
    {
        g_file_logger->debug("this campaign can not show in the appid");    
        return false;
    }
    
    if((m_target_appcategory.empty() == false)&&(camp_target.has_appcategory())&&(appCriteria_valid(appcategory, m_target_appcategory) == false))
    {
        g_file_logger->debug("this campaign can not show in the appcategory");  
        return false;
    }
    
    if((m_target_supmapp.empty()==false)&&(m_target_supmapp != supmapp))
    {
         g_file_logger->debug("supmapp error:{0},{1}", m_target_supmapp, supmapp);
         return false;
    }
    if((m_target_supmweb.empty()==false)&&(m_target_supmweb != supmweb))
    {
         g_file_logger->debug("supmweb error:{0},{1}", m_target_supmweb, supmweb);
         return false;
    }
    if((m_target_devphone.empty()==false)&&(m_target_devphone != devphone))
    {
         g_file_logger->debug("devphone error:{0},{1}", m_target_devphone, devphone);
         return false;
    }
    if((m_target_devtablet.empty()==false)&&(m_target_devtablet != devtablet))
    {
         g_file_logger->debug("devtablet error:{0},{1}", m_target_devtablet, devtablet);
         return false;
    }
   /* if((m_target_inventory != "reviewed")&&(ventory == "reviewed"))
    {
         g_file_logger->debug("ventory error:{0},{1}", m_target_inventory, ventory);
         return false;
    }*/
    if((m_target_traffic.empty()==false)&&(request_tranffic > tranffic))
    {
         g_file_logger->debug("m_target_traffic error:{0:d},{1:d}", request_tranffic, tranffic);
         return false;
    }
    return true;
}

	void verifyTarget::display(shared_ptr<spdlog::logger>& file_logger)
	{
	    file_logger->debug("m_target_supmapp:{0}", m_target_supmapp);
	    file_logger->debug("m_target_supmweb:{0}", m_target_supmweb);
	    file_logger->debug("m_target_devphone:{0}", m_target_devphone);        
	    file_logger->debug("m_target_devtablet:{0}", m_target_devtablet);
	    file_logger->debug("m_target_traffic:{0}", m_target_traffic);
	    file_logger->debug("m_target_inventory:{0}", m_target_inventory);
	    file_logger->debug("m_target_appid:{0}", m_target_appid);
	    file_logger->debug("m_target_appcategory:{0}", m_target_appcategory);

	}

    bool campaignInformation::frequency_valid(	const ::google::protobuf::RepeatedPtrField< ::com::rj::protos::mobile::MobileAdRequest_Frequency >& frequency
        , const CampaignProtoEntity_Targeting_Frequency& camp_frequency)
    {
            int lifeTime_imps_max=camp_frequency.lifetime();
            int week_imps_max = camp_frequency.week();
            int month_imps_max = camp_frequency.month();
            int day_imps_max= camp_frequency.day();
            int fiveMinutes_imps_max = camp_frequency.five_minutes();
            if(
                  (camp_frequency.has_lifetime()&&(lifeTime_imps_max))
                ||(camp_frequency.has_week()&&(week_imps_max))
                ||(camp_frequency.has_month()&&(month_imps_max))
                ||(camp_frequency.has_day()&&(day_imps_max))
                ||(camp_frequency.has_five_minutes()&&(fiveMinutes_imps_max))
                )
            {
                set_camFrequecy(true);
            }
            
            int MaxTimes_frequency = 0;
            for(auto it = frequency.begin(); it != frequency.end(); it++)
            {
                auto& fre = *it;
                const string& camp_str = fre.id();
                if(m_id==camp_str)
                {
                    int size = fre.frequencyvalue_size();
                    for(auto k = 0; k < size; k++)
                    {
                        const MobileAdRequest_Frequency_FrequencyValue& value = fre.frequencyvalue(k);
                        const string& fre_type = value.frequencytype();
                        int imps_times = atoi(value.times().c_str());
                        
                        if((fre_type=="fe")&&(lifeTime_imps_max)&&(imps_times>=lifeTime_imps_max))
                        {               
                            return false; 
                        }
                        else if((fre_type=="m")&&(month_imps_max)&&(imps_times>=month_imps_max))//month
                        {
                            return false;                  
                        }
                        else if((fre_type == "w")&&(week_imps_max)&&(imps_times>=week_imps_max))
                        {
                            return false;                 
                        }
                        else if((fre_type=="d")&&(day_imps_max)&&(imps_times>=day_imps_max))
                        {
                            return false;
                        }
                        else if((fre_type=="fm")&&(fiveMinutes_imps_max)&&(imps_times>=fiveMinutes_imps_max))
                        {
                            return false;     
                        }
                        else
                        {
                            break;
                        }
                    }
                    break;
                }
            }
            return true;
    }

    bool campaignInformation::expectecpm(MobileAdRequest &mobile_request, CampaignProtoEntity &campaign_proto
        , string& invetory)
    {
        if(invetory != "reviewed")
        {
            const CampaignProtoEntity_Targeting& camp_targeting = campaign_proto.targeting();
            const string &camp_vetory = camp_targeting.inventoryquality(); 
            if(camp_vetory == "reviewed")
            {
                if(mobile_request.has_aid())
                {
                    auto   aidstr     = mobile_request.aid();
                    const string& appReviewed = aidstr.app_reviewed();
                    if(appReviewed !=  "1") 
                    {
                        g_file_logger->trace("aid appReviewed:{0}", appReviewed);
                        return false;
                    }
                }
            }
        }
        
        if(mobile_request.has_aid())
        {
            auto   aidstr     = mobile_request.aid();
            bool index_externBuying = false;
            if(campaign_proto.has_externalbuying())  index_externBuying = campaign_proto.externalbuying();
            const string& request_network_reselling = aidstr.network_reselling();
            const string& request_app_reselling = aidstr.app_resell();
            const string& request_network_idstr = aidstr.networkid();
            g_file_logger->trace("mobile request aid, id, networkid, appid:{0},{1},{2},{3}",aidstr.id()
                , aidstr.networkid(), aidstr.publisher_id(), aidstr.app_reviewed());
            if(aidstr.has_networkid() == false)
            {
                g_file_logger->trace("mobile request aid has not network_id:", aidstr.networkid());
                return false;
            }   
            
            int  request_network_reselling_id = atoi(request_network_reselling.c_str());
            int  request_app_reselling_id = atoi(request_app_reselling.c_str());
            int  request_network_idstr_id = atoi(request_network_idstr.c_str());
            if((index_externBuying == false)||(request_network_reselling_id == 0) || (request_app_reselling_id == 0))
            {
                if(request_network_idstr_id != campaign_proto.networkid())  
                {
                    g_file_logger->trace("network is not equal between request and index:{0:d},{1:d}", request_network_idstr_id
                        ,campaign_proto.networkid());
                    return false;
                }
                else
                {
                    g_file_logger->trace("aid externBuying, network_reselling,app_resell:{0:d},{1:d},{2:d}"
                        , (int)index_externBuying, request_network_reselling_id, request_app_reselling_id);   
                }
            }

            if(request_network_idstr_id == campaign_proto.networkid())  
            {
                g_file_logger->trace("aid networkid equal,{0:d}", request_network_idstr_id);
                m_expectEcmp = campaign_proto.expectcpm();
            }
            else
            {
                g_file_logger->trace("aid networkid not equal,{0:d},{1:d}", request_network_idstr_id, campaign_proto.networkid());
                m_expectEcmp = campaign_proto.thirdpartyexpectcpm();
            } 
        }
        else
        {
            g_file_logger->trace("campaing has not aid structure");
        }
        return true;
    }

    bool campaignInformation::set_appSession(MobileAdRequest &mobile_request, const CampaignProtoEntity_Targeting& camp_targeting)
    {
        auto   appsession = mobile_request.appsession();
        if(camp_targeting.has_session() && camp_targeting.session())
        {
            m_camAppSession = true;
            auto sesion = camp_targeting.session();
            g_worker_logger->trace("index session:{0:d}", sesion);
            for(auto it = appsession.begin(); it != appsession.end(); it++)
            {
                 const MobileAdRequest_AppSession& sessionID_req = *it;
                 int times_req = atoi(sessionID_req.times().c_str());
                 if(sessionID_req.id() == m_id)
                 {
                    if(sesion == 0)  break;
                    if(times_req >= sesion)
                    {
                        g_file_logger->trace("app session reached the maximum: requestSesion-{0:d}", times_req);
                        return false;
                    }
                    break;
                 }
            }
        }
        else
        {
            g_worker_logger->debug("campaign has not session");
        }
        return true;
    }

    bool campaignInformation::parseCreativeSize(const string &creativeSize, int &width, int &height)
    {
        int widthCnt = 0;
        bool find = false;
        for(auto it = creativeSize.begin(); it != creativeSize.end(); it++)
        {
           char ch = *it;
           widthCnt++;
           if(ch == '_')
           {
                string widthStr = creativeSize.substr(0, widthCnt);
                string heightStr = creativeSize.substr(widthCnt+1, creativeSize.size()-widthCnt);
                width = atoi(widthStr.c_str());
                height = atoi(heightStr.c_str());
                find = true;
           }
        } 
        return find;
    }
    bool campaignInformation::parse_campaign(char *data, int dataLen , verifyTarget& cam_target,string& creativeSize
        ,MobileAdRequest &mobile_request)
	{
        CampaignProtoEntity campaign_proto;
        campaign_proto.ParseFromArray(data, dataLen);
        auto   frequency = mobile_request.frequency(); 
        auto   aidstr     = mobile_request.aid();
        const CampaignProtoEntity_Targeting& camp_targeting = campaign_proto.targeting();   
        const CampaignProtoEntity_Action&    camp_action = campaign_proto.action();

        m_id = campaign_proto.id();
        m_state = campaign_proto.state();
        m_curency = campaign_proto.currency();
        m_advertiserID = campaign_proto.advertiserid();
        m_biddingType = campaign_proto.biddingtype();
        m_biddingValue = campaign_proto.biddingvalue();
        m_action.set(camp_action.actiontypename(), camp_action.inapp(), camp_action.content());
        double pacingRate = 1;//campaign_proto.pacingrate();
        double randNum = rand()*(1.0)/RAND_MAX;

        if(randNum>pacingRate) 
        {
            g_file_logger->trace("randnum > pacingRate , drop...:{0:f},{1:f}",randNum, pacingRate);
            return false;
        }
        
        //check target
        if(cam_target.target_valid(camp_targeting) == false)
        {
            g_file_logger->trace("campaign do not meet the requirements with target");
            return false;
        }
        if(camp_targeting.has_frequency())
        {
            const CampaignProtoEntity_Targeting_Frequency& camp_frequency = camp_targeting.frequency();
            if(frequency_valid(frequency, camp_frequency)==false)
            {
                g_file_logger->trace("campaign do not meet the requirements with frequency");
                return false;
            }
        }
        else
        {
            g_file_logger->trace("campaign has not frequecy");
        } 
        
        if(expectecpm(mobile_request, campaign_proto, cam_target.get_inventory()) == false)
        {
            g_file_logger->trace("campaign do not meet the requirements with expectecmp");
            return false;
        }
        
        if(set_appSession(mobile_request, camp_targeting) == false)
        {
            g_file_logger->trace("campaign do not meet the requirements with appSession");
            return false;
        }
        
        int creativeCnt = campaign_proto.creatives_size();
        for(auto i = 0; i < creativeCnt; i++)
        {
            const CampaignProtoEntity_Creatives& camp_creative = campaign_proto.creatives(i);
            
            string anySize("0_0");
            const string& ad_widthstr = mobile_request.adspacewidth();
            const string& ad_heightstr = mobile_request.adspaceheight(); 
            int  ad_width = atoi(ad_widthstr.c_str());
            int  ad_heigh = atoi(ad_heightstr.c_str());
            const string& campCretiveSize = camp_creative.size();
            int  campHeight = 0;
            int  campWidth = 0;
            parseCreativeSize(campCretiveSize, campWidth, campHeight);

            g_file_logger->trace("creative size:{0},{1:d},{2:d}", camp_creative.size(), campWidth, campHeight);
            if(creativeSize.empty()||camp_creative.size()==anySize||((campWidth==ad_width)&&(campHeight<=ad_heigh)))
            {
                int creativeSizeCnt = camp_creative.datas_size();
                int idx = rand()%creativeSizeCnt;
                const CampaignProtoEntity_Creatives_Datas& creative_data = camp_creative.datas(idx);
                m_creativeID = creative_data.id();
                m_caterogyID = creative_data.categoryid();
                m_mediaTypeID = creative_data.mediatypeid();
                m_mediaSubTypeID = creative_data.mediasubtypeid();
                g_file_logger->trace("creativeID-mediatyeid-mediatsubtypeid:{0},{1},{2}", m_creativeID, m_mediaTypeID, m_mediaSubTypeID);
                return true;
            }
        }
	    return false;
	}

	void campaignInformation::display()
	{
	    g_worker_logger->trace("campaign id:{0}", m_id);
	    g_worker_logger->trace("campaign state:{0}", m_state);		
	    g_worker_logger->trace("campaign biddingtype:{0}", m_biddingType);
	    g_worker_logger->trace("campaign biddingvalue:{0}", m_biddingValue);
	    g_worker_logger->trace("campaign currecy:{0}", m_curency);
	    g_worker_logger->trace("campaign advertiseID:{0}", m_advertiserID);
	    g_worker_logger->trace("campaign actionInapp:{0}", m_action.get_inApp());
	    g_worker_logger->trace("campaign actioncontent:{0}", m_action.get_content());
	    g_worker_logger->trace("campaign actiontypename:{0}", m_action.get_actionTypeName());
	}

