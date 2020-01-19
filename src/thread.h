#ifndef THREAD_H_
#define THREAD_H_

#include "def.h"

// macro wrappers
static INLINE int return_0(int ret){
	return 0;
}
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

#define thread_create		__thread_create
#define thread_join		__thread_join
#define thread_detach		__thread_detach
#define thread_yield()		return_void(SwitchToThread())

#define mutex_init(m)		return_0((InitializeCriticalSection(m), 0))
#define mutex_destroy		DeleteCriticalSection
#define mutex_lock		EnterCriticalSection
#define mutex_unlock		LeaveCriticalSection

#define condvar_init(c)		return_0((InitializeConditionVariable(c), 0))
#define condvar_destroy(...)	return_void(0)
#define condvar_wait(c,m)	return_0((SleepConditionVariableCS((c), (m), INFINITE), 0))
#define condvar_timedwait(c,m,t) return_0((SleepConditionVariableCS((c), (m), (DWORD)(t)), 0))
#define condvar_signal		WakeConditionVariable
#define condvar_broadcast	WakeAllConditionVariable

#else

#include <pthread.h>

typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t condvar_t;

int __condvar_timedwait(convar_t *c, mutex_t *m, long msec);

#define thread_create(a,b,c)	pthread_create((a), NULL, (b), (c))
#define thread_join		pthread_join
#define thread_detach(a)	return_void(pthread_detach(a))
#define thread_yield()		return_void(sched_yield())

#define mutex_init(a)		pthread_mutex_init((a), NULL)
#define mutex_destroy(a)	return_void(pthread_mutex_destroy(a))
#define mutex_lock(a)		return_void(pthread_mutex_lock(a))
#define mutex_unlock(a)		return_void(pthread_mutex_unlock(a))

#define condvar_init(a)		pthread_cond_init((a), NULL)
#define condvar_destroy(a)	return_void(pthread_cond_destroy(a))
#define condvar_wait		pthread_cond_wait
#define condvar_timedwait	__condvar_timedwait
#define condvar_signal(a)	return_void(pthread_cond_signal(a))
#define condvar_broadcast(a)	return_void(pthread_cond_broadcast(a))

#endif

#endif //THREAD_H_
