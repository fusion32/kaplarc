#include "log.h"
#include "sstring.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define MAX_FILE_NAME_SIZE 64
static char	filename[MAX_FILE_NAME_SIZE];
static FILE	*file = nullptr;

bool log_start(void){
	time_t curTime;
	struct tm *timeptr;

	if(file == nullptr){
		// name example: "Dec-31-1999-235959.log"
		curTime = time(nullptr);
		timeptr = localtime(&curTime);
		strftime(filename, MAX_FILE_NAME_SIZE, "%b-%d-%Y-%H%M%S.log", timeptr);

		// open file
		file = fopen(filename, "a");
	}
	return (file != nullptr);
}

void log_stop(void){
	if(file == nullptr)
		return;

	fclose(file);
	file = nullptr;
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
	SString<1024> log_entry;

	// time str
	curtime = time(nullptr);
	timeptr = localtime(&curtime);
	// format example: "Dec 31 1999 23:59:59"
	strftime(timestr, 64, "%b %d %Y %H:%M:%S", timeptr);

	// concatenate log entry
	log_entry.format("[%s] %8s | ", timestr, tag);
	log_entry.vappendf(fmt, ap);
	log_entry.append("\n");
	if(log_entry.size() <= 0){
		printf("<ERROR> Failed to concatenate log entry (tag = %s)!\n", tag);
		return;
	}

	// output to console
	printf("%s", log_entry.c_str());

	// if saving, output to log file
	if(file != nullptr && fwrite(log_entry, 1, log_entry.size(), file) != 1)
		printf("<ERROR> Failed to write log entry to file! (filename = %s)\n", filename);
}

