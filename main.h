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

#define OBJECT_PARAMS \
	bool (*intersects)(Line*, Object, double*);\
	Color color;

#define OBJECT_INIT_PARAMS Color color

#define OBJECT_INIT(object) \
	object->intersects = &intersects_##object;\
	memcpy(object->color, color, sizeof(Color))

#define EPSILON 1e-6

typedef unsigned char Color[3];
typedef struct Plane Plane;
typedef struct Line Line;
typedef struct Sphere Sphere;
typedef struct Triangle Triangle;
typedef struct Image Image;
typedef struct Camera Camera;
typedef struct CommonObject CommonObject;
typedef union Object Object;

Color background_color = {0, 0, 0};

typedef struct CommonObject {
	OBJECT_PARAMS
} CommonObject;

typedef struct Plane {//normal = {a,b,c}, ax + by + cz = d
	OBJECT_PARAMS
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

typedef struct Triangle {//triangle ABC
	OBJECT_PARAMS
	Vec3 vertices[3];
	Vec3 edges[2]; //Vectors BA and CA
} Triangle;

typedef struct Image {
	int resolution[2];
	Vec2 size;
	Vec3 corner; //Top left corner of image
	Vec3 vectors[2]; //Vectors for image plane traversal by 1 pixel in X and Y directions
	Color *pixels;
} Image;

typedef struct Camera {
	Vec3 position;
	Vec3 vectors[3]; //vectors are perpendicular to eachother and normalized. vectors[3] is normal to projection_plane.
	double focal_length;
	Image image;
} Camera;

typedef union Object {
	CommonObject *common;
	Sphere *sphere;
	Triangle *triangle;
	Plane *plane;
} Object;

#endif
