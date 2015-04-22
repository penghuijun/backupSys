#include "rtbkit.h"
using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
bool directDealsObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_id.stringToJson(value, allocator, "id");
    m_bidfloor.toJson(value, allocator, "bidfloor");
    m_bidfloorcur.stringToJson(value, allocator, "bidfloorcur");
    m_wseatArray.stringArrayToJson(value, allocator, "wseat");
    m_wadomainArray.stringArrayToJson(value, allocator, "wadomain");
    m_at.toJson(value, allocator, "at");
}
void directDealsObject::parse_mobile(const MobileAdRequest& mobileReq)
{}

bool pmpObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
	m_private_auction.toJson(value, allocator, "private_auction");
	m_dealsArray.objArrayToJson(value, allocator, "deals");

}
void pmpObject::parse_mobile(const MobileAdRequest& mobileReq)
{}   

bool regulationsObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_coppa.toJson(value, allocator, "coppa");
}

void regulationsObject::parse_mobile(const MobileAdRequest& mobileReq)
{
}

bool segmentObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{	
    m_id.stringToJson(value, allocator, "id");
    m_name.stringToJson(value, allocator, "name");
    m_value.stringToJson(value, allocator, "value");
}
void segmentObject::parse_mobile(const MobileAdRequest& mobileReq)
{}

bool dataObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_id.stringToJson(value, allocator, "id");
    m_name.stringToJson(value, allocator, "name");
    m_segment.objArrayToJson(value, allocator, "segment");
}
void dataObject::parse_mobile(const MobileAdRequest& mobileReq)
{}

bool producerObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_id.stringToJson(value, allocator, "id");
    m_name.stringToJson(value, allocator, "name");
    m_domain.stringToJson(value, allocator, "value");
    m_cat.stringArrayToJson(value, allocator, "cat");
}

void producerObject::parse_mobile(const MobileAdRequest& mobileReq)
{}

bool contentObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_id.stringToJson(value, allocator, "id");
    m_episode.toJson(value, allocator, "episode");
    m_title.stringToJson(value, allocator, "title");
    m_series.stringToJson(value, allocator, "series");
    m_season.stringToJson(value, allocator, "season");
    m_url.stringToJson(value, allocator, "url");
    m_catArray.stringArrayToJson(value, allocator, "cat");
    m_videoquality.toJson(value, allocator, "videoquality");
    m_keywords.stringToJson(value, allocator, "keywords");
    m_contentrating.stringToJson(value, allocator, "contentrating");
    m_userrating.stringToJson(value, allocator, "userrating");
    m_context.stringToJson(value, allocator, "context");

    m_livestream.toJson(value, allocator, "livestream");
    m_sourcerelationship.toJson(value, allocator, "sourcerelationship");
    m_producer.objToJson(value, allocator, "producer");
    m_len.toJson(value, allocator, "len");
    m_qugmediarating.toJson(value, allocator, "qugmediarating");
    m_embeddable.toJson(value, allocator, "embeddable");
    m_language.stringToJson(value, allocator, "language");
}

void contentObject::parse_mobile(const MobileAdRequest& mobileReq)
{}


bool siteObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_id.stringToJson(value, allocator, "id");
    m_name.stringToJson(value, allocator, "name");
    m_domain.stringToJson(value, allocator, "domain");
    m_catArray.stringArrayToJson(value, allocator, "cat");
    m_sectioncatArray.stringArrayToJson(value, allocator, "sectioncat");
    m_pagecatArray.stringArrayToJson(value, allocator, "pagecat");
    m_page.stringToJson(value, allocator, "page");
    m_privacypolicy.toJson(value, allocator, "privacypolicy");   
    m_ref.stringToJson(value, allocator, "ref");
    m_search.stringToJson(value, allocator, "search");
    m_publisher.objToJson(value, allocator, "publisher");
    m_content.objToJson(value, allocator, "content");
    m_keywords.stringToJson(value, allocator, "keywords");
}

void siteObject::parse_mobile(const MobileAdRequest& mobileReq)
{}


