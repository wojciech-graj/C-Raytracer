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

#include "object.h"
#include "error.h"
#include "mem.h"
#include "type.h"
#include "system.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>

typedef struct _Sphere {
	Object object;
	Vec3 position;
	float radius;
} Sphere;

typedef struct _Triangle {//triangle ABC
	Object object;
	Vec3 vertices[3];
	Vec3 edges[2]; //Vectors BA and CA
	Vec3 normal;
} Triangle;

#ifdef UNBOUND_OBJECTS
typedef struct _Plane {//normal = {a,b,c}, ax + by + cz = d
	Object object;
	Vec3 normal;
	float d;
} Plane;
#endif

typedef struct _STLTriangle {
	float normal[3]; //normal is unreliable so it is not used.
	float vertices[3][3];
	uint16_t attribute_bytes; //attribute bytes is unreliable so it is not used.
} __attribute__ ((packed)) STLTriangle;

/* Sphere */
void sphere_postinit(Object *object);
void sphere_delete(Object *object);
bool sphere_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal);
bool sphere_intersects_in_range(const Object *object, const Line *ray, const float min_distance);
void sphere_get_corners(const Object *object, Vec3 corners[2]);
void sphere_scale(const Object *object, const Vec3 neg_shift, const float scale);
void sphere_get_light_point(const Object *object, const Vec3 point, Vec3 light_point);
bool line_intersects_sphere(const Vec3 sphere_position, const float sphere_radius, const Vec3 line_position, const Vec3 line_vector, const float epsilon, float *distance);

/* Triangle */
void triangle_postinit(Object *object);
void triangle_delete(Object *object);
bool triangle_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal);
bool triangle_intersects_in_range(const Object *object, const Line *ray, float min_distance);
void triangle_get_corners(const Object *object, Vec3 corners[2]);
void triangle_scale(const Object *object, const Vec3 neg_shift, const float scale);
void triangle_get_light_point(const Object *object, const Vec3 point, Vec3 light_point);
bool moller_trumbore(const Vec3 vertex, Vec3 edges[2], const Vec3 line_position, const Vec3 line_vector, const float epsilon, float *distance);

/* Plane */
#ifdef UNBOUND_OBJECTS
void plane_postinit(Object *object);
void plane_delete(Object *object);
bool plane_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal);
bool plane_intersects_in_range(const Object *object, const Line *ray, float min_distance);
void plane_scale(const Object *object, const Vec3 neg_shift, const float scale);
#endif

/* Mesh */
void stl_load_objects(FILE *file, const char *filename, Object *object, const Vec3 position, const Vec3 rot, const float scale, size_t *i_object);
uint32_t stl_get_num_triangles(FILE *file);

