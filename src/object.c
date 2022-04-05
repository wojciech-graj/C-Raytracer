/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Objects
 **/

#include "object.h"
#include "error.h"
#include "mem.h"
#include "system.h"
#include "calc.h"
#include "material.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>

struct Sphere {
	struct Object object;
	v3 position;
	float radius;
};

struct Triangle {//triangle ABC
	struct Object object;
	v3 vertices[3];
	v3 edges[2]; //Vectors BA and CA
	v3 normal;
};

#ifdef UNBOUND_OBJECTS
struct Plane {//normal = {a,b,c}, ax + by + cz = d
	struct Object object;
	v3 normal;
	float d;
};
#endif

struct STLTriangle {
	float normal[3]; //normal is unreliable so it is not used.
	float vertices[3][3];
	uint16_t attribute_bytes; //attribute bytes is unreliable so it is not used.
} __attribute__ ((packed));

/* Sphere */
void sphere_postinit(struct Object *object);
void sphere_delete(struct Object *object);
bool sphere_get_intersection(const struct Object *object, const struct Ray *ray, float *distance, v3 normal);
bool sphere_intersects_in_range(const struct Object *object, const struct Ray *ray, const float min_distance);
void sphere_get_corners(const struct Object *object, v3 corners[2]);
void sphere_scale(const struct Object *object, const v3 neg_shift, const float scale);
void sphere_get_light_point(const struct Object *object, const v3 point, v3 light_point);
bool line_intersects_sphere(const v3 sphere_position, const float sphere_radius, const v3 line_position, const v3 line_vector, const float epsilon, float *distance);

/* Triangle */
void triangle_postinit(struct Object *object);
void triangle_delete(struct Object *object);
bool triangle_get_intersection(const struct Object *object, const struct Ray *ray, float *distance, v3 normal);
bool triangle_intersects_in_range(const struct Object *object, const struct Ray *ray, float min_distance);
void triangle_get_corners(const struct Object *object, v3 corners[2]);
void triangle_scale(const struct Object *object, const v3 neg_shift, const float scale);
void triangle_get_light_point(const struct Object *object, const v3 point, v3 light_point);
bool moller_trumbore(const v3 vertex, v3 edges[2], const v3 line_position, const v3 line_vector, const float epsilon, float *distance);

/* Plane */
#ifdef UNBOUND_OBJECTS
void plane_postinit(struct Object *object);
void plane_delete(struct Object *object);
bool plane_get_intersection(const struct Object *object, const struct Ray *ray, float *distance, v3 normal);
bool plane_intersects_in_range(const struct Object *object, const struct Ray *ray, float min_distance);
void plane_scale(const struct Object *object, const v3 neg_shift, const float scale);
#endif

/* Mesh */
void stl_load_objects(FILE *file, const char *filename, struct Object *object, const v3 position, const v3 rot, const float scale, size_t *i_object);
uint32_t stl_get_num_triangles(FILE *file);

static const struct ObjectVTable OBJECT_DATA[] = {
#ifdef UNBOUND_OBJECTS
	[OBJECT_PLANE] = {
		.type = OBJECT_PLANE,
		.is_bounded = false,
		.postinit = &plane_postinit,
		.get_intersection = &plane_get_intersection,
		.intersects_in_range = &plane_intersects_in_range,
		.delete = &plane_delete,
		.scale = &plane_scale,
	},
#endif
	[OBJECT_SPHERE] = {
		.type = OBJECT_PLANE,
#ifdef UNBOUND_OBJECTS
		.is_bounded = true,
#endif
		.postinit = &sphere_postinit,
		.get_intersection = &sphere_get_intersection,
		.intersects_in_range = &sphere_intersects_in_range,
		.delete = &sphere_delete,
		.get_corners = &sphere_get_corners,
		.scale = &sphere_scale,
		.get_light_point = &sphere_get_light_point,
	},
	[OBJECT_TRIANGLE] = {
		.type = OBJECT_TRIANGLE,
#ifdef UNBOUND_OBJECTS
		.is_bounded = true,
#endif
		.postinit = &triangle_postinit,
		.get_intersection = &triangle_get_intersection,
		.intersects_in_range = &triangle_intersects_in_range,
		.delete = &triangle_delete,
		.get_corners = &triangle_get_corners,
		.scale = &triangle_scale,
		.get_light_point = &triangle_get_light_point,
	},
};

