#include "log.h"
#include "def.h"
#include "string.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define MAX_FILE_NAME_SIZE 64
static char	filename[MAX_FILE_NAME_SIZE];
static FILE	*file = NULL;

bool log_start(void){
	time_t curTime;
	struct tm *timeptr;

	if(file == NULL){
		// name example: "Dec-31-1999-235959.log"
		curTime = time(NULL);
		timeptr = localtime(&curTime);
		strftime(filename, MAX_FILE_NAME_SIZE, "%b-%d-%Y-%H%M%S.log", timeptr);

		// open file
		file = fopen(filename, "a");
	}
	return (file != NULL);
}

void log_stop(void){
	if(file == NULL)
		return;

	fclose(file);
	file = NULL;
}

void log_add(const char *tag, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	log_add1(tag, fmt, ap);
	va_end(ap);
}

void log_add1(const char *tag, const char *fmt, va_list ap){
	time_t curtime;
	struct tm *timeptr;
	char timestr[64];
	char buffer[1024];
	struct string_ptr entry = {ARRAY_SIZE(buffer), 0, buffer};

	// time str
	curtime = time(NULL);
	timeptr = localtime(&curtime);
	// format example: "Dec 31 1999 23:59:59"
	strftime(timestr, 64, "%b %d %Y %H:%M:%S", timeptr);

	// concatenate log entry
	str_format(&entry, "[%s] %8s | ", timestr, tag);
	str_vappend(&entry, fmt, ap);
	str_append(&entry, "\n");
	if(entry.len <= 0){
		printf("<ERROR> Failed to concatenate log entry (tag = %s)!\n", tag);
		return;
	}

	// output to console
	printf("%s", entry.data);

	// if saving, output to log file
	if(file != NULL && fwrite(entry.data, 1, entry.len, file) != 1)
		printf("<ERROR> Failed to write log entry to file! (filename = %s)\n", filename);
}

