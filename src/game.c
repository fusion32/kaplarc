#include "game.h"
#include "buffer_util.h"
#include "server/server.h"
#include "task_dbuffer.h"
#include "thread.h"

// @TODO: If needed use the frame allocator to send additional data
// with `game_add_task`. This data will be valid when the task is run
// but will be invalidated after a couple frames.
// (make a lifetime of at least 2 frames for server tasks as well?)
//static struct mem_arena *frame_allocator[2]; = NULL;

#define MAX_GAME_TASKS 1024
static struct task_dbuffer *game_tasks = NULL;

#define MAX_SERVER_TASKS 1024
static struct task_dbuffer *server_tasks = NULL;


bool game_add_task(void (*fp)(void*), void *arg){
	return task_dbuffer_add(game_tasks, fp, arg);
}

bool game_add_server_task(void (*fp)(void*), void *arg){
	return task_dbuffer_add(server_tasks, fp, arg);
}

bool game_init(void){
	game_tasks = task_dbuffer_create(MAX_GAME_TASKS);
	server_tasks = task_dbuffer_create(MAX_SERVER_TASKS);
	return true;
}

void game_shutdown(void){
	task_dbuffer_destroy(server_tasks);
	task_dbuffer_destroy(game_tasks);
}

static void server_maintenance_routine(void *arg){
	// DO ANY WORK ON THE SERVER THREAD
	task_dbuffer_swap_and_run(server_tasks);
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
		server_exec(server_maintenance_routine, NULL);
		task_dbuffer_swap_and_run(game_tasks);

		// stall until the next frame if we finished too early
		frame_end = kpl_clock_monotonic_msec();
		if(frame_end < next_frame)
			kpl_sleep_msec(next_frame - frame_end);

#ifdef BUILD_DEBUG
		// calc frame statistics
		#define GAME_MAX_FPS (1000/GAME_FRAME_INTERVAL + 1)
		static int64 frame_user_time[GAME_MAX_FPS];
		static int64 frame_idle_time[GAME_MAX_FPS];
		static int num_frames = 0;
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
