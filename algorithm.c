#include "algorithm.h"

//Möller–Trumbore intersection algorithm
bool moller_trumbore(Vec3 vertex, Vec3 edges[2], Vec3 line_position, Vec3 line_vector, float *distance)
{
	Vec3 h, s, q;
	cross(line_vector, edges[1], h);
	float a = dot3(edges[0], h);
	if(a < EPSILON && a > -EPSILON) //ray is parallel to line
		return false;
	float f = 1.f / a;
	subtract3(line_position, vertex, s);
	float u = f * dot3(s, h);
	if(u < 0.f || u > 1.f)
		return false;
	cross(s, edges[0], q);
	float v = f * dot3(line_vector, q);
	if(v < 0.f || u + v > 1.f)
		return false;
	*distance = f * dot3(edges[1], q);
	if(*distance > EPSILON)
		return true;
	else
		return false;
}

bool line_intersects_sphere(Vec3 sphere_position, float sphere_radius, Vec3 line_position, Vec3 line_vector, float *distance)
{
	Vec3 relative_position;
	subtract3(line_position, sphere_position, relative_position);
	float a = dot3(line_vector, line_vector);
	float b = 2 * dot3(line_vector, relative_position);
	float c = dot3(relative_position, relative_position) - sqr(sphere_radius);
	float determinant = sqr(b) - 4 * a * c;
	if(determinant < 0) //no collision
		return false;
	else {
		*distance = (sqrtf(determinant) + b) / (-2 * a);
		if(*distance > EPSILON)//if in front of origin of ray
			return true;
		else
			return false;
	}
}
