#include "env.h"

static uint64	flags;
static int	lock[64];

void env_set(int idx){
	flags |= (1ULL << idx);
}

bool env_reset(int idx){
	if(lock[idx] > 0)
		return false;
	flags &= ~(1ULL << idx);
	return true;
}

bool env_lock(int *idx, int n){
	// check all bits are set
	int k;
	for(k = 0; k < n; ++k){
		if((flags & (1ULL << idx[k])) == 0)
			return false;
	}

	// lock requested bits
	for(k = 0; k < n; ++k)
		lock[idx[k]] += 1;
	return true;
}

void env_unlock(int *idx, int n){
	for(int k = 0; k < n; ++k)
		lock[idx[k]] -= 1;
}
