// @TODO: this should be called something else because
// right now it is managing important systems but not
// managing any gameplay itself

#include "game.h"
#include "ringbuffer.h"
#include "server/server.h"
#include "thread.h"

// double buffered server input
// -----------------------------------------------
#define MAX_SERVER_INPUT_SIZE (4 * 1024 * 1024) // 4MB
static int server_input_write_idx = 0;
// we would suffer from false sharing if we were constantly
// reading and writing on these variables which is not the
// case with `server_input_pos`
static uint32 server_input_pos[2] = {0, 0};
static uint8 server_input_buffer[2][MAX_SERVER_INPUT_SIZE];

/*
static bool add_server_input(uint16 command, uint32 handle, uint8 *data, uint16 datalen){
	uint8 *ptr;
	int idx = server_input_write_idx;
	uint32 pos = server_input_pos[idx];
	uint16 cmd_len = 8 + datalen;
	if((pos + cmd_len) >= MAX_SERVER_INPUT_SIZE){
		LOG_ERROR("add_server_input: server input buffer overflow");
		return false;
	}
	// encode command data
	ptr = &server_input_buffer[idx][pos];
	encode_u16(ptr + 0, cmd_len);
	encode_u16(ptr + 2, command);
	encode_u32(ptr + 4, handle);
	memcpy(ptr + 8, data, datalen);
	// advance buffer pos
	server_input_pos[idx] += cmd_len;
	return true;
}
*/

// double buffered server output
// -----------------------------------------------
#define MAX_SERVER_TASKS 1024
static mutex_t server_tasks_write_mtx;
static int server_tasks_write_idx = 0;
static uint32 server_tasks_count[2] = {0, 0};
static struct task server_tasks[2][MAX_SERVER_TASKS];

static bool add_server_task(void (*fp)(void*), void *arg){
	struct task *task;
	uint32 count;
	int idx;

	mutex_lock(&server_tasks_write_mtx);
	idx = server_tasks_write_idx;
	count = server_tasks_count[idx];
	if(count >= MAX_SERVER_TASKS){
		mutex_unlock(&server_tasks_write_mtx);
		return false;
	}
	task = &server_tasks[idx][count];
	task->fp = fp;
	task->arg = arg;
	server_tasks_count[idx] += 1;
	mutex_unlock(&server_tasks_write_mtx);
	return true;
}

static void run_server_tasks(void){
	struct task *task;
	uint32 count;
	int idx;

	mutex_lock(&server_tasks_write_mtx);
	// get tasks buffer info
	idx = server_tasks_write_idx;
	count = server_tasks_count[idx];
	task = &server_tasks[idx][0];
	// flip double buffer and reset task counter
	server_tasks_write_idx = 1 - idx;
	server_tasks_count[server_tasks_write_idx] = 0;
	mutex_unlock(&server_tasks_write_mtx);

	while(count > 0){
		task->fp(task->arg);
		task += 1;
		count -= 1;
	}
}

//
// -----------------------------------------------

bool game_init(void){
	mutex_init(&server_tasks_write_mtx);
	return true;
}

void game_shutdown(void){
	mutex_destroy(&server_tasks_write_mtx);
}

bool game_add_net_task(void (*fp)(void*), void *arg){
	return add_server_task(fp, arg);
}

static void server_sync_routine(void *arg){
	// DO ANY SYNC BETWEEN SERVER THREAD
}

static void server_maintenance_routine(void *arg){
	// DO ANY WORK ON THE SERVER THREAD
	run_server_tasks();
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
		server_sync(server_sync_routine, NULL,
			server_maintenance_routine, NULL);

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
