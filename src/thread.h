#ifndef KAPLAR_THREAD_H_
#define KAPLAR_THREAD_H_ 1

#include "common.h"

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
#else // PLATFORM_LINUX || PLATFORM_FREEBSD
#include <pthread.h>
typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t condvar_t;
#endif

int thread_init(thread_t *thr, void *(*fp)(void*), void *arg);
int thread_join(thread_t *thr, void **ret);
void thread_detach(thread_t *thr);
void thread_yield(void);

void mutex_init(mutex_t *mtx);
void mutex_destroy(mutex_t *mtx);
void mutex_lock(mutex_t *mtx);
void mutex_unlock(mutex_t *mtx);

void condvar_init(condvar_t *cv);
void condvar_destroy(condvar_t *cv);
void condvar_wait(condvar_t *cv, mutex_t *mtx);
void condvar_timedwait(condvar_t *cv, mutex_t *mtx, long msec);
void condvar_signal(condvar_t *cv);
void condvar_broadcast(condvar_t *cv);

#endif //KAPLAR_THREAD_H_
