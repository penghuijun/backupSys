#include "campaign.h"
using namespace com::rj::targeting::protos;

bool verifyTarget::target_valid(const CampaignProtoEntity_Targeting &camp_target)
{
    const string& supmapp = camp_target.supplytypemobileapp();
    const string& supmweb = camp_target.supplytypemobileweb();
    const string& devphone = camp_target.devicetypephone();
    const string& devtablet = camp_target.devicetypetablet();
    int tranffic = camp_target.trafficquality();
    int request_tranffic = atoi(m_target_traffic.c_str());
    
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
    if((m_target_traffic.empty()==false)&&(request_tranffic > tranffic))
    {
         g_file_logger->debug("m_target_traffic error:{0:d},{1:d}", request_tranffic, tranffic);
         return false;
    }
    return true;
}

	void verifyTarget::display(shared_ptr<spdlog::logger>& file_logger)
	{
	    display_string(file_logger, "supmapp", m_target_supmapp);
        display_string(file_logger, "supmweb", m_target_supmweb);
        display_string(file_logger, "devphone", m_target_devphone);
        display_string(file_logger, "devtablet", m_target_devtablet);
        display_string(file_logger, "traffic", m_target_traffic);
        display_string(file_logger, "inventory", m_target_inventory);

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
    
    bool campaignInformation::appCriteria_valid(const CampaignProtoEntity_Targeting_AppCriteria& appcriteria
        , const string& request_appid)
    {    
        bool appAllow = appcriteria.in();
        if(request_appid.empty()) return true;
        int numappid = atoi(request_appid.c_str());
        if(appAllow)
        {
           bool inner = false;
           auto idsCnt = appcriteria.ids_size();
           if(idsCnt == 0) return true;
           for(auto i = 0; i < idsCnt; i++)
           {
              int num = appcriteria.ids(i);
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
          if(idsCnt == 0) return true;
          for(auto i = 0; i < idsCnt; i++)
          {
              int num = appcriteria.ids(i);
              if(num == numappid)
              {
                  return false;
              }
          }
       }
       return true;
    }

    bool campaignInformation::WebsiteCriteria_valid(const CampaignProtoEntity_Targeting_WebsiteCriteria& websitecriteria
        , const string& request_webid)
    {    
        bool websiteAllow = websitecriteria.in();
        if(request_webid.empty()) return true;
        int numwebid = atoi(request_webid.c_str());
        if(websiteAllow)
        {
           bool inner = false;
           auto idsCnt = websitecriteria.ids_size();
           if(idsCnt == 0) return true;
           for(auto i = 0; i < idsCnt; i++)
           {
              int num = websitecriteria.ids(i);
              if(num == numwebid)
              {
                  inner = true;
                  break;
              }
           }
           if(inner == false) return false;
       }
       else
       {
          auto idsCnt = websitecriteria.ids_size();
          if(idsCnt == 0) return true;
          for(auto i = 0; i < idsCnt; i++)
          {
              int num = websitecriteria.ids(i);
              if(num == numwebid)
              {
                  return false;
              }
          }
       }
       return true;
    }

    bool campaignInformation::expectecpm(MobileAdRequest &mobile_request, CampaignProtoEntity &campaign_proto
        , string& invetory)
    {
        const CampaignProtoEntity_Targeting& camp_targeting = campaign_proto.targeting();
        if(mobile_request.has_aid())
        {
            const string &camp_vetory = camp_targeting.inventoryquality(); 
            auto   aidstr     = mobile_request.aid();
            const string& appReviewed = aidstr.appreviewed();
            if(appReviewed !=  "1") 
            {
                if(camp_vetory == "reviewed")
                {
                    g_file_logger->trace("app is not reviewed, but camp need reviewed:{0}", appReviewed);
                    return false;
                }
            }
        }
        else if(mobile_request.has_wid())
        {
            const string &camp_vetory = camp_targeting.inventoryquality(); 
            auto   widstr     = mobile_request.wid();
            const string& webReviewed = widstr.reviewed();
            if(webReviewed !=  "1") 
            {
                if(camp_vetory == "reviewed")
                {
                    g_file_logger->trace("web is not reviewed, but camp need reviewed:{0}", webReviewed);
                    return false;
                }
            }
        }
        else
        {
            g_file_logger->trace("mobile request has not aid struecture");
            return false;
        }
            
        if(mobile_request.has_aid())
        {
            auto   aidstr     = mobile_request.aid();
            bool index_externBuying = false;
            if(campaign_proto.has_externalbuying())  index_externBuying = campaign_proto.externalbuying();
            const string& request_network_reselling = aidstr.networkreselling();
            const string& request_app_reselling = aidstr.appresell();
            const string& request_network_idstr = aidstr.networkid();
            if(aidstr.has_networkid() == false)
            {
                g_file_logger->trace("mobile request aid has not network_id:{0}", aidstr.networkid());
                return false;
            }   
            
            int  request_network_reselling_id = atoi(request_network_reselling.c_str());
            int  request_app_reselling_id = atoi(request_app_reselling.c_str());
            int  request_network_idstr_id = atoi(request_network_idstr.c_str());
            if((index_externBuying == false)||(request_network_reselling_id == 0) || (request_app_reselling_id == 0))
            {
                if(request_network_idstr_id != campaign_proto.networkid())  
                {
                    g_file_logger->trace("campid {0} network is not equal between request and index:{1:d},{2:d}", m_id, 
                        request_network_idstr_id, campaign_proto.networkid());
                    return false;
                }
            }

            const CampaignProtoEntity_Targeting_AppCriteria& directapps = camp_targeting.directapps();
            const CampaignProtoEntity_Targeting_AppCriteria& indirectapps = camp_targeting.indirectapps();
            const string& appid = aidstr.id();
            
            if(request_network_idstr_id == campaign_proto.networkid())  
            {  
                m_expectEcmp = campaign_proto.expectcpm();
            }
            else
            {

                m_expectEcmp = campaign_proto.thirdpartyexpectcpm();
            } 

            const string& publishSource = campaign_proto.publishersource();
            if(publishSource == "direct")  
            {   
                if((appid.empty() == false)&&(camp_targeting.has_directapps())&&(appCriteria_valid(directapps, appid) == false))
                {
                    g_file_logger->debug("camp {0} can not show in the directapps", m_id);    
                    return false;
                }   
            }
            else if(publishSource == "third-party")
            {

                if((appid.empty() == false)&&(camp_targeting.has_indirectapps())&&(appCriteria_valid(indirectapps, appid) == false))
                {
                    g_file_logger->debug("camp {0} can not show in the third-party apps", m_id);    
                    return false;
                }
            } 
            else if(publishSource == "direct,third-party")
            {
                if(appid.empty()) return true;
                bool direct = true;
                bool indirect = true;
                if(camp_targeting.has_directapps())
                {
                    direct =  appCriteria_valid(directapps, appid);
                }

                if(camp_targeting.has_indirectapps())
                {
                    indirect = appCriteria_valid(indirectapps, appid);
                }

                if((indirect == false)&&(direct == false))
                {
                    g_file_logger->debug("camp {0} can not show in the directapps or third-party apps", m_id);    
                    return false;
                } 
            }
            else
            {
                g_file_logger->debug("camp {0} publishSource exception", m_id); 
                return false;
            }
            
        }
        if(mobile_request.has_wid())
        {
            MobileAdRequest_Wid widstr = mobile_request.wid();
            bool index_externBuying = false;
            if(campaign_proto.has_externalbuying())  index_externBuying = campaign_proto.externalbuying();
            const string& request_network_reselling = widstr.networkreselling();
            const string& request_web_reselling = widstr.resell();
            const string& request_network_idstr = widstr.networkid();
            if(widstr.has_networkid() == false)
            {
                g_file_logger->trace("mobile request wid has not network_id:{0}", widstr.networkid());
                return false;
            }   
            
            int  request_network_reselling_id = atoi(request_network_reselling.c_str());
            int  request_web_reselling_id = atoi(request_web_reselling.c_str());
            int  request_network_idstr_id = atoi(request_network_idstr.c_str());
            if((index_externBuying == false)||(request_network_reselling_id == 0) || (request_web_reselling_id == 0))
            {
                if(request_network_idstr_id != campaign_proto.networkid())  
                {
                    g_file_logger->trace("campid {0} network is not equal between request and index:{1:d},{2:d}", m_id, 
                        request_network_idstr_id, campaign_proto.networkid());
                    return false;
                }
            }
            
            const CampaignProtoEntity_Targeting_WebsiteCriteria& directWebsites = camp_targeting.directwebsites();
            const CampaignProtoEntity_Targeting_WebsiteCriteria& inDirectWebsites = camp_targeting.indirectwebsites();
            const string& webid = widstr.id();
            
            if(request_network_idstr_id == campaign_proto.networkid())  
            {  
                m_expectEcmp = campaign_proto.expectcpm();
            }
            else
            {

                m_expectEcmp = campaign_proto.thirdpartyexpectcpm();
            } 

            const string& publishSource = campaign_proto.publishersource();
            if(publishSource == "direct")  
            {   
                if((webid.empty() == false)&&(camp_targeting.has_directapps())&&(WebsiteCriteria_valid(directWebsites, webid) == false))
                {
                    g_file_logger->debug("camp {0} can not show in the directwebsites", m_id);    
                    return false;
                }   
            }
            else if(publishSource == "third-party")
            {

                if((webid.empty() == false)&&(camp_targeting.has_indirectapps())&&(WebsiteCriteria_valid(inDirectWebsites, webid) == false))
                {
                    g_file_logger->debug("camp {0} can not show in the third-party websites", m_id);    
                    return false;
                }
            } 
            else if(publishSource == "direct,third-party")
            {
                if(webid.empty()) return true;
                bool direct = true;
                bool indirect = true;
                if(camp_targeting.has_directapps())
                {
                    direct =  WebsiteCriteria_valid(directWebsites, webid);
                }

                if(camp_targeting.has_indirectapps())
                {
                    indirect = WebsiteCriteria_valid(inDirectWebsites, webid);
                }

                if((indirect == false)&&(direct == false))
                {
                    g_file_logger->debug("camp {0} can not show in the directwebsites or third-party websites", m_id);    
                    return false;
                } 
            }
            else
            {
                g_file_logger->debug("camp {0} publishSource exception", m_id); 
                return false;
            }
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
          //  g_worker_logger->trace("index session:{0:d}", sesion);
            for(auto it = appsession.begin(); it != appsession.end(); it++)
            {
                 const MobileAdRequest_AppSession& sessionID_req = *it;
                 int times_req = atoi(sessionID_req.times().c_str());
                 if(sessionID_req.id() == m_id)
                 {
                    if(sesion == 0)  break;
                    if(times_req >= sesion)
                    {
                        g_file_logger->trace("cid {0} app session reached the maximum:{1:d}",m_id, sesion);
                        return false;
                    }
                    break;
                 }
            }
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
                string heightStr = creativeSize.substr(widthCnt, creativeSize.size()-widthCnt);
                width = atoi(widthStr.c_str());
                height = atoi(heightStr.c_str());
                find = true;
           }
        } 
        return find;
    }

    void campaignInformation::displayCampaign(CampaignProtoEntity &campaign_proto)
    {
        ostringstream os;
        os<<"id-state-cur-aid-bidType-value-pacirate-exbuying-ecpm-thirdecpm-networkid-pubSrc:";
        os<<m_id<<"-"<<m_state<<"-"<<m_curency<<"-"<<m_advertiserID<<"-"<<m_biddingType<<"-"<<m_biddingValue
            <<"-"<<campaign_proto.pacingrate()<<"-"<<campaign_proto.externalbuying()<<"-"<<campaign_proto.expectcpm()
            <<"-"<<campaign_proto.thirdpartybiddingtype()<<"-"<<campaign_proto.networkid()<<"-"<<campaign_proto.publishersource();
        g_file_logger->trace("{0}", os.str());

        os.str("");
        const CampaignProtoEntity_Targeting& camp_targeting = campaign_proto.targeting();  
        os<<"web-app-tablet-phone-traffic-inventory-session:";
        os<<camp_targeting.supplytypemobileweb()<<"-"<<camp_targeting.supplytypemobileapp()<<"-"<<camp_targeting.devicetypetablet()
            <<"-"<<camp_targeting.devicetypephone()<<"-"<<camp_targeting.trafficquality()<<"-"<<camp_targeting.inventoryquality()
            <<"-"<<camp_targeting.session();
        g_file_logger->trace("{0}", os.str());

        os.str("");
        const CampaignProtoEntity_Targeting_Frequency& camp_frequency = camp_targeting.frequency();
        os<<"frequecy(no_track,fivem, day, week, month, life):"<<camp_frequency.no_track()<<","<<camp_frequency.five_minutes()<<","
            <<camp_frequency.day()<<","<<camp_frequency.week()<<","<<camp_frequency.month()<<","<<camp_frequency.lifetime();
        g_file_logger->trace("{0}", os.str());

        os.str("");
        os<<"creativelist:[";
        auto creativelist = campaign_proto.creatives();
        for(auto it = creativelist.begin(); it != creativelist.end(); it++)
        {
            if(it != creativelist.begin()) os<<"   ";
            const CampaignProtoEntity_Creatives &creative = *it;
            os<<"size:"<<creative.size()<<"  cids<";
            auto datalist= creative.datas();
            for(auto at = datalist.begin(); at != datalist.end(); at++)
            {
                if(at != datalist.begin()) os<<" ";
                const CampaignProtoEntity_Creative &data = *at;
                os<<data.creativeid(); 
            }
            os<<">";
        }
        os<<"]";
        g_file_logger->trace("{0}\n", os.str());   
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
        double pacingRate = campaign_proto.pacingrate();
        double randNum = rand()*(1.0)/RAND_MAX;
        displayCampaign(campaign_proto);

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
            return false;
        }
        
        if(set_appSession(mobile_request, camp_targeting) == false)
        {
            return false;
        }
        
        /**
         * creatives parse
         */
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

           // g_file_logger->trace("---------------:cw:{0:d},ch:{1:d},rw:{2:d},rh:{3:d}", campWidth, campHeight, ad_width, ad_heigh);
            if(creativeSize.empty()||camp_creative.size()==anySize||((campWidth<=ad_width)&&(campHeight==ad_heigh)))
            {
                int creativeSizeCnt                               = camp_creative.datas_size();
                int idx                                           = rand()%creativeSizeCnt;
                const CampaignProtoEntity_Creative& creative_data = camp_creative.datas(idx);
                m_creativeID                                      = creative_data.creativeid();
                // m_caterogyID                                   = creative_data.categoryid();
                m_ctr                                             = creative_data.ctr();
                m_cs                                              = creative_data.session();
                m_mediaTypeID                                     = creative_data.mediatypeid();
                m_mediaSubTypeID                                  = creative_data.mediasubtypeid();
                m_uuid                                            = creative_data.uuid();
                m_adChannelType                                   = creative_data.adchanneltype();
                g_file_logger->trace("campid {0} find valid size", m_id);
                return true;
            }
        }
        g_file_logger->debug("campid {0} can not find valid size", m_id);
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

