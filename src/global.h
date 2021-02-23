#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>
#include <stdio.h>

#define PI 3.1415927f

typedef uint8_t Color[3];
typedef struct Line Line;

typedef struct Line {
	float vector[3];
	float position[3];
} Line;

#define err_assert(expr, err_code)\
	do { \
		if(! (expr)) {\
			printf(ERR_FORMAT_STRING, ERR_MESSAGES[err_code]);\
			exit(err_code);\
		}\
	} while(0)

enum errcode{
	ERR_ARGC,
	ERR_ARGV_FILE_EXT,
	ERR_ARGV_NUM_THREADS,
	ERR_ARGV_MULTITHREADING,
	ERR_ARGV_REFLECTION,
	ERR_ARGV_FOV,
	ERR_ARGV_UNRECOGNIZED,
	ERR_ARGV_IO_OPEN_SCENE,
	ERR_ARGV_IO_OPEN_OUTPUT,
	ERR_IO_WRITE_IMG,
	ERR_JSON_KEY_NOT_STRING,
	ERR_JSON_UNRECOGNIZED_KEY,
	ERR_JSON_VALUE_NOT_FLOAT,
	ERR_JSON_VALUE_NOT_ARRAY,
	ERR_JSON_ARRAY_SIZE,
	ERR_JSON_FILENAME_NOT_STRING,
	ERR_JSON_IO_OPEN,
	ERR_JSON_ARGC,
	ERR_JSON_UNRECOGNIZED,
	ERR_JSON_IO_READ,
	ERR_JSON_NUM_TOKENS,
	ERR_JSON_READ_TOKENS,
	ERR_JSON_FIRST_TOKEN,
	ERR_JSON_ARGC_SCENE,
	ERR_MALLOC,
	ERR_JSON_NO_CAMERA,
	ERR_JSON_NO_OBJECTS,
	ERR_JSON_NO_LIGHTS,
	ERR_STL_IO_FP,
	ERR_STL_IO_READ,
	ERR_STL_ENCODING,
	ERR_END, //Used for determining number of error codes
};

extern const char *ERR_FORMAT_STRING;
extern const char *ERR_MESSAGES[ERR_END];
extern const char *HELPTEXT;

#endif
