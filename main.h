#ifndef MAIN_H
#define MAIN_H

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <float.h>

#include "vector.h"


typedef struct Plane Plane;
typedef struct Line Line;
typedef struct Sphere Sphere;
typedef struct Image Image;
typedef struct Camera Camera;
typedef struct CommonObject CommonObject;
typedef union Object Object;

#define OBJECT_PARAMS \
	bool (*intersects)(Line*, Object, double*);\
	Color color;

typedef unsigned char Color[3];

Color background_color = {0, 0, 0};

typedef struct CommonObject {
	OBJECT_PARAMS
} CommonObject;

typedef struct Plane {//normal = {a,b,c}, ax + by + cz + d = 0
	Vec3 normal;
	double d;
} Plane;

typedef struct Line {
	Vec3 vector;
	Vec3 position;
} Line;

typedef struct Sphere {
	OBJECT_PARAMS
	Vec3 position;
	double radius;
} Sphere;

typedef struct Image {
	int resolution[2];
	Vec2 size;
	Vec3 corner;
	Vec3 vectors[2];
	Color *pixels;
} Image;

typedef struct Camera {
	Vec3 position;
	Vec3 vectors[3]; //vectors are perpendicular to eachother and normalized. vectors[3] is normal to projection_plane.
	double focal_length;
	Image image;
	Plane projection_plane;
} Camera;

typedef union Object {
	CommonObject *common;
	Sphere *sphere;
} Object;

#endif
