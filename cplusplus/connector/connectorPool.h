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
#include "bidderConfig.h"
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
	socket_valid
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

	
	void release();
	void reset_recv_state();
	void clearRecvCache();
	void add_connector_libevent(struct event_base* base ,  libevent_fun fun, void *serv);
	int send(char *str, int strLen);

	
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

	const char* get_ip() const{return m_ip.c_str();}
	unsigned short get_port() const{return m_port;}
	void set_disconnect_sym(bool disconnect)
	{
		m_disconnect_sym = disconnect;
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



class exchangeConnectorPool
{
public:
	exchangeConnectorPool(){}
	void set_externBidInfo(string ip, unsigned short port, vector<string>& key,unsigned short connNum=1);
	exchangeConnectorPool* exchange_connector_pool_init(string ip, unsigned short port, vector<string> &subkey, struct event_base* base , libevent_fun fun, void *serv, unsigned short connNum=1);
	void release_exchange_connector(int fd);	
	exchangeConnectorPool* update_exchange_connector_pool(string ip, unsigned short port, vector<string> &subkey, struct event_base* base , libevent_fun fun, void *serv, unsigned short connNum=1);
	exchangeConnector* get_exchange_connector_state(socket_state &sockState);
	exchangeConnector* get_exchange_connector_state(string &publishKey, socket_state &sockState);
	exchangeConnector* get_ECP_exchange_connector(int fd);
	
	bool compare_addr(string &ip, unsigned short port)
	{
		return(((ip==m_externBidIP) && (port==m_externBidPort))?true:false);
	}

	bool traval_exchange_connector_pool();
	void add_exchange_connector_pool_libevent(struct event_base* base , libevent_fun fun, void *serv);

	void display();	
private:
	//extern ip, port, publishKey
	string m_externBidIP;
	unsigned short m_externBidPort;
	vector<string> m_publishKey;
	vector<exchangeConnector*> m_exchangeConnectorPool; 
	unsigned short m_exChangeConnectorNum = 1;
};


//connector pool , may include more than one exchangeConnectorPool
class connectorPool
{
public:
	connectorPool(){}
	
	exchangeConnector *get_CP_exchange_connector(int fd);//get exchange connector from subsribe
	exchangeConnector* get_CP_exchange_connector(string &publishKey, socket_state &sockState);//get exchange connector form subsribe	
	exchangeConnectorPool *get_exchange_connector_pool(string &ip, unsigned short port);
	void erase_connector(int fd);
	void connector_pool_init(const vector<exBidderInfo*>& vec, struct event_base* base , libevent_fun fun, void *serv);
	bool  cp_send(exchangeConnector *conntor, const char * str, string &publishKey, socket_state &sockState);
	bool traval_connector_pool();
	
	void connector_pool_libevent_add(struct event_base* base , libevent_fun fun, void *serv);
	void update_connector_pool(string ip, unsigned short port, vector<string> &subkey, struct event_base* base , libevent_fun fun, void *serv,unsigned short connNum=1);

	void display();

private:
	vector<exchangeConnectorPool *> m_connectorPool;
};

#endif
