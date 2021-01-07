#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>

#define PI 3.1415927f

typedef float Vec3[3];
typedef uint8_t Color[3];
typedef struct Line Line;

typedef struct Line {
	Vec3 vector;
	Vec3 position;
} Line;

#endif
