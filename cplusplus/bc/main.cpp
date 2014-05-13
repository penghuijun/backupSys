/**/
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "AdBidderResponseServ.h"
#include "redisClient.h"
#include "subscribeClient.h"
#include "expiredDataServ.h"
#include "bcConfig.h"

using namespace std;
pthread_mutex_t redisMutex;
pthread_mutex_t bcDataMutex;


void* subThreadProc( void* s)
{
    subscribeClient *sc = (subscribeClient*)s;
	sc->run();
}

void* bidReqThreadProc(void *b)
{
    adBidderRspServ *bidServ = (adBidderRspServ *) b;
	bidServ->run();
}


void *UUIDExpireThreadProc(void *e)
{
    expireDataServ *expireServ = (expireDataServ *)e;
	expireServ->run();
}


void *checkRedisThreadProc(void *r)
{
    redisClient *rc = (redisClient *) r;

	while(1)
	{
#ifdef DEBUG
		cout << "check and del redis, timeval: "<< rc->getDelInterval()<< endl;
#endif
		rc->delRedisItem();
	    sleep(3);
	}
}

/*
double get_cpu_mhz(void)
{
	FILE* f;
	char buf[256];
	double mhz = 0.0;

    f = fopen("/proc/cpuinfo","r"); //打开 proc/cpuinfo 文件
	if (!f)
		return 0.0;
	while(fgets(buf, sizeof(buf), f)) {
		double m;
		int rc;
    rc = sscanf(buf, "cpu MHz : %lf", &m); //读取 cpu MHz
		if (mhz == 0.0) {
			mhz = m;
			break;
		}
	}
	fclose(f);
return mhz; //返回 HZ 值
}
*/

#define rdtsc(low,high) __asm__ \
 __volatile__("rdtsc" : "=a" (low), "=d" (high))


unsigned long long get_cycles()
{
	unsigned low, high;
	unsigned long long val;
	rdtsc(low,high);
	val = high;
    val = (val << 32) | low; //将 low 和 high 合成一个 64 位值
	return val;
}


#define FILEBUFSIZE 256
int main(int argc, char *argv[])
{
#ifdef DEBUG
cout << "-------------------------------------------DEBUG   MODE----------------------------------------" << endl;
#else
cout << "-------------------------------------------RELEASE MODE----------------------------------------" << endl;
#endif
    string bcConfTxt("bcConfig.txt");
    bcConfig bcConf(bcConfTxt);  
    int *ret;
  
    redisClient     delRcClient(bcConf.get_redisSaveIP(), bcConf.get_redisSavePort(), bcConf.get_redisSaveTime());
	expireDataServ  UUIDExpireServ(bcConf.get_bcServIP(), bcConf.get_bcListenExpirePort());
	subscribeClient subClient(bcConf.get_redisPublishIP(), bcConf.get_redisPublishPort(), bcConf.get_redisPublishKey(),  bcConf.get_redisSaveIP(), bcConf.get_redisSavePort());
	adBidderRspServ bidRspServ(bcConf.get_bcServIP(), bcConf.get_bcListenBidPort(), bcConf.get_redisSaveIP(), bcConf.get_redisSavePort());
    
    delRcClient.run();

    pthread_mutex_init(&redisMutex, NULL);
    pthread_mutex_init(&bcDataMutex, NULL);

    pthread_t bidReqThread;
    pthread_t UUIDExpireThread;
    pthread_t subThread;
    pthread_t delRedisThread;
    
    pthread_create(&bidReqThread, NULL, bidReqThreadProc, (void *)&bidRspServ);
    pthread_create(&UUIDExpireThread, NULL, UUIDExpireThreadProc, (void *)&UUIDExpireServ);
    pthread_create(&subThread, NULL, subThreadProc, (void *)&subClient);
    pthread_create(&delRedisThread, NULL, checkRedisThreadProc, (void *)&delRcClient);
    pthread_join(bidReqThread, (void **)&ret);
    pthread_join(UUIDExpireThread, (void **)&ret);
    pthread_join(subThread, (void **)&ret);
    pthread_join(delRedisThread, (void **)&ret);

    pthread_mutex_destroy (&redisMutex);
    pthread_mutex_destroy(&bcDataMutex);

	return 0;
}


