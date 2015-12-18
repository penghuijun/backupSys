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
#include "http_parse.h"



using namespace std;

class dspObject;

#define BUF_SIZE	1024*16		//16K
enum dspType
{
	TELE,		//China telecom
	GYIN,		//GuangYin
	SMAATO,	//Smaato
	INMOBI,  	//InMobi
	BAIDU		//Baidu
};

struct listenObject
{
	int sock;
	struct event *_event;
};

struct connectDsp_t
{
	struct event_base * base;
	event_callback_fn fn;
	void *arg;
	dspObject *dspObj;
	//dspType dsptype;
};
class dspObject
{
public:
	dspObject():curConnectNum(0),curFlowCount(0)
	{
		m_listenObjectListLock.init();
		m_listenObjectList = new list<listenObject *>();		
	}
	void readDSPconfig(dspType type);
	bool addr_init();
	void gen_HttpHeader(char *headerBuf, int Con_len, string& ua);
	string& getAdReqIP(){return adReqIP;}
	string& getAdReqPort(){return adReqPort;}
	string& getAdReqType(){return adReqType;}
	string& getAdReqUrl(){return adReqUrl;}
	string& getAdReqDomain(){return adReqDomain;}
	string& getHttpVersion(){return httpVersion;}
	string& getConnection(){return Connection;}
	string& getContentType(){return ContentType;}
	string& getcharset(){return charset;}
	string& getExtNetId(){return extNetId;}
	string& getIntNetId(){return intNetId;}
	static void addConnectToDSP(void * arg);
	struct listenObject* findListenObject(int sock);
	void eraseListenObject(int sock);	
	
	void setMaxConnectNum(int num){maxConnectNum = num;}
	int getCurConnectNum(){return curConnectNum;}
	int getMaxConnectNum(){return maxConnectNum;}
	int getPreConnectNum(){return preConnectNum;}
	void connectNumReduce(){curConnectNum--;}
	void connectNumIncrease(){curConnectNum++;}	
	
	int getMaxFlowLimit(){return maxFlowLimit;}
	int getCurFlowCount(){return curFlowCount;}
	void curFlowCountIncrease(){curFlowCount++;}
	void curFlowCountClean(){curFlowCount = 0;}
	
	list<listenObject *> *getListenObjectList(){return m_listenObjectList;}
	void listenObjectList_Lock()
	{
		m_listenObjectListLock.lock();
	}
	void listenObjectList_unLock()
	{
		m_listenObjectListLock.unlock();
	}
	struct sockaddr_in * getSockAddr_in(){return sin;}
	~dspObject()
	{
		delete m_listenObjectList;
	}
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
	string ContentType;	
	string charset;
	string Host;
	string Cookie;
	string Forwarded;
	string Accept;

	//FILTER
	string extNetId;
	string intNetId;

	int maxFlowLimit;
	int curFlowCount;
	
	int curConnectNum;
	int maxConnectNum;
	int preConnectNum;
	
	mutex_lock			 m_listenObjectListLock;
	list<listenObject *> 	*m_listenObjectList;

	struct sockaddr_in *	sin; 
};
class chinaTelecomObject : public dspObject
{
public:
	chinaTelecomObject()
	{
		readDSPconfig(TELE);
		readChinaTelecomConfig();		
		if(!addr_init())
			exit(1);
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
	bool sendAdRequestToChinaTelecomDSP(const char *data, int dataLen, bool enLogRsq, string& ua);
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
	guangYinObject()
	{
		readDSPconfig(GYIN);
		readGuangYinConfig();
		if(!addr_init())
			exit(1);
	}
	void readGuangYinConfig();		
	bool sendAdRequestToGuangYinDSP(const char *data, int dataLen, string& ua);
	bool getTestValue(){return test;}
	string& getPublisherID(){return publisherId;}	
	
	~guangYinObject(){}
private:	
	//FILTER
	string publisherId;	
	bool test;
	
};

class smaatoObject: public dspObject
{
public:
	smaatoObject()
	{
		smaatoSocketList_Lock.init();
		smaatoSocketList = new list<int>();
		readDSPconfig(SMAATO);
		readSmaatoConfig();
		if(!addr_init())
			exit(1);
	}
	void readSmaatoConfig();
	void smaatoConnectDSP();
	static void smaatoAddConnectToDSP(void *arg);
	int sendAdRequestToSmaatoDSP(const char *data, int dataLen, string& uuid, string& ua);
	bool recvBidResponseFromSmaatoDsp(int sock, struct spliceData_t *fullData_t);
	string& getAdSpaceId(){return adSpaceId;}
	string& getPublisherId(){return publisherId;}
	int getPrice(){return price;}
	void smaatoSocketList_Locklock()
	{
		smaatoSocketList_Lock.lock();
	}
	void smaatoSocketList_Lockunlock()
	{
		smaatoSocketList_Lock.unlock();
	}
	list<int>* getSmaatoSocketList()
	{
		return smaatoSocketList;
	}
	~smaatoObject(){}
private:
	string adSpaceId;
	string publisherId;
	int price;
	mutex_lock smaatoSocketList_Lock;
	list<int>*	smaatoSocketList;
};

class inmobiObject: public dspObject
{
public:
	inmobiObject()
	{
		inmobiSocketList_Lock.init();
		inmobiSocketList = new list<int>();
		readDSPconfig(INMOBI);
		readInmobiConfig();
		if(!addr_init())
			exit(1);
	}
	void readInmobiConfig();
	static void inmobiAddConnectToDSP(void *argc);
	int sendAdRequestToInMobiDSP(const char *data, int dataLen, bool enLogRsq, string& ua);
	bool recvBidResponseFromInmobiDsp(int sock, struct spliceData_t *fullData_t);
	string& getSiteId(){return siteId;}
	string& getPlacementId(){return placementId;}
	int getPrice(){return price;}
	void inmobiSocketList_Locklock()
	{
		inmobiSocketList_Lock.lock();
	}
	void inmobiSocketList_Lockunlock()
	{
		inmobiSocketList_Lock.unlock();
	}
	list<int>* getInmobiSocketList()
	{
		return inmobiSocketList;
	}
	~inmobiObject(){}
private:
	string siteId;
	string placementId;	
	int price;
	mutex_lock inmobiSocketList_Lock;
	list<int>* inmobiSocketList;
};

class baiduObject: public dspObject
{
public:
	baiduObject()
	{
		baiduSocketList_Lock.init();
		readDSPconfig(BAIDU);
		readBaiduConfig();
		if(!addr_init())
			exit(1);
	}
	void readBaiduConfig();
	static void baiduAddConnectToDSP(void *arg);
	int sendAdRequestToBaiduDSP(const char *data, int dataLen, string& uuid, string& ua);
	bool recvBidResponseFromBaiduDsp(int sock, struct spliceData_t *fullData_t);
	void baiduSocketList_Locklock()
	{
		baiduSocketList_Lock.lock();
	}
	void baiduSocketList_Lockunlock()
	{
		baiduSocketList_Lock.unlock();
	}
	list<int>* getBaiduSocketList()
	{
		return baiduSocketList;
	}
private:
	int price;
	mutex_lock baiduSocketList_Lock;
	list<int>*	baiduSocketList;
};

#endif
