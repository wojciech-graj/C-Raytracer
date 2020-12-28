#include "objects.h"

/*************************************************************
*	BOUNDING SPHERE
*/

typedef struct BoundingSphere {
	BOUNDING_SHAPE_PARAMS
	Vec3 position;
	float radius;
} BoundingSphere;

bool intersects_bounding_sphere(BoundingShape shape, Line *ray)
{
	BoundingSphere *sphere = shape.sphere;
	float distance;//NOTE: maybe this value could be used to check if something is in front of the mesh, and if so, don't check the triangles
	return line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, &distance);
}

BoundingSphere *init_bounding_sphere(Vec3 position, float radius)
{
	BOUNDING_SHAPE_INIT(BoundingSphere, bounding_sphere);
	memcpy(bounding_sphere->position, position, sizeof(Vec3));
	bounding_sphere->radius = radius;
	return bounding_sphere;
}

/*************************************************************
*	POLYGON
*/

typedef struct PolyTriangle {//triangle ABC
	Vec3 vertices[3];
	Vec3 edges[2]; //Vectors BA and CA
	Vec3 normal;
} PolyTriangle;

typedef struct Mesh {
	OBJECT_PARAMS
	Vec3 position;
	uint32_t num_triangles;
	PolyTriangle *triangles;
	BoundingShape bounding_shape;
} Mesh;

bool get_intersection_mesh(Object object, Line *ray, float *distance, Vec3 normal)
{
	Mesh *mesh = object.mesh;
	if(! mesh->bounding_shape.common->intersects(mesh->bounding_shape, ray))
		return false;
	else {
		#ifndef SHOW_BOUNDING_SHAPES
		*distance = FLT_MAX;
		PolyTriangle *closest_triangle = NULL;
		float cur_distance;
		uint32_t i;
		for(i = 0; i < mesh->num_triangles; i++)
		{
			PolyTriangle *triangle = &mesh->triangles[i];
			if(moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, &cur_distance)) {
				if(cur_distance < *distance) {
					*distance = cur_distance;
					closest_triangle = triangle;
				}
			}
		}
		if(*distance == FLT_MAX)
			return false;
		else {
			memcpy(normal, closest_triangle->normal, sizeof(Vec3));
			return true;
		}
		#else
			memcpy(normal, mesh->triangles[0].normal, sizeof(Vec3));
			*distance = 1.f;
			return true;
		#endif
	}
}

bool intersects_in_range_mesh(Object object, Line *ray, float min_distance)
{
	Mesh *mesh = object.mesh;
	if(mesh->bounding_shape.common->intersects(mesh->bounding_shape, ray)) {
		float distance;
		uint32_t i;
		for(i = 0; i < mesh->num_triangles; i++)
		{
			PolyTriangle *triangle = &mesh->triangles[i];
			if(moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, &distance)) {
				if(distance < min_distance)
					return true;
			}
		}
	}
	return false;
}

void delete_mesh(Object object)
{
	Mesh *mesh = object.mesh;
	free(mesh->triangles);
	free(mesh->bounding_shape.common);
	free(mesh);
}

Mesh *init_mesh(OBJECT_INIT_PARAMS, uint32_t num_triangles)
{
	OBJECT_INIT(Mesh, mesh);
	mesh->num_triangles = num_triangles;
	mesh->triangles = malloc(sizeof(PolyTriangle) * num_triangles);
	return mesh;
}

void mesh_set_triangle(Mesh *mesh, uint32_t index, Vec3 vertices[3])
{
	PolyTriangle *triangle = &mesh->triangles[index];
	memcpy(triangle->vertices, vertices, sizeof(Vec3[3]));
	subtract3(vertices[1], vertices[0], triangle->edges[0]);
	subtract3(vertices[2], vertices[0], triangle->edges[1]);
	cross(triangle->edges[0], triangle->edges[1], triangle->normal);
}

void mesh_generate_bounding_sphere(Mesh *mesh)
{
	//Ritter's bounding sphere algorithm
	Vec3 min_points[3] = {
		{FLT_MAX, 0.f, 0.f},
		{0.f, FLT_MAX, 0.f},
		{0.f, 0.f, FLT_MAX}};
	Vec3 max_points[3] = {
		{FLT_MIN, 0.f, 0.f},
		{0.f, FLT_MIN, 0.f},
		{0.f, 0.f, FLT_MIN}};
	uint32_t triangle_index;
	int vertex_index, direction;
	for(triangle_index = 0; triangle_index < mesh->num_triangles; triangle_index++)
	{
		for(vertex_index = 0; vertex_index < 3; vertex_index++)
		{
			float *vertex = mesh->triangles[triangle_index].vertices[vertex_index];
			for(direction = 0; direction < 3; direction++)
			{
				if(vertex[direction] > max_points[direction][direction])
					memcpy(max_points[direction], vertex, sizeof(Vec3));
				if(vertex[direction] < min_points[direction][direction])
					memcpy(min_points[direction], vertex, sizeof(Vec3));
			}
		}
	}

	Vec3 distance_vectors[3];
	direction = -1;
	float max_distance = FLT_MIN;
	int i;
	for(i = 0; i < 3; i++)
	{
		subtract3(max_points[i], min_points[i], distance_vectors[i]);
		float distance = magnitude3(distance_vectors[i]);
		if(max_distance < distance) {
			max_distance = distance;
			direction = i;
		}
	}

	Vec3 sphere_position;
	multiply3(distance_vectors[direction], .5f, sphere_position);
	add3(sphere_position, min_points[direction], sphere_position);
	float sphere_radius = .5f * max_distance;
	float sphere_radius_sqr = sqr(sphere_radius);
	for(triangle_index = 0; triangle_index < mesh->num_triangles; triangle_index++)
	{
		for(vertex_index = 0; vertex_index < 3; vertex_index++)
		{
			float *vertex = mesh->triangles[triangle_index].vertices[vertex_index];
			Vec3 sphere_to_point;
			subtract3(vertex, sphere_position, sphere_to_point);
			float distance_sqr = sqr(sphere_to_point[0]) + sqr(sphere_to_point[1]) + sqr(sphere_to_point[2]);
			if(sphere_radius_sqr < distance_sqr) {
				float half_distance = .5f * (sqrtf(distance_sqr) - sphere_radius) + EPSILON;
				sphere_radius += half_distance;
				sphere_radius_sqr = sqr(sphere_radius);
				normalize3(sphere_to_point);
				multiply3(sphere_to_point, half_distance, sphere_to_point);
				add3(sphere_position, sphere_to_point, sphere_position);
			}
		}
	}

	mesh->bounding_shape.sphere = init_bounding_sphere(sphere_position, sphere_radius);
}

