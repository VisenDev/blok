#ifndef BLOK_PROFILER_C
#define BLOK_PROFILER_C


#ifdef BLOK_PROFILER_DISABLE
#   define blok_profiler_init(filepath)
#   define blok_profiler_deinit()
#   define blok_profiler_start(name)
#   define blok_profiler_stop(name)
#   define blok_profiler_do(name)
#else

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdbool.h>
#include <assert.h>

static FILE * blok_profiler_output_file = NULL;

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

bool blok_profiler_log(const char * name, bool begin, uint64_t ts, const char * file, const int line) {
    static bool blok_profiler_prepend_comma = false;

    assert(blok_profiler_output_file != NULL);
    if(blok_profiler_prepend_comma) {
        fprintf(blok_profiler_output_file, ",\n");
    } else {
        blok_profiler_prepend_comma = true;
    }
    const char ph = begin ? 'B' : 'E';
    fprintf(blok_profiler_output_file,
            "    { \"name\": \"%s\", \"ph\": \"%c\", \"ts\": %"PRIu64", \"tid\": 1, "
            "\"pid\": 1, \"args\": { \"file\": \"%s\", \"line\": %d } }",
            name, ph, ts, file, line);
    return begin;
}


#define BLOK_CONCAT_(a, b) a##b
#define BLOK_CONCAT(a, b) BLOK_CONCAT_(a, b)

#define blok_profiler_start(name) blok_profiler_log(name, true, blok_profiler_timestamp(), __FILE__, __LINE__)
#define blok_profiler_stop(name) blok_profiler_log(name, false, blok_profiler_timestamp(), __FILE__, __LINE__)
#define blok_profiler_do_internal(name, unique) for(bool unique = blok_profiler_start(name); unique; unique = blok_profiler_stop(name)) 
#define blok_profiler_do(name) blok_profiler_do_internal(name, BLOK_CONCAT(_i, __LINE__))

#endif /*BLOK_PROFILER_DISABLE*/

#endif /*BLOK_PROFILER_C*/

