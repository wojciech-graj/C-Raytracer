#include "objects.h"

/*************************************************************
*	BOUNDING SPHERE
*/

typedef struct BoundingSphere {
	BOUNDING_SHAPE_PARAMS
	Vec3 position;
	double radius;
} BoundingSphere;

bool intersects_bounding_sphere(BoundingShape shape, Line *ray)
{
	BoundingSphere *sphere = shape.sphere;
	float distance;//NOTE: maybe this value could be used to check if something is in front of the mesh, and if so, don't check the triangles
	return line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, &distance);
}

BoundingSphere *init_bounding_sphere(Vec3 position, double radius)
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
	PolyTriangle *closest_triangle;
	BoundingShape bounding_shape;
} Mesh;

bool intersects_mesh(Object object, Line *ray, float *distance)
{
	Mesh *mesh = object.mesh;
	if(! mesh->bounding_shape.common->intersects(mesh->bounding_shape, ray))
		return false;
	else {
		#ifndef SHOW_BOUNDING_SHAPES
		*distance = FLT_MAX;
		float cur_distance;
		uint32_t i;
		for(i = 0; i < mesh->num_triangles; i++)
		{
			PolyTriangle *triangle = &mesh->triangles[i];
			if(moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, &cur_distance)) {
				if(cur_distance < *distance) {
					*distance = cur_distance;
					mesh->closest_triangle = triangle;
				}
			}
		}
		if(*distance == FLT_MAX)
			return false;
		else
			return true;
		#else
			mesh->closest_triangle = &mesh->triangles[0];
			*distance = 1.f;
			return true;
		#endif
	}
}

void get_normal_mesh(Object object, Line *ray, Vec3 position, Vec3 normal)
{
	(void)position;
	Mesh *mesh = object.mesh;
	if(dot3(mesh->closest_triangle->normal, ray->vector) < 0.f) {
		memcpy(normal, mesh->closest_triangle->normal, sizeof(Vec3));
	} else {
		multiply3(mesh->closest_triangle->normal, -1.f, normal);
	}
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
	mesh->closest_triangle = NULL;
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

bool intersects_sphere(Object object, Line *ray, float *distance)
{
	Sphere *sphere = object.sphere;
	return line_intersects_sphere(sphere->position, sphere->radius, ray->position, ray->vector, distance);
}

void get_normal_sphere(Object object, Line *ray, Vec3 position, Vec3 normal)
{
	(void)ray;
	Sphere *sphere = object.sphere;
	subtract3(position, sphere->position, normal);
	if(dot3(normal, ray->vector) > 0) //if collision from within sphere
		multiply3(normal, -1, normal);
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

bool intersects_triangle(Object object, Line *ray, float *distance)
{
	Triangle *triangle = object.triangle;
	return moller_trumbore(triangle->vertices[0], triangle->edges, ray->position, ray->vector, distance);
}

void get_normal_triangle(Object object, Line *ray, Vec3 position, Vec3 normal)
{
	(void)position;
	Triangle *triangle = object.triangle;
	if(dot3(triangle->normal, ray->vector) < 0.f)
		memcpy(normal, triangle->normal, sizeof(Vec3));
	else
		multiply3(triangle->normal, -1.f, normal);
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

bool intersects_plane(Object object, Line *ray, float *distance)
{
	Plane *plane = object.plane;
	float a = dot3(plane->normal, ray->vector);
	if(a < EPSILON && a > -EPSILON) //ray is parallel to line
		return false;
	*distance = (plane->d - dot3(plane->normal, ray->position)) / dot3(plane->normal, ray->vector);
	if(*distance > EPSILON)
		return true;
	else
		return false;
}

void get_normal_plane(Object object, Line *ray, Vec3 position, Vec3 normal)
{
	(void)position;
	Plane *plane = object.plane;
	if(dot3(plane->normal, ray->vector) < 0.f)
		memcpy(normal, plane->normal, sizeof(Vec3));
	else
		multiply3(plane->normal, -1.f, normal);
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
