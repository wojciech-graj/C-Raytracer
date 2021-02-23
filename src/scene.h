#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>
#include <stdio.h>

#include "global.h"
#include "objects.h"
#include "stl.h"

#define MAX_JSON_TOKENS 32768

#define NUM_SCENE_ELEM_TYPES 8
#define NUM_CAMERA_ELEMS 3
#define NUM_OBJECT_ELEMS 8
#define NUM_SPHERE_ELEMS 2
#define NUM_TRIANGLE_ELEMS 3
#define NUM_PLANE_ELEMS 2
#define NUM_MESH_ELEMS 4
#define NUM_LIGHT_ELEMS 2
#define NUM_LIGHTAREA_ELEMS 5

#define DYNAMIC_ARRAY_INCREMENT 3

void scene_load(FILE *scene_file, Camera **camera, unsigned *num_objects, Object **objects, unsigned *num_lights, Light **lights, Vec3 ambient_light_intensity, unsigned resolution[2], Vec2 image_size, float focal_length);

#endif
