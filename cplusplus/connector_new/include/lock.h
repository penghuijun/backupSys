#ifndef __LOCK_H__
#define __LOCK_H__
#include <pthread.h>

class read_write_lock
{
public:
	read_write_lock(){}
	int init()
	{
		return pthread_rwlock_init(&m_rw_lock, NULL);
	}
	int read_lock()
	{
		return pthread_rwlock_rdlock(&m_rw_lock);
	}
	int write_lock()
	{
		return pthread_rwlock_wrlock(&m_rw_lock);
	}
	int read_trylock()
	{
		return pthread_rwlock_tryrdlock(&m_rw_lock);
	}
	int write_trylock()
	{
		return pthread_rwlock_trywrlock(&m_rw_lock);
	}

	
	int read_write_unlock()
	{
		return pthread_rwlock_unlock(&m_rw_lock);
	}
	int destroy()
	{
		return pthread_rwlock_destroy(&m_rw_lock);
	}
	~read_write_lock(){destroy();}
private:
	pthread_rwlock_t m_rw_lock;	
};

class mutex_lock
{
public:
	mutex_lock(){}
	int init()
	{
		return pthread_mutex_init(&m_mutex_lock, NULL);
	}
	int lock()
	{
		return pthread_mutex_lock(&m_mutex_lock);
	}
	int trylock()
	{

		return pthread_mutex_trylock(&m_mutex_lock);
	}

	int unlock()
	{
		return pthread_mutex_unlock(&m_mutex_lock);	
	}
	int destroy()
	{
		return pthread_mutex_destroy(&m_mutex_lock);
	}

	void operator=(pthread_mutex_t &mutex_init)
	{
		m_mutex_lock = mutex_init;
	}
	~mutex_lock()
	{
		destroy();
	}
private:
	pthread_mutex_t m_mutex_lock;	
};

#endif
