#ifndef __DSPCONFIG_H__
#define __DSPCONFIG_H__
#include <iostream>
#include <string.h>
#include <fstream>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "spdlog/spdlog.h"
#include "json/json.h"
#include <event.h>
#include <list>


using namespace std;

struct listenObject
{
	int sock;
	struct event *_event;
};
class dspObject
{
public:
	dspObject(){}
	list<listenObject *>& getListenObjectList(){return m_listenObjectList;}
	struct listenObject* findListenObject(int sock);
	void eraseListenObject(int sock);	
	~dspObject(){}
private:
	list<listenObject *> m_listenObjectList;
};
class chinaTelecomObject : public dspObject
{
public:
	chinaTelecomObject()
	{
		readChinaTelecomConfig();		
	}
	//bool string_find(string& str1, const char* str2);
	//void strGet(string& Dest,const char* Src);
	void readChinaTelecomConfig();
	string& getName(){return name;}
	string& getTokenType(){return tokenType;}
	string& getTokenIP(){return tokenIP;}
	string& getTokenPort(){return tokenPort;}
	string& getTokenUrl(){return tokenUrl;}
	string& getUser(){return user;}
	string& getPasswd(){return passwd;}
	string& getAdReqType(){return adReqType;}
	string& getAdReqIP(){return adReqIP;}
	string& getAdReqPort(){return adReqPort;}
	string& getAdReqUrl(){return adReqUrl;}
	string& getHttpVersion(){return httpVersion;}
	string& getConnection(){return Connection;}	
	string& getUserAgent(){return UserAgent;}	
	string& getCeritifyCode(){return CeritifyCode;}	
	string& getExtNetId(){return extNetId;}
	string& getIntNetId(){return intNetId;}
	//list<listenObject *>& getListenObjectList(){return m_listenObjectList;}
	//struct listenObject* findListenObject(int sock);
	//void eraseListenObject(int sock);	
	bool parseCertifyStr(char * Src);
	bool getCeritifyCodeFromChinaTelecomDSP();	
	
	bool isCeritifyCodeEmpty();
	bool sendAdRequestToChinaTelecomDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg);
	~chinaTelecomObject(){}
private:
	string name;	
	string tokenType;
	string tokenIP;
	string tokenPort;
	string tokenUrl;
	string user;
	string passwd;

	string adReqType;
	string adReqIP;
	string adReqPort;
	string adReqUrl;
	string CeritifyCode;

	string httpVersion;

	//HTTP header 
	string Connection;	
	string UserAgent;
	string ContentType;	
	string charset;
	string Host;
	string Cookie;

	//FILTER
	string extNetId;
	string intNetId;
		
	//list<listenObject *> m_listenObjectList;
};

class guangYinObject : public dspObject
{
public:
	guangYinObject()
	{
		readGuangYinConfig();
	}
	void readGuangYinConfig();
	bool sendAdRequestToGuangYinDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg);
	bool getTestValue(){return test;}
	string& getPublisherID(){return publisherId;}
	~guangYinObject(){}
private:
	string name;
	string adReqType;
	string adReqIP;
	string adReqPort;
	string adReqUrl;

	string httpVersion;

	//HTTP header
	string Connection;
	string UserAgent;
	string ContentType;
	string Host;

	//FILTER
	string publisherId;
	string extNetId;
	string intNetId;
	bool test;

	//list<listenObject *> m_listenObjectList;	
};
#endif