bool userObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_id.stringToJson(value, allocator, "id");
    m_buyeruid.stringToJson(value, allocator, "buyeruid");
    m_yob.toJson(value, allocator, "yob");
    m_gender.stringToJson(value, allocator, "gender");
    m_keywords.stringToJson(value, allocator, "keywords");
    m_customdata.stringToJson(value, allocator, "customdata");
    m_geo.objToJson(value, allocator, "geo");
    m_dataArray.objArrayToJson(value, allocator, "data");
}
void userObject::parse_mobile(const MobileAdRequest& mobileReq)
{
    const MobileAdRequest_User &user = mobileReq.user();
    m_id = user.uid();
    MobileAdRequest_Gender gendor = user.gender();
    if(gendor == MobileAdRequest_Gender_FEMALE)
    {
        string gdr("F");
        m_gender = gdr;
    }
    else if(gendor == MobileAdRequest_Gender_MALE)
    {
        string gdr("M");
        m_gender = gdr;
    }
    m_yob = atoi(user.age().c_str());
}


bool geoObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_lat.toJson(value, allocator, "lat");
    m_lon.toJson(value, allocator, "lon");
    m_country.stringToJson(value, allocator, "country");
    m_region.stringToJson(value, allocator, "region");
    m_regionfips104.stringToJson(value, allocator, "regionfips104");
    m_metro.stringToJson(value, allocator, "metro");
    m_city.stringToJson(value, allocator, "city");
    m_zip.stringToJson(value, allocator, "zip");
    m_type.toJson(value, allocator, "type");
}
void geoObject::parse_mobile(const MobileAdRequest& mobileReq)
{
    const MobileAdRequest_Device &dev = mobileReq.device();
    const MobileAdRequest_GeoInfo &geo = mobileReq.geoinfo();
    const MobileAdRequest_rtbGeoInfo &rtbGeo = geo.rtbgeoinfo();
    if(geo.latitude().empty() == false) m_lat = atof(geo.latitude().c_str());
    if(geo.longitude().empty() == false) m_lon = atof(geo.longitude().c_str());
    if(rtbGeo.rtb_coutry().empty() == false) m_country = rtbGeo.rtb_coutry();
    if(rtbGeo.rtb_region().empty() == false) m_region = rtbGeo.rtb_region();
    if(rtbGeo.rtb_city().empty() == false) m_city = rtbGeo.rtb_city();
}

bool deviceObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_dnt.toJson(value, allocator, "dnt");
    m_ua.stringToJson(value, allocator, "ua");
    m_ip.stringToJson(value, allocator, "ip");
    m_geo.objToJson(value, allocator, "geo");
    m_didsha1.stringToJson(value, allocator, "didsha1");
    m_didmd5.stringToJson(value, allocator, "didmd5");
    m_dpidsha1.stringToJson(value, allocator, "dpidsha1");
    m_dpidmd5.stringToJson(value, allocator, "dpidmd5");
    m_macsha1.stringToJson(value, allocator, "macsha1");
    m_macmd5.stringToJson(value, allocator, "macmd5");
    m_ipv6.stringToJson(value, allocator, "ipv6");
    m_carrier.stringToJson(value, allocator, "carrier");
    m_language.stringToJson(value, allocator, "language");
    m_make.stringToJson(value, allocator, "make");
    m_model.stringToJson(value, allocator, "model");
    m_os.stringToJson(value, allocator, "os");
    m_osv.stringToJson(value, allocator, "osv");
    m_js.toJson(value, allocator, "js");
    m_connectiontype.toJson(value, allocator, "connectiontype");
    m_devicetype.toJson(value, allocator, "devicetype");
    m_flashver.stringToJson(value, allocator, "flashver");
    m_ifa.stringToJson(value, allocator, "ifa");  
}
void deviceObject::parse_mobile(const MobileAdRequest& mobileReq)
{
    const MobileAdRequest_Device &dev = mobileReq.device();
    const MobileAdRequest_GeoInfo &geo = mobileReq.geoinfo();
    const MobileAdRequest_rtbDevice& rtbdev = dev.rtbdevice();
    m_ip = dev.ipaddress();
    m_ua = dev.ua();
    m_ifa = dev.udid();
    m_dpidmd5 = dev.hidmd5();
    m_dpidsha1 = dev.hidsha1();
    m_language = rtbdev.rtb_language();
    m_os = rtbdev.rtb_platform();
    m_osv = rtbdev.rtb_platformversion();
    m_devicetype = atoi(rtbdev.rtb_devicetype().c_str());
    m_connectiontype = atoi(rtbdev.rtb_connectiontype().c_str());
    m_make = rtbdev.rtb_vendor();
    m_model = rtbdev.rtb_modelname();
    m_geo.parse_mobile(mobileReq);
    if((geo.mnc().empty() == false)||(geo.mcc().empty() == false))
    {
        ostringstream os;
        os<<geo.mnc()<<"-"<<geo.mcc();
        m_carrier = os.str();
    }
}


