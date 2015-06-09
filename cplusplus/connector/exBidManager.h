#ifndef __EXBID_MANAGER_H__
#define __EXBID_MANAGER_H__
#include <iostream>
#include <string>
#include <vector>
#include "httpProto.h"
#include "lock.h"
using namespace std;


class adRequestInfo
{
public:
	adRequestInfo(){}
	adRequestInfo(const string& busiCode, const string& codeType, const string& pubkey, const string& vastStr, int fd)
	{
		m_business_code = busiCode;
		m_data_coding_type = codeType;
		m_publishKey = pubkey;
		m_vastStr = vastStr;
		m_fd = fd;
	}

	int get_fd(){return m_fd;}
	void set_fd(int fd){m_fd = fd;}
	const string& get_busicode(){return m_business_code;}
	const string& get_codeType(){return m_data_coding_type;}
	const string& get_publishkey(){return m_publishKey;}
	const string& get_dataStr(){return m_vastStr;}
	~adRequestInfo(){}
private:
	string m_business_code;
	string m_data_coding_type;
	string m_publishKey;
	string m_vastStr;
	int    m_fd = -1;
};


class adRequestManager
{
public:
	adRequestManager()
	{
		m_requestList_lock.init();
	}
	void addAdRequest(const string& busiCode, const string& codeType, const string& pubkey, const string& vastStr, int fd)
	{
		m_requestList_lock.write_lock();
		if(m_adRequestList.size() < m_adreqestList_max)
		{
			adRequestInfo *info = new adRequestInfo(busiCode, codeType, pubkey, vastStr, fd);
			m_adRequestList.push_back(info);
		}
		m_requestList_lock.read_write_unlock();
	}

	adRequestInfo* adRequest_pop(int fd)
	{
		adRequestInfo* reqInfo = NULL;
		m_requestList_lock.write_lock();
		for(auto it = m_adRequestList.begin(); it != m_adRequestList.end(); it++)
		{
			adRequestInfo *info = *it;
			if(info == NULL) continue;
			if(info->get_fd() == fd)
			{
				it = m_adRequestList.erase(it);
				reqInfo = info;
				break;
			}
		}
		m_requestList_lock.read_write_unlock();
		return reqInfo;
	}
	adRequestInfo* get_nextReqJson()
	{
		adRequestInfo* reqinfo = NULL;
		m_requestList_lock.write_lock();
		for(auto it = m_adRequestList.begin(); it != m_adRequestList.end(); it++)
		{
			adRequestInfo *info = *it;
			if(info == NULL) continue;
			if(info->get_fd() == -1)
			{
				reqinfo = info;
				break;
			}
		}
		m_requestList_lock.read_write_unlock();
		return reqinfo;
	}

	void erase(int fd)
	{
		m_requestList_lock.write_lock();
		for(auto it = m_adRequestList.begin(); it != m_adRequestList.end(); it++)
		{
			adRequestInfo *info = *it;
			if(info == NULL) continue;
			if(info->get_fd() == fd)
			{
				delete info;
				m_adRequestList.erase(it);
				break;
			}
		}
		m_requestList_lock.read_write_unlock();
	}
	
	~adRequestManager()
	{
		m_requestList_lock.write_lock();
		for(auto it = m_adRequestList.begin(); it != m_adRequestList.end();)
		{
			delete *it;
			it = m_adRequestList.erase(it);
		}
		m_requestList_lock.read_write_unlock();
		m_requestList_lock.destroy();
	}

private:
	read_write_lock      m_requestList_lock;
	const int m_adreqestList_max = 10000;
	list<adRequestInfo*> m_adRequestList;
};


class exBidObject
{
public:
	exBidObject(int id, const string& ip, unsigned short port, unsigned short connectCnt
		, const string& protocolName, const string& dataFormat)
	{
		m_id = id;
		m_ip = ip;
		m_port = port;
		m_connectCnt = connectCnt;
		m_protocolName = protocolName;
		m_dataFormat = dataFormat;
	}

	bool connect()
	{
		for(auto i = 0; i < m_connectCnt; i++)
		{
			if(m_protocolName == "http")
			{
				transProtocol *connector = new httpProtocol(m_ip, m_port);
				connector->pro_connect();
				m_connectorPool.push_back(connector);
			}
		}
	}