static const ObjectVTable OBJECT_DATA[] = {
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

Object **objects;
size_t num_objects;
Object **emittant_objects;
size_t num_emittant_objects;
#ifdef UNBOUND_OBJECTS
Object **unbound_objects;
size_t num_unbound_objects;
#endif

void objects_init(void)
{
	printf_log("Initializing objects.");

	objects = safe_malloc(sizeof(Object*) * num_objects);
	emittant_objects = safe_malloc(sizeof(Object*) * num_emittant_objects);
#ifdef UNBOUND_OBJECTS
	if (num_unbound_objects)
		unbound_objects = safe_malloc(sizeof(Object*) * num_unbound_objects);
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

void object_init(Object *object, const Material *material, const float epsilon, const uint32_t num_lights, const ObjectType object_type)
{
	object->object_data = &OBJECT_DATA[object_type];
	object->material = material;
	object->epsilon = epsilon;
	object->num_lights = num_lights;
}

#ifdef UNBOUND_OBJECTS
void unbound_objects_get_closest_intersection(const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance)
{
	float distance;
	Vec3 normal;
	size_t i;
	for (i = 0; i < num_unbound_objects; i++) {
		Object *object = unbound_objects[i];
		if (object->object_data->get_intersection(object, ray, &distance, normal) && distance < *closest_distance) {
			*closest_distance = distance;
			*closest_object = object;
			memcpy(closest_normal, normal, sizeof(Vec3));

		}
	}
}

bool unbound_objects_is_light_blocked(const Line *ray, const float distance, Vec3 light_intensity, const Object *emittant_object)
{
	(void)emittant_object; //NOTE: is unused because planes cant be lights
	size_t i;
	for (i = 0; i < num_unbound_objects; i++) {
		Object *object = unbound_objects[i];
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

void get_objects_extents(Vec3 min, Vec3 max)
{
	min[X] = FLT_MAX; min[Y] = FLT_MAX; min[Z] = FLT_MAX;
	max[X] = FLT_MIN; max[Y] = FLT_MIN; max[Z] = FLT_MIN;

	size_t i, j;
	for (i = 0; i < num_objects; i++) {
		Object *object = objects[i];
#ifdef UNBOUND_OBJECTS
		if (object->object_data->is_bounded) {
#endif
			Vec3 corners[2];
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

void sphere_postinit(Object *object)
{
	Sphere *sphere = (Sphere*)object;

	if (sphere->object.epsilon == -1.f)
		sphere->object.epsilon = sphere->radius * 0.0003f;
}

Object *sphere_new(const Vec3 position, const float radius)
{
	Sphere *sphere = safe_malloc(sizeof(Sphere));

	sphere->radius = radius;
	memcpy(sphere->position, position, sizeof(Vec3));

	return (Object*)sphere;
}

void sphere_delete(Object *object)
{
	free(object);
}

bool sphere_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal)
{
	Sphere *sphere = (Sphere*)object;
	if (line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, sphere->object.epsilon, distance)) {
			mul3s(ray->vector, *distance, normal);
			add3v(normal, ray->position, normal);
			sub3v(normal, sphere->position, normal);
			mul3s(normal, 1.f / sphere->radius, normal);
			return true;
		}
	return false;
}

bool sphere_intersects_in_range(const Object *object, const Line *ray, const float min_distance)
{
	Sphere *sphere = (Sphere*)object;
	float distance;
	bool intersects = line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, sphere->object.epsilon, &distance);
	if (intersects && distance < min_distance)
		return true;
	return false;
}

void sphere_get_corners(const Object *object, Vec3 corners[2])
{
	Sphere *sphere = (Sphere*)object;
	sub3s(sphere->position, sphere->radius, corners[0]);
	add3s(sphere->position, sphere->radius, corners[1]);
}

void sphere_scale(const Object *object, const Vec3 neg_shift, const float scale)
{
	Sphere *sphere = (Sphere*)object;
	sphere->object.epsilon *= scale;
	sphere->radius *= scale;
	sub3v(sphere->position, neg_shift, sphere->position);
	mul3s(sphere->position, scale, sphere->position);
}

void sphere_get_light_point(const Object *object, const Vec3 point, Vec3 light_point)
{
	Sphere *sphere = (Sphere*)object;
	Vec3 normal;
	sub3v(sphere->position, point, normal);
	float inclination = rand_flt() * 2.f * PI;
	float azimuth = rand_flt() * 2.f * PI;
	Vec3 light_direction = SPHERICAL_TO_CARTESIAN(sphere->radius, inclination, azimuth);
	if (dot3(normal, light_direction))
		mul3s(light_direction, -1.f, light_direction);
	add3v(sphere->position, light_direction, light_point);
}

bool line_intersects_sphere(const Vec3 sphere_position, const float sphere_radius, const Vec3 line_position, const Vec3 line_vector, const float epsilon, float *distance)
{
	Vec3 relative_position;
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

void triangle_postinit(Object *object)
{
	Triangle *triangle = (Triangle*)object;

	sub3v(triangle->vertices[1], triangle->vertices[0], triangle->edges[0]);
	sub3v(triangle->vertices[2], triangle->vertices[0], triangle->edges[1]);
	cross(triangle->edges[0], triangle->edges[1], triangle->normal);
	norm3(triangle->normal);

	if (triangle->object.epsilon == -1.f) {
		float magab = mag3(triangle->edges[0]) * mag3(triangle->edges[1]);
		triangle->object.epsilon = 0.003f * powf(0.5f * magab * sinf(acosf(dot3(triangle->edges[0], triangle->edges[1]) / magab)), 0.75f);
	}
}

Object *triangle_new(Vec3 vertices[3])
{
	Triangle *triangle = safe_malloc(sizeof(Triangle));

	memcpy(triangle->vertices, vertices, sizeof(Vec3[3]));

	return (Object*)triangle;
}

void triangle_delete(Object *object)
{
	free(object);
}

bool triangle_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal)
{
	Triangle *triangle = (Triangle*)object;
	bool intersects = moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, triangle->object.epsilon, distance);
	if (intersects) {
		memcpy(normal, triangle->normal, sizeof(Vec3));
		return true;
	}
	return false;
}

bool triangle_intersects_in_range(const Object *object, const Line *ray, float min_distance)
{
	Triangle *triangle = (Triangle*)object;
	float distance;
	bool intersects = moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, triangle->object.epsilon, &distance);
	return intersects && distance < min_distance;
}

void triangle_get_corners(const Object *object, Vec3 corners[2])
{
	Triangle *triangle = (Triangle*)object;
	memcpy(corners[0], triangle->vertices[2], sizeof(Vec3));
	memcpy(corners[1], triangle->vertices[2], sizeof(Vec3));
	size_t i, j;
	for (i = 0; i < 2; i++)
		for (j = 0; j < 3; j++) {
			if (corners[0][j] > triangle->vertices[i][j])
				corners[0][j] = triangle->vertices[i][j];
			else if (corners[1][j] < triangle->vertices[i][j])
				corners[1][j] = triangle->vertices[i][j];
		}
}

void triangle_scale(const Object *object, const Vec3 neg_shift, const float scale)
{
	Triangle *triangle = (Triangle*)object;
	triangle->object.epsilon *= scale;
	size_t i;
	for (i = 0; i < 3; i++) {
		sub3v(triangle->vertices[i], neg_shift, triangle->vertices[i]);
		mul3s(triangle->vertices[i], scale, triangle->vertices[i]);
	}
	for (i = 0; i < 2; i++)
		mul3s(triangle->edges[i], scale, triangle->edges[i]);
}

void triangle_get_light_point(const Object *object, const Vec3 point, Vec3 light_point)
{
	//NOTE: this method may be inefficient due to the 3 square root operations, but it is unlikely to be used often
	(void)point;
	Triangle *triangle = (Triangle*)object;
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
bool moller_trumbore(const Vec3 vertex, Vec3 edges[2], const Vec3 line_position, const Vec3 line_vector, const float epsilon, float *distance)
{
	float a, f, u, v;
	Vec3 h, s, q;
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
void plane_postinit(Object *object)
{
	Plane *plane = (Plane*)object;

	error_check(!plane->object.material->emittant, "Plane cannot be emittant");
	if (plane->object.epsilon == -1.f)
		plane->object.epsilon = 1.e-6f;
}

Object *plane_new(Vec3 position, Vec3 normal)
{
	Plane *plane = safe_malloc(sizeof(Plane));

	memcpy(plane->normal, normal, sizeof(Vec3));
	norm3(plane->normal);
	plane->d = dot3(plane->normal, position);

	return (Object*)plane;
}

void plane_delete(Object *object)
{
	free(object);
}

bool plane_get_intersection(const Object *object, const Line *ray, float *distance, Vec3 normal)
{
	Plane *plane = (Plane*)object;
	float a = dot3(plane->normal, ray->vector);
	if (fabsf(a) < plane->object.epsilon) //ray is parallel to line
		return false;
	*distance = (plane->d - dot3(plane->normal, ray->position)) / dot3(plane->normal, ray->vector);
	if (*distance > plane->object.epsilon) {
		if (signbit(a))
			memcpy(normal, plane->normal, sizeof(Vec3));
		else
			mul3s(plane->normal, -1.f, normal);
		return true;
	}
	return false;
}

bool plane_intersects_in_range(const Object *object, const Line *ray, float min_distance)
{
	Plane *plane = (Plane*)object;
	float a = dot3(plane->normal, ray->vector);
	if (fabsf(a) < plane->object.epsilon) //ray is parallel to line
		return false;
	float distance = (plane->d - dot3(plane->normal, ray->position)) / dot3(plane->normal, ray->vector);
	return distance > plane->object.epsilon && distance < min_distance;
}

void plane_scale(const Object *object, const Vec3 neg_shift, const float scale)
{
	Plane *plane = (Plane*)object;
	Vec3 point = {1.f, 1.f, 1.f};
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

void mesh_to_objects(const char *filename, Object *object, const Vec3 position, const Vec3 rotation, const float scale, size_t *i_object)
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
void stl_load_objects(FILE *file, const char *filename, Object *object, const Vec3 position, const Vec3 rot, const float scale, size_t *i_object)
{
	//ensure that file is binary instead of ascii
	char header[5];
	size_t nmemb_read = fread(header, sizeof(char), 5, file);
	error_check(nmemb_read == 5, "Failed to read header of mesh file [%s].", filename);
	error_check(strncmp("solid", header, 5), "Mesh file [%s] does not use binary encoding.", filename);

	float a = cosf(rot[Z]) * sinf(rot[Y]);
	float b = sinf(rot[Z]) * sinf(rot[Y]);
	Mat3 rotation_matrix = {
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
	objects = safe_realloc(objects, sizeof(Object) * num_objects);

	uint32_t i;
	for (i = 0; i < num_triangles; i++) {
		STLTriangle stl_triangle;
		nmemb_read = fread(&stl_triangle, sizeof(STLTriangle), 1, file);
		error_check(nmemb_read == 1, "Failed to read triangle in mesh file [%s].", filename);

		uint32_t j;
		for (j = 0; j < 3; j++) {
			Vec3 temp_vertices;
			mulm3(rotation_matrix, stl_triangle.vertices[j], temp_vertices);
			mul3s(temp_vertices, scale, stl_triangle.vertices[j]);
			add3v(stl_triangle.vertices[j], position, stl_triangle.vertices[j]);
		}
		Object *triangle = triangle_new(stl_triangle.vertices);
		memcpy(triangle, object, sizeof(Object));
		triangle_postinit(triangle);
		objects[*i_object] = triangle;
		*i_object += 1;
	}
}
