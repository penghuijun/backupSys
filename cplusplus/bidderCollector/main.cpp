/*
  *project: bidder
  *auth: yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#include "adConfig.h"
#include "bcServ.h"
#include "spdlog/spdlog.h"
shared_ptr<spdlog::logger> g_file_logger;
shared_ptr<spdlog::logger> g_manager_logger;

int main(int argc, char *argv[])
{
    g_file_logger = spdlog::rotating_logger_mt("debug", "logs/debugfile", 1048576*500, 3, true); 
    g_manager_logger = spdlog::rotating_logger_mt("manager", "logs/managerfile", 1048576*500, 3, true); 

#ifdef DEBUG
    g_manager_logger->info("-------------------------------------DEBUG   MODE-------------------------------------");
#else
    g_manager_logger->info("-------------------------------------RELEASE MODE-------------------------------------");
#endif
    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    g_manager_logger->info("Current 0MQ version is {0:d}.{1:d}.{2:d}", major, minor, patch);

    string configFileName("../adManagerConfig.txt");
    if(argc == 2)
    {
        configFileName = argv[1];
    }

    configureObject configure(configFileName);

    bcServ bc(configure);
    bc.run();
    return 0;
}


