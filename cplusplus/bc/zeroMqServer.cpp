
#include <string>
#include <zmq.h>
#include <iostream>
#include <vector>
#include <time.h>

#include "zeroMqServer.h"

#include "AdBidderResponseTemplate.pb.h"
#include "CommonMessage.pb.h"


using namespace com::rj::protos::msg;
using namespace std;




zeroMqServer::zeroMqServer(const char * ip,unsigned short port)
{
	set(ip, port);
}


zeroMqServer::zeroMqServer(string  ip,unsigned short port)
{
	set(ip, port);
}


void zeroMqServer::set(const char * ip,unsigned short port)
{
	m_ip = ip;
	m_port = port;

}



void zeroMqServer::set(string  ip,unsigned short port)
{
	m_ip = ip;
	m_port = port;

}

