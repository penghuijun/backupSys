#ifndef __BUFFERMANAGER_H__
#define __BUFFERMANAGER_H__
#include <string>
#include <string.h>
#include <iostream>
#include <queue>
using namespace std;
class stringBuffer
{
public:
	stringBuffer(const char* data, int dataLen)
	{
		string_new(data, dataLen);
	}

	void string_set(char* data, int dataLen)
	{
		m_data = data;
		m_dataLen = dataLen;
	}
	void string_new(const char* data, int dataLen)
	{
		if(dataLen > 0)
		{
			m_data = new char[dataLen];
			memcpy(m_data, data, dataLen);
			m_dataLen = dataLen;
		}
	}
	char *get_data(){return m_data;}
	int   get_dataLen(){return m_dataLen;}
	~stringBuffer()
	{
		delete[] m_data;
		m_data = NULL;
	}
	
private:
	char *m_data=NULL;
	int   m_dataLen=0;
};

class bufferManager
{
public:
	bufferManager(){}
	void add_buffer(char *data, int dataLen)
	{
		stringBuffer *buf = new stringBuffer(data, dataLen);
		m_bufferList.push_back(buf);
	}
    vector<stringBuffer*>& get_bufferList(){return m_bufferList;}	
	int bufferListSize(){return m_bufferList.size();}
	~bufferManager()
	{
		for(auto it = m_bufferList.begin(); it != m_bufferList.end();)
		{
			delete *it;
			it = m_bufferList.erase(it);
		}
	}
	
private:
	vector<stringBuffer*> m_bufferList;
};

class logManager
{
public:
	logManager()
	{
		m_logMutext.init();
	}
	void addLog(const string& log)
	{
        stringBuffer *logStr=new stringBuffer(log.c_str(), log.size());
		m_logMutext.lock();
        m_logBufQueue.push(logStr);	
		m_logMutext.unlock();
	}
	
	void addLog(const char* log, int logLen)
	{
        stringBuffer *logStr=new stringBuffer(log, logLen);
		m_logMutext.lock();
		m_logBufQueue.push(logStr);	
		m_logMutext.unlock();
	}

	void popLog(vector<stringBuffer*> &strVec,unsigned int size = 10000)
	{
		auto getSize = size;
		
		m_logMutext.lock();
		auto queSize = m_logBufQueue.size();
		if(queSize < getSize) getSize = queSize;
		for(auto i = 0; i < getSize; i++)
		{
			stringBuffer* strbuf = m_logBufQueue.front();
			if(strbuf)
			{
				strVec.push_back(strbuf);
				m_logBufQueue.pop();
			}
			m_logMutext.unlock();
		}
		m_logMutext.unlock();
	}
	~logManager(){}
private:
	queue<stringBuffer*> m_logBufQueue;	
	mutex_lock m_logMutext;
};

#endif
