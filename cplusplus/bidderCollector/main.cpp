/*
  *project: bidder
  *auth: yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#include "bcConfig.h"
#include "bcServ.h"

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

    configureObject configure("bcConfig.txt");
    configure.display();
    bcServ bc(configure);
    bc.run();
  //  pthread_mutex_destroy(&tmMutex);
    return 0;
}


