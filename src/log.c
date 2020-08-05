#include "log.h"
#include <stdio.h>
#include <time.h>

#define MAX_FILE_NAME_SIZE 64
static char filename[MAX_FILE_NAME_SIZE];
static FILE *output = NULL;

void log_to_file(void){
	time_t now;
	struct tm *timeptr;
	FILE *newoutput;

	// name example: "Dec-31-1999-235959.log"
	now = time(NULL);
	timeptr = localtime(&now);
	strftime(filename, MAX_FILE_NAME_SIZE, "%b-%d-%Y-%H%M%S.log", timeptr);

	// open file
	newoutput = fopen(filename, "a");
	if(newoutput == NULL){
		LOG_ERROR("log_to_file: failed to open"
			" `filename` for logging", filename);
	}else{
		output = newoutput;
	}

}

void log_to_stdout(void){
	if(output != stdout && output != NULL)
		fclose(output);
	output = stdout;
}

void log_add(const char *tag, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	log_addv(tag, fmt, ap);
	va_end(ap);
}

void log_addv(const char *tag, const char *fmt, va_list ap){
	time_t now;
	struct tm *timeptr;
	char timestr[32];

	if(output == NULL)
		output = stdout;
	now = time(NULL);
	timeptr = localtime(&now);
	strftime(timestr, 32, "%F %T", timeptr);
	fprintf(output, "[%s] %6s | ", timestr, tag);
	vfprintf(output, fmt, ap);
	fprintf(output, "\n");
}

