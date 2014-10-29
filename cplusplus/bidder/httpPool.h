#ifndef __CONNECTOR_POOL_H__
#define __CONNECTOR_POOL_H__
#include <iostream>
#include <string>
#include <vector>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <event.h>
#include<algorithm>

//#include "http.h"

typedef void (*libevent_fun)(int, short, void *);


using namespace std;
#define BUF_SIZE 4096
struct httpTrunkNode
{
	char *buf;
	int bufLen;
	int haveRcevedLen;
	int trunkSizeLen;
};

enum httpTransType
{
	trans_invalid,
	trans_context_length,
	trans_encode_trunk,
	trans_nobody
};

enum socket_state
{
	socket_invalid,
	socket_sending,
	socket_valid,
	socket_send_failure
};


struct exBidderInfo
{
	vector<string> subKey;
	string exBidderIP;
	unsigned short exBidderPort;
	unsigned short connectNum;
};

class exchangeConnector
{
public:
	exchangeConnector(){}
	exchangeConnector(string ip, unsigned short port):m_ip(ip), m_port(port){};
	int exchange_connector_connect();
	socket_state get_excahnge_connector_state();
	int connect_async(struct event_base* base ,  libevent_fun fun, void *serv);
	char* recv_async(char *buf, int recvSize, int *httpLen);
	bool need_update_libevent();

	char *gen_http_response(char *buf, int recvSize,int *httpLen);
	bool parse_http_truck(char *buf, int bufSize);
	bool get_content_length(string &src, string& subStr, int &value);

	void connector_close()
	{
		close(m_sockfd);
		m_sockfd=-1;
	}
	void release();
	void reset_recv_state();
	void clearRecvCache();
	void add_connector_libevent(struct event_base* base ,  libevent_fun fun, void *serv);

	bool get_connector_send_state() const
	{
		return m_reqSending;
	}
	int get_sockfd() const
	{
		return m_sockfd;
	}
	
	bool get_disconnect_sym() const
	{
		return m_disconnect_sym;
	}


	void set_disconnect_sym(bool disconnect)
	{
		m_disconnect_sym = disconnect;
	}

	bool get_connector() 
	{
		if(m_reqSending == false)
		{
			m_reqSending = true;
			return true;
		}
		return false;
	}

	void connector_sendState_init()
	{
		m_reqSending = false;
	}

	int http_send(int creativeID, socket_state &sockState)
	{	static int aaa = 0;
		if(m_sockfd <= 0)
		{
			sockState = socket_invalid;
			return -1;
		}

		char httpRequest[BUF_SIZE];
		char tmpBuf[BUF_SIZE];
		memset(httpRequest, 0, BUF_SIZE);
		sprintf(tmpBuf, "GET /creative?id=%d HTTP/1.1\r\n", creativeID);
		strcat(httpRequest, tmpBuf);//index.html
		sprintf(tmpBuf, "Host: %s:%d\r\n", m_ip.c_str(), m_port);
		strcat(httpRequest, tmpBuf);//202.108.22.5	www.baidu.com
		strcat(httpRequest, "Content-Type: application/json\r\n");//202.108.22.5  www.baidu.com
		strcat(httpRequest, tmpBuf);
		strcat(httpRequest, "Connection: keep-alive\r\n\r\n");//keep-alive
		int byte = write(m_sockfd, httpRequest,strlen(httpRequest));
		cout<<"send bytes:"<<get_sockfd()<<"---"<<byte<<"---"<<aaa++<<endl;
		if(byte > 0)
		{
			sockState = socket_valid;
		}
		else
		{
			sockState = socket_send_failure;
		}
		return byte;
	}	
private:
	int hex_to_dec(char *str, int size) const;

	int  m_sockfd = -1;//socket	
	bool m_reqSending=false;//if the connector send over but not recv rsp,and not timeout, then can't send next
	struct event *m_event = NULL;//event
	
	bool m_recvHead = true;//ture ,recving httphead, false, not
	bool m_truckEncode = false;//true,transform is trucked , false not
	string m_httpHead;//http head

