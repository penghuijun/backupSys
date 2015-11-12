//#include <iostream>
#include "connector.h"
#include "spdlog/spdlog.h"
#include "adConfig.h"


shared_ptr<spdlog::logger> g_master_logger;
shared_ptr<spdlog::logger> g_manager_logger;
shared_ptr<spdlog::logger> g_worker_logger;
shared_ptr<spdlog::logger> g_workerGYIN_logger;
shared_ptr<spdlog::logger> g_workerSMAATO_logger;
shared_ptr<spdlog::logger> g_workerINMOBI_logger;



int main(int argc,char *argv[])
{
    //g_master_logger = spdlog::rotating_logger_mt("master", "logs/debugfile", 1048576*500, 3, true);
    g_master_logger = spdlog::daily_logger_mt("master", "logs/debugfile", true); 
    g_manager_logger = spdlog::daily_logger_mt("manager", "logs/managerfile", true); 
#ifdef DEBUG
    g_manager_logger->info("-------------------------------------DEBUG   MODE-------------------------------------");
#else
    g_manager_logger->info("-------------------------------------RELEASE MODE-------------------------------------");
#endif

    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    g_manager_logger->info("Current 0MQ version is {0:d}.{1:d}.{2:d}", major, minor, patch);

    string configFileName("adManagerConfig.txt");
    configureObject configure(configFileName);
    configure.display();

    connectorServ connector(configure);
    connector.run();

    return 0;
}