/*************************************************************
*	SPHERE
*/

typedef struct Sphere {
	OBJECT_PARAMS
	Vec3 position;
	float radius;
} Sphere;

bool get_intersection_sphere(Object object, Line *ray, float *distance, Vec3 normal)
{
	Sphere *sphere = object.sphere;
	bool intersects = line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, distance);
	if(intersects) {//TODO: optimize?
		Vec3 position;
		multiply3(ray->vector, *distance, position);
		add3(position, ray->position, position);
		subtract3(position, sphere->position, normal);
		return true;
	} else
		return false;
}

bool intersects_in_range_sphere(Object object, Line *ray, float min_distance)
{
	Sphere *sphere = object.sphere;
	float distance;
	bool intersects = line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, &distance);
	if(intersects && distance < min_distance)
		return true;
	else
		return false;
}

void delete_sphere(Object object)
{
	free(object.common);
}

Sphere *init_sphere(OBJECT_INIT_PARAMS, Vec3 position, float radius)
{
	OBJECT_INIT(Sphere, sphere);
	memcpy(sphere->position, position, sizeof(Vec3));
	sphere->radius = radius;
	return sphere;
}

/*************************************************************
*	TRIANGLE
*/

typedef struct Triangle {//triangle ABC
	OBJECT_PARAMS
	Vec3 vertices[3];
	Vec3 edges[2]; //Vectors BA and CA
	Vec3 normal;
} Triangle;

bool get_intersection_triangle(Object object, Line *ray, float *distance, Vec3 normal)
{
	Triangle *triangle = object.triangle;
	bool intersects = moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, distance);
	if(intersects) {
		memcpy(normal, triangle->normal, sizeof(Vec3));
		return true;
	} else
		return false;
}

bool intersects_in_range_triangle(Object object, Line *ray, float min_distance)
{
	Triangle *triangle = object.triangle;
	float distance;
	bool intersects = moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, &distance);
	if(intersects && distance < min_distance)
		return true;
	else
		return false;
}

void delete_triangle(Object object)
{
	free(object.common);
}

Triangle *init_triangle(OBJECT_INIT_PARAMS, Vec3 vertices[3])
{
	OBJECT_INIT(Triangle, triangle);
	memcpy(triangle->vertices, vertices, sizeof(Vec3[3]));
	subtract3(vertices[1], vertices[0], triangle->edges[0]);
	subtract3(vertices[2], vertices[0], triangle->edges[1]);
	cross(triangle->edges[0], triangle->edges[1], triangle->normal);
	return triangle;
}

/*************************************************************
*	PLANE
*/

typedef struct Plane {//normal = {a,b,c}, ax + by + cz = d
	OBJECT_PARAMS
	Vec3 normal;
	float d;
} Plane;

bool get_intersection_plane(Object object, Line *ray, float *distance, Vec3 normal)
{
	Plane *plane = object.plane;
	float a = dot3(plane->normal, ray->vector);
	if(a < EPSILON && a > -EPSILON) //ray is parallel to line
		return false;
	*distance = (plane->d - dot3(plane->normal, ray->position)) / dot3(plane->normal, ray->vector);
	if(*distance > EPSILON) {
		memcpy(normal, plane->normal, sizeof(Vec3));
		return true;
	} else
		return false;
}

bool intersects_in_range_plane(Object object, Line *ray, float min_distance)
{
	Plane *plane = object.plane;
	float a = dot3(plane->normal, ray->vector);
	if(a < EPSILON && a > -EPSILON) //ray is parallel to line
		return false;
	float distance = (plane->d - dot3(plane->normal, ray->position)) / dot3(plane->normal, ray->vector);
	if(distance > EPSILON && distance < min_distance)
		return true;
	else
		return false;
}

void delete_plane(Object object)
{
	free(object.common);
}

Plane *init_plane(OBJECT_INIT_PARAMS, Vec3 normal, Vec3 point)
{
	OBJECT_INIT(Plane, plane);
	memcpy(plane->normal, normal, sizeof(Vec3));
	plane->d = dot3(normal, point);
	return plane;
}
