#ifndef STL_H
#define STL_H

#include <stdio.h>
#include <stdint.h>

#include "global.h"
#include "objects.h"

typedef struct  STLTriangle {
	float normal[3];//normal is unreliable so it is not used.
	float vertices[3][3];
	uint16_t attribute_bytes;//attribute bytes is unreliable so it is not used.
} __attribute__ ((packed)) STLTriangle;

Mesh *stl_load(OBJECT_INIT_PARAMS, FILE *file, Vec3 position, Vec3 rot, float scale);

#endif
