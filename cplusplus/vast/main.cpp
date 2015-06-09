/*
  *project: bidder
  *auth: yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#include "vastConfig.h"
#include "vast.h"
#include "zmq.h"
#include <unistd.h>

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


    configureObject configure("vastConfig.txt");
    configure.display();
    vast vs(configure);
    int sendRapid = 0;
    int sendCnt = 0;
    if(argc == 3) 
    {
        sendRapid = atoi(argv[1]);
        sendCnt = atoi(argv[2]);
    }
    
    vs.run(sendRapid, sendCnt);
    return 0;
}