	int m_context_length=0;//not trucked ,need context length
	int m_have_recved_length=0;//have recved content length
	char *m_http_context;//context
	vector<httpTrunkNode*> m_httpTrunkList;//if transform is trucked, then write into the list	

	string m_ip;
	unsigned short m_port = 0;
	bool m_disconnect_sym = false;
	string m_recvCache;
	int m_headSize=0;
};



class httpPool
{
public:
	httpPool()
	{
	}
	exchangeConnector *get_connector_p(int fd)
	{
	
	}
	void set_externBidInfo(string ip, unsigned short port, vector<string>& key,unsigned short connNum=1);
	httpPool* exchange_connector_pool_init(string ip, unsigned short port, vector<string> &subkey, struct event_base* base , libevent_fun fun, void *serv, unsigned short connNum=1);
	bool release_exchange_connector(int fd);	
	httpPool* update_exchange_connector_pool(string ip, unsigned short port, vector<string> &subkey, struct event_base* base , libevent_fun fun, void *serv, unsigned short connNum=1);
	bool subsrcibeKeyExist(string &publishKey);
	exchangeConnector* get_http_connector(int fd);
	
	bool compare_addr(string &ip, unsigned short port)
	{
		return(((ip==m_externBidIP) && (port==m_externBidPort))?true:false);
	}

	bool traval_exchange_connector_pool();
	void add_exchange_connector_pool_libevent(struct event_base* base , libevent_fun fun, void *serv);
	exchangeConnector* http_send_pool(int creativeID, socket_state &sockState, int &size)
	{    
		for(auto it = m_exchangeConnectorPool.begin(); it != m_exchangeConnectorPool.end(); it++)
		{
			exchangeConnector *ex = *it;
			if(ex)
			{
				bool result = ex->get_connector();
				if(result)
				{
					int sendBytes = ex->http_send(creativeID,sockState);
					size = sendBytes;
					return ex;
				}
			}
		}
		sockState = socket_sending;
		return NULL;
	}

	
	void display();	
	~httpPool()
	{
	}
private:

	//extern ip, port, publishKey
	string m_externBidIP;
	unsigned short m_externBidPort;
	vector<string> m_publishKey;
	vector<exchangeConnector*> m_exchangeConnectorPool; 
	unsigned short m_exChangeConnectorNum = 1;
};

//connector pool , may include more than one exchangeConnectorPool
class httpPoolManager
{
public:
	httpPoolManager()
	{
	}
	void init(const vector<exBidderInfo*>& vec, struct event_base* base , libevent_fun fun, void *serv);
	exchangeConnector *get_exchange_connector(int fd);//get exchange connector from subsribe
	httpPool *get_exchange_connector_pool(string &ip, unsigned short port);
	void erase_connector(int fd);
	void connector_pool_libevent_add(struct event_base* base , libevent_fun fun, void *serv);
	void update_connector_pool(string ip, unsigned short port, vector<string> &subkey, struct event_base* base , libevent_fun fun, void *serv,unsigned short connNum=1);
	exchangeConnector* http_send(string &publishKey, int creativeID, socket_state &sockState)
	{
		for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
		{
			httpPool *ecpool = *it;
			if(ecpool)
			{
				if(ecpool->subsrcibeKeyExist(publishKey))
				{
					int bytes = 1;
					exchangeConnector* result = ecpool->http_send_pool(creativeID,sockState, bytes);

					if(result&&(bytes <= 0))
					{
						result->connector_close();
						return NULL;
					}
					return result;
				}
			}
		}
		sockState = socket_invalid;
		return NULL;
	}

	exchangeConnector *get_connector_m(int fd)
	{
		for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
		{
			httpPool *ecpool = *it;
			if(ecpool)
			{
				exchangeConnector* con = ecpool->get_connector_p(fd);
				if(con)
				{
					return con;
				}
			}
		}		
		return NULL;
	}
	void display();
	~httpPoolManager()
	{

	}
	
private:
	vector<httpPool *> m_connectorPool;
};

#endif