	bool close(int fd)
	{
		for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
		{
			transProtocol *proto = *it;
			if(proto)
			{
				if(proto->get_fd() == fd) 
				{
					proto->pro_close();
					return true;
				}
			}
		}			
		return false;
	}

	transErrorStatus recv_async(int fd, stringBuffer &recvBuf)
	{
		for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
		{
			transProtocol *proto = *it;
			if(proto &&(proto->get_fd() == fd))
			{
				return proto->recv_async(recvBuf);
			}
		}			
		return error_cannot_find_fd;//can not find the fd in the externbidder
	}

	bool connect_asyn(struct event_base * base, event_callback_fn fn, void * arg)
	{
		for(auto i = 0; i < m_connectCnt; i++)
		{
			cerr<<m_protocolName<<endl;
			if(m_protocolName == "http")
			{
				transProtocol *connector = new httpProtocol(m_ip, m_port);
				connector->pro_connect_asyn(base, fn, arg);
				m_connectorPool.push_back(connector);
			}
		}

	}
	void check_connect(struct event_base * base, event_callback_fn fn, void * arg)
	{
		for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
		{
			transProtocol *proto = *it;
			if(proto)
			{
				proto->check_connect(base, fn, arg);
			}
		}		
	}
	
	int send(const string& busiCode, const string& codeType, const string& pubkey, const string& vastStr)
	{
		int fd = -1;
		for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
		{
			transProtocol *proto = *it;
			if(proto)
			{
				fd = proto->pro_send(vastStr.c_str());
				if(fd > 0) break;
			}
		}
		m_adRequest_manager.addAdRequest(busiCode, codeType, pubkey, vastStr, fd);
		return fd;

	}

	void send()
	{	
		adRequestInfo* adRequest = m_adRequest_manager.get_nextReqJson();
		if(adRequest == NULL) return;
		const string& jsonReq = adRequest->get_dataStr();
		for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
		{
			transProtocol *proto = *it;
			if(proto == NULL) continue;
			int fd = proto->pro_send(jsonReq.c_str());
			if(fd > 0) 
			{
				adRequest->set_fd(fd);
				break;
			}
		}		
	}

	bool update_connect(int fd)
	{
		for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
		{
			transProtocol *proto = *it;
			if(proto == NULL) continue;
			if(proto->update_connect(fd)) return true;
		}
		return false;
	}
	adRequestInfo* adRequest_pop(int fd)
	{
		adRequestInfo* info =  m_adRequest_manager.adRequest_pop(fd);
		for(auto it = m_connectorPool.begin(); it != m_connectorPool.end(); it++)
		{
			transProtocol *pool = *it;
			if(pool) 
			{
				pool->update_connect(fd);
			}
		}
		return info;
	}

	const string& get_ip()const{return m_ip;}
	unsigned short get_port()const{return m_port;}
	~exBidObject()
	{
		cout<<"extern bidder erase"<<endl;
		for(auto it = m_connectorPool.begin(); it != m_connectorPool.end();)
		{
			delete *it;
			it = m_connectorPool.erase(it);
		}
	}
private:
	int m_id;
	string m_ip;
	unsigned short m_port;
	unsigned short m_connectCnt;
	string m_protocolName;
	string m_dataFormat;
	vector<transProtocol*> m_connectorPool;	
	adRequestManager       m_adRequest_manager;	
};

class exBidManager
{
public:
	exBidManager()
	{
		m_exBidManager_lock.init();
	}

	void init(const externbidInformation& bInfo, struct event_base * base, event_callback_fn fn, void * arg)
	{
		m_base = base;
		m_fn = fn;
		m_arg = arg;
		auto exBidList = bInfo.get_exbidConfigList();
		for(auto it = exBidList.begin(); it != exBidList.end(); it++)
		{
			externbidConfig* eConf = *it;
			if(eConf)
			{
				cout<<"init a extern bidder:"<<eConf->get_ip() <<endl;
				add(eConf->get_id(), eConf->get_ip(), eConf->get_port(), eConf->get_connectCnt(), eConf->get_protocol()
					, eConf->get_dataFormat(), base, fn, arg);
			}
		}
	}

	void libevent_timeout(int fd)
	{
		m_exBidManager_lock.write_lock();
		for(auto it = m_exBidList.begin(); it != m_exBidList.end(); it++)
		{
			exBidObject *bidder = *it;
			if(bidder == NULL) continue;
			if(bidder->update_connect(fd)) break;
		}
		m_exBidManager_lock.read_write_unlock();	
	}

