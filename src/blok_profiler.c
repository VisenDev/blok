#ifndef BLOK_PROFILER_C
#define BLOK_PROFILER_C

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

static inline int blok_profile_interal(FILE * output, const char * name, bool active) {
    static bool first = true;
    if(first) {
        first = false;
    } else {
        fprintf(output, ",\n");
    }
    fprintf(output, "{ \"name\": %s, \"ph\": \"%s\", \"ts\": %llu }", name, active ? "B" : "E", blok_profiler_timestamp());
    return 0;
}

#define blok_profile(name) \
    for(int i = blok_profile_internal(; i < 1; ++i) 

#endif /*BLOK_PROFILER_C*/

