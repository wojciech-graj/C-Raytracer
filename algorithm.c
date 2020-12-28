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
