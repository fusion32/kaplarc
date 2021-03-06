#ifndef KAPLAR_CONFIG_H_
#define KAPLAR_CONFIG_H_ 1

#include "common.h"

void config_init(int argc, char **argv);
bool config_load(void);
bool config_load_from_path(const char *path);
const char *config_get(const char *key);
bool config_getb(const char *key);
int config_geti(const char *key);
float config_getf(const char *key);

#endif //KAPLAR_CONFIG_H_
