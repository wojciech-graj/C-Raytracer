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
	bool (*intersects)(Object, Line*, float*, Vec3);\
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
	name->delete = &delete_##name;\
	memcpy(name->ks, ks, sizeof(Vec3));\
	memcpy(name->kd, kd, sizeof(Vec3));\
	memcpy(name->ka, ka, sizeof(Vec3));\
	name->alpha = alpha;

#define BOUNDING_SHAPE_PARAMS\
	bool (*intersects)(BoundingShape, Line*);

#define BOUNDING_SHAPE_INIT(type, name)\
	type *name = malloc(sizeof(type));\
	name->intersects = &intersects_##name;

typedef struct Plane Plane;
typedef struct Sphere Sphere;
typedef struct Triangle Triangle;
typedef struct PolyTriangle PolyTriangle;
typedef struct Mesh Mesh;
typedef struct CommonObject CommonObject;
typedef union Object Object;

typedef struct CommonBoundingShape CommonBoundingShape;
typedef struct BoundingSphere BoundingSphere;
typedef union BoundingShape BoundingShape;

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

typedef struct CommonBoundingShape {
	BOUNDING_SHAPE_PARAMS
} CommonBoundingShape;

typedef union BoundingShape {
	CommonBoundingShape *common;
	BoundingSphere *sphere;
} BoundingShape;

Mesh *init_mesh(OBJECT_INIT_PARAMS, uint32_t num_triangles);
void mesh_set_triangle(Mesh *mesh, uint32_t index, Vec3 vertices[3]);
void mesh_generate_bounding_sphere(Mesh *mesh);

Triangle *init_triangle(OBJECT_INIT_PARAMS, Vec3 vertices[3]);

Sphere *init_sphere(OBJECT_INIT_PARAMS, Vec3 position, float radius);

Plane *init_plane(OBJECT_INIT_PARAMS, Vec3 normal, Vec3 point);


#endif
