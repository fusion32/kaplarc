#include "thread.h"

#ifdef PLATFORM_WINDOWS

static unsigned int __stdcall thread_wrapper(void *arg){
	thread_t *thr = arg;
	thr->arg = thr->fp(thr->arg);
	return 0;
}
int __thread_create(struct thread *thr, void *(*fp)(void *), void *arg){
	thr->fp = fp;
	thr->arg = arg;
	thr->handle = (HANDLE)_beginthreadex(NULL, 0, thread_wrapper, thr, 0, NULL);
	if(thr->handle == NULL)
		return errno;
	return 0;
}
int __thread_join(struct thread *thr, void **ret){
	if(WaitForSingleObject(thr->handle, INFINITE) == WAIT_OBJECT_0){
		if(ret != NULL)
			*ret = thr->arg;
		return 0;
	}
	return EINVAL;
}
void __thread_detach(struct thread *thr){
	CloseHandle(thr->handle);
}

#else // PLATFORM_WINDOWS

int __condvar_timedwait(convar_t *c, mutex_t *m, long msec){
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += (msec / 1000);
	ts.tv_nsec += (msec % 1000) * 1000000;
	return pthread_cond_timedwait(c, m, &ts);
}

#endif //PLATFORM_WINDOWS
