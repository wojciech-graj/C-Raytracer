#ifndef OBJECTS_H
#define OBJECTS_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "global.h"
#include "vector.h"
#include "algorithm.h"

#define OBJECT_PARAMS\
	bool (*get_intersection)(Object, Line*, float*, Vec3);\
	bool (*intersects_in_range)(Object, Line*, float);\
	void (*delete)(Object);\
	Vec3 ks;  /*specular reflection constant*/\
	Vec3 kd; /*diffuse reflection constant*/\
	Vec3 ka; /*ambient reflection constant*/\
	Vec3 kr; /*specular interreflection constant*/\
	Vec3 kt; /*transparency constant*/\
	float shininess; /*shininess constant*/\
	float refractive_index;\
	bool reflective;\
	bool transparent;

#define OBJECT_INIT_PARAMS\
	Vec3 ks,\
	Vec3 kd,\
	Vec3 ka,\
	Vec3 kr,\
	Vec3 kt,\
	float shininess,\
	float refractive_index

#define OBJECT_INIT_VARS\
	ks,\
	kd,\
	ka,\
	kr,\
	kt,\
	shininess,\
	refractive_index

#define OBJECT_INIT_VARS_DECL\
	Vec3 ks, kd, ka, kr, kt;\
	float shininess, refractive_index;

#define OBJECT_INIT(type, name)\
	type *name = malloc(sizeof(type));\
	name->get_intersection = &get_intersection_##name;\
	name->intersects_in_range = &intersects_in_range_##name;\
	name->delete = &delete_##name;\
	memcpy(name->ks, ks, sizeof(Vec3));\
	memcpy(name->kd, kd, sizeof(Vec3));\
	memcpy(name->ka, ka, sizeof(Vec3));\
	memcpy(name->kr, kr, sizeof(Vec3));\
	memcpy(name->kt, kt, sizeof(Vec3));\
	name->shininess = shininess;\
	name->refractive_index = refractive_index;\
	name->reflective = magnitude3(kr) > epsilon;\
	name->transparent = magnitude3(kt) > epsilon;

#define BOUNDING_SHAPE_PARAMS\
	bool (*intersects)(BoundingShape, Line*);

#define BOUNDING_SHAPE_INIT(type, name)\
	type *name = malloc(sizeof(type));\
	name->intersects = &intersects_##name;

typedef struct Image Image;
typedef struct Camera Camera;
typedef struct Light Light;

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
	float focal_length;
	Image image;
} Camera;

typedef struct Light {
	Vec3 position;
	Vec3 intensity;
} Light;

typedef struct Plane Plane;
typedef struct Sphere Sphere;
typedef struct Triangle Triangle;
typedef struct MeshTriangle MeshTriangle;
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

void init_camera(Camera *camera, Vec3 position, Vec3 vectors[2], float focal_length, int image_resolution[2], Vec2 image_size);
void save_image(FILE *file, Image *image);

void init_light(Light *light, Vec3 position, Vec3 intensity);

Mesh *init_mesh(OBJECT_INIT_PARAMS, uint32_t num_triangles);
void mesh_set_triangle(Mesh *mesh, uint32_t index, Vec3 vertices[3]);
void mesh_generate_bounding_sphere(Mesh *mesh);

Triangle *init_triangle(OBJECT_INIT_PARAMS, Vec3 vertices[3]);

Sphere *init_sphere(OBJECT_INIT_PARAMS, Vec3 position, float radius);

Plane *init_plane(OBJECT_INIT_PARAMS, Vec3 normal, Vec3 point);


#endif
