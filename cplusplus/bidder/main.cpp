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

unsigned short bidder_daemon;
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

    g_file_logger = spdlog::rotating_logger_mt("debug", "logs/debugfile", 1048576*500, 3, true); 
    g_manager_logger = spdlog::rotating_logger_mt("manager", "logs/managerfile", 1048576*500, 3, true); 
#ifdef DEBUG
    g_manager_logger->info("-------------------------------------DEBUG   MODE-------------------------------------");
#else
    g_manager_logger->info("-------------------------------------RELEASE MODE-------------------------------------");
#endif

    int major, minor, patch;
    /**
     * init bidder_daemon
     */
    bidder_daemon       = 0;
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
    if (bidder_conf_file == NULL) {
        printf("%s -c config_path, -c option is requires.\n", argv[0]);
        exit(-1);
    }
    zmq_version (&major, &minor, &patch);
    g_manager_logger->info("Current 0MQ version is {0:d}.{1:d}.{2:d}", major, minor, patch);

    string configFileName(bidder_conf_file);
    if(argc==2)
    {
        configFileName = argv[1];
    }
    
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

            case 'd':
                bidder_daemon = 1;
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
        "Usage: bidder_mobile [-?hvdc]\n\n" 
        "Options:\n" 
        "\t-?, -h\t\t : help info\n" 
        "\t-c filename\t : set bidder run config file path\n" 
        "\t-v\t\t : show version\n" 
        "\t-d\t\t : usage daemon run\n" 
        "Restart:\n" 
        "\tkill -SIGHUP master_pid\n\n" 
    );
}



