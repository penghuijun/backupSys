/*
  *project: bidder
  *auth: yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#include "adConfig.h"
#include "connector.h"
#include "spdlog/spdlog.h"
shared_ptr<spdlog::logger> g_file_logger;
shared_ptr<spdlog::logger> g_manager_logger;
shared_ptr<spdlog::logger> g_worker_logger;

pthread_mutex_t tmMutex;
int main(int argc, char *argv[])
{

    g_file_logger = spdlog::rotating_logger_mt("debug", "logs/cdebugfile", 1048576*500, 3, true); 
    g_manager_logger = spdlog::rotating_logger_mt("manager", "logs/cmanagerfile", 1048576*500, 3, true); 
#ifdef DEBUG
    g_manager_logger->info("-------------------------------------DEBUG   MODE-------------------------------------");
#else
    g_manager_logger->info("-------------------------------------RELEASE MODE-------------------------------------");
#endif

    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    g_manager_logger->info("Current 0MQ version is {0:d}.{1:d}.{2:d}", major, minor, patch);

    pthread_mutex_init(&tmMutex, NULL);

    string configFileName("../adManagerConfig.txt");
    if(argc==2)
    {
        configFileName = argv[1];
    }
    
    configureObject configure(configFileName);
    configure.display();
    connectorServ connector(configure);
    connector.run();
    pthread_mutex_destroy(&tmMutex);
    return 0;
}




