#ifndef GLOBAL_H
#define GLOBAL_H

#define EPSILON 1e-6f
#define PI 3.1415927f

typedef float Vec3[3];
typedef struct Line Line;

typedef struct Line {
	Vec3 vector;
	Vec3 position;
} Line;

#endif
