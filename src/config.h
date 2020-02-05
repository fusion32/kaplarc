#ifndef CONFIG_H_
#define CONFIG_H_

#include "def.h"

void config_cmdline(int argc, char **argv);
bool config_load(void);
bool config_load_from_path(const char *path);
const char *config_get(const char *name);
bool config_getb(const char *name);
int config_geti(const char *name);
float config_getf(const char *name);
void config_set(const char *name, const char *val);
void config_setb(const char *name, bool val);
void config_seti(const char *name, int val);
void config_setf(const char *name, float val);


#endif //CONFIG_H_
