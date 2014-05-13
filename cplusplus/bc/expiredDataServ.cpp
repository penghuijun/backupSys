
#include <iostream>
#include <string>
#include <pthread.h>
#include <time.h>
#include <map>
#include <set>
#include <string>
#include <utility>
#include "zmq.h"

#include "AdBidderResponseServ.h"
#include "CommonMessage.pb.h"
#include "AdBidderResponseTemplate.pb.h"
#include "expireddata.pb.h"

#include "expiredDataServ.h"

using namespace com::rj::protos::msg;
using namespace com::rj::protos;
using namespace std;

extern pthread_mutex_t bcDataMutex;
extern map<string, bcDataRec*> bcDataRecList;


int expireTimes = 0;
int en = 0;
int bidSuc = 0;
int aaa = 0;
int bbb = 0;
bool expireDataServ::run()
{
	cout<< "expireDataServ::run" << endl;

    while(1)
    {
    	char tcp[32]={0};
    	void *context;
    	void *rspHandler ;
    	int rc;
    
        do
        {
    		context = zmq_ctx_new();
    		rspHandler = zmq_socket(context, ZMQ_DEALER);
    		memset(tcp, 0x00, sizeof(tcp));
    		sprintf(tcp, "tcp://%s:%d",get_ip(), get_port());
    		rc = zmq_bind(rspHandler, tcp);
    		if(rc != 0)
    		{	
    			zmq_close(rspHandler);
    			zmq_term(context);
    		}
        }while(rc != 0);

        
		cout<< "expireDataServ start, bind success" << tcp << endl;
		set_context(context);
		set_rspHandler(rspHandler);
    
    	while(1)
    	{
    	
    		char buf[1024];
    		int size = zmq_recv(get_rspHander(), buf, sizeof(buf), 0);
    		try{
    			if(size)
    			{
                    string msgStr(buf);
    				CommonMessage commMsg;
    				commMsg.ParseFromString(msgStr);
    				string rsp = commMsg.data();
    				ExpiredMessage expireMsg;
    				expireMsg.ParseFromString(rsp);
    				string uuid = expireMsg.uuid();
                    string expiredata = expireMsg.status();   
#ifdef DEBUG
                    cout << "expire: F <"<< uuid << "> " << en++ << endl;
#endif
                    
                    pthread_mutex_lock(&bcDataMutex);
                    map<string, bcDataRec*>::iterator bcDataIt = bcDataRecList.find(uuid);
                    if(bcDataIt != bcDataRecList.end())
                    {
//#ifdef DEBUG                        
                        cout << "request timeout times:" << expireTimes++ << endl;
//#endif
                        bcDataRec *bcData = bcDataIt->second;
                        if(bcData)
                        {
                            if(bcData->buf) delete[] bcData->buf;
                            delete bcData;
                        }
    			        bcDataRecList.erase(uuid); 

                    }
                    else
                    {
#ifdef DEBUG
                        cout <<"expireDataServ bid success" << bidSuc++ <<endl;
#endif
                    }
                    pthread_mutex_unlock(&bcDataMutex);  
                }
    	    }
    		catch(...)
    		{
              cout << "throw error" << endl;
                exit(-1);
    		}
    			
    		zmq_send(get_rspHander(), "OK", 2, 0);
    	}
    }
}
