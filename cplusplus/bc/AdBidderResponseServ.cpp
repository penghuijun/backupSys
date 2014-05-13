#include <iostream>
#include <pthread.h>
#include <string>
#include <time.h>
#include <set>
#include <map>
#include <iterator>
#include <utility>
#include <unistd.h>
#include "zmq.h"

#include "CommonMessage.pb.h"
#include "AdBidderResponseTemplate.pb.h"

#include "AdBidderResponseServ.h"


using namespace com::rj::protos::msg;
using namespace std;

#define BUFSIZE (1024*1024)


extern pthread_mutex_t bcDataMutex;
extern pthread_mutex_t redisMutex;
extern map<string, bcDataRec*> bcDataRecList;
map<string, redisDataRec*> redisRecList;


adBidderRspServ::adBidderRspServ(const char *ip, unsigned short port, const char *redisIP, unsigned short redisPort ):zeroMqServer(ip, port)
{
    m_redisServIp= redisIP;
    m_redisServPort = redisPort;
}


adBidderRspServ::adBidderRspServ(string ip, unsigned short port, string redisIP, unsigned short redisPort ):zeroMqServer(ip, port)
{
    m_redisServIp = redisIP;
    m_redisServPort = redisPort;
}


void *bidFunc(void *bid)
{	     
	adBidderRspServ *serv = (adBidderRspServ*) bid;
	serv->worker_routine(serv->get_context());
	
}

int aNum = 0;
int insNum = 0;
int failHset = 0;
int threeType = 0;

void *adBidderRspServ::worker_routine(void *context)
{
    CommonMessage commMsg;
    string rsp;
    BidderResponse bidRsp;
    string uuid;
    string bidID;
    string sockID;
    int size;

    struct timeval tv;



    char buf[BUFSIZE]={0};
	void *receiver = zmq_socket(context, ZMQ_REP);
	zmq_connect(receiver, "inproc://workers");

    redisClient redisC(m_redisServIp, m_redisServPort);
    redisC.connectaRedis();
    
	while(1)
	{
		size= zmq_recv(get_rspHander(), buf, sizeof(buf), 0);
		if(size <= 0)
		{
			continue;
		}
        
#ifdef DEBUG
        if(size >100)
        {
            cout << "bider first recv size:" << size << endl;
        }
#endif
		
		try
		{            
			sockID = buf;
			memset(buf, 0, BUFSIZE);
                   
			size = zmq_recv(get_rspHander(), buf, sizeof(buf), 0);
	 
			if(size <= 0)
			{
				cout <<"222222222"<< endl << endl << endl;
				continue;
			}	
            

			commMsg.ParseFromString(buf);
			rsp = commMsg.data();
			bidRsp.ParseFromString(rsp);
			uuid = bidRsp.id();
			bidID = bidRsp.bidderid();

            time_t timep;
            time(&timep);

            string bidIDandTime=bidID+":" ;
            char timebuf[32] ={0};
            sprintf(timebuf, "<%d>", timep);
            bidIDandTime += timebuf;

            pthread_mutex_lock(&bcDataMutex);
            map<string, bcDataRec*>::iterator bcDataIt = bcDataRecList.find(uuid);
                  
            if(bcDataIt != bcDataRecList.end())
            {
                string bcDataUuid = bcDataIt->first;
                bcDataRec *bcData = bcDataIt->second;
                if(bcData)
                {
                 
                    BCStatus status = bcData->status;
                    if(bcData->buf) delete[] bcData->buf;
                    delete bcData;
                    bcDataRecList.erase(bcDataIt);
#ifdef DEBUG
                    unsigned long long cyc = get_cycles();
                    cout << "adBidder<" << uuid << ">::" <<  pthread_self() << "@" << aNum++<<"  ===size: " << size << endl;
#endif
                    pthread_mutex_unlock(&bcDataMutex);

                    if(status == BC_recvSub)//只处理这种情况
                    {  
                    
                       redisReply *reply = (redisReply *)redisCommand(redisC.get_redisContex(), "hset %s %s %b",uuid.c_str(),  bidIDandTime.c_str() , buf, size);// 
#ifdef DEBUG
                                   unsigned long long cyc1 = get_cycles(); 
                                   unsigned long long diff = (cyc1-cyc)/( 3092.889);
                                   if(diff >= 10000) cout<< "$$$$$$$$$$$$$$$$$:::pthreadid:" <<pthread_self()<< "<"<<uuid <<"," << diff << ">"<< endl;
#endif

                        if(reply== NULL) //严重错误需要重新连接redis
                        {
                            cout << "<bidder> serious problem" << endl;
                            redisFree(redisC.get_redisContex());
                            if(redisC.connectaRedis() == false)
                            {
                                cout <<"<bidder> connectaRedis failure!!!" <<endl;
                            }
                            continue;
                        }
#ifdef DEBUG
                        if(reply->type == 3)
                        {
                            cout <<"h t c:" << insNum++ << endl;;
                        }   
                        else
                        {
                            cout << "redis hset failure:"<<reply->type << endl; 
                        }
#endif

                        freeReplyObject(reply);
                        redisDataRec *redis = new redisDataRec;
                        if(redis)
                        {  
                            redis->bidIDAndTime = bidIDandTime;
                            redis->time = timep;
                            pthread_mutex_lock(&redisMutex);
                            redisRecList.insert(pair<string, redisDataRec*>(uuid, redis));
                            pthread_mutex_unlock(&redisMutex);
                        }
                    }
            
                }
                else
                {
                    bcDataRecList.erase(bcDataIt);
                    pthread_mutex_unlock(&bcDataMutex);
                }
            }
            else
            {
                bcDataRec *bData = new bcDataRec;
                if(bData)
                {
                    bData->status = BC_recvBid;
                    bData->buf = new char[size];
                    memcpy(bData->buf, buf, size);
                    bData->bufSize = size;
                    bData->time = timep;
                    bData->bidIDAndTime = bidIDandTime;
                    bcDataRecList.insert(pair<string, bcDataRec*>(uuid, bData));

                }
                
                pthread_mutex_unlock(&bcDataMutex);
             } 

        }
		catch(...)
		{
			cout<< "worker_routine try error" << endl;
            exit(-1);
		}
 
		zmq_send(get_rspHander(), "OK", 2, 0);
	}
	zmq_close(receiver);
	return NULL;
}

bool adBidderRspServ::run()
{
	char tcp[32]={0};
    void *context;
    void *rspHandler;
    int rc;

    do
    {
	    context = zmq_ctx_new();
        rspHandler = zmq_socket(context, ZMQ_ROUTER);
		
    	memset(tcp, 0x00, sizeof(tcp));
	    sprintf(tcp, "tcp://%s:%d", get_ip(), get_port());
	    rc = zmq_bind(rspHandler, tcp);
	    if(rc != 0)
	    {	
	        cout << "<bid> look at me, some err in bind" << endl;
		    zmq_close(rspHandler);
    		zmq_term(context);
	    }
    }while(rc!=0);

    
	cout<< "adBidderRspServ start, bind success" << tcp << endl;
	set_context(context);
	set_rspHandler(rspHandler);

		
	void *workers = zmq_socket(context, ZMQ_DEALER);
	zmq_bind(workers, "inproc://workers");

	int thread_nbr;
    pthread_t worker;
    int *ret;
	for(thread_nbr = 0; thread_nbr < 1; thread_nbr++)
	{
		pthread_create(&worker, NULL, bidFunc, this);
	}

	pthread_join(worker, (void **)&ret);

	zmq_proxy(rspHandler, workers, NULL);
	return true;
}

