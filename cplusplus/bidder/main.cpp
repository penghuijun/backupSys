/*
  *project: bidder
  *auth: yanjun.xiang
  *date:2014-7-22
  *All rights reserved
  */
#include "adConfig.h"
#include "bidder.h"
#include "spdlog/spdlog.h"
shared_ptr<spdlog::logger> g_file_logger;
shared_ptr<spdlog::logger> g_manager_logger;
shared_ptr<spdlog::logger> g_worker_logger;

unsigned short bidder_show_help;
unsigned short bidder_show_version;
/**
 * bidder config path
 */
char *bidder_conf_file;

static int cmd_get_options(int argc, char *const *argv);
static inline void print_help();


int main(int argc, char *argv[])
{
    int major, minor, patch;
    bidder_show_help    = 0;
    bidder_show_version = 0;
    bidder_conf_file    = NULL;

    if (cmd_get_options(argc, argv) == -1) 
        exit(-1);
    
    if (bidder_show_help == 1) {
        print_help();
        exit(0);
    }
    if (bidder_show_version == 1) {
        printf("bidder version: bidder_mobile/%s\n", BIDDER_VERSION);
        exit(0);
    }
    
    g_file_logger = spdlog::rotating_logger_mt("debug", "logs/debugfile", 1048576*500, 3, true); 
    g_manager_logger = spdlog::rotating_logger_mt("manager", "logs/managerfile", 1048576*500, 3, true); 
#ifdef DEBUG
    g_manager_logger->info("-------------------------------------DEBUG   MODE-------------------------------------");
#else
    g_manager_logger->info("-------------------------------------RELEASE MODE-------------------------------------");
#endif
    
    zmq_version (&major, &minor, &patch);
    g_manager_logger->info("Current 0MQ version is {0:d}.{1:d}.{2:d}", major, minor, patch);

    string configFileName(bidder_conf_file == NULL ? "../adManagerConfig.txt" : bidder_conf_file);
    configureObject configure(configFileName);
    configure.display();

    bidderServ bidder(configure);
    bidder.run();

    return 0;
}


static int
cmd_get_options(int argc, char *const *argv)
{
    u_char     *p;
    int   i;

    for (i = 1; i < argc; i++) {

        p = (u_char *) argv[i];

        if (*p++ != '-') {
            printf("invalid option: \"%s\"", argv[i]);
            return -1;
        }

        while (*p) {

            switch (*p++) {

            case '?':
            case 'h':
                bidder_show_help = 1;
                break;

            case 'v':
                bidder_show_version = 1;
                break;

            case 'c':
                if (*p) {
                    bidder_conf_file = (char *)p;
                    goto next;
                }

                if (argv[++i]) {
                    bidder_conf_file = (char *) argv[i];
                    goto next;
                }

                printf("option \"-c\" requires file name");
                return -1;

            default:
                printf("invalid option: \"%c\"", *(p - 1));
                return -1;
            }
        }

next:
    continue;
    }

}


static inline void
print_help()
{
    printf(
        "Usage: bidder_mobile [-?hvc]\n\n" 
        "Options:\n" 
        "\t-?, -h\t\t : help info\n" 
        "\t-c filename\t : set bidder run config file path. default path is '../adManagerConfig.txt'\n" 
        "\t-v\t\t : show version\n" 
        "run method:\n" 
        "\tkill -SIGHUP  master_pid \t: restart server\n" 
        "\tkill -SIGTERM master_pid \t: ungraceful stop server\n" 
        "\tkill -SIGINT  master_pid \t: graceful stop server\n\n" 
    );
}