bool publisherObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{	
    m_id.stringToJson(value, allocator, "id");
    m_name.stringToJson(value, allocator, "name");
    m_domain.stringToJson(value, allocator, "domain");
    m_cat.stringArrayToJson(value, allocator, "cat");
}
void publisherObject::parse_mobile(const MobileAdRequest& mobileReq)
{
    if(mobileReq.aid().empty() == false) m_id = mobileReq.aid();
}


bool appObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_id.stringToJson(value, allocator, "id");
    m_name.stringToJson(value, allocator, "name");
    m_domain.stringToJson(value, allocator, "domain");
    m_catArray.stringArrayToJson(value, allocator, "cat");
    m_sectioncatArray.stringArrayToJson(value, allocator, "sectioncat");
    m_pagecatArray.stringArrayToJson(value, allocator, "mimes");
    m_ver.stringToJson(value, allocator, "ver");
    m_bundle.stringToJson(value, allocator, "bundle");
    m_privacypolicy.toJson(value, allocator, "privacypolicy");
    m_paid.toJson(value, allocator, "paid");    
    m_publisher.objToJson(value, allocator, "publisher");
    m_content.objToJson(value, allocator, "content");
    m_keywords.stringToJson(value, allocator, "keywords");
    m_storeurl.stringToJson(value, allocator, "storeurl");
}
void appObject::parse_mobile(const MobileAdRequest& mobileReq)
{
    m_id = mobileReq.appid();
    m_publisher.parse_mobile(mobileReq);
    if(mobileReq.schema().empty() == false) m_domain = mobileReq.schema();
    if(mobileReq.packagename().empty() == false)m_bundle = mobileReq.packagename();
    if(mobileReq.section().empty() == false)
    {
        const string& section = mobileReq.section();
        m_sectioncatArray.push_back(section);
    }
}

bool videoObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_mimesArray.stringArrayToJson(value, allocator, "mimes");
    m_minduration.toJson(value, allocator, "minduration"); 
    m_maxduration.toJson(value, allocator, "maxduration");  
    m_protocol.toJson(value, allocator, "protocol");
    m_protocolsArray.arrayToJson(value, allocator, "protocols");
    m_w.toJson(value, allocator, "w");
    m_h.toJson(value, allocator, "h");
    m_startdelay.toJson(value, allocator, "startdelay");
    m_linearity.toJson(value, allocator, "linearity");
    m_sequence.toJson(value, allocator, "sequence");
    m_battrArray.arrayToJson(value, allocator, "battr");

      m_maxextended.toJson(value, allocator, "maxextended");
    m_minbitrate.toJson(value, allocator, "minbitrate");
    m_maxbitrate.toJson(value, allocator, "maxbitrate");
    m_boxingallowed.toJson(value, allocator, "boxingallowed");
    m_playbackmethodArray.arrayToJson(value, allocator, "playbackmethod");
    m_deliveryArray.arrayToJson(value, allocator, "delivery");
    m_pos.toJson(value, allocator, "pos");
    m_companionadArray.objArrayToJson(value, allocator, "companionad");
    m_api.toJson(value, allocator, "api");
    m_companiontype.toJson(value, allocator, "companiontype");

}

void videoObject::parse_mobile(const MobileAdRequest& mobileReq)
{}

