#include "game.h"


bool game_init(void){
	net_input_bufptr[0] = 0;
	net_input_bufptr[1] = 0;
	return true;
}

void game_shutdown(void){
}

static void game_server_sync_routine(void *arg){
	// 1 - iterate over LOGIN RESOLVES and send it
	/*
	for(resolve in login_resolves){
		protocol_login_send(resolve.connection, resolve.output);
	}
	*/

	// 2 - iterate over PLAYERS and if they have an outbuf, send it
	/* METHOD 1
	{ // OUTSIDE THE SYNC ROUTINE
		struct outbuf *output;
		struct{
			uint32 connection;
			struct outbuf *output;
		} output_list[MAX_PLAYERS];
		int num_output = 0;
		for(player in players){
			output = player.output;
			if(output != NULL){
				output_list[num_output].connection = player.connection;
				output_list[num_output].output = output;
				num_output += 1;
			}
		}
	}

	{ // NOW INSIDE THE SYNC ROUTINE
		for(int i = 0; i < num_output; i += 1){
			connection_send(output_list[i].connection,
				output_list[i].output->data,
				output_list[i].output->datalen);
		}
	}
	*/
	/* METHOD 2
	for(player in players){
		protocol_game_send(player);
	}

	*/

	// 3 - swap the input buffers
	net_idx = 1 - net_idx;
	net_input_bufptr[net_idx] = 0;
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
		game_consume_net_input();

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
