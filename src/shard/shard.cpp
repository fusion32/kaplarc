struct Shard{
	void *connection;	// connection handle
	void *addr;		// transfer addr
	int port;		// transfer port
	int flags;		// shard flags
	int *zones;		// zones owned
	int numzones;		// zones count
};
/*
// shard functions
void	*shard_self(void);
bool	shard_owns_zone(int zone);
void	*shard_get_by_zone(int zone);
void	shard_init_transfer(void *toshard, void *player);

int	player_getzone(void *player);

void shard_transfer(void *player, int tozone){
	void *toshard;
	int fromzone = player_getzone(player);
	if(fromzone == tozone){
		// abort transfer: same zone
		return;
	}
	if(shard_owns_zone(tozone)){
		// abort transfer: same shard
		return;
	}
	toshard = shard_get_by_zone(tozone);
	shard_init_transfer(toshard, player);
}
*/
