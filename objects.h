#ifndef OBJECTS_H
#define OBJECTS_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <stdint.h>

#include "global.h"
#include "vector.h"
#include "algorithm.h"

#define OBJECT_PARAMS\
	bool (*intersects)(Object, Line*, float*);\
	void (*get_normal)(Object, Line*, Vec3, Vec3);\
	void (*delete)(Object);\
	Vec3 ks;  /*specular reflection constant*/\
	Vec3 kd; /*diffuse reflection constant*/\
	Vec3 ka; /*ambient reflection constant*/\
	float alpha; /*shininess constant*/

#define OBJECT_INIT_PARAMS\
	Vec3 ks,\
	Vec3 kd,\
	Vec3 ka,\
	float alpha

#define OBJECT_INIT_VARS\
	ks,\
	kd,\
	ka,\
	alpha

#define OBJECT_INIT(type, name)\
	type *name = malloc(sizeof(type));\
	name->intersects = &intersects_##name;\
	name->get_normal = &get_normal_##name;\
	name->delete = &delete_##name;\
	memcpy(name->ks, ks, sizeof(Vec3));\
	memcpy(name->kd, kd, sizeof(Vec3));\
	memcpy(name->ka, ka, sizeof(Vec3));\
	name->alpha = alpha;

typedef struct Plane Plane;
typedef struct Sphere Sphere;
typedef struct Triangle Triangle;
typedef struct PolyTriangle PolyTriangle;
typedef struct Mesh Mesh;
typedef struct CommonObject CommonObject;
typedef union Object Object;

typedef struct CommonObject {
	OBJECT_PARAMS
} CommonObject;

typedef union Object {
	CommonObject *common;
	Sphere *sphere;
	Triangle *triangle;
	Plane *plane;
	Mesh *mesh;
} Object;

Mesh *init_mesh(OBJECT_INIT_PARAMS, uint32_t num_triangles);
void mesh_set_triangle(Mesh *mesh, uint32_t index, Vec3 vertices[3]);

Triangle *init_triangle(OBJECT_INIT_PARAMS, Vec3 vertices[3]);

Sphere *init_sphere(OBJECT_INIT_PARAMS, Vec3 position, float radius);

Plane *init_plane(OBJECT_INIT_PARAMS, Vec3 normal, Vec3 point);


#endif
