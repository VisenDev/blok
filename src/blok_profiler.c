#ifndef BLOK_PROFILER_C
#define BLOK_PROFILER_C


#ifdef BLOK_PROFILER_DISABLE
#   define blok_profiler_init(filepath)
#   define blok_profiler_deinit()
#   define blok_profiler_start(name)
#   define blok_profiler_stop(name)
#   define blok_profile(name)
#else

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdbool.h>



static FILE * blok_profiler_output_file = NULL;
static bool blok_profiler_prepend_comma = false;

uint64_t blok_profiler_timestamp(void) {
    struct timeval currentTime = {0};
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * 1000000 + currentTime.tv_usec;
}

void blok_profiler_init(const char * output_file_path) {
    blok_profiler_output_file = fopen(output_file_path, "w");
    fprintf(blok_profiler_output_file, "[\n");
}


void blok_profiler_deinit(void) {
    fprintf(blok_profiler_output_file, "\n]\n");
    fclose(blok_profiler_output_file);
}

void blok_profiler_log(const char * name, const char ph, uint64_t ts) {
    if(blok_profiler_prepend_comma) {
        fprintf(blok_profiler_output_file, ",\n");
    } else {
        blok_profiler_prepend_comma = true;
    }
    fprintf(blok_profiler_output_file, "    { \"name\": \"%s\", \"ph\": \"%c\", \"ts\": %llu, \"tid\": 1, \"pid\": 1 }", name, ph, ts);
}

bool blok_profiler_start(const char * name) {
    blok_profiler_log(name, 'B', blok_profiler_timestamp());
    return true;
}

bool blok_profiler_stop(const char * name) {
    blok_profiler_log(name, 'E', blok_profiler_timestamp());
    return false;
}

#define blok_profile(name) for(bool name##_i= blok_profiler_start(#name); name##_i; name##_i= blok_profiler_stop(#name)) 

#endif /*BLOK_PROFILER_DISABLE*/

#endif /*BLOK_PROFILER_C*/

