#ifndef __RTBKIT_H__
#define __RTBKIT_H__
#include <iostream>
#include <string>
#include <vector>
#include "document.h"		// rapidjson's DOM-style API
#include "prettywriter.h"	// for stringify JSON
#include "filestream.h"	// wrapper of C stream for prettywriter as output
#include "stringbuffer.h"
#include "CommonMessage.pb.h"
#include "AdVastRequestTemplate.pb.h"
#include "AdBidderResponseTemplate.pb.h"
#include "MobileAdRequest.pb.h"
#include "MobileAdResponse.pb.h"
#include "adConfig.h"

using namespace com::rj::protos::mobile;
using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace rapidjson;
using namespace std;

template<class T, class D=string>
class bidReqDataType
{
public:
	bidReqDataType(){}
	bidReqDataType(const T& data)
	{
		m_data = data;
		m_valid = true;
	}
	void set(T data)
	{
		m_data = data;
		m_valid = true;
	}

	void parse_mobile(const MobileAdRequest& mobileReq)
	{
		m_data.parse_mobile(mobileReq);
		m_valid = true;
	}

	void push_back(const D& data)
	{
		m_data.push_back(data);
		m_valid = true;
	}	
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator,const char* name)
	{
		if(m_valid)
		{
			value.AddMember(name, m_data, allocator);
		}
	}

	bool objToJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator, const char* name)
	{
		rapidjson::Value jsonData(rapidjson::kObjectType);
		m_data.toJson(jsonData, allocator);//m_data change to json
		toJson(value, jsonData, allocator, name);
	}

	bool stringToJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator,const char* name)
	{
		if(m_valid&&(m_data.empty()==false))
		{
			value.AddMember(name, m_data.c_str(), allocator);
		}
	}
	bool stringArrayToJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator,const char* name)
	{	
		if(m_valid)
		{
			rapidjson::Value arrayA(rapidjson::kArrayType);
			for(auto it = m_data.begin(); it != m_data.end(); it++)
			{
				const string &arrayData = *it;
				arrayA.PushBack(arrayData.c_str(), allocator);
			}
			value.AddMember(name, arrayA, allocator);
		}
	}
	bool arrayToJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator,const char* name)
	{	
		if(m_valid)
		{
			rapidjson::Value arrayA(rapidjson::kArrayType);
			for(auto it = m_data.begin(); it != m_data.end(); it++)
			{
				const D &arrayData = *it;
				arrayA.PushBack(arrayData, allocator);
			}
			value.AddMember(name, arrayA, allocator);
		}
	}

	bool objArrayToJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator,const char* name)
	{	
		if(m_valid)
		{
			rapidjson::Value arrayA(rapidjson::kArrayType);
			for(auto it = m_data.begin(); it != m_data.end(); it++)
			{
				const D arrayData = *it;
				if(arrayData)
				{
					rapidjson::Value objValue(rapidjson::kObjectType);
					arrayData->toJson(objValue, allocator);
					arrayA.PushBack(objValue, allocator);
				}
			}
			value.AddMember(name, arrayA, allocator);
		}
	}

	bidReqDataType& operator=(const T& data)
	{
		m_data = data;
		m_valid = true;
		return *this;
	}


	
	const T& get_data() const {return m_data;}
	bool     get_valid() const {return m_valid;}
	bool     valid() const{return (m_valid == true)?true:false;}
	void eraseArray()
	{
		for(auto it = m_data.begin(); it != m_data.end();)
		{
			delete *it;
			it = m_data.erase(it);
		}
	}
	~bidReqDataType(){}
private:
	bool toJson(rapidjson::Value& value, rapidjson::Value& data, rapidjson::Document::AllocatorType &allocator,const char* name)
	{
		if(m_valid)
		{
			value.AddMember(name, data, allocator);
		}
	}

	T     m_data;
	bool  m_valid = false;
};


class geoObject
{
public:
	geoObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~geoObject(){}
private:
	bidReqDataType<float> m_lat;
	bidReqDataType<float> m_lon;
	bidReqDataType<string> m_country;
	bidReqDataType<string> m_region;
	bidReqDataType<string> m_regionfips104;
	bidReqDataType<string> m_metro;
	bidReqDataType<string> m_city;
	bidReqDataType<string> m_zip;
	bidReqDataType<int> m_type;
};

