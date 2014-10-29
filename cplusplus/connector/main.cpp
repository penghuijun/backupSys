/*
  *project: bidder
  *auth: yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#include "bidderConfig.h"
#include "bidder.h"

pthread_mutex_t tmMutex;
int main(int argc, char *argv[])
{
#ifdef DEBUG
cout << "-------------------------------------------DEBUG   MODE----------------------------------------" << endl;
#else
cout << "-------------------------------------------RELEASE MODE----------------------------------------" << endl;
#endif
    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    cout <<"Current 0MQ version is "<<major<<"."<<minor<<"."<< patch<<endl;

    pthread_mutex_init(&tmMutex, NULL);
    configureObject configure("bidderConfig.txt");
    configure.display();
    bidderServ bidder(configure);
    bidder.run();
    pthread_mutex_destroy(&tmMutex);
    return 0;
}