void bannerObject::parse_mobile(const MobileAdRequest& mobileReq)
{
    if(mobileReq.adspacewidth().empty() == false)
    {
        m_w = atoi(mobileReq.adspacewidth().c_str());
    }
    if(mobileReq.adspaceheight().empty() == false)
    {
        m_h = atoi(mobileReq.adspaceheight().c_str());
    }
}
bool bannerObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_w.toJson(value, allocator, "w");
    m_h.toJson(value, allocator, "h");
    m_wmax.toJson(value, allocator, "wmax");
    m_wmin.toJson(value, allocator, "wmin");
    m_hmax.toJson(value, allocator, "hmax");
    m_hmin.toJson(value, allocator, "hmin");
    m_id.stringToJson(value, allocator, "id");
    m_pos.toJson(value, allocator, "pos");
    m_topFrame.toJson(value, allocator, "topFrame");
    m_mimesArray.stringArrayToJson(value, allocator, "mimesArray");
    m_btypeArray.arrayToJson(value, allocator, "btypeArray");
    m_battrArray.arrayToJson(value, allocator, "battrArray");
    m_expdir.arrayToJson(value, allocator, "expdir");
    m_api.arrayToJson(value, allocator, "api");
}

void nativeObject::parse_mobile(const MobileAdRequest& mobileReq)
{
}
bool nativeObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_request.stringToJson(value, allocator, "request");
    m_ver.stringToJson(value, allocator, "ver");
    m_api.arrayToJson(value, allocator, "api");
    m_battr.arrayToJson(value, allocator, "battr");
}


void impressionObject::parse_mobile(const MobileAdRequest& mobileReq)
{
    MobileAdRequest_AdType type = mobileReq.type();
    if(type == MobileAdRequest_AdType_BANNER)
    {
        m_banner.parse_mobile(mobileReq);
    }
    else if(type == MobileAdRequest_AdType_NATIVE)
    {
        m_native.parse_mobile(mobileReq);
    }
    
    if(mobileReq.version().empty() == false)
    {
        m_displaymanagerver = mobileReq.version();
    }
}

bool impressionObject::toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator)
{
    m_id.stringToJson(value, allocator, "id");
    m_displaymanager.stringToJson(value, allocator, "displaymanager");
    m_displaymanagerver.stringToJson(value, allocator, "displaymanagerver");
    m_instl.toJson(value, allocator, "instl");
    m_tagid.stringToJson(value, allocator, "tagid");
    m_bidfloor.toJson(value, allocator, "bidfloor");
    m_bidfloorcur.stringToJson(value, allocator, "bidfloorcur");
    m_secure.toJson(value, allocator, "secure");
    m_iframebusterArray.stringToJson(value, allocator, "iframebuster");

    m_banner.objToJson(value, allocator, "banner");
    m_video.objToJson(value, allocator, "video");
    m_native.objToJson(value, allocator, "native");
    
    m_pmp.objToJson(value, allocator, "pmp");

    return true;
}

void bidRequestObject::parse_mobile(const string& mobileReqStr)
{
    MobileAdRequest mobileReq;
    mobileReq.ParseFromString(mobileReqStr); 
    m_id = mobileReq.id();//reqired

    int i = 1;
    ostringstream os;
    os<<i++;
    string impresionID = os.str();
    impressionObject *imp = new impressionObject(impresionID);
    imp->parse_mobile(mobileReq);
    m_impArray.push_back(imp);// reqired
    if(mobileReq.apptype()=="app")//app or web
    {
        m_app.parse_mobile(mobileReq);
    }
    m_device.parse_mobile(mobileReq);
    m_user.parse_mobile(mobileReq);
}


