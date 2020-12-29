#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <stdbool.h>
#include <stdint.h>

#include "global.h"
#include "vector.h"

bool moller_trumbore(Vec3 vertex, Vec3 edges[2], Vec3 line_position, Vec3 line_vector, float *distance);
bool line_intersects_sphere(Vec3 sphere_position, float sphere_radius, Vec3 line_position, Vec3 line_vector, float *distance);
uint32_t djb_hash(const char* cp);

#endif
