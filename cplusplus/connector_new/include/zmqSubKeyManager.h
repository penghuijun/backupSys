#ifndef __ZMQSUBKEYMANAGER_H__
#define __ZMQSUBKEYMANAGER_H__
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <event.h>
#include <unistd.h>
#include <algorithm>
#include "zmq.h"
#include "adConfig.h"
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include "lock.h"

class zmqSubKeyObject
{
public:
	zmqSubKeyObject(){}
	zmqSubKeyObject(const string& clientIP, unsigned short clientManagerPort,
                            const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort);
	void set(const string& clientIP, unsigned short clientManagerPort,
              const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort);
	string&			get_clientIP(){return m_clientIP;}
	unsigned short 	get_clientManagerPort(){return m_clientManagerPort;}
	string& 		get_bcIP(){return m_bcIP;}
	unsigned short 	get_bcManagerPort(){return m_bcManagerPort;}
	unsigned short 	get_bcDataPort(){return m_bcDataPort;}
	string&			get_subKey(){return m_subKey;}
	~zmqSubKeyObject(){}

private:
	string 							m_clientIP;
	unsigned short 					m_clientManagerPort;
	string 							m_bcIP;
	unsigned short 					m_bcManagerPort;
	unsigned short 					m_bcDataPort;
	string							m_subKey;
};
class zmqSubKeyManager
{
public:
	zmqSubKeyManager();
	string& add_subKey(const string& clientIP, unsigned short clientManagerPort,
							const string& bc_ip, unsigned short bcManagerPort, unsigned short bcDataPOort);
	void erase_subKey(string& key);
	~zmqSubKeyManager(){}
private:
	vector<zmqSubKeyObject*> 		m_subKey_list;
	mutex_lock 						m_subKeyList_lock;
};

#endif
