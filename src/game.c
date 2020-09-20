// @TODO: this should be called something else because
// right now it is managing important systems but not
// managing any gameplay itself

#include "game.h"
#include "ringbuffer.h"
#include "server/server.h"
#include "thread.h"

static mutex_t nettasks_mtx;
DECL_TASK_RINGBUFFER(nettasks, 1024)


bool game_init(void){
	mutex_init(&nettasks_mtx);
	return true;
}

void game_shutdown(void){
	mutex_destroy(&nettasks_mtx);
}

bool game_add_net_task(void (*fp)(void*), void *arg){
	struct task *task;
	mutex_lock(&nettasks_mtx);
	if(RINGBUFFER_FULL(nettasks)){
		mutex_unlock(&nettasks_mtx);
		return false;
	}
	task = RINGBUFFER_UNCHECKED_PUSH(nettasks);
	task->fp = fp;
	task->arg = arg;
	mutex_unlock(&nettasks_mtx);
	return true;
}

static void game_server_sync_routine(void *arg){
	// TODO: this might cause unwanted contention but will work for now (IMPORTANT)
	struct task *task;
	mutex_lock(&nettasks_mtx);
	while(!RINGBUFFER_EMPTY(nettasks)){
		task = RINGBUFFER_UNCHECKED_POP(nettasks);
		task->fp(task->arg);
	}
	mutex_unlock(&nettasks_mtx);

	// @TODO: handle player i/o separately
}

// frame interval in milliseconds:
//	16 is ~60fps
//	33 is ~30fps
//	66 is ~15fps
#define GAME_FRAME_INTERVAL 33

#ifdef BUILD_DEBUG
static void calc_stats(int64 *data, int datalen,
		int64 *out_avg, int64 *out_min, int64 *out_max){
	DEBUG_ASSERT(datalen > 0);
	int64 avg, min, max;
	avg = min = max = data[0];
	for(int i = 1; i < datalen; i += 1){
		if(data[i] < min)
			min = data[i];
		else if(data[i] > max)
			max = data[i];
		avg += data[i];
	}
	*out_avg = avg / datalen;
	*out_min = min;
	*out_max = max;
}
#endif

void game_run(void){
	int64 frame_start;
	int64 frame_end;
	int64 next_frame;
	while(1){
		// calculate frame times so each frame takes the
		// same time to complete (in a perfect scenario)
		frame_start = kpl_clock_monotonic_msec();
		next_frame = frame_start + GAME_FRAME_INTERVAL;

		// do work
		server_sync(game_server_sync_routine, NULL);

		// stall until the next frame if we finished too early
		frame_end = kpl_clock_monotonic_msec();
		if(frame_end < next_frame)
			kpl_sleep_msec(next_frame - frame_end);

#ifdef BUILD_DEBUG
		// calc frame statistics
		#define GAME_MAX_FPS (1000/GAME_FRAME_INTERVAL + 1)
		static int64 frame_user_time[GAME_MAX_FPS];
		static int64 frame_idle_time[GAME_MAX_FPS];
		static int num_frames;
		if(num_frames >= GAME_MAX_FPS){
			int64 avg, min, max;
			DEBUG_LOG("frame stats over %d frames", num_frames);
			calc_stats(frame_user_time, num_frames, &avg, &min, &max);
			DEBUG_LOG("    user time: (avg = %lld, min = %lld, max = %lld)", avg, min, max);
			calc_stats(frame_idle_time, num_frames, &avg, &min, &max);
			DEBUG_LOG("    idle time: (avg = %lld, min = %lld, max = %lld)", avg, min, max);
			num_frames = 0;
		}
		frame_user_time[num_frames] = frame_end - frame_start;
		frame_idle_time[num_frames] = next_frame - frame_end;
		num_frames += 1;
#endif
	}
}