struct Object **objects;
size_t num_objects;
struct Object **emittant_objects;
size_t num_emittant_objects;
#ifdef UNBOUND_OBJECTS
struct Object **unbound_objects;
size_t num_unbound_objects;
#endif

void objects_init(void)
{
	printf_log("Initializing objects.");

	objects = safe_malloc(sizeof(struct Object*) * num_objects);
	emittant_objects = safe_malloc(sizeof(struct Object*) * num_emittant_objects);
#ifdef UNBOUND_OBJECTS
	if (num_unbound_objects)
		unbound_objects = safe_malloc(sizeof(struct Object*) * num_unbound_objects);
#endif
}

void objects_deinit(void)
{
	size_t i;
	for (i = 0; i < num_objects; i++)
		objects[i]->object_data->delete(objects[i]);
	free(objects);
#ifdef UNBOUND_OBJECTS
	free(unbound_objects);
#endif
	free(emittant_objects);
}

void object_init(struct Object *object, const struct Material *material, const float epsilon, const uint32_t num_lights, const enum ObjectType object_type)
{
	object->object_data = &OBJECT_DATA[object_type];
	object->material = material;
	object->epsilon = epsilon;
	object->num_lights = num_lights;
}

#ifdef UNBOUND_OBJECTS
void unbound_objects_get_closest_intersection(const struct Ray *ray, struct Object **closest_object, v3 closest_normal, float *closest_distance)
{
	float distance;
	v3 normal;
	size_t i;
	for (i = 0; i < num_unbound_objects; i++) {
		struct Object *object = unbound_objects[i];
		if (object->object_data->get_intersection(object, ray, &distance, normal) && distance < *closest_distance) {
			*closest_distance = distance;
			*closest_object = object;
			assign3(closest_normal, normal);

		}
	}
}

bool unbound_objects_is_light_blocked(const struct Ray *ray, const float distance, v3 light_intensity, const struct Object *emittant_object)
{
	(void)emittant_object; //NOTE: is unused because planes cant be lights
	size_t i;
	for (i = 0; i < num_unbound_objects; i++) {
		struct Object *object = unbound_objects[i];
		if (object->object_data->intersects_in_range(unbound_objects[i], ray, distance)) {
			if (object->material->transparent)
				mul3v(light_intensity, object->material->kt, light_intensity);
			else
				return true;
			}
	}
	return false;
}
#endif /* UNBOUND_OBJECTS */

void get_objects_extents(v3 min, v3 max)
{
	min[X] = FLT_MAX; min[Y] = FLT_MAX; min[Z] = FLT_MAX;
	max[X] = FLT_MIN; max[Y] = FLT_MIN; max[Z] = FLT_MIN;

	size_t i, j;
	for (i = 0; i < num_objects; i++) {
		struct Object *object = objects[i];
#ifdef UNBOUND_OBJECTS
		if (object->object_data->is_bounded) {
#endif
			v3 corners[2];
			object->object_data->get_corners(object, corners);
			for (j = 0; j < 3; j++) {
				if (corners[0][j] < min[j])
					min[j] = corners[0][j];
				if (corners[1][j] > max[j])
					max[j] = corners[1][j];
			}
#ifdef UNBOUND_OBJECTS
		}
#endif
	}
}

/*******************************************************************************
*	Sphere
*******************************************************************************/

void sphere_postinit(struct Object *object)
{
	struct Sphere *sphere = (struct Sphere*)object;

	if (sphere->object.epsilon == -1.f)
		sphere->object.epsilon = sphere->radius * 0.0003f;
}

struct Object *sphere_new(const v3 position, const float radius)
{
	struct Sphere *sphere = safe_malloc(sizeof(struct Sphere));

	sphere->radius = radius;
	assign3(sphere->position, position);

	return (struct Object*)sphere;
}

void sphere_delete(struct Object *object)
{
	free(object);
}

bool sphere_get_intersection(const struct Object *object, const struct Ray *ray, float *distance, v3 normal)
{
	struct Sphere *sphere = (struct Sphere*)object;
	if (line_intersects_sphere(sphere->position, sphere->radius, ray->point, ray->direction, sphere->object.epsilon, distance)) {
			mul3s(ray->direction, *distance, normal);
			add3v(normal, ray->point, normal);
			sub3v(normal, sphere->position, normal);
			mul3s(normal, 1.f / sphere->radius, normal);
			return true;
		}
	return false;
}