class directDealsObject
{
public:
	directDealsObject()
	{
		m_bidfloor = 0;
		m_bidfloorcur = "USD";
	}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~directDealsObject(){}
private:
	bidReqDataType<string> m_id;
	bidReqDataType<float> m_bidfloor;
	bidReqDataType<string> m_bidfloorcur;
	bidReqDataType<vector<string>> m_wseatArray;
	bidReqDataType<vector<string>> m_wadomainArray;
	bidReqDataType<int> m_at;
};
class pmpObject
{
public:
	pmpObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~pmpObject()
	{
		m_dealsArray.eraseArray();
	}
private:
	bidReqDataType<int> m_private_auction;
	bidReqDataType<vector<directDealsObject*>, directDealsObject*> m_dealsArray;
};
class regulationsObject
{
public:
	regulationsObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~regulationsObject(){}
private:
	bidReqDataType<int> m_coppa;

};
class segmentObject
{
public:
	segmentObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~segmentObject(){}
private:
	bidReqDataType<string> m_id;
	bidReqDataType<string> m_name;
	bidReqDataType<string> m_value;

};
class dataObject
{
public:
	dataObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~dataObject()
	{
		m_segment.eraseArray();
	}
private:
	
	bidReqDataType<string> m_id;
	bidReqDataType<string> m_name;
	bidReqDataType<vector<segmentObject*>, segmentObject*> m_segment;

};
class userObject
{
public:
	userObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);

	void parse_mobile(const MobileAdRequest& mobileReq);
	~userObject()
	{
		m_dataArray.eraseArray();
	}
private:
	bidReqDataType<string> m_id;
	bidReqDataType<string> m_buyeruid;
	bidReqDataType<int> m_yob;
	bidReqDataType<string> m_gender;
	bidReqDataType<string> m_keywords;
	bidReqDataType<string> m_customdata;
	bidReqDataType<geoObject> m_geo;
	bidReqDataType<vector<dataObject*>, dataObject*> m_dataArray;
};


class deviceObject
{
public:
	deviceObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~deviceObject(){}
private:
	bidReqDataType<int> m_dnt;
	bidReqDataType<string> m_ua;
	bidReqDataType<string> m_ip;
	bidReqDataType<geoObject> m_geo;
	bidReqDataType<string> m_didsha1;
	bidReqDataType<string> m_didmd5;
	bidReqDataType<string> m_dpidsha1;
	bidReqDataType<string> m_dpidmd5;
	bidReqDataType<string> m_macsha1;
	bidReqDataType<string> m_macmd5;
	bidReqDataType<string> m_ipv6;
	bidReqDataType<string> m_carrier;
	bidReqDataType<string> m_language;
	bidReqDataType<string> m_make;
	bidReqDataType<string> m_model;
	bidReqDataType<string> m_os;
	bidReqDataType<string> m_osv;
	bidReqDataType<int> m_js;
	bidReqDataType<int> m_connectiontype;
	bidReqDataType<int> m_devicetype;
	bidReqDataType<string> m_flashver;
	bidReqDataType<string> m_ifa;
};
class producerObject
{
public:
	producerObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~producerObject(){}
private:
	bidReqDataType<string> m_id;
	bidReqDataType<string> m_name;
	bidReqDataType<vector<string>> m_cat;
	bidReqDataType<string> m_domain;
};

class publisherObject
{
public:
	publisherObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~publisherObject(){}
private:
	bidReqDataType<string> m_id;
	bidReqDataType<string> m_name;
	bidReqDataType<vector<string>> m_cat;
	bidReqDataType<string> m_domain;
};

