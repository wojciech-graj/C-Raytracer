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

#include "type.h"

struct Material;

enum ObjectType {
	OBJECT_SPHERE,
	OBJECT_TRIANGLE,
#ifdef UNBOUND_OBJECTS
	OBJECT_PLANE,
#endif
};

struct Ray {
	v3 direction;
	v3 point;
};

struct Object;

struct ObjectVTable {
	enum ObjectType type;
#ifdef UNBOUND_OBJECTS
	bool is_bounded;
#endif
	void (*postinit)(struct Object*);
	bool (*get_intersection)(const struct Object*, const struct Ray*, float*, v3);
	bool (*intersects_in_range)(const struct Object*, const struct Ray*, float);
	void (*delete)(struct Object*);
	void (*get_corners)(const struct Object*, v3[2]);
	void (*scale)(const struct Object*, const v3, const float);
	void (*get_light_point)(const struct Object*, const v3, v3);
};

struct Object {
	struct ObjectVTable const *object_data;
	uint32_t num_lights;
	float epsilon;
	struct Material *material;
};

/* require num_objects, num_eminnant_objects, num_unbound_objects to be set. */
void objects_init(void);

void objects_deinit(void);

/* Set parent Object */
void object_init(struct Object *object, const struct Material *material, float epsilon, uint32_t num_lights, enum ObjectType object_type);

/* Creates new object and allocates memory. Requires object_init and object_data.postinit to be called after. */
struct Object *sphere_new(const v3 position, float radius);
struct Object *triangle_new(v3 vertices[3]);
#ifdef UNBOUND_OBJECTS
struct Object *plane_new(v3 position, v3 normal);
#endif

void mesh_to_objects(const char *filename, struct Object *object, const v3 position, const v3 rotation, float scale, size_t *i_object);

void get_objects_extents(v3 min, v3 max);

#ifdef UNBOUND_OBJECTS
void unbound_objects_get_closest_intersection(const struct Ray *ray, struct Object **closest_object, v3 closest_normal, float *closest_distance);
bool unbound_objects_is_light_blocked(const struct Ray *ray, const float distance, v3 light_intensity, const struct Object *emittant_object);
#endif

extern struct Object **objects;
extern size_t num_objects;
extern struct Object **emittant_objects;
extern size_t num_emittant_objects;
#ifdef UNBOUND_OBJECTS
extern struct Object **unbound_objects; //Since planes do not have a bounding cuboid and cannot be included in the BVH, they must be looked at separately
extern size_t num_unbound_objects;
#endif

#endif /* __OBJECT_H__ */
