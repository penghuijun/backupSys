#ifndef __CONNECTORMANAGER_H__
#define __CONNECTORMANAGER_H__
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <event.h>
#include <unistd.h>
#include <algorithm>
#include "zmq.h"
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

#include "lock.h"
#include "spdlog/spdlog.h"
#include "adConfig.h"
#include "login.h"


class connectorManager
{
public:
	connectorManager(){}
	void init(connectorInformation& connectorInfo);
	connectorConfig get_connector_config(){return m_connectorConfig;}
	bool update(connectorInformation &new_connector_info);	
	string& get_connectorIdentify(){return m_connectorIdentify;}	
	~connectorManager(){}

private:
	connectorConfig				m_connectorConfig;
	string          			m_redisIP;
	unsigned short  			m_redisPort;
	string          			m_connectorIdentify;
	
};

#endif
