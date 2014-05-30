#include <string.h>
#include <iostream> 
#include "masterWorker.h"
#include <pthread.h>
#include <queue>
extern pthread_mutex_t mtx;
extern pthread_cond_t cond;
extern queue<throttleBuf*> g_throttleBuf;


volatile sig_atomic_t srv_graceful_end = 0;
volatile sig_atomic_t srv_ungraceful_end = 0;
volatile sig_atomic_t srv_restart = 0;
volatile sig_atomic_t sigalrm_recved = 0;;

void signal_handler(int signo)
{
    switch (signo)
    {
        case SIGTERM:
	        cout << "SIGTERM+++++++++++++++++++++" << endl;
            srv_ungraceful_end = 1;
            break;
        case SIGINT:
	        cout << "SIGINT----------------------" << endl;
            srv_graceful_end = 1;
            break;
        case SIGHUP:
            cout << "SIGHUP#####################" << endl;
            srv_restart = 1;
            break;
        case SIGALRM:
            cout << "SIGALRM@@@@@@@@@@@@@@@@@@@@" << endl;
            sigalrm_recved = 1;
            break;
    }
}

void *startThrottle(void *throttle)
{
    throttleServ *serv = (throttleServ*) throttle;
    if(serv)
    {
        serv->masterRun();
    }
    else
    {
        cout <<"error: startThrottle param is null"<< endl;
        return nullptr;
    }
}

 
void masterWorkerServer::workerLoop(throttleServ &throttle)
{
    sigalrm_recved = 0;
    char buf[1024];
    int size;
    try
    {
        throttle.workerStart();
        while (true)
        {    
            size = throttle.workerRecvPullData(buf, sizeof(buf));
            if(size > 0)
            {
              //  cout << "recv pull data size:" << size << endl;
                throttle.workerHandler(buf, size);
            }
      
            if (srv_ungraceful_end || srv_graceful_end || srv_restart)
            {
                exit(0);
            }
 
            if (sigalrm_recved)
            {
                printf("worker pid : %d can recv alarm\n", getpid());
            }
        }
    }
    catch(...)
    {
        cout <<"worker exit:"<<getpid()<<endl;
    }
}


BC_process_t* masterWorkerServer::getWorkerPro(pid_t pid)
{
    auto it = m_workerList.begin();
    for(it = m_workerList.begin(); it != m_workerList.end(); it++)
    {
        BC_process_t *pro = *it;
        if(pro == NULL) continue;
        if(pro->pid == pid) return pro;
    }
    return NULL;
}


void masterWorkerServer::run(throttleServ &throttle)
{
    int num_children = 0;
    int restart_finished = 1;
    int is_child = 0;
    int i;
    bool mainProStart=false;
    static int proIdx = 0;
   // register signal handler
    struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = signal_handler;
     
    sigaction(SIGALRM, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT , &act, NULL);
    sigaction(SIGHUP , &act, NULL);
     
    
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGALRM);
    sigdelset(&set, SIGTERM);
    sigdelset(&set, SIGINT);
    sigdelset(&set, SIGHUP);
     
    sigprocmask(SIG_SETMASK, &set, NULL);
        
    struct itimerval timer;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 10;
    timer.it_interval.tv_usec = 0;
     
    setitimer(ITIMER_REAL, &timer, NULL);

    m_masterPid = getpid();
    
    while(true)
    {
        pid_t pid;
        if(num_children != 0)
        {
            pid = wait(NULL);
            if(pid != -1)
            {
                num_children--;
                cout <<pid << "exit************" << endl;
                BC_process_t* pro =  getWorkerPro(pid);
                if(pro && pro->pid == pid)
                {
                    pro->pid = 0;
                    pro->status = PRO_INIT;
                }
            }

            if (srv_graceful_end || srv_ungraceful_end)
            {
                if (num_children == 0)
                {
                    break;    //get out of while(true)
                }

                auto it = m_workerList.begin();
                for (it = m_workerList.begin(); it != m_workerList.end(); it++)
                {
                    BC_process_t *pro = *it;
                    if(pro&&pro->pid != 0)
                    {
                        kill(pro->pid, srv_graceful_end ? SIGINT : SIGTERM);
                    }
                }
                continue;    //this is necessary.
            }

            if (srv_restart)
            {
                srv_restart = 0;    
 
                if (restart_finished)
                {
                    restart_finished = 0;
                    auto it = m_workerList.begin();
                    for (it = m_workerList.begin(); it != m_workerList.end(); it++)
                    {
                        BC_process_t *pro = *it;
                        if(pro == nullptr) continue;
                        pro->status = PRO_RESTART; 
                    }
                }
            }

            if (!restart_finished) //not finish
            {
                int ndx;
                BC_process_t *pro=NULL;

                auto it = m_workerList.begin();
                for (it = m_workerList.begin(); it != m_workerList.end(); it++)
                {
                    pro = *it;
                    if (pro->status == PRO_RESTART)
                    {
                        break;
                    }
                }
            
                // 上一次发送SIGHUP时正处于工作的worker已经全部
                // 重启完毕.
                if (it == m_workerList.end())
                {
                    restart_finished = 1;
                }
                else
                {
                    kill(pro->pid, SIGHUP);
                    //child_info[ndx].state = 3;  重启中
                 
                    //还是为了尽量避免连续的SIGHUP的不良操作带来的颠簸
                    //,所以决定取消(3重启中)这个状态
                    //并不是说连续SIGHUP会让程序出错,只是不断的挂掉新进程很愚蠢
                }
            }
 
        }

        char buf[12];
        ssize_t n;
        
        auto it = m_workerList.begin();
        for (it = m_workerList.begin(); it != m_workerList.end(); it++)
        {
            BC_process_t *pro = *it;
            if(pro == NULL) continue;
            if (pro->status == PRO_INIT)
            {
                pid = fork();
 
                switch (pid)
                {
                    case -1:
                        break;
                    case 0:
                    {
                        pro->pid = getpid();
                        is_child = 1; 
                    }
                    break;
                    default:
                    {

                         pro->pid = pid;
                        ++num_children;
                    }
                    break;
                }
                pro->status = PRO_IDLE;
                if (!pid) break; 
            }
        }
        
        if(is_child==0 && m_masterStart==false)
        {
             //master serv start
             m_masterStart = true;  
             pthread_t worker;
             pthread_create(&worker, NULL, startThrottle, (void *)&throttle);	
        }
        else
        {
 
        }
       // cout << "is_child:" << is_child << endl;
        if (!pid) break; 
    }
 
    if (!is_child)    
    {
        cout << "master exit@@@@@@@@@@@@@@@@@@"<<endl;
        exit(0);    
    }
 
    workerLoop(throttle);  
}