bool sphere_intersects_in_range(const struct Object *object, const struct Ray *ray, const float min_distance)
{
	struct Sphere *sphere = (struct Sphere*)object;
	float distance;
	bool intersects = line_intersects_sphere(sphere->position, sphere->radius, ray->point, ray->direction, sphere->object.epsilon, &distance);
	if (intersects && distance < min_distance)
		return true;
	return false;
}

void sphere_get_corners(const struct Object *object, v3 corners[2])
{
	struct Sphere *sphere = (struct Sphere*)object;
	sub3s(sphere->position, sphere->radius, corners[0]);
	add3s(sphere->position, sphere->radius, corners[1]);
}

void sphere_scale(const struct Object *object, const v3 neg_shift, const float scale)
{
	struct Sphere *sphere = (struct Sphere*)object;
	sphere->object.epsilon *= scale;
	sphere->radius *= scale;
	sub3v(sphere->position, neg_shift, sphere->position);
	mul3s(sphere->position, scale, sphere->position);
}

void sphere_get_light_point(const struct Object *object, const v3 point, v3 light_point)
{
	struct Sphere *sphere = (struct Sphere*)object;
	v3 normal;
	sub3v(sphere->position, point, normal);
	float inclination = rand_flt() * 2.f * PI;
	float azimuth = rand_flt() * 2.f * PI;
	v3 light_direction = SPHERICAL_TO_CARTESIAN(sphere->radius, inclination, azimuth);
	if (dot3(normal, light_direction))
		mul3s(light_direction, -1.f, light_direction);
	add3v(sphere->position, light_direction, light_point);
}

bool line_intersects_sphere(const v3 sphere_position, const float sphere_radius, const v3 line_position, const v3 line_vector, const float epsilon, float *distance)
{
	v3 relative_position;
	sub3v(line_position, sphere_position, relative_position);
	float b = -dot3(line_vector, relative_position);
	float c = dot3(relative_position, relative_position) - sqr(sphere_radius);
	float det = sqr(b) - c;
	if (det < 0) //no collision
		return false;
	float sqrt_det = sqrtf(det);
	*distance = b - sqrt_det;
	if (*distance > epsilon)//if in front of origin of ray
		return true;
	*distance = b + sqrt_det;
	return *distance > epsilon; //check if the further distance is positive
}

/*******************************************************************************
*	Triangle
*******************************************************************************/

void triangle_postinit(struct Object *object)
{
	struct Triangle *triangle = (struct Triangle*)object;

	sub3v(triangle->vertices[1], triangle->vertices[0], triangle->edges[0]);
	sub3v(triangle->vertices[2], triangle->vertices[0], triangle->edges[1]);
	cross(triangle->edges[0], triangle->edges[1], triangle->normal);
	norm3(triangle->normal);

	if (triangle->object.epsilon == -1.f) {
		float magab = mag3(triangle->edges[0]) * mag3(triangle->edges[1]);
		triangle->object.epsilon = 0.003f * powf(0.5f * magab * sinf(acosf(dot3(triangle->edges[0], triangle->edges[1]) / magab)), 0.75f);
	}
}

struct Object *triangle_new(v3 vertices[3])
{
	struct Triangle *triangle = safe_malloc(sizeof(struct Triangle));

	memcpy(triangle->vertices, vertices, sizeof(v3[3]));

	return (struct Object*)triangle;
}

void triangle_delete(struct Object *object)
{
	free(object);
}

bool triangle_get_intersection(const struct Object *object, const struct Ray *ray, float *distance, v3 normal)
{
	struct Triangle *triangle = (struct Triangle*)object;
	bool intersects = moller_trumbore(triangle->vertices[0], triangle->edges, ray->point, ray->direction, triangle->object.epsilon, distance);
	if (intersects) {
		assign3(normal, triangle->normal);
		return true;
	}
	return false;
}

bool triangle_intersects_in_range(const struct Object *object, const struct Ray *ray, float min_distance)
{
	struct Triangle *triangle = (struct Triangle*)object;
	float distance;
	bool intersects = moller_trumbore(triangle->vertices[0], triangle->edges, ray->point, ray->direction, triangle->object.epsilon, &distance);
	return intersects && distance < min_distance;
}

