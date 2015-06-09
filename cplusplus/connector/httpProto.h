#ifndef __HTTP_PROTO_H__
#define __HTTP_PROTO_H__
#include "transProto.h"
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


typedef void (*libevent_fun)(int, short, void *);

using namespace std;

class httpTrunkNode
{
public:
	httpTrunkNode(){}
	httpTrunkNode(char *buf, int bufLen, int truckSize, int recvedLen)
	{
		set_buf(buf, bufLen);
		set_recvedLen(recvedLen);
		set_truckSizeLen(truckSize);
	}
	const char* get_buf() const {return m_buf;}
	int         get_bufLen() const {return m_bufLen;}
	int         get_recvedLen() const{return m_recvedLen;}
	int         get_truckSizeLen() const{return m_trunkSizeLen;}
	void        set_buf(char *buf, int bufLen)
	{
		m_buf = buf;
		m_bufLen = bufLen;
	}
	void set_recvedLen(int len){ m_recvedLen = len;}
	void set_truckSizeLen(int len){m_trunkSizeLen = len;}
	char* add_buf(char *buf, int len, int &notTreatSize)
	{
		if(m_buf == NULL) return NULL;
		int needRecvByte = m_bufLen - m_recvedLen;//需要填满最后一个truck所需字节
		if(needRecvByte < len)//if recv byte more than one truck ,then parse next truck
		{
			memcpy(m_buf + m_recvedLen, buf, needRecvByte);
			m_recvedLen = m_bufLen;
			notTreatSize = len - needRecvByte;
			return buf+needRecvByte;
		}
		else
		{
			memcpy(m_buf + m_recvedLen, buf, len);
			m_recvedLen += len;
			return NULL;
		}
	}
	~httpTrunkNode(){delete[] m_buf;}
private:
	char *m_buf = NULL;
	int m_bufLen;
	int m_recvedLen;
	int m_trunkSizeLen;
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


class httpProtocol : public transProtocol
{
public:
	httpProtocol(){}
	httpProtocol(const string& ip, unsigned short port):transProtocol(ip, port)
	{
			
	}
	bool pro_connect();
    bool pro_connect_asyn(struct event_base * base, event_callback_fn fn, void * arg);	
	bool pro_close();
	int pro_send(const char * str);
	bool  parse_http_response_head(string &resp_head);
	transErrorStatus recv_async(stringBuffer &recvBuf);
	void init()
	{
		   m_recvHead = true;
		   m_recvCache.clear();
		   m_context_length = 0;
		   
		   auto it = m_httpTrunkList.begin();
		   for(it = m_httpTrunkList.begin(); it != m_httpTrunkList.end();)
		   {
			   httpTrunkNode *trunk = *it;
				delete trunk;
			   it = m_httpTrunkList.erase(it);
		   }
	
	}
	bool get_content_length(string &src,string& subStr, int &value);
	char *gen_http_response(char *buf, int recvSize,int &httpLen);
	bool parse_http_truck(char *buf, int bufSize);
	int  hex_to_dec(char *str, int size);
	bool update_connect(int fd)
	{
		if(m_fd == fd)
		{
			m_sending = false;
			
			m_recvHead = true;
			m_truckEncode = false;
			m_recvCache.clear();
			m_headSize = 0;
			m_context_length = 0;

			for(auto it = m_httpTrunkList.begin(); it != m_httpTrunkList.end(); )
			{
				delete *it;
				it = m_httpTrunkList.erase(it);
			}
			return true;
		}
		return false;
	}
	~httpProtocol(){}
private:
	bool m_recvHead = true;//ture ,recving httphead, false, not
	bool m_truckEncode = false;//true,transform is trucked , false not

	string m_recvCache;
	int m_headSize=0;
	int m_context_length=0;//not trucked ,need context length
	vector<httpTrunkNode*> m_httpTrunkList;//if transform is trucked, then write into the list	
};

#endif

