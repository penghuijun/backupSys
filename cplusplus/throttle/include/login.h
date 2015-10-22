#ifndef __LOGIN_H__
#define __LOGIN_H__
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <event.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

#include "zeromqConnect.h"
#include "managerProto.pb.h"
using namespace com::rj::protos::manager;
using namespace std;
class ipAddress
{
public:
	ipAddress(){}
	ipAddress(const string &ip, unsigned short port)
	{
		set(ip, port);
	}
	bool ip_equal(ipAddress &ip_addr)
	{
		return ((ip_addr.get_ip()==m_ip)&&(ip_addr.get_port()==m_port));
	}
	void set(const string &ip, unsigned short port)
	{
		m_ip = ip;
		m_port = port;
	}
	void set(ipAddress& addr)
	{
		m_ip = addr.get_ip();
		m_port = addr.get_port();
	}
	string &get_ip(){return m_ip;}
	unsigned short get_port(){return m_port;}
	static void get_local_ipaddr(vector<string> &ipAddrList)
	{
		struct ifaddrs * ifAddrStruct=NULL;
	    void * tmpAddrPtr=NULL;

	    getifaddrs(&ifAddrStruct);
	    while (ifAddrStruct!=NULL) {
	        if (ifAddrStruct->ifa_addr->sa_family==AF_INET) { // check it is IP4
	            // is a valid IP4 Address
	            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
	            char addressBuffer[INET_ADDRSTRLEN];
	            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
				ipAddrList.push_back(string(addressBuffer));
	        } else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6) { // check it is IP6
	            // is a valid IP6 Address
	            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
	            char addressBuffer[INET6_ADDRSTRLEN];
	            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
				ipAddrList.push_back(string(addressBuffer));
	        } 
	        ifAddrStruct=ifAddrStruct->ifa_next;
	    }
	}

	
	~ipAddress(){}
private:
	string m_ip;
	unsigned short m_port;
};

class managerProPackage
{
public:
	managerProPackage(){}
	static int send(void *handler,const managerProtocol_messageTrans &from
		,const managerProtocol_messageType &type ,const string& ip, unsigned short managerPort, unsigned short dataPort=0)
	{
        managerProtocol manager_pro;
        manager_pro.set_messagefrom(from);
        manager_pro.set_messagetype(type);
        managerProtocol_messageValue* msg_value = manager_pro.mutable_messagevalue();
        msg_value->set_ip(ip);
        msg_value->add_port(managerPort);
		if(dataPort)
		{
			msg_value->add_port(dataPort);
		}
        int dataLen = manager_pro.ByteSize();
        char* comMessBuf = new char[dataLen];
        manager_pro.SerializeToArray(comMessBuf, dataLen);
		if(handler)
		{
			int sendSize = zmq_send(handler, comMessBuf, dataLen, ZMQ_NOBLOCK);
		}
		else
		{
			g_manager_logger->info("send handler is null");
		}
        delete[] comMessBuf;
		return dataLen;
	}

	static int send(void *handler, const managerProtocol_messageTrans &from,const managerProtocol_messageType &type 
				,const string& registerKey, const string& ip, unsigned short managerPort, unsigned short dataPort=0)
	{
        managerProtocol manager_pro;
        manager_pro.set_messagefrom(from);
        manager_pro.set_messagetype(type);
        managerProtocol_messageValue* msg_value = manager_pro.mutable_messagevalue();
		msg_value->set_key(registerKey);
        msg_value->set_ip(ip);
        msg_value->add_port(managerPort);
		if(dataPort)
		{
			msg_value->add_port(dataPort);
		}
        int dataLen = manager_pro.ByteSize();
        char* comMessBuf = new char[dataLen];
        manager_pro.SerializeToArray(comMessBuf, dataLen);
		if(handler)
		{
			int sendSize = zmq_send(handler, comMessBuf, dataLen, ZMQ_NOBLOCK);	
		}
		else
		{
			g_manager_logger->info("handler is null");
		}
        delete[] comMessBuf;
		return dataLen;
		return 0;
	}


	static int send_response(void *handler, string& identify, const managerProtocol_messageTrans &from
		,const managerProtocol_messageType &type ,const string& ip, unsigned short managerPort, unsigned short dataPort=0)
	{
        managerProtocol manager_pro;
        manager_pro.set_messagefrom(from);
        manager_pro.set_messagetype(type);
        managerProtocol_messageValue* msg_value = manager_pro.mutable_messagevalue();
        msg_value->set_ip(ip);
        msg_value->add_port(managerPort);
		if(dataPort)
		{
			msg_value->add_port(dataPort);
		}
        int dataLen = manager_pro.ByteSize();
        char* comMessBuf = new char[dataLen];
        manager_pro.SerializeToArray(comMessBuf, dataLen);
		if(handler)
		{
			zmq_send(handler, identify.c_str(), identify.size(), ZMQ_SNDMORE);
			int sendSize = zmq_send(handler, comMessBuf, dataLen, ZMQ_NOBLOCK);	
		}
		else
		{
			g_manager_logger->info("handler is null");
		}
        delete[] comMessBuf;
		return dataLen;
	}

	static int send_response(void *handler, string& identify, const managerProtocol_messageTrans &from
		,const managerProtocol_messageType &type , const string& registerKey, const string& ip, unsigned short managerPort
		, unsigned short dataPort=0)
	{
        managerProtocol manager_pro;
        manager_pro.set_messagefrom(from);
        manager_pro.set_messagetype(type);
        managerProtocol_messageValue* msg_value = manager_pro.mutable_messagevalue();
		msg_value->set_key(registerKey);
        msg_value->set_ip(ip);
        msg_value->add_port(managerPort);
		if(dataPort)
		{
			msg_value->add_port(dataPort);
		}
        int dataLen = manager_pro.ByteSize();
        char* comMessBuf = new char[dataLen];
        manager_pro.SerializeToArray(comMessBuf, dataLen);
		if(handler)
		{
			zmq_send(handler, identify.c_str(), identify.size(), ZMQ_SNDMORE);
			int sendSize = zmq_send(handler, comMessBuf, dataLen, ZMQ_NOBLOCK);	
		}
		else
		{
			g_manager_logger->info("handler is null");
		}
		delete[] comMessBuf;
		return dataLen;
	}


	~managerProPackage(){}
private:
};

#endif
