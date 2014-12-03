#ifndef TASKPOOL_H
#define TASKPOOL_H
/* purpose @ ����أ���Ҫ�ǻ����ⲿ�߲�������������manager�����������
 *          ����ؿ��Զ����ٳ�ʱ����е�Task����
 *          ��ͨ��CHECK_IDLE_TASK_INTERVAL���ü��idle���н�����ѵ�ȴ�ʱ��
 *          TASK_DESTROY_INTERVAL ����Task����ʱ�䣬�������ʱ��ֵ���ᱻCheckIdleTask�߳�����
 * date    @ 2013.12.23
 * author  @ haibin.wang
 */

#include <list>
#include <pthread.h>
#include "commondef.h"

//���е��û�����Ϊһ��task��
typedef void (*task_fun)(void *);
struct Task
{
    task_fun fun; //��������
    void* data; //����������
    time_t last_time; //������ж��е�ʱ�䣬�����Զ�����
};

//����أ����������Ͷ�ݵ�������У������̸߳�������Ͷ�ݸ��̳߳�
class TaskPool
{
public:
	/* pur @ ��ʼ������أ���������ؿ��ж����Զ������߳�
     * para @ maxSize ���������������0
    */ 
    TaskPool(const int & poolMaxSize);
    ~TaskPool();

    /* pur @ �������������е�β��
     * para @ task�� ��������
     * return @ 0 ��ӳɹ������� ���ʧ��
    */    
    int AddTask(task_fun fun, void* arg);
	
    /* pur @ �������б��ͷ��ȡһ������
     * return @  ����б����������򷵻�һ��Taskָ�룬���򷵻�һ��NULL
    */    
    Task* GetTask();

    /* pur @ ����������񵽿��ж�����
     * para @ task �ѱ�����ִ�е�����
     * return @ 
    */
    void SaveIdleTask(Task*task);
	
    void StopPool();
public:
    void LockIdle();
    void UnlockIdle();
    void CheckIdleWait();
    int RemoveIdleTask();
    bool GetStop();
private:
    static void * CheckIdleTask(void *);
    /* pur @ ��ȡ���е�task
     * para @ 
     * para @ 
     * return @ NULL˵��û�п��еģ������m_idleList�л�ȡһ��
    */
    Task* GetIdleTask();
    int GetTaskSize();
private:
    int m_poolSize; //����ش�С
    int m_taskListSize; // ͳ��taskList�Ĵ�С����Ϊ��List�Ĵ�С�������������������ʱ����
    bool m_bStop; //�Ƿ�ֹͣ
    std::list<Task*> m_taskList;//���д����������б�
    std::list<Task*> m_idleList;//���п��������б�
    pthread_mutex_t m_lock; //�������б���м�������֤ÿ��ֻ��ȡһ������
    pthread_mutex_t m_idleMutex; //�������������
    pthread_cond_t m_idleCond; //���ж��еȴ�����
    pthread_t m_idleId;;
};
#endif