void triangle_get_corners(const struct Object *object, v3 corners[2])
{
	struct Triangle *triangle = (struct Triangle*)object;
	assign3(corners[0], triangle->vertices[2]);
	assign3(corners[1], triangle->vertices[2]);
	size_t i, j;
	for (i = 0; i < 2; i++)
		for (j = 0; j < 3; j++) {
			if (corners[0][j] > triangle->vertices[i][j])
				corners[0][j] = triangle->vertices[i][j];
			else if (corners[1][j] < triangle->vertices[i][j])
				corners[1][j] = triangle->vertices[i][j];
		}
}

void triangle_scale(const struct Object *object, const v3 neg_shift, const float scale)
{
	struct Triangle *triangle = (struct Triangle*)object;
	triangle->object.epsilon *= scale;
	size_t i;
	for (i = 0; i < 3; i++) {
		sub3v(triangle->vertices[i], neg_shift, triangle->vertices[i]);
		mul3s(triangle->vertices[i], scale, triangle->vertices[i]);
	}
	for (i = 0; i < 2; i++)
		mul3s(triangle->edges[i], scale, triangle->edges[i]);
}

void triangle_get_light_point(const struct Object *object, const v3 point, v3 light_point)
{
	//NOTE: this method may be inefficient due to the 3 square root operations, but it is unlikely to be used often
	(void)point;
	struct Triangle *triangle = (struct Triangle*)object;
	float p = rand_flt(), q = rand_flt();

	if (p + q > 1.f) {
		p = 1.f - p;
		q = 1.f - q;
	}

	size_t i;
#pragma GCC unroll 3
	for (i = 0; i < 3; i++)
		light_point[i] = triangle->vertices[0][i] + (triangle->vertices[1][i] - triangle->vertices[0][i]) * p + (triangle->vertices[2][i] - triangle->vertices[0][i]) * q;
}

//Möller–Trumbore intersection algorithm
bool moller_trumbore(const v3 vertex, v3 edges[2], const v3 line_position, const v3 line_vector, const float epsilon, float *distance)
{
	float a, f, u, v;
	v3 h, s, q;
	cross(line_vector, edges[1], h);
	a = dot3(edges[0], h);
	if (unlikely(a < epsilon && a > -epsilon)) //ray is parallel to line
		return false;
	f = 1.f / a;
	sub3v(line_position, vertex, s);
	u = f * dot3(s, h);
	if (u < 0.f || u > 1.f)
		return false;
	cross(s, edges[0], q);
	v = f * dot3(line_vector, q);
	if (v < 0.f || u + v > 1.f)
		return false;
	*distance = f * dot3(edges[1], q);
	return *distance > epsilon;
}

/*******************************************************************************
*	Plane
*******************************************************************************/

#ifdef UNBOUND_OBJECTS
void plane_postinit(struct Object *object)
{
	struct Plane *plane = (struct Plane*)object;

	error_check(!plane->object.material->emittant, "Plane cannot be emittant");
	if (plane->object.epsilon == -1.f)
		plane->object.epsilon = 1.e-6f;
}

struct Object *plane_new(v3 position, v3 normal)
{
	struct Plane *plane = safe_malloc(sizeof(struct Plane));

	assign3(plane->normal, normal);
	norm3(plane->normal);
	plane->d = dot3(plane->normal, position);

	return (struct Object*)plane;
}

void plane_delete(struct Object *object)
{
	free(object);
}

bool plane_get_intersection(const struct Object *object, const struct Ray *ray, float *distance, v3 normal)
{
	struct Plane *plane = (struct Plane*)object;
	float a = dot3(plane->normal, ray->direction);
	if (fabsf(a) < plane->object.epsilon) //ray is parallel to line
		return false;
	*distance = (plane->d - dot3(plane->normal, ray->point)) / dot3(plane->normal, ray->direction);
	if (*distance > plane->object.epsilon) {
		if (signbit(a))
			assign3(normal, plane->normal);
		else
			mul3s(plane->normal, -1.f, normal);
		return true;
	}
	return false;
}

