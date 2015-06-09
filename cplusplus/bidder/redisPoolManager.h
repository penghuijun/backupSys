#ifndef __REDISPOOL_MANAGER_H__
#define __REDISPOOL_MANAGER_H__

#include "redisPool.h"
class redisPoolManager
{
public:
	redisPoolManager();
	redisPoolManager(const string &ip, unsigned short port, unsigned short connNum);
	void connectorPool_set(const string &ip, unsigned short port, unsigned short connNum);
	void connectorPool_init(void);
	void connectorPool_init(const string &ip, unsigned short port, unsigned short connNum);
	void redisPool_update(const string &ip, unsigned short port, unsigned short connNum);
	void connectorPool_earse();
	bool redis_get_camp_pipe(operationTarget &target_obj, bufferManager &buf_manager, unsigned short  conver_num);
	bool redis_lpush(const char *key, const char* value, int len);
	bool redis_get_creative_pipe(vector<string>& creatvieIDList, vector<string> &creatvieList);
	int redisSize();
	void display();
	~redisPoolManager();
private:
	redisClient *get_idle_redis();
	
	mutex_lock m_redis_pool_lock;
	string m_ip;
	unsigned short m_port;
	unsigned short m_conncoter_number=0;
	vector<redisClient*> m_redis_client;
};

#endif
