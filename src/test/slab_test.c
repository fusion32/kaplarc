#ifdef BUILD_TEST

#include "../def.h"
#include "../log.h"
#include "../slab.h"

#define SLAB_SLOTS 1024
#define SLAB_STRIDE sizeof(struct element)

struct element{
	void *a;
	void *b;
	void *c;
	void *d;
};


static struct slab *slab;
static struct element *elements[SLAB_SLOTS];

bool slab_test(void){
	bool ret = true;

	slab = slab_create(SLAB_SLOTS, SLAB_STRIDE);
	if(slab == NULL){
		LOG_ERROR("slab_test: failed to create slab");
		return false;
	}

	for(int i = 0; i < SLAB_SLOTS; i += 1){
		elements[i] = slab_alloc(slab);
		if(elements[i] == NULL){
			LOG_ERROR("slab_test: allocation failed");
			ret = false;
			break;
		}
	}

	if(ret && slab_alloc(slab) != NULL){
		LOG_ERROR("slab_test: allocating more slots than it should");
		ret = false;
	}

	if(ret){
		for(int i = 0; i < SLAB_SLOTS; i += 1){
			if(!slab_free(slab, elements[i])){
				LOG_ERROR("slab_test: failed to free element");
				ret = false;
				break;
			}
		}
	}
	return slab_is_empty(slab);
}

#endif
