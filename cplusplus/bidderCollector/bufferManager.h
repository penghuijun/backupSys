#ifndef __BUFFERMANAGER_H__
#define __BUFFERMANAGER_H__
#include <string>
#include <string.h>
#include <iostream>
using namespace std;
class stringBuffer
{
public:
	stringBuffer(char* data, int dataLen)
	{
		string_new(data, dataLen);
	}

	void string_set(char* data, int dataLen)
	{
		m_data = data;
		m_dataLen = dataLen;
	}
	void string_new(char* data, int dataLen)
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

#endif
