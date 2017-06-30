#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "cstring.h"
#include "log.h"

#define MAX_FILE_NAME_SIZE 64
static char	filename[MAX_FILE_NAME_SIZE];
static FILE	*file = NULL;
static bool	saving = false;

bool log_start()
{
	time_t curTime;
	struct tm *timeptr;

	if(!saving){
		// name example: "Jan-01-1999-133700.log"
		curTime = time(NULL);
		timeptr = localtime(&curTime);
		strftime(filename, MAX_FILE_NAME_SIZE, "%b-%d-%Y-%H%M%S.log", timeptr);

		// open file
		file = fopen(filename, "a");
		saving = file != nullptr;
	}
	return saving;
}

void log_stop()
{
	if(!saving)
		return;

	saving = false;
	fclose(file);
	file = NULL;
}

void log_add(const char *tag, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	log_add1(tag, fmt, ap);
	va_end(ap);
}

void log_add1(const char *tag, const char *fmt, va_list ap)
{
	time_t curtime;
	struct tm *timeptr;
	char timestr[64];
	CString<256> log_entry;

	// time str
	curtime = time(NULL);
	timeptr = localtime(&curtime);
	// format example: "Jan 01 1999 13:37:00"
	strftime(timestr, 64, "%b %d %Y %H:%M:%S", timeptr);

	// concatenate log entry
	log_entry.format("[%s] %8s | ", timestr, tag);
	log_entry.vformat_append(fmt, ap);
	log_entry.append("\n");
	if(log_entry.size() <= 0){
		printf("<ERROR> Failed to concatenate log entry (tag = %s)!\n", tag);
		return;
	}

	// output to console
	printf(log_entry);

	// if saving, output to log file
	if(saving){
		if(fwrite(log_entry, 1, log_entry.size(), file) != 1)
			printf("<ERROR> Failed to write log entry to file! (filename = %s)\n", filename);
	}
}