class contentObject
{
public:
	contentObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~contentObject(){}
private:
	bidReqDataType<string> m_id;
	bidReqDataType<int> m_episode;
	bidReqDataType<string> m_title;
	bidReqDataType<string> m_series;
	bidReqDataType<string> m_season;
	bidReqDataType<string> m_url;
	bidReqDataType<vector<string>> m_catArray;
	bidReqDataType<int> m_videoquality;
	bidReqDataType<string> m_keywords;
	bidReqDataType<string> m_contentrating;
	bidReqDataType<string> m_userrating;
	bidReqDataType<string> m_context;
	bidReqDataType<int> m_livestream;
	bidReqDataType<int> m_sourcerelationship;
	bidReqDataType<producerObject> m_producer;
	bidReqDataType<int> m_len;
	bidReqDataType<int> m_qugmediarating;
	bidReqDataType<int> m_embeddable;
	bidReqDataType<string> m_language;
};
class appObject
{
public:
	appObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~appObject(){}
private:
	bidReqDataType<string> m_id;	
	bidReqDataType<string> m_name;
	bidReqDataType<string> m_domain;
	bidReqDataType<vector<string>> m_catArray;
	bidReqDataType<vector<string>> m_sectioncatArray;
	bidReqDataType<vector<string>> m_pagecatArray;
	bidReqDataType<string> m_ver;
	bidReqDataType<string> m_bundle;
	bidReqDataType<int> m_privacypolicy;
	bidReqDataType<int> m_paid;
	bidReqDataType<publisherObject> m_publisher;
	bidReqDataType<contentObject> m_content;
	bidReqDataType<string> m_keywords;
	bidReqDataType<string> m_storeurl;

};

class siteObject
{
public:
	siteObject(){}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~siteObject(){}
private:
	bidReqDataType<string> m_id;
	bidReqDataType<string> m_name;
	bidReqDataType<string> m_domain;
	bidReqDataType<vector<string>> m_catArray;
	bidReqDataType<vector<string>> m_sectioncatArray;
	bidReqDataType<vector<string>> m_pagecatArray;
	bidReqDataType<string> m_page;
	bidReqDataType<int> m_privacypolicy;
	bidReqDataType<string> m_ref;
	bidReqDataType<string> m_search;
	bidReqDataType<publisherObject> m_publisher;
	bidReqDataType<contentObject> m_content;
	bidReqDataType<string> m_keywords;
};

class nativeObject
{
public:
	nativeObject()
	{
	}
	void parse_mobile(const MobileAdRequest& mobileReq);
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	~nativeObject(){}
private:
	bidReqDataType<string> m_request;	
	bidReqDataType<string> m_ver;
	bidReqDataType<vector<int>, int> m_api;
	bidReqDataType<vector<int>, int> m_battr;
};


class bannerObject
{
public:
	bannerObject()
	{
		m_topFrame = 0;
	}
	void parse_mobile(const MobileAdRequest& mobileReq);
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	~bannerObject(){}
private:
	bidReqDataType<int> m_w;	
	bidReqDataType<int> m_h;
	bidReqDataType<int> m_wmax;
	bidReqDataType<int> m_wmin;
	bidReqDataType<int> m_hmax;
	bidReqDataType<int> m_hmin;
	bidReqDataType<string> m_id;
	bidReqDataType<int> m_pos;
	bidReqDataType<vector<int>, int> m_btypeArray;
	bidReqDataType<vector<int>, int> m_battrArray;
	bidReqDataType<vector<string>> m_mimesArray;
	bidReqDataType<int> m_topFrame;
	bidReqDataType<vector<int>, int> m_expdir;
	bidReqDataType<vector<int>, int> m_api;
};

class videoObject
{
public:
	videoObject(){}
	void init()
	{
		m_sequence = 1;
		m_boxingallowed = 1;
	}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);	
	~videoObject()
	{
		m_companionadArray.eraseArray();
	}
private:
	bidReqDataType<vector<string>> m_mimesArray;	
	bidReqDataType<int> m_minduration;
	bidReqDataType<int> m_maxduration;
	bidReqDataType<int> m_protocol;
	bidReqDataType<vector<int>, int> m_protocolsArray;
	bidReqDataType<int> m_w;	
	bidReqDataType<int> m_h;
	bidReqDataType<int> m_startdelay;
	bidReqDataType<int> m_linearity;
	bidReqDataType<int> m_sequence;
	bidReqDataType<vector<int>, int> m_battrArray;
	bidReqDataType<int> m_maxextended;	
	bidReqDataType<int> m_minbitrate;
	bidReqDataType<int> m_maxbitrate;
	bidReqDataType<int> m_boxingallowed;
	bidReqDataType<vector<int>, int> m_playbackmethodArray;
	bidReqDataType<vector<int>, int> m_deliveryArray;
	bidReqDataType<int> m_pos;
	bidReqDataType<vector<bannerObject*>, bannerObject*> m_companionadArray;
	bidReqDataType<int> m_api;
	bidReqDataType<int> m_companiontype;
};