bool bidRequestObject::toJson(string& jsonStr)
{
	rapidjson::Document document;
   	document.SetObject();
   	rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
    
    m_id.stringToJson(document, allocator, "id");
    m_app.objToJson(document, allocator, "app");
    m_site.objToJson(document, allocator, "site");
    m_device.objToJson(document, allocator, "device");    
    m_user.objToJson(document, allocator, "user");
    m_regs.objToJson(document, allocator, "regs");

    m_at.toJson(document, allocator, "at");
    m_tmax.toJson(document, allocator, "tmax");
    m_allimps.toJson(document, allocator, "allimps");
    m_wseatArray.stringArrayToJson(document, allocator, "wseat");
    m_curArray.stringArrayToJson(document, allocator, "cur");
    m_bcatArray.stringArrayToJson(document, allocator, "bcat");
    m_badvArray.stringArrayToJson(document, allocator, "badv");
    m_impArray.objArrayToJson(document, allocator, "impressions");

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer); 
    document.Accept(writer);    
    jsonStr = buffer.GetString();

    return true;
}


bidRequestObject::~bidRequestObject()
{
    m_impArray.eraseArray();
}


void bidObject::parse(const rapidjson::Value &value)
{
	if((value.IsNull() == false)&&(value.IsObject() == false))
	{
		return;
	}	
	if((value["id"].IsNull() == false)&&(value["id"].IsString()))
	{
		m_id = value["id"].GetString();
	}
	if((value["impid"].IsNull() == false)&&(value["impid"].IsString()))
	{
		m_impid = value["impid"].GetString();
	}
	if((value["price"].IsNull() == false)&&(value["price"].IsDouble()))
	{
		m_price = value["price"].GetDouble();
	}
	if((value["adid"].IsNull() == false)&&(value["adid"].IsString()))
	{
		m_adid = value["adid"].GetString();
	}
	if((value["nurl"].IsNull() == false)&&(value["nurl"].IsString()))
	{
		m_nurl = value["nurl"].GetString();
	}
	if((value["adm"].IsNull() == false)&&(value["adm"].IsString()))
	{
		m_adm = value["adm"].GetString();
	}
	if((value["lurl"].IsNull() == false)&&(value["lurl"].IsString()))
	{
		m_lurl = value["lurl"].GetString();
	}
	if((value["cid"].IsNull() == false)&&(value["cid"].IsString()))
	{
		m_cid = value["cid"].GetString();
	}
	if((value["crid"].IsNull() == false)&&(value["crid"].IsString()))
	{
		m_crid = value["crid"].GetString();
	}
	if((value["dealid"].IsNull() == false)&&(value["dealid"].IsString()))
	{
		m_dealid = value["dealid"].GetString();
	}
	if((value["h"].IsNull() == false)&&(value["h"].IsInt()))
	{
		m_h = value["h"].GetInt();
	}
	if((value["w"].IsNull() == false)&&(value["w"].IsInt()))
	{
		m_w = value["w"].GetInt();
	}
		
    const rapidjson::Value &j_adomain = value["adomain"];
    if((j_adomain.IsNull()==false)&&j_adomain.IsArray())
    {
        for(size_t j = 0; j < j_adomain.Size(); j++)
        {
            const rapidjson::Value &j_val = j_adomain[j];
			if((j_val.IsNull() == false)&&(j_val.IsString()))
			{
				const char* ch = j_val.GetString();
				m_adomainArray.push_back(ch);
			}				
		}
    }

	const rapidjson::Value &j_attr = value["attr"];
	if((j_attr.IsNull()==false)&&j_attr.IsArray())
	{
		for(size_t j = 0; j < j_attr.Size(); j++)
		{
			const rapidjson::Value &j_val = j_attr[j];
			if((j_val.IsNull() == false)&&(j_val.IsInt()))
			{
				int ch = j_val.GetInt();
				m_attrArray.push_back(ch);
			}				
		}
	}

}

bool bidObject::toProtobuf(MobileAdResponse_mobileBid *bidContent)
{
	ostringstream os;
	string value;
	if(m_price.valid())
	{
		os<<m_price.get_data()<<endl;
		value = os.str();
	}
	bidContent->set_biddingtype("CPM");
	bidContent->set_biddingvalue(value);
	bidContent->set_campaignid(m_cid);
	MobileAdResponse_Creative* creative = bidContent->add_creative();
	creative->set_creativeid(m_crid);
	creative->set_admarkup(m_adm);
		
	if(m_h.valid())
	{
		os.str("");
		os<<m_h.get_data()<<endl;
		value = os.str();
	}
	creative->set_height(value);
		
	if(m_w.valid())
	{
		os.str("");
		os<<m_w.get_data()<<endl;
		value = os.str();
	}
	creative->set_width(value);
}


