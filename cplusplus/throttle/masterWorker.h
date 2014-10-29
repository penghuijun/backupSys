#ifndef __MASTERWORK_H__
#define __MASTERWORK_H__
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "throttle.h"
#include "throttle.h"

using namespace std;

enum proStatus
{
	PRO_INIT,
    PRO_BUSY,
	PRO_RESTART,
    PRO_IDLE
};

typedef struct BCProessInfo
{
    pid_t               pid;
    proStatus         status;
} BC_process_t; 

#define SHM_BUFSIZE 1024
#define SEMID 251
class masterWorkerServer
{
public:
	masterWorkerServer()
	{
		BC_process_t *pro = new BC_process_t;
		if(pro == NULL) return;
		pro->pid = 0;
		pro->status = PRO_INIT;
		m_workerList.push_back(pro);
	}
	masterWorkerServer(int workerNum):m_workerNum(workerNum)
	{
		auto i = 0;
		for(i = 0; i < m_workerNum; i++)
		{
			BC_process_t *pro = new BC_process_t;
			if(pro == NULL) return;
			pro->pid = 0;
			pro->status = PRO_INIT;
			m_workerList.push_back(pro);
		}
	};
	void run(throttleServ &throttle);
	void workerLoop(throttleServ &throttle);
	BC_process_t* getWorkerPro(pid_t pid);
	void displayWorker() const
	{
		auto it = m_workerList.begin();
		for(it = m_workerList.begin(); it != m_workerList.end(); it++)
		{
			BC_process_t *pro = *it;
			if(pro==nullptr) continue;
			cout << pro->pid << "--------" << pro->status << endl;
		}	
	}

private:

	int m_workerNum=1;
	vector<BC_process_t*>  m_workerList;	
	pid_t m_masterPid;
	bool m_masterStart=false;
};

#endif
