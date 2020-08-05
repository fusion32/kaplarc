#ifndef KAPLAR_THREAD_H_
#define KAPLAR_THREAD_H_ 1

#include "common.h"

// macro wrappers
static INLINE void return_void(int ret){
	return;
}

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <process.h>
typedef struct thread{
	HANDLE handle;
	void *(*fp)(void*);
	void *arg;
} thread_t;
typedef CRITICAL_SECTION mutex_t;
typedef CONDITION_VARIABLE condvar_t;

int __thread_create(thread_t *thr, void *(*fp)(void*), void *arg);
int __thread_join(thread_t *thr, void **ret);
void __thread_detach(thread_t *thr);

#define thread_create(thr,fp,arg) __thread_create((thr), (fp), (arg))
#define thread_join(thr,ret)	__thread_join((thr), (ret))
#define thread_detach(thr)	__thread_detach(thr)
#define thread_yield()		return_void(SwitchToThread())

#define mutex_init(m)		InitializeCriticalSection(m)
#define mutex_destroy(m)	DeleteCriticalSection(m)
#define mutex_lock(m)		EnterCriticalSection(m)
#define mutex_unlock(m)		LeaveCriticalSection(m)

#define condvar_init(c)		InitializeConditionVariable(c)
#define condvar_destroy(c)	((void)0)
#define condvar_wait(c,m)	ASSERT(SleepConditionVariableCS((c), (m), INFINITE) == TRUE)
#define condvar_timedwait(c,m,t) ASSERT(SleepConditionVariableCS((c), (m), (DWORD)(t)) == TRUE)
#define condvar_signal(c)	WakeConditionVariable(c)
#define condvar_broadcast(c)	WakeAllConditionVariable(c)

#else

#include <pthread.h>

typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t condvar_t;

int __condvar_timedwait(convar_t *c, mutex_t *m, long msec);

#define thread_create(thr,fp,arg) pthread_create((thr), NULL, (fp), (arg))
#define thread_join(thr,ret)	pthread_join((thr),(ret))
#define thread_detach(thr)	pthread_detach(thr)
#define thread_yield()		return_void(sched_yield())

#define mutex_init(a)		ASSERT(pthread_mutex_init((a), NULL) == 0)
#define mutex_destroy(a)	ASSERT(pthread_mutex_destroy(a) == 0)
#define mutex_lock(a)		ASSERT(pthread_mutex_lock(a) == 0)
#define mutex_unlock(a)		ASSERT(pthread_mutex_unlock(a) == 0)

#define condvar_init(a)		ASSERT(pthread_cond_init((a), NULL) == 0)
#define condvar_destroy(a)	ASSERT(pthread_cond_destroy(a) == 0)
#define condvar_wait(c,m)	ASSERT(pthread_cond_wait((c), (m)) == 0)
#define condvar_timedwait(c,m,t) ASSERT(__condvar_timedwait((c), (m), (t)) == 0)
#define condvar_signal(a)	ASSERT(pthread_cond_signal(a) == 0)
#define condvar_broadcast(a)	ASSERT(pthread_cond_broadcast(a) == 0)

#endif

#endif //KAPLAR_THREAD_H_