void seatBidObject::parse(const rapidjson::Value &value)
{
    if((value.IsNull() == false)&&(value.IsObject() == false))
    {
        return;
    }   
    const rapidjson::Value &j_bid_arr = value["bid"];
    if((j_bid_arr.IsNull()==false)&&j_bid_arr.IsArray())
    {
        for(size_t j = 0; j < j_bid_arr.Size(); j++)
        {
            const rapidjson::Value &j_bid = j_bid_arr[j];
            if((j_bid.IsNull() == false)&&(j_bid.IsObject()))
            {
                bidObject *bid = new bidObject;
                bid->parse(j_bid);
                m_bidArray.push_back(bid);
            }
        }
    }
    if((value["seat"].IsNull() == false)&&(value["seat"].IsString()))
    {
        m_seat = value["seat"].GetString();
    }
    if((value["group"].IsNull() == false)&&(value["group"].IsInt()))
    {
        m_group = value["group"].GetInt();
    }    
}
bool seatBidObject::toProtobuf(MobileAdResponse_mobileBid *bidContent)
{
    for(auto it = m_bidArray.begin(); it != m_bidArray.end(); it++)
    {
        bidObject* bid = *it;
        bid->toProtobuf(bidContent);
    }
}

void bidResponseObject::parse(rapidjson::Document& document)
{
    if((document.IsNull() == false)&&(document.IsObject() == false))
    {
        return;
    }   
    
    if((document["id"].IsNull() == false)&&(document["id"].IsString()))
    {
        m_id = document["id"].GetString();
    }
    if((document["bidid"].IsNull() == false)&&(document["bidid"].IsString()))
    {
        m_bidid = document["bidid"].GetString();
    }
    if((document["cur"].IsNull() == false)&&(document["cur"].IsString()))
    {
        m_cur = document["cur"].GetString();
    }
    if((document["customdata"].IsNull() == false)&&(document["customdata"].IsString()))
    {
        m_customdata = document["customdata"].GetString();
    }
    if((document["nbr"].IsNull() == false)&&(document["nbr"].IsInt()))
    {
        m_nbr = document["nbr"].GetInt();
    }

    const  rapidjson::Value &j_seatbid = document["seatbid"];
    if((j_seatbid.IsNull()==false)&&j_seatbid.IsArray())
    {
         for(size_t i = 0; i < j_seatbid.Size(); i++)
         {
             const rapidjson::Value &j_seat_bid = j_seatbid[i];
             if((j_seat_bid.IsNull()==false)&&j_seatbid.IsObject())
             {
                seatBidObject* seatBid = new seatBidObject;
                seatBid->parse(j_seat_bid);
                m_seatbidArray.push_back(seatBid);
             }
         }
    }
}
bool bidResponseObject::toProtobuf(const string& busicode, const string& datacode, CommonMessage& commMsg)
{
    char buf[BUF_SIZE]; 
    memset(buf, 0x00, sizeof(buf));
    MobileAdResponse mobile_resp;
    mobile_resp.set_id(m_id);
    mobile_resp.set_bidid(m_bidid);
    MobileAdResponse_Bidder *bidder = mobile_resp.mutable_bidder();
    bidder->set_bidderid(m_bidid);
    for(auto it = m_seatbidArray.begin(); it != m_seatbidArray.end(); it++)
    {
        seatBidObject* seatbidd = *it;
        if(seatbidd)
        {
            MobileAdResponse_mobileBid *bidContent = mobile_resp.add_bidcontent();
            bidContent->set_currency(m_cur);
            seatbidd->toProtobuf(bidContent);
        }
    }
    
    mobile_resp.SerializeToArray(buf, sizeof(buf));
    int rspSize = mobile_resp.ByteSize();
    commMsg.set_businesscode(busicode);
    commMsg.set_datacodingtype(datacode);
    commMsg.set_data(buf, rspSize);
    return true;
}

