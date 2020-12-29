#ifndef SCENE_H
#define SCENE_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <errno.h>

#include "global.h"
#include "objects.h"
#include "stl.h"

#define MAX_JSON_TOKENS 32768

#define NUM_SCENE_ELEM_TYPES 8
#define NUM_CAMERA_ELEMS 3
#define NUM_OBJECT_ELEMS 5
#define NUM_SPHERE_ELEMS 2
#define NUM_TRIANGLE_ELEMS 3
#define NUM_PLANE_ELEMS 2
#define NUM_MESH_ELEMS 4
#define NUM_LIGHT_ELEMS 2

#define DYNAMIC_ARRAY_INCREMENT 3

enum scene_elems{CAMERA, SPHERE, TRIANGLE, PLANE, MESH, LIGHT, AMBIENTLIGHT, EPSILON};
enum camera_elems{CAMERA_POSITION, CAMERA_VECTOR1, CAMERA_VECTOR2};
enum object_elems{OBJECT_KS, OBJECT_KD, OBJECT_KA, OBJECT_KR, OBJECT_SHININESS};
enum sphere_elems{SPHERE_POSITION, SPHERE_RADIUS};
enum triangle_elems{TRIANGLE_VERTEX1, TRIANGLE_VERTEX2, TRIANGLE_VERTEX3};
enum plane_elems{PLANE_POSITION, PLANE_NORMAL};
enum mesh_elems{MESH_FILENAME, MESH_POSITION, MESH_ROTATION, MESH_SCALE};
enum light_elems{LIGHT_POSITION, LIGHT_INTENSITY};

void scene_load(FILE *scene_file, Camera **camera, int *num_objects, Object **objects, int *num_lights, Light **lights, Vec3 ambient_light_intensity, int resolution[2], Vec2 image_size, float focal_length);

#endif
