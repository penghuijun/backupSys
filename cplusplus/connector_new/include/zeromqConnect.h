#ifndef __ZEROMQCONNECT_H__
#define __ZEROMQCONNECT_H__

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string.h>
#include <unistd.h>
#include "zmq.h"
#include "spdlog/spdlog.h"

extern shared_ptr<spdlog::logger> g_manager_logger;
class zeromqConnect
{
public:
	zeromqConnect(){}
	void init()
	{
		m_zmqContext =zmq_ctx_new();
		if(m_zmqContext == NULL)
		{
			g_manager_logger->emerg("zmq_ctx_new exception");
			exit(1);
		}
	}
	void* establishConnect(bool client, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd)
	{
			ostringstream os;
			string pro;
			int rc;
			void *handler = nullptr;
			size_t size;
			int linger = 0;
		
			os.str("");
			os << transType << "://" << addr << ":" << port;
			pro = os.str();
			handler = zmq_socket (m_zmqContext, zmqType);
			zmq_setsockopt(handler,  ZMQ_LINGER, &linger, sizeof(linger));	
			if(client)
			{
			   rc = zmq_connect(handler, pro.c_str());
			}
			else
			{
			   rc = zmq_bind (handler, pro.c_str());
			}
		
			if(rc!=0)
			{
				g_manager_logger->info("{0},{1:d} connect or bind error", addr, port);
				zmq_close(handler);
				return nullptr; 	
			}
		
			if(fd != nullptr&& handler!=nullptr)
			{
				size = sizeof(int);
				rc = zmq_getsockopt(handler, ZMQ_FD, fd, &size);
				if(rc != 0 )
				{
					g_manager_logger->info("zmq_setsockopt  ZMQ_FD faliure");
					zmq_close(handler);
					return nullptr; 
				}
			}
			return handler;
	}

	void* establishConnect(bool client, const char * transType,int zmqType,const char * addr, int *fd)
	{
			ostringstream os;
			string pro;
			int rc;
			void *handler = nullptr;
			size_t size;
			int linger = 0;
		
			os.str("");
			os << transType << "://" << addr;
			pro = os.str();
			handler = zmq_socket (m_zmqContext, zmqType);
			zmq_setsockopt(handler,  ZMQ_LINGER, &linger, sizeof(linger));	
			if(client)
			{
			   rc = zmq_connect(handler, pro.c_str());
			}
			else
			{
			   rc = zmq_bind (handler, pro.c_str());
			}
		
			if(rc!=0)
			{
				g_manager_logger->info("{0} connect or bind error", addr);
				zmq_close(handler);
				return nullptr; 	
			}
		
			if(fd != nullptr&& handler!=nullptr)
			{
				size = sizeof(int);
				rc = zmq_getsockopt(handler, ZMQ_FD, fd, &size);
				if(rc != 0 )
				{
					g_manager_logger->info("zmq_setsockopt  ZMQ_FD faliure");
					zmq_close(handler);
					return nullptr; 
				}
			}
			return handler;
	}


	void* establishConnect(bool client, bool identy, string& id, const char * transType,int zmqType,const char * addr,unsigned short port, int *fd)
	{
			ostringstream os;
			string pro;
			int rc;
			void *handler = nullptr;
			size_t size;
		
		
			os.str("");
			os << transType << "://" << addr << ":" << port;
			pro = os.str();
		
			handler = zmq_socket (m_zmqContext, zmqType);
			if(identy)
			{
				int hwm = 1;
				int linger = 0;
				zmq_setsockopt(handler,  ZMQ_IDENTITY, id.c_str(), id.size());
                zmq_setsockopt(handler,  ZMQ_SNDHWM, &hwm, sizeof(hwm));
			    zmq_setsockopt(handler,  ZMQ_LINGER, &linger, sizeof(linger));	

			}
			if(client)
			{
			   rc = zmq_connect(handler, pro.c_str());
			}
			else
			{
			   rc = zmq_bind (handler, pro.c_str());
			}
		
			if(rc!=0)
			{
				g_manager_logger->info("{0},{1:d} connect or bind error", addr, port);
				zmq_close(handler);
				return nullptr; 	
			}
		
			if(fd != nullptr&& handler!=nullptr)
			{
				size = sizeof(int);
				rc = zmq_getsockopt(handler, ZMQ_FD, fd, &size);
				if(rc != 0 )
				{
					g_manager_logger->info("zmq_setsockopt  ZMQ_FD faliure");
					zmq_close(handler);
					return nullptr; 
				}
			}
			return handler;
	}	
	~zeromqConnect()
	{
		zmq_ctx_destroy(m_zmqContext);
	}
private:
	void *m_zmqContext = nullptr;	
};
#endif

