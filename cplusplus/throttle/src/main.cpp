/**/
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "adConfig.h"
#include "throttle.h"
#include "thread/threadpoolmanager.h"
#include "spdlog/spdlog.h"

shared_ptr<spdlog::logger> g_file_logger;
shared_ptr<spdlog::logger> g_manager_logger;

int main(int argc, char *argv[])
{
    //g_file_logger = spdlog::rotating_logger_mt("debug", "logs/debugfile", 1048576*500, 3, true); 
    //g_manager_logger = spdlog::rotating_logger_mt("manager", "logs/managerfile", 1048576*500, 3, true); 
    g_file_logger = spdlog::daily_logger_mt("debug", "logs/debugfile", true); 
    g_manager_logger = spdlog::daily_logger_mt("manager", "logs/managerfile", true); 
#ifdef DEBUG
    g_manager_logger->info("-------------------------------------DEBUG   MODE-------------------------------------");
#else
    g_manager_logger->info("-------------------------------------RELEASE MODE-------------------------------------");
#endif
    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    g_manager_logger->info("Current 0MQ version is {0:d}.{1:d}.{2:d}", major, minor, patch);


    string configFileName("../adManagerConfig.txt");
    if(argc==2)
    {
        configFileName = argv[1];
    }

    configureObject throttle(configFileName);
    throttle.display();
    throttleServ thServ(throttle);
    thServ.run();
    return 0;
}


