#ifndef OBJECTS_H
#define OBJECTS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "global.h"
#include "vector.h"

/*******************************************************************************
*	OBJECT
*******************************************************************************/

#define OBJECT_MEMBERS\
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
	float epsilon;\
	bool reflective;\
	bool transparent

#define OBJECT_INIT_PARAMS\
	Vec3 ks,\
	Vec3 kd,\
	Vec3 ka,\
	Vec3 kr,\
	Vec3 kt,\
	float shininess,\
	float refractive_index,\
	float epsilon

#define OBJECT_INIT_VARS\
	ks,\
	kd,\
	ka,\
	kr,\
	kt,\
	shininess,\
	refractive_index,\
	epsilon

#define OBJECT_INIT_VARS_DECL\
	Vec3 ks, kd, ka, kr, kt;\
	float shininess, refractive_index, epsilon

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
	name->epsilon = epsilon;\
	name->reflective = magnitude3(kr) > epsilon;\
	name->transparent = magnitude3(kt) > epsilon;

#define SCENE_OBJECT_LOAD_VARS\
	buffer,\
	tokens,\
	token_index,\
	ks,\
	kd,\
	ka,\
	kr,\
	kt,\
	&shininess,\
	&refractive_index,\
	&epsilon

typedef struct Plane Plane;
typedef struct Sphere Sphere;
typedef struct Triangle Triangle;
typedef struct MeshTriangle MeshTriangle;
typedef struct Mesh Mesh;
typedef struct CommonObject CommonObject;
typedef union Object Object;

typedef struct CommonObject {
	OBJECT_MEMBERS;
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
void mesh_generate_bounding_sphere(Mesh *mesh);
void mesh_generate_bounding_cuboid(Mesh *mesh);
Triangle *init_triangle(OBJECT_INIT_PARAMS, Vec3 vertices[3]);
Sphere *init_sphere(OBJECT_INIT_PARAMS, Vec3 position, float radius);
Plane *init_plane(OBJECT_INIT_PARAMS, Vec3 normal, Vec3 point);

/*******************************************************************************
*	BOUNDING SHAPE
*******************************************************************************/

#define BOUNDING_SHAPE_MEMBERS\
	bool (*intersects)(BoundingShape, Line*);\
	float epsilon

#define BOUNDING_SHAPE_INIT_PARAMS\
	float epsilon

#define BOUNDING_SHAPE_INIT(type, name)\
	type *name = malloc(sizeof(type));\
	name->intersects = &intersects_##name;\
	name->epsilon = epsilon;

typedef struct CommonBoundingShape CommonBoundingShape;
typedef struct BoundingSphere BoundingSphere;
typedef struct BoundingCuboid BoundingCuboid;
typedef union BoundingShape BoundingShape;

typedef struct CommonBoundingShape {
	BOUNDING_SHAPE_MEMBERS;
} CommonBoundingShape;

typedef union BoundingShape {
	CommonBoundingShape *common;
	BoundingSphere *sphere;
	BoundingCuboid *cuboid;
} BoundingShape;

BoundingCuboid *init_bounding_cuboid(BOUNDING_SHAPE_INIT_PARAMS, Vec3 corners[2]);
BoundingSphere *init_bounding_sphere(BOUNDING_SHAPE_INIT_PARAMS, Vec3 position, float radius);

/*******************************************************************************
*	MISCELLANEOUS
*******************************************************************************/

typedef struct Image Image;
typedef struct Camera Camera;
typedef struct Light Light;

typedef struct Image {
	unsigned resolution[2];
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

void init_camera(Camera *camera, Vec3 position, Vec3 vectors[2], float focal_length, unsigned image_resolution[2], Vec2 image_size);
void save_image(FILE *file, Image *image);
void init_light(Light *light, Vec3 position, Vec3 intensity);

#endif