	transErrorStatus recv_async(int fd, stringBuffer &recvBuf)
	{
		transErrorStatus status = error_cannot_find_fd;
		m_exBidManager_lock.read_lock();
		for(auto it = m_exBidList.begin(); it != m_exBidList.end(); it++)
		{
			exBidObject *bidder = *it;
			if(bidder == NULL) continue;
			status = bidder->recv_async(fd, recvBuf);
			if(status != error_cannot_find_fd) break;//if find the fd break loop
		}		
		m_exBidManager_lock.read_write_unlock();				
		return status;	
	}

	void close(int fd)
	{
		m_exBidManager_lock.write_lock();
		for(auto it = m_exBidList.begin(); it != m_exBidList.end(); it++)
		{
			exBidObject *bidder = *it;
			if(bidder == NULL) continue;
			if(bidder->close(fd)) break;
		}
		m_exBidManager_lock.read_write_unlock();					
	}

	
	bool add(int id, const string& ip, unsigned short port, unsigned short connectCnt
		, const string& protocolName, const string& dataFormat, struct event_base * base, event_callback_fn fn, void * arg)
	{
	    m_exBidManager_lock.write_lock();
		for(auto it = m_exBidList.begin(); it != m_exBidList.end(); it++)
		{
			exBidObject *bidder = *it;
			if(bidder == NULL) continue;
			if((ip == bidder->get_ip())&&(port == bidder->get_port()))
			{
			    m_exBidManager_lock.read_write_unlock();
				return false;
			}
		}

		exBidObject *bidder = new exBidObject(id, ip, port, connectCnt, protocolName, dataFormat);
		bidder->connect_asyn(base, fn, arg);
	    m_exBidList.push_back(bidder);
	    m_exBidManager_lock.read_write_unlock();
		return true;
	}	

	void check_connect()
	{
	    m_exBidManager_lock.write_lock();
		for(auto it = m_exBidList.begin(); it != m_exBidList.end(); it++)
		{
			exBidObject *bidder = *it;
			if(bidder == NULL) continue;
			bidder->check_connect(m_base, m_fn, m_arg);
		}
	    m_exBidManager_lock.read_write_unlock();
	}


	void send(const string& busiCode, const string& codeType, const string& pubkey, const string& vastStr)
	{
		m_exBidManager_lock.write_lock();
		for(auto it = m_exBidList.begin(); it != m_exBidList.end(); it++)
		{
			exBidObject *exbid = *it;
			if(exbid) 
			{
				exbid->send(busiCode, codeType, pubkey, vastStr);
			}
			
		}	
		m_exBidManager_lock.read_write_unlock();			
	}

	void send()
	{
		m_exBidManager_lock.write_lock();
		for(auto it = m_exBidList.begin(); it != m_exBidList.end(); it++)
		{
			exBidObject *exbid = *it;
			if(exbid) 
			{
				exbid->send();
			}
		}	
		m_exBidManager_lock.read_write_unlock();			
	}

	adRequestInfo* adRequest_pop(int fd)
	{	
		adRequestInfo* reqinof = NULL;
		m_exBidManager_lock.write_lock();
		for(auto it = m_exBidList.begin(); it != m_exBidList.end(); it++)
		{
			exBidObject *exbid = *it;
			if(exbid) 
			{
				reqinof = exbid->adRequest_pop(fd);
				if(reqinof) break;
			}
		}	
		m_exBidManager_lock.read_write_unlock();	
		return reqinof;
	}
	void delete_exBid(const string& ip, unsigned short port)
	{
		m_exBidManager_lock.write_lock();
		for(auto it = m_exBidList.begin(); it != m_exBidList.end();)
		{
			delete *it;
			it = m_exBidList.erase(it);
		}	
		m_exBidManager_lock.read_write_unlock();
	}
	~exBidManager()
	{
		m_exBidManager_lock.write_lock();
		for(auto it = m_exBidList.begin(); it != m_exBidList.end();)
		{
			delete *it;
			it = m_exBidList.erase(it);
		}
		m_exBidManager_lock.read_write_unlock();
	}
private:
	vector<exBidObject*> m_exBidList;
	read_write_lock m_exBidManager_lock;
	struct event_base *m_base = NULL;
	event_callback_fn m_fn;
	void * m_arg = NULL;

};

#endif
