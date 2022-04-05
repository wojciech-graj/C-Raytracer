/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Objects rendered by raytracer
 **/

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "material.h"
#include "calc.h"

#include <stdint.h>

typedef enum _ObjectType {
	OBJECT_SPHERE,
	OBJECT_TRIANGLE,
#ifdef UNBOUND_OBJECTS
	OBJECT_PLANE,
#endif
} ObjectType;

typedef struct _Line {
	Vec3 vector;
	Vec3 position;
} Line;

typedef struct _Object Object;

typedef struct _ObjectVTable {
	ObjectType type;
#ifdef UNBOUND_OBJECTS
	bool is_bounded;
#endif
	void (*postinit)(Object*);
	bool (*get_intersection)(const Object*, const Line*, float*, Vec3);
	bool (*intersects_in_range)(const Object*, const Line*, float);
	void (*delete)(Object*);
	void (*get_corners)(const Object*, Vec3[2]);
	void (*scale)(const Object*, const Vec3, const float);
	void (*get_light_point)(const Object*, const Vec3, Vec3);
} ObjectVTable;

struct _Object {
	ObjectVTable const *object_data;
	uint32_t num_lights;
	float epsilon;
	Material *material;
};

/* require num_objects, num_eminnant_objects, num_unbound_objects to be set. */
void objects_init(void);

void objects_deinit(void);

/* Set parent Object */
void object_init(Object *object, const Material *material, float epsilon, uint32_t num_lights, ObjectType object_type);

/* Creates new object and allocates memory. Requires object_init and object_data.postinit to be called after. */
Object *sphere_new(const Vec3 position, float radius);
Object *triangle_new(Vec3 vertices[3]);
#ifdef UNBOUND_OBJECTS
Object *plane_new(Vec3 position, Vec3 normal);
#endif

void mesh_to_objects(const char *filename, Object *object, const Vec3 position, const Vec3 rotation, float scale, size_t *i_object);

void get_objects_extents(Vec3 min, Vec3 max);

#ifdef UNBOUND_OBJECTS
void unbound_objects_get_closest_intersection(const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance);
bool unbound_objects_is_light_blocked(const Line *ray, const float distance, Vec3 light_intensity, const Object *emittant_object);
#endif

extern Object **objects;
extern size_t num_objects;
extern Object **emittant_objects;
extern size_t num_emittant_objects;
#ifdef UNBOUND_OBJECTS
extern Object **unbound_objects; //Since planes do not have a bounding cuboid and cannot be included in the BVH, they must be looked at separately
extern size_t num_unbound_objects;
#endif

#endif /* __OBJECT_H__ */