class impressionObject
{
public:
	impressionObject()
	{
		m_id = "1";
		init();
	}
	impressionObject(const string& id)
	{
		m_id = id;
		init();
	}
	void init()
	{
		m_instl = 0;
		m_bidfloor = 0;
		m_bidfloorcur = "USD";
	}
	bool toJson(rapidjson::Value& value, rapidjson::Document::AllocatorType &allocator);
	void parse_mobile(const MobileAdRequest& mobileReq);
	~impressionObject(){}
private:
	bidReqDataType<string> m_id;
	bidReqDataType<bannerObject> m_banner;
	bidReqDataType<videoObject> m_video;
	bidReqDataType<nativeObject> m_native;	
	bidReqDataType<string> m_displaymanager;		
	bidReqDataType<string> m_displaymanagerver;	
	bidReqDataType<int> m_instl;		
	bidReqDataType<string> m_tagid;	
	bidReqDataType<float> m_bidfloor;		
	bidReqDataType<string> m_bidfloorcur;	
	bidReqDataType<int> m_secure;		
	bidReqDataType<string> m_iframebusterArray;	
	bidReqDataType<pmpObject> m_pmp;
};


class bidRequestObject
{
public:
	bidRequestObject()
	{
		m_at = 2;
		m_allimps = 0;
		m_tmax = 80;	
		m_test = 0;
		m_curArray.push_back("USD");
	}
	bool toJson(string& jsonStr);
	void parse_mobile(const string& mobileReqStr);
 	~bidRequestObject();
private:
	bidReqDataType<string> m_id;
	bidReqDataType<vector<impressionObject*>, impressionObject*> m_impArray;	
	bidReqDataType<siteObject> m_site;
	bidReqDataType<appObject> m_app;
	bidReqDataType<deviceObject> m_device;
	bidReqDataType<userObject> m_user;
	bidReqDataType<regulationsObject> m_regs;		
	bidReqDataType<int>    m_at;
	bidReqDataType<int>    m_test;
	bidReqDataType<int>    m_tmax;
	bidReqDataType<int>    m_allimps;
	bidReqDataType<vector<string>> m_wseatArray;
	bidReqDataType<vector<string>> m_curArray;
	bidReqDataType<vector<string>> m_bcatArray;
	bidReqDataType<vector<string>> m_badvArray;
};

class bidObject
{
public:
	bidObject(){}
	void parse(const rapidjson::Value &value);
	bool toProtobuf(MobileAdResponse_mobileBid *bidContent);

	~bidObject(){}
private:
	string m_id;
	string m_impid;
	bidReqDataType<float> m_price;
	string m_adid;
	string m_nurl;
	string m_adm;
	vector<string> m_adomainArray;
	string m_lurl;
	string m_cid;
	string m_crid;
	vector<int> m_attrArray;
	string m_dealid;
	bidReqDataType<int> m_h;
	bidReqDataType<int> m_w;
};

class seatBidObject
{
public:
	seatBidObject(){}
	void parse(const rapidjson::Value &value);
	bool toProtobuf(MobileAdResponse_mobileBid *bidContent);

	~seatBidObject(){}
private:
	vector<bidObject*> m_bidArray;
	bidReqDataType<string> m_seat;
	bidReqDataType<int> m_group;
};

class bidResponseObject
{
public:
	bidResponseObject(){}
	void parse(rapidjson::Document& document);
	bool toProtobuf(const string& busicode, const string& datacode, CommonMessage& commMsg);
	~bidResponseObject()
	{
		for(auto it = m_seatbidArray.begin(); it != m_seatbidArray.end();)
		{
			delete *it;
			it = m_seatbidArray.erase(it);
		}
	}
private:
	string m_id;
	vector<seatBidObject*> m_seatbidArray;
	string m_bidid;
	string m_cur;
	string m_customdata;
	bidReqDataType<int> m_nbr;
};

class rtbKitObject
{
public:
	bool parseBidRequestProtobuf(const string& request)
	{
		return true;
	}
};


#endif

