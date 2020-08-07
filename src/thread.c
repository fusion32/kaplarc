#include "thread.h"

#ifdef PLATFORM_WINDOWS

// thread
static unsigned int __stdcall thread_wrapper(void *arg){
	thread_t *thr = arg;
	thr->arg = thr->fp(thr->arg);
	return 0;
}
int thread_init(struct thread *thr, void *(*fp)(void *), void *arg){
	thr->fp = fp;
	thr->arg = arg;
	thr->handle = (HANDLE)_beginthreadex(NULL, 0, thread_wrapper, thr, 0, NULL);
	if(thr->handle == NULL)
		return errno;
	return 0;
}
int thread_join(struct thread *thr, void **ret){
	if(WaitForSingleObject(thr->handle, INFINITE) == WAIT_OBJECT_0){
		if(ret != NULL)
			*ret = thr->arg;
		return 0;
	}
	return EINVAL;
}
void thread_detach(struct thread *thr){
	CloseHandle(thr->handle);
}
void thread_yield(void){
	SwitchToThread();
}

// mutex
void mutex_init(mutex_t *mtx){
	InitializeCriticalSection(mtx);
}
void mutex_destroy(mutex_t *mtx){
	DeleteCriticalSection(mtx);
}
void mutex_lock(mutex_t *mtx){
	EnterCriticalSection(mtx);
}
void mutex_unlock(mutex_t *mtx){
	LeaveCriticalSection(mtx);
}

// condvar
void condvar_init(condvar_t *cv){
	InitializeConditionVariable(cv);
}
void condvar_destroy(condvar_t *cv){
	return;
}
void condvar_wait(condvar_t *cv, mutex_t *mtx){
	ASSERT(SleepConditionVariableCS(cv, mtx, INFINITE) == TRUE);
}
void condvar_timedwait(condvar_t *cv, mutex_t *mtx, long msec){
	ASSERT(SleepConditionVariableCS(cv, mtx, (DWORD)msec) == TRUE);
}
void condvar_signal(condvar_t *cv){
	WakeConditionVariable(cv);
}
void condvar_broadcast(condvar_t *cv){
	WakeAllConditionVariable(cv);
}

#else // PLATFORM_WINDOWS

// thread
int thread_init(struct thread *thr, void *(*fp)(void *), void *arg){
	return pthread_create(thr, NULL, fp, arg);
}
int thread_join(struct thread *thr, void **ret){
	return pthread_join(thr, ret);
}
void thread_detach(struct thread *thr){
	pthread_detach(thr);
}
void thread_yield(void){
	sched_yield();
}

// mutex
void mutex_init(mutex_t *mtx){
	ASSERT(pthread_mutex_init(mtx, NULL) == 0);
}
void mutex_destroy(mutex_t *mtx){
	ASSERT(pthread_mutex_destroy(mtx) == 0);
}
void mutex_lock(mutex_t *mtx){
	ASSERT(pthread_mutex_lock(mtx) == 0);
}
void mutex_unlock(mutex_t *mtx){
	ASSERT(pthread_mutex_unlock(mtx) == 0);
}

// condvar
void condvar_init(condvar_t *cv){
	ASSERT(pthread_cond_init(cv, NULL) == 0);
}
void condvar_destroy(condvar_t *cv){
	ASSERT(pthread_cond_destroy(cv) == 0);
}
void condvar_wait(condvar_t *cv, mutex_t *mtx){
	ASSERT(pthread_cond_wait(cv, mtx) == 0);
}
void condvar_timedwait(condvar_t *cv, mutex_t *mtx, long msec){
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += (msec / 1000);
	ts.tv_nsec += (msec % 1000) * 1000000;
	ASSERT(pthread_cond_timedwait(c, m, &ts) == 0);
}
void condvar_signal(condvar_t *cv){
	ASSERT(pthread_cond_signal(cv) == 0);
}
void condvar_broadcast(condvar_t *cv){
	ASSERT(pthread_cond_broadcast(cv) == 0);
}

#endif // PLATFORM_WINDOWS
