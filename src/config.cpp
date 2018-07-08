#include "config.h"
#include "def.h"
#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

static struct {
	char name[64];
	char value[64];
} table[] = {
	// command line
	{"config",		"config.lua"},

	// server variables
	{"sv_test_port",	"7777"},
	{"sv_login_port",	"7171"},
	{"sv_info_port",	"7171"},
	{"sv_game_port",	"7172"},

	// cassandra variables
	{"cass_contact_points", "localhost"},
	{"cass_keyspace",	"kaplar"},
	{"cass_auth_role",	"cassandra"},
	{"cass_auth_pwd",	"cassandra"},

	// postgres variables
	{"pgsql_host",		"localhost"},
	{"pgsql_port",		"5432"},
	{"pgsql_dbname",	"kaplar"},
	{"pgsql_user",		"admin"},
	{"pgsql_pwd",		"admin"},
};

void config_cmdline(int argc, char **argv){
	int i, j;
	char *arg;
	size_t len;
	bool valid;

	for(j = 1; j < argc; j++){
		valid = false;
		arg = argv[j];
		for(i = 0; i < array_size(table); i++){
			len = strlen(table[i].name);
			if(len <= strlen(arg) && strncmp(table[i].name, arg, len) == 0){
				valid = true;
				arg += len;
				snprintf(table[i].value, 64, "%s",
					((arg[0] != '=' || arg[1] == 0)
						? "true" : arg+1));
				break;
			}
		}

		if(!valid)
			LOG_WARNING("invalid command line option `%s`", arg);
	}
}

bool config_load(void){
	const char *config = config_get("config");
	if(*config == 0)
		return false;
	return config_load(config);
}

bool config_load(const char *path){
	lua_State *L;
	int i;

	L = luaL_newstate();
	if(L == nullptr)
		return false;

	// load file
	if(luaL_loadfile(L, path) != 0){
		LOG_ERROR("config_load: failed to load config `%s`", path);
		lua_close(L);
		return false;
	}

	// run file
	if(lua_pcall(L, 0, 0, 0) != 0){
		LOG_ERROR("config_load: %s", lua_tostring(L, -1));
		lua_close(L);
		return false;
	}

	// parse variables
	for(i = 0; i < array_size(table); i++){
		lua_getglobal(L, table[i].name);
		if(lua_isstring(L, -1) != 0)
			snprintf(table[i].value, 64, "%s", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	lua_close(L);
	return true;
}

bool config_save(const char *path, bool overwrite){
	FILE *f;
	int i;

	// open file
	f = fopen(path, "r+");
	if(f == nullptr){
		f = fopen(path, "w+");
		if(f == nullptr)
			return false;
	}else if(!overwrite) {
		fclose(f);
		return false;
	}

	// write variables
	for(i = 0; i < array_size(table); i++)
		fprintf(f, "%s = \"%s\"\n", table[i].name, table[i].value);
	fclose(f);
	return true;
}

const char *config_get(const char *name){
	int i;
	for(i = 0; i < array_size(table); i++){
		if(strcmp(table[i].name, name) == 0)
			return table[i].value;
	}
	LOG_WARNING("config_get: trying to fetch invalid variable `%s`", name);
	return "";
}

bool config_getb(const char *name){
	const char *value = config_get(name);
	return strcmp(value, "false") != 0;
}

int config_geti(const char *name){
	const char *value = config_get(name);
	return (int)strtol(value, NULL, 10);
}

float config_getf(const char *name){
	const char *value = config_get(name);
	return strtof(value, nullptr);
}

static void config_vset(const char *name, const char *fmt, ...){
	int i;
	bool valid;
	va_list ap;

	valid = false;
	va_start(ap, fmt);
	for(i = 0; i < array_size(table); i++){
		if(strcmp(table[i].name, name) == 0){
			valid = true;
			vsnprintf(table[i].value, 64, fmt, ap);
			break;
		}
	}
	va_end(ap);

	if(!valid) LOG_WARNING("config_vset: trying to set invalid variable `%s`", name);
}

void config_set(const char *name, const char *val){
	if(val == nullptr)
		val = "";
	config_vset(name, "%s", val);
}

void config_setb(const char *name, bool val){
	config_vset(name, "%s", (val ? "true" : "false"));
}

void config_seti(const char *name, int val){
	config_vset(name, "%d", val);
}

void config_setf(const char *name, float val){
	config_vset(name, "%g", val);
}
