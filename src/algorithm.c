#include "algorithm.h"

//Möller–Trumbore intersection algorithm
bool moller_trumbore(Vec3 vertex, Vec3 edges[2], Vec3 line_position, Vec3 line_vector, float epsilon, float *distance)
{
	Vec3 h, s, q;
	cross(line_vector, edges[1], h);
	float a = dot3(edges[0], h);
	if(a < epsilon && a > -epsilon) //ray is parallel to line
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
	if(*distance > epsilon)
		return true;
	else
		return false;
}

bool line_intersects_sphere(Vec3 sphere_position, float sphere_radius, Vec3 line_position, Vec3 line_vector, float epsilon, float *distance)
{
	Vec3 relative_position;
	subtract3(line_position, sphere_position, relative_position);
	//should be a = dot3(line_vector, line_vector), however dot of normalized vector gives 1
	float b = -2 * dot3(line_vector, relative_position);
	float c = dot3(relative_position, relative_position) - sqr(sphere_radius);
	float determinant = sqr(b) - 4 * c;
	if(determinant < 0) //no collision
		return false;
	else {
		float sqrt_determinant = sqrtf(determinant);
		*distance = (b - sqrt_determinant) * .5f;
		if(*distance > epsilon)//if in front of origin of ray
			return true;
		else {//check if the further distance is positive
			*distance = (b + sqrt_determinant) * .5f;
			if(*distance > epsilon)
				return true;
			else
				return false;
		}

	}
}

// D. J. Bernstein hash function
uint32_t djb_hash(const char* cp)
{
    uint32_t hash = 5381;
    while (*cp)
        hash = 33 * hash ^ (unsigned char) *cp++;
    return hash;
}
