#include "game.h"
#include "buffer_util.h"
#include "ringbuffer.h"
#include "server/server.h"
#include "thread.h"


// @TODO: maybe use another approach so the database thread
// could send tasks to the game thread
//bool game_add_task(void (*fp)(void*), void *arg,
//		uint8 *local_data, uint16 local_datalen);

// double buffered server cmd buffer (SERVER -> GAME)
// -----------------------------------------------
#define MAX_SERVER_CMD_BUFFER_SIZE (8 * 1024 * 1024) // 8MB
static int server_cmd_write_idx = 0;
// we would suffer from false sharing if we were constantly
// reading and writing on these variables which is not the
// case with `server_cmd_buffer_pos`
static uint32 server_cmd_pos[2] = {0, 0};
static uint8 server_cmd_buffer[2][MAX_SERVER_CMD_BUFFER_SIZE];

bool game_add_server_cmd(uint16 command, uint32 handle, uint8 *data, uint16 datalen){
	uint8 *ptr;
	int idx = server_cmd_write_idx;
	uint32 pos = server_cmd_pos[idx];
	uint16 cmd_len = 8 + datalen;
	if((pos + cmd_len) >= MAX_SERVER_CMD_BUFFER_SIZE){
		LOG_ERROR("add_server_input: server input buffer overflow");
		return false;
	}
	// encode command data
	ptr = &server_cmd_buffer[idx][pos];
	encode_u16(ptr + 0, cmd_len);
	encode_u16(ptr + 2, command);
	encode_u32(ptr + 4, handle);
	memcpy(ptr + 8, data, datalen);
	// advance buffer pos
	server_cmd_pos[idx] += cmd_len;
	return true;
}

static void swap_server_cmd_buffers(void){
	server_cmd_write_idx = 1 - server_cmd_write_idx;
	server_cmd_pos[server_cmd_write_idx] = 0;
}