bool plane_intersects_in_range(const struct Object *object, const struct Ray *ray, float min_distance)
{
	struct Plane *plane = (struct Plane*)object;
	float a = dot3(plane->normal, ray->direction);
	if (fabsf(a) < plane->object.epsilon) //ray is parallel to line
		return false;
	float distance = (plane->d - dot3(plane->normal, ray->point)) / dot3(plane->normal, ray->direction);
	return distance > plane->object.epsilon && distance < min_distance;
}

void plane_scale(const struct Object *object, const v3 neg_shift, const float scale)
{
	struct Plane *plane = (struct Plane*)object;
	v3 point = {1.f, 1.f, 1.f};
	size_t i;
	for (i = 0; i < 3; i++)
		if (fabsf(plane->normal[i]) > plane->object.epsilon)
			break;
	point[i] = 0.f;
	point[i] = (plane->d - dot3(point, plane->normal)) / plane->normal[i];
	sub3v(point, neg_shift, point);
	mul3s(point, scale, point);
	plane->d = dot3(plane->normal, point);
	plane->object.epsilon *= scale;
}
#endif /* UNBOUND_OBJECTS */

/*******************************************************************************
*	Mesh
*******************************************************************************/

void mesh_to_objects(const char *filename, struct Object *object, const v3 position, const v3 rotation, const float scale, size_t *i_object)
{
	FILE *file = fopen(filename, "rb");
	error_check(file, "Failed to open mesh file %s.", filename);

	stl_load_objects(file, filename, object, position, rotation, scale, i_object);

	fclose(file);
}

uint32_t stl_get_num_triangles(FILE *file)
{
	int fseek_ret = fseek(file, sizeof(uint8_t) * 80, SEEK_SET);
	error_check(!fseek_ret, "Failed to read header of mesh file %s.");
	uint32_t num_triangles;
	size_t nmemb_read = fread(&num_triangles, sizeof(uint32_t), 1, file);
	error_check(nmemb_read == 1, "Failed to read triangle count in mesh file.");
	return num_triangles;
}

//assumes that file is at SEEK_SET
void stl_load_objects(FILE *file, const char *filename, struct Object *object, const v3 position, const v3 rot, const float scale, size_t *i_object)
{
	//ensure that file is binary instead of ascii
	char header[5];
	size_t nmemb_read = fread(header, sizeof(char), 5, file);
	error_check(nmemb_read == 5, "Failed to read header of mesh file [%s].", filename);
	error_check(strncmp("solid", header, 5), "Mesh file [%s] does not use binary encoding.", filename);

	float a = cosf(rot[Z]) * sinf(rot[Y]);
	float b = sinf(rot[Z]) * sinf(rot[Y]);
	m3 rotation_matrix = {
		{
			cosf(rot[Z]) * cosf(rot[Y]),
			a * sinf(rot[X]) - sinf(rot[Z]) * cosf(rot[X]),
			a * cosf(rot[X]) + sinf(rot[Z]) * sinf(rot[X])
		}, {
			sinf(rot[Z]) * cosf(rot[Y]),
			b * sinf(rot[X]) + cosf(rot[Z]) * cosf(rot[X]),
			b * cosf(rot[X]) - cosf(rot[Z]) * sinf(rot[X])
		}, {
			-sinf(rot[Y]),
			cosf(rot[Y]) * sinf(rot[X]),
			cosf(rot[Y]) * cosf(rot[X])
		}
	};

	uint32_t num_triangles = stl_get_num_triangles(file);
	num_objects += num_triangles - 1;
	objects = safe_realloc(objects, sizeof(struct Object) * num_objects);

	uint32_t i;
	for (i = 0; i < num_triangles; i++) {
		struct STLTriangle stl_triangle;
		nmemb_read = fread(&stl_triangle, sizeof(struct STLTriangle), 1, file);
		error_check(nmemb_read == 1, "Failed to read triangle in mesh file [%s].", filename);

		uint32_t j;
		for (j = 0; j < 3; j++) {
			v3 temp_vertices;
			mulmv(rotation_matrix, stl_triangle.vertices[j], temp_vertices);
			mul3s(temp_vertices, scale, stl_triangle.vertices[j]);
			add3v(stl_triangle.vertices[j], position, stl_triangle.vertices[j]);
		}
		struct Object *triangle = triangle_new(stl_triangle.vertices);
		memcpy(triangle, object, sizeof(struct Object));
		triangle_postinit(triangle);
		objects[*i_object] = triangle;
		*i_object += 1;
	}
}
