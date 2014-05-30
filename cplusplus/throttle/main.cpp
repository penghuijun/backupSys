/**/
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "throttleConfig.h"
#include "throttle.h"
#include "masterWorker.h"
#include "threadpoolmanager.h"

#define rdtsc(low,high) __asm__ \
 __volatile__("rdtsc" : "=a" (low), "=d" (high))

pthread_mutex_t mtx=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond=PTHREAD_COND_INITIALIZER;

unsigned long long get_cycles()
{
	unsigned low, high;
	unsigned long long val;
	rdtsc(low,high);
	val = high;
    val = (val << 32) | low; //将 low 和 high 合成一个 64 位值
	return val;
}

int main(int argc, char *argv[])
{
  
    throttleConfig throttle("throttleConfig.txt");
    throttle.display();
    throttleServ thServ(throttle);
    masterWorkerServer throServ(throttle.get_throttleworkerNum());
    throServ.run(thServ);
    cout << "end point" << endl;
    return 0;
}