static void consume_server_commands(void){
	int idx = 1 - server_cmd_write_idx;
	uint32 len = server_cmd_pos[idx];
	uint8 *buffer = &server_cmd_buffer[idx][0];

	uint16 cmd_len;
	uint16 cmd_ident;
	uint32 cmd_handle;
	uint8 *cmd_data;
	while(len >= 8){ // commands are at least 8 bytes
		cmd_len = decode_u16(buffer + 0);
		DEBUG_ASSERT(cmd_len <= len && cmd_len >= 8);
		cmd_ident = decode_u16(buffer + 2);
		cmd_handle = decode_u32(buffer + 4);
		cmd_data = buffer + 8;

		switch(cmd_ident){
		case SV_CMD_PLAYER_LOGOUT:
		case SV_CMD_PLAYER_KEEP_ALIVE:
		case SV_CMD_PLAYER_AUTO_WALK:
		case SV_CMD_PLAYER_MOVE_NORTH:
		case SV_CMD_PLAYER_MOVE_EAST:
		case SV_CMD_PLAYER_MOVE_SOUTH:
		case SV_CMD_PLAYER_MOVE_WEST:
		case SV_CMD_PLAYER_STOP_AUTO_WALK:
		case SV_CMD_PLAYER_MOVE_NORTHEAST:
		case SV_CMD_PLAYER_MOVE_SOUTHEAST:
		case SV_CMD_PLAYER_MOVE_SOUTHWEST:
		case SV_CMD_PLAYER_MOVE_NORTHWEST:
		case SV_CMD_PLAYER_TURN_NORTH:
		case SV_CMD_PLAYER_TURN_EAST:
		case SV_CMD_PLAYER_TURN_SOUTH:
		case SV_CMD_PLAYER_TURN_WEST:
		case SV_CMD_PLAYER_ITEM_THROW:
		case SV_CMD_PLAYER_SHOP_LOOK:
		case SV_CMD_PLAYER_SHOP_BUY:
		case SV_CMD_PLAYER_SHOP_SELL:
		case SV_CMD_PLAYER_SHOP_CLOSE:
		case SV_CMD_PLAYER_TRADE_REQUEST:
		case SV_CMD_PLAYER_TRADE_LOOK:
		case SV_CMD_PLAYER_TRADE_ACCEPT:
		case SV_CMD_PLAYER_TRADE_CLOSE:
		case SV_CMD_PLAYER_ITEM_USE:
		case SV_CMD_PLAYER_ITEM_USE_EX:
		case SV_CMD_PLAYER_BATTLE_WINDOW:
		case SV_CMD_PLAYER_ITEM_ROTATE:
		case SV_CMD_PLAYER_CONTAINER_CLOSE:
		case SV_CMD_PLAYER_CONTAINER_PARENT:
		case SV_CMD_PLAYER_ITEM_WRITE:
		case SV_CMD_PLAYER_HOUSE_WRITE:
		case SV_CMD_PLAYER_LOOK:
		case SV_CMD_PLAYER_SAY:
		case SV_CMD_PLAYER_CHANNELS_REQUEST:
		case SV_CMD_PLAYER_CHANNEL_OPEN:
		case SV_CMD_PLAYER_CHANNEL_CLOSE:
		case SV_CMD_PLAYER_PVT_CHAT_OPEN:
		case SV_CMD_PLAYER_NPC_CHAT_CLOSE:
		case SV_CMD_PLAYER_BATTLE_MODE:
		case SV_CMD_PLAYER_ATTACK:
		case SV_CMD_PLAYER_FOLLOW:
		case SV_CMD_PLAYER_PARTY_INVITE:
		case SV_CMD_PLAYER_PARTY_JOIN:
		case SV_CMD_PLAYER_PARTY_INVITE_REVOKE:
		case SV_CMD_PLAYER_PARTY_LEADERSHIP_PASS:
		case SV_CMD_PLAYER_PARTY_LEAVE:
		case SV_CMD_PLAYER_PARTY_SHARED_EXP:
		case SV_CMD_PLAYER_CHANNEL_CREATE:
		case SV_CMD_PLAYER_CHANNEL_INVITE:
		case SV_CMD_PLAYER_CHANNEL_KICK:
		case SV_CMD_PLAYER_HOLD:
		case SV_CMD_PLAYER_TILE_UPDATE_REQUEST:
		case SV_CMD_PLAYER_CONTAINER_UPDATE_REQUEST:
		case SV_CMD_PLAYER_OUTFITS_REQUEST:
		case SV_CMD_PLAYER_OUTFIT_SET:
		//case SV_CMD_PLAYER_MOUNT:
		case SV_CMD_PLAYER_VIP_ADD:
		case SV_CMD_PLAYER_VIP_REMOVE:
		//case SV_CMD_PLAYER_BUG_REPORT:
		//case SV_CMD_PLAYER_VIOLATION_WINDOW:
		//case SV_CMD_PLAYER_DEBUG_ASSERT:
		case SV_CMD_PLAYER_QUESTLOG_REQUEST:
		case SV_CMD_PLAYER_QUESTLINE_REQUEST:
		//case SV_CMD_PLAYER_VIOLATION_REPORT:
		default:
			break;
		}

		DEBUG_LOG("server command: len = %u, command = %u, handle = %u",
			(unsigned)cmd_len, (unsigned)cmd_ident, (unsigned)cmd_handle);
		buffer += cmd_len;
		len -= cmd_len;
	}
}

// double buffered server tasks
// -----------------------------------------------
#define MAX_SERVER_TASKS 1024
static mutex_t server_tasks_write_mtx;
static condvar_t server_tasks_full;
static int server_tasks_write_idx = 0;
static uint32 server_tasks_count[2] = {0, 0};
static struct task server_tasks[2][MAX_SERVER_TASKS];

bool game_add_server_task(void (*fp)(void*), void *arg){
	struct task *task;
	uint32 count;
	int idx;

	mutex_lock(&server_tasks_write_mtx);
	idx = server_tasks_write_idx;
	count = server_tasks_count[idx];
	if(count >= MAX_SERVER_TASKS){
		condvar_wait(&server_tasks_full, &server_tasks_write_mtx);
		count = server_tasks_count[idx];
		if(count >= MAX_SERVER_TASKS){
			mutex_unlock(&server_tasks_write_mtx);
			return false;
		}
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
	// signal any thread that might be waiting for
	// server tasks to have room
	if(count >= MAX_SERVER_TASKS)
		condvar_broadcast(&server_tasks_full);
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
	condvar_init(&server_tasks_full);
	return true;
}

void game_shutdown(void){
	condvar_destroy(&server_tasks_full);
	mutex_destroy(&server_tasks_write_mtx);
}

static void server_sync_routine(void *arg){
	// DO ANY SYNC BETWEEN SERVER THREAD AND GAME THREAD
	swap_server_cmd_buffers();
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
		consume_server_commands();

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
