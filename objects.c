#include "objects.h"

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
} Mesh;

bool intersects_mesh(Object object, Line *ray, float *distance)
{//TODO: use a box around mesh
	Mesh *mesh = object.mesh;
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
	Vec3 relative_position;
	subtract3(ray->position, sphere->position, relative_position);
	float a = dot3(ray->vector, ray->vector);
	float b = 2 * dot3(ray->vector, relative_position);
	float c = dot3(relative_position, relative_position) - sqr(sphere->radius);
	float determinant = sqr(b) - 4 * a * c;
	if(determinant < 0) //no collision
		return false;
	else {
		*distance = (sqrtf(determinant) + b) / (-2 * a);
		if(*distance > 0)//if in front of origin of ray
			return true;
		else
			return false;
	}
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
