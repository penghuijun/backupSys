#ifndef __PUBKEYMANAGER_H__
#define __PUBKEYMANAGER_H__

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

//using namespace boost::interprocess;

typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> CharAllocator;
typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator> MyShmString;
typedef boost::interprocess::allocator<MyShmString, boost::interprocess::managed_shared_memory::segment_manager> StringAllocator;
typedef boost::interprocess::vector<MyShmString, StringAllocator> MyShmStringVector;

class hashFnv1a64
{
public:
	static uint32_t hash_fnv1a_64(const char *key, size_t key_length);
	static uint32_t get_rand_index(const char* uuid);

private:
	static const uint64_t FNV_64_INIT = UINT64_C(0xcbf29ce484222325);
	static const uint64_t FNV_64_PRIME = UINT64_C(0x100000001b3);
};


enum bidderSymDevType
{
	sys_throttle,
	sys_bidder,
	sys_connector,
	sys_bc
};

class zmqSubscribeKey
{
public:
	zmqSubscribeKey(){}
	zmqSubscribeKey(const string& key);

	zmqSubscribeKey(const string& bidderIP, unsigned short bidderPort,const string& bcIP,
		unsigned short bcManagerPort, unsigned short bcDataPort);
	void set(const string& bidderIP, unsigned short bidderPort,const string& bcIP
		, unsigned short bcManagerPort, unsigned short bcDataPort);
	
	const string&        get_bidder_ip() const{return m_bidder_ip;}
    unsigned short get_bidder_port() const{return m_bidder_port;}
	const string&        get_bc_ip() const{return m_bc_ip;}
	unsigned short get_bcManangerPort() const{return m_bcManagerport;}
	unsigned short get_bcDataPort() const{return m_bcDataPort;}
	const string&        get_subKey() const{return m_subKey;}

	bool parse();
	~zmqSubscribeKey()
	{
		g_manager_logger->info("========erase publish key========:{0}", m_subKey);
	}
private:
	string         m_bidder_ip;
	unsigned short m_bidder_port;
	string    	   m_bc_ip;
	unsigned short m_bcManagerport;
	unsigned short m_bcDataPort;
	string 		   m_subKey;	
};


class bcSubKeyManager
{
public:
	bcSubKeyManager(){}
	
	bcSubKeyManager(bool fromBidder, const string& bidderIP, unsigned short bidderPort,const string& bcIP,
		unsigned short bcManagerPort, unsigned short bcDataPort);

	void set(const string& bcIP, unsigned short bcManagerPort, unsigned short bcDataPort);
	bool add(bool fromBidder, const string& bidderIP, unsigned short bidderPort,const string& bcIP,
		unsigned short bcManagerPort, unsigned short bcDataPort);
	bool addKey(vector<zmqSubscribeKey*>& keyList, const string& bidderIP, unsigned short bidderPort,const string& bcIP,
		unsigned short bcManagerPort, unsigned short bcDataPort );

	void get_keypipe(const char* uuid, string& bidderKey, string& connectorKey);

	bool erase_publishKey(bidderSymDevType type, string& ip, unsigned short managerport);

 	bool erase_publishKey(vector<zmqSubscribeKey*>& keyList, string& ip, unsigned short managerPort)  ;	
	const string&        get_bc_ip() const;
	unsigned short get_bcManangerPort() const;
	unsigned short get_bcDataPort() const;

	bool bidder_publishExist(const string& ip, unsigned short port);
	bool connector_publishExist(const string& ip, unsigned short port);
	void publishData(void *pubVastHandler, char *msgData, int msgLen);
	void syncShareMemory(MyShmStringVector *vec);
	~bcSubKeyManager();

private:
	string    	   m_bc_ip;
	unsigned short m_bcManagerport;
	unsigned short m_bcDataPort;
	vector<zmqSubscribeKey*> m_connectorKeyList;
	vector<zmqSubscribeKey*> m_bidderKeyList;
};


class throttlePubKeyManager
{
public:
	throttlePubKeyManager();	
	bool add_publishKey(bool frombidder, const string& bidder_ip, unsigned short bidder_port,const string& bc_ip,
		unsigned short bcManagerPort, unsigned short bcDataPOort);
	bool get_publish_key(const char* uuid, string& bidderKey, string& connectorKey);
	void erase_publishKey(bidderSymDevType type, string& ip, unsigned short managerport);
	void erase_publishKey(bidderSymDevType type, vector<ipAddress>& addrList);
	bool publishExist(bidderSymDevType sysType, const string& ip, unsigned short port);	
	bool bc_publishExist(const string& ip, unsigned short port);
	bool bidder_publishExist(const string& ip, unsigned short port);
	bool connector_publishExist(const string& ip, unsigned short port);
	void publishData(void *pubVastHandler, char *msgData, int msgLen);
	void workerPublishData(void *pubVastHandler, char *msgData, int msgLen);
	void syncShmSubKeyVector();
	~throttlePubKeyManager();
private:

	read_write_lock          m_publishKey_lock;
	vector<bcSubKeyManager*> m_bcSubkeyManagerList;
	MyShmStringVector		*shmSubKeyVector;
};

#endif
