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
#include <event.h>
#include <list>
#include <fcntl.h>
#include <sys/epoll.h>

#include "spdlog/spdlog.h"
#include "json/json.h"
#include "lock.h"



using namespace std;

enum dspType
{
	TELE,		//China telecom
	GYIN,		//GuangYin
	SMAATO		//Smaato
};

struct listenObject
{
	int sock;
	struct event *_event;
};
class dspObject
{
public:
	dspObject():curConnectNum(0)
	{
		m_listenObjectListLock.init();
		m_listenObjectWaitListLock.init();
	}
	void readDSPconfig(dspType type);
	void gen_HttpHeader(char *headerBuf, int Con_len);
	string& getAdReqType(){return adReqType;}
	string& getAdReqUrl(){return adReqUrl;}
	string& getAdReqDomain(){return adReqDomain;}
	string& getHttpVersion(){return httpVersion;}
	string& getConnection(){return Connection;}
	string& getUserAgent(){return UserAgent;}
	string& getContentType(){return ContentType;}
	string& getcharset(){return charset;}
	string& getExtNetId(){return extNetId;}
	string& getIntNetId(){return intNetId;}
	void creatConnectDSP(struct event_base * base, event_callback_fn fn, void *arg);	
	bool addConnectToDSP(struct event_base * base, event_callback_fn fn, void *arg);
	struct listenObject* findListenObject(int sock);
	struct listenObject* findWaitListenObject(int sock);
	void eraseListenObject(int sock);	
	void eraseWaitListenObject(int sock);
	bool moveListenObjectFromWait2Idle(int sock);
	void setMaxConnectNum(int num){maxConnectNum = num;}
	int getCurConnectNum(){return curConnectNum;}
	int getMaxConnectNum(){return maxConnectNum;}
	void connectNumReduce(){curConnectNum--;}
	void connectNumIncrease(){curConnectNum++;}	
	list<listenObject *>& getListenObjectList(){return m_listenObjectList;}
	list<listenObject *>& getListenObjectWaitList(){return m_listenObjectWaitList;}
	void listenObjectList_Lock()
	{
		m_listenObjectListLock.lock();
	}
	void listenObjectList_unLock()
	{
		m_listenObjectListLock.unlock();
	}
	void listenObjectWaitList_Lock()
	{
		m_listenObjectWaitListLock.lock();
	}
	void listenObjectWaitList_unLock()
	{
		m_listenObjectWaitListLock.unlock();
	}
	
	~dspObject(){}
private:
	string name;	
	string adReqType;
	string adReqIP;
	string adReqDomain;
	string adReqPort;
	string adReqUrl;

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

	
	int curConnectNum;
	int maxConnectNum;
	mutex_lock			m_listenObjectListLock;
	list<listenObject *> 	m_listenObjectList;

	mutex_lock			m_listenObjectWaitListLock;
	list<listenObject *>	 	m_listenObjectWaitList;
};
class chinaTelecomObject : public dspObject
{
public:
	chinaTelecomObject()
	{
		readDSPconfig(TELE);
		readChinaTelecomConfig();		
	}
	
	void readChinaTelecomConfig();
	string& getTokenType(){return tokenType;}
	string& getTokenIP(){return tokenIP;}
	string& getTokenPort(){return tokenPort;}
	string& getTokenUrl(){return tokenUrl;}
	string& getUser(){return user;}
	string& getPasswd(){return passwd;}	
		
	string& getCeritifyCode(){return CeritifyCode;}	
	
	bool parseCertifyStr(char * Src);
	bool getCeritifyCodeFromChinaTelecomDSP();	
	
	bool isCeritifyCodeEmpty();
	bool sendAdRequestToChinaTelecomDSP(struct event_base * base, const char *data, int dataLen, bool enLogRsq, event_callback_fn fn, void *arg);
	~chinaTelecomObject(){}
private:
	
	string tokenType;
	string tokenIP;
	string tokenPort;
	string tokenUrl;
	string user;
	string passwd;
	
	string CeritifyCode;		
};

class guangYinObject : public dspObject
{
public:
	guangYinObject():curFlowCount(0)
	{
		readDSPconfig(GYIN);
		readGuangYinConfig();
	}
	void readGuangYinConfig();		
	bool sendAdRequestToGuangYinDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg);
	bool getTestValue(){return test;}
	string& getPublisherID(){return publisherId;}	
	int getMaxFlowLimit(){return maxFlowLimit;}
	int getCurFlowCount(){return curFlowCount;};
	void curFlowCountIncrease(){curFlowCount++;}
	void curFlowCountClean(){curFlowCount = 0;}
	~guangYinObject(){}
private:	
	//FILTER
	string publisherId;	
	bool test;
	
	int maxFlowLimit;
	int curFlowCount;
	
};

class smaatoObject: public dspObject
{
public:
	smaatoObject()
	{
		readDSPconfig(SMAATO);
	}
	bool sendAdRequestToSmaatoDSP(struct event_base * base, const char *data, int dataLen, event_callback_fn fn, void *arg);
	~smaatoObject(){}
private:
	
};

#endif
