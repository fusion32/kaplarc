#if 0
#include "protocol.h"
#include "buffer_util.h"
#include "hash.h"
#include "mem.h"
#include "server.h"
#include "packed_data.h"
#include "tibia_rsa.h"
#include "crypto/xtea.h"

/* PROTOCOL HANDLE */
#define GAME_HANDLE_SIZE sizeof(struct game_handle)
#define MAX_KNOWN_CREATURES 150
#define OUTPUT_BUFFER_SIZE 16384
struct game_handle{
	uint32 connection;
	uint32 player;

	// client known creatures
	uint32 known_creatures;
	uint32 known_creatures_list[MAX_KNOWN_CREATURES];

	// output double buffering
	//uint32 current_output_idx;
	//uint8 output_buffers[2][OUTPUT_BUFFER_SIZE];
};

/* PROTOCOL DECL */
static bool identify(uint8 *data, uint32 datalen);
static bool create_handle(uint32 c, void **handle);
static void destroy_handle(uint32 c, void *handle);
static void on_close(uint32 c, void *handle);
static protocol_status_t on_connect(uint32 c, void *handle);
static protocol_status_t on_write(uint32 c, void *handle);
static protocol_status_t on_recv_message(uint32 c,
	void *handle, uint8 *data, uint32 datalen);
static protocol_status_t on_recv_first_message(uint32 c,
	void *handle, uint8 *data, uint32 datalen);
struct protocol protocol_game = {
	.name = "game",
	.sends_first = true,
	.identify = identify,

	.create_handle = create_handle,
	.destroy_handle = destroy_handle,
	.on_close = on_close,
	.on_connect = on_connect,
	.on_write = on_write,
	.on_recv_message = on_recv_message,
	.on_recv_first_message = on_recv_first_message,
};
#endif

/*
class ProtocolGame : public Protocol
{
private:
  std::list<uint32_t> knownCreatureList;
  Player* player;
  uint32_t eventConnect;
  bool m_debugAssertSent;
  bool m_acceptPackets;
};
*/
