#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <stdbool.h>

#include "global.h"
#include "vector.h"

bool moller_trumbore(Vec3 vertex, Vec3 edges[2], Vec3 line_position, Vec3 line_vector, float *distance);

#endif
