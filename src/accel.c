/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Ray-Object intersection acceleration through Bounding Volume Heirarchy using bounding cuboids
 **/

#include "accel.h"
#include "mem.h"
#include "calc.h"
#include "system.h"

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>

typedef struct _BoundingCuboid {
	float epsilon;
	Vec3 corners[2];
} BoundingCuboid;

typedef struct _BVH BVH;

typedef union _BVHChild {
	BVH *bvh;
	Object *object;
} BVHChild;

struct _BVH {
	bool is_leaf;
	BoundingCuboid *bounding_cuboid;
	BVHChild children[];
};

typedef struct _BVHWithMorton { //Only used when constructing BVH tree
	uint32_t morton_code;
	BVH *bvh;
} BVHWithMorton;

/* Helper Funcs */
uint32_t expand_bits(uint32_t num);
uint32_t morton_code(const Vec3 vec);

/* BoundingCuboid */
BoundingCuboid *bounding_cuboid_new(const float epsilon, Vec3 corners[2]);
BoundingCuboid *bounding_cuboid_new_from_object(const Object *object);
void bounding_cuboid_delete(BoundingCuboid *bounding_cuboid);
bool bounding_cuboid_intersects(const BoundingCuboid *cuboid, const Line *ray, float *tmax, float *tmin);

/* BVH */
BVH *bvh_new(bool is_leaf, const BoundingCuboid *bounding_cuboid);
int bvh_morton_code_compare(const void *p1, const void *p2);
BoundingCuboid *bvh_generate_bounding_cuboid_leaf(const BVHWithMorton *leaf_array, const size_t first, const size_t last);
BoundingCuboid *bvh_generate_bounding_cuboid_node(const BVH *bvh_left, const BVH *bvh_right);
BVH *bvh_generate_node(const BVHWithMorton *leaf_array, const size_t first, const size_t last);
void bvh_delete(BVH *bvh);
void bvh_get_closest_intersection(const BVH *bvh, const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance);
bool bvh_is_light_blocked(const BVH *bvh, const Line *ray, float distance, Vec3 light_intensity, const Object *emittant_object);

static BVH *accel;

//Expands a number to only use 1 in every 3 bits
// Adapted from https://developer.nvidia.com/blog/thinking-parallel-part-iii-tree-construction-gpu/
uint32_t expand_bits(uint32_t num)
{
	num = (num * 0x00010001u) & 0xFF0000FFu;
	num = (num * 0x00000101u) & 0x0F00F00Fu;
	num = (num * 0x00000011u) & 0xC30C30C3u;
	num = (num * 0x00000005u) & 0x49249249u;
	return num;
}

//Calcualtes 30-bit morton code for a point in cube [0,1]. Does not perform bounds checking
// Adapted from https://developer.nvidia.com/blog/thinking-parallel-part-iii-tree-construction-gpu/
uint32_t morton_code(const Vec3 vec)
{
	return expand_bits((uint32_t)(1023.f * vec[X])) * 4u
		+ expand_bits((uint32_t)(1023.f * vec[Y])) * 2u
		+ expand_bits((uint32_t)(1023.f * vec[Z]));
}

BoundingCuboid *bounding_cuboid_new(const float epsilon, Vec3 corners[2])
{
	BoundingCuboid *bounding_cuboid = safe_malloc(sizeof(BoundingCuboid));
	bounding_cuboid->epsilon = epsilon;
	memcpy(bounding_cuboid->corners, corners, sizeof(Vec3) * 2);
	return bounding_cuboid;
}

BoundingCuboid *bounding_cuboid_new_from_object(const Object *object)
{
	BoundingCuboid *bounding_cuboid = safe_malloc(sizeof(BoundingCuboid));
	object->object_data->get_corners(object, bounding_cuboid->corners);
	bounding_cuboid->epsilon = object->epsilon;
	return bounding_cuboid;
}

void bounding_cuboid_delete(BoundingCuboid *bounding_cuboid)
{
	free(bounding_cuboid);
}

// Adapted from http://people.csail.mit.edu/amy/papers/box-jgt.pdf
bool bounding_cuboid_intersects(const BoundingCuboid *cuboid, const Line *ray, float *tmax, float *tmin)
{
	float tymin, tymax;

	float divx = 1 / ray->vector[X];
	if (divx >= 0) {
		*tmin = (cuboid->corners[0][X] - ray->position[X]) * divx;
		*tmax = (cuboid->corners[1][X] - ray->position[X]) * divx;
	} else {
		*tmin = (cuboid->corners[1][X] - ray->position[X]) * divx;
		*tmax = (cuboid->corners[0][X] - ray->position[X]) * divx;
	}
	float divy = 1 / ray->vector[Y];
	if (divy >= 0) {
		tymin = (cuboid->corners[0][Y] - ray->position[Y]) * divy;
		tymax = (cuboid->corners[1][Y] - ray->position[Y]) * divy;
	} else {
		tymin = (cuboid->corners[1][Y] - ray->position[Y]) * divy;
		tymax = (cuboid->corners[0][Y] - ray->position[Y]) * divy;
	}

	if ((*tmin > tymax) || (tymin > *tmax))
		return false;
	if (tymin > *tmin)
		*tmin = tymin;
	if (tymax < *tmax)
		*tmax = tymax;

	float tzmin, tzmax;

	float divz = 1 / ray->vector[Z];
	if (divz >= 0) {
		tzmin = (cuboid->corners[0][Z] - ray->position[Z]) * divz;
		tzmax = (cuboid->corners[1][Z] - ray->position[Z]) * divz;
	} else {
		tzmin = (cuboid->corners[1][Z] - ray->position[Z]) * divz;
		tzmax = (cuboid->corners[0][Z] - ray->position[Z]) * divz;
	}

	if (*tmin > tzmax || tzmin > *tmax)
		return false;
	if (tzmin > *tmin)
		*tmin = tzmin;
	if (tzmax < *tmax)
		*tmax = tzmax;
	return *tmax > cuboid->epsilon;
}

BVH *bvh_new(const bool is_leaf, const BoundingCuboid *bounding_cuboid)
{
	BVH *bvh = safe_malloc(sizeof(BVH) + (is_leaf ? 1 : 2) * sizeof(BVHChild));
	bvh->is_leaf = is_leaf;
	bvh->bounding_cuboid = bounding_cuboid;
	return bvh;
}

void accel_deinit(void)
{
	bvh_delete(accel);
}

void bvh_delete(BVH *bvh)
{
	bounding_cuboid_delete(bvh->bounding_cuboid);
	if (!bvh->is_leaf) {
		bvh_delete(bvh->children[0].bvh);
		bvh_delete(bvh->children[1].bvh);
	}
	free(bvh);
}

int bvh_morton_code_compare(const void *p1, const void *p2)
{
	return (int)((BVHWithMorton*)p1)->morton_code - (int)((BVHWithMorton*)p2)->morton_code;
}

BoundingCuboid *bvh_generate_bounding_cuboid_leaf(const BVHWithMorton *leaf_array, const size_t first, const size_t last)
{
	Vec3 corners[2] = {{FLT_MAX}, {FLT_MIN}};
	float epsilon = 0.f;
	size_t i, j;
	for (i = first; i <= last; i++) {
		BoundingCuboid *bounding_cuboid = leaf_array[i].bvh->bounding_cuboid;
		if (epsilon < bounding_cuboid->epsilon)
			epsilon = bounding_cuboid->epsilon;
		for (j = 0; j < 3; j++) {
			if (bounding_cuboid->corners[0][j] < corners[0][j])
				corners[0][j] = bounding_cuboid->corners[0][j];
			if (bounding_cuboid->corners[1][j] > corners[1][j])
				corners[1][j] = bounding_cuboid->corners[1][j];
		}
	}
	return bounding_cuboid_new(epsilon, corners);
}

BoundingCuboid *bvh_generate_bounding_cuboid_node(const BVH *bvh_left, const BVH *bvh_right)
{
	BoundingCuboid *left = bvh_left->bounding_cuboid, *right = bvh_right->bounding_cuboid;
	float epsilon = fmaxf(left->epsilon, right->epsilon);
	Vec3 corners[2] = {
		{
			fminf(left->corners[0][X], right->corners[0][X]),
			fminf(left->corners[0][Y], right->corners[0][Y]),
			fminf(left->corners[0][Z], right->corners[0][Z]),
		},
		{
			fmaxf(left->corners[1][X], right->corners[1][X]),
			fmaxf(left->corners[1][Y], right->corners[1][Y]),
			fmaxf(left->corners[1][Z], right->corners[1][Z]),
		},
	};
	return bounding_cuboid_new(epsilon, corners);
}

// Adapted from https://developer.nvidia.com/blog/thinking-parallel-part-iii-tree-construction-gpu/
BVH *bvh_generate_node(const BVHWithMorton *leaf_array, const size_t first, const size_t last)
{
	if (first == last)
		return leaf_array[first].bvh;

	uint32_t first_code = leaf_array[first].morton_code;
	uint32_t last_code = leaf_array[last].morton_code;

	size_t split;
	if (first_code == last_code) {
		split = (first + last) / 2;
	} else {
		split = first;
		uint32_t common_prefix = clz(first_code ^ last_code);
		size_t step = last - first;

		do {
			step = (step + 1) >> 1; // exponential decrease
			size_t new_split = split + step; // proposed new position

			if (new_split < last) {
				uint32_t split_code = leaf_array[new_split].morton_code;
				if (first_code ^ split_code) {
					uint32_t split_prefix = clz(first_code ^ split_code);
					if (split_prefix > common_prefix)
						split = new_split; // accept proposal
				}
			}
		} while (step > 1);
	}

	BVH *bvh_left = bvh_generate_node(leaf_array, first, split);
	BVH *bvh_right = bvh_generate_node(leaf_array, split + 1, last);
	BVH *bvh = bvh_new(false, bvh_generate_bounding_cuboid_node(bvh_left, bvh_right));
	bvh->children[0].bvh = bvh_left;
	bvh->children[1].bvh = bvh_right;
	return bvh;
}

void accel_init(void)
{
	printf_log("Generating BVH.");
#ifdef UNBOUND_OBJECTS
	size_t num_leaves = num_objects - num_unbound_objects;
#else
	size_t num_leaves = num_objects;
#endif
	BVHWithMorton *leaf_array = safe_malloc(sizeof(BVHWithMorton) * num_leaves);

	size_t i, j = 0;
	for (i = 0; i < num_objects; i++) {
		Object *object = objects[i];
#ifdef UNBOUND_OBJECTS
		if (object->object_data->is_bounded) {
#endif
			BVH *bvh = bvh_new(true, bounding_cuboid_new_from_object(object));
			bvh->children[0].object = object;
			leaf_array[j++].bvh = bvh;
#ifdef UNBOUND_OBJECTS
		}
#endif
	}

	Vec3 min, max;
	get_objects_extents(min, max);

	Vec3 mul;
	sub3v(max, min, mul);
	inv3(mul);

	/* Embed halving of bounding cuboid corners to get mean */
	mul3s(mul, 0.5f, mul);
	mul3s(min, 2.f, min);

	for (i = 0; i < num_leaves; i++) {
		BoundingCuboid *bounding_cuboid = leaf_array[i].bvh->bounding_cuboid;
		Vec3 norm_position;
		add3v(bounding_cuboid->corners[0], bounding_cuboid->corners[1], norm_position);
		mul3v(norm_position, mul, norm_position);
		sub3v(norm_position, min, norm_position);
		leaf_array[i].morton_code = morton_code(norm_position);
	}

	qsort(leaf_array, num_leaves, sizeof(BVHWithMorton), &bvh_morton_code_compare);

	accel = bvh_generate_node(leaf_array, 0, num_leaves - 1);

	free(leaf_array);
}

void accel_get_closest_intersection(const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance)
{
	bvh_get_closest_intersection(accel, ray, closest_object, closest_normal, closest_distance);
}

void bvh_get_closest_intersection(const BVH *bvh, const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance)
{
	if (bvh->is_leaf) {
		Vec3 normal;
		Object *object = bvh->children[0].object;
		float distance;
		if (object->object_data->get_intersection(object, ray, &distance, normal) && distance < *closest_distance) {
			*closest_distance = distance;
			*closest_object = object;
			memcpy(closest_normal, normal, sizeof(Vec3));
		}
		return;
	}

	bool intersect_l, intersect_r;
	float tmin_l, tmin_r, tmax;
	intersect_l = bounding_cuboid_intersects(bvh->children[0].bvh->bounding_cuboid, ray, &tmax, &tmin_l) && tmin_l < *closest_distance;
	intersect_r = bounding_cuboid_intersects(bvh->children[1].bvh->bounding_cuboid, ray, &tmax, &tmin_r) && tmin_r < *closest_distance;
	if (intersect_l && intersect_r) {
		if (tmin_l < tmin_r) {
			bvh_get_closest_intersection(bvh->children[0].bvh, ray, closest_object, closest_normal, closest_distance);
			bvh_get_closest_intersection(bvh->children[1].bvh, ray, closest_object, closest_normal, closest_distance);
		} else {
			bvh_get_closest_intersection(bvh->children[1].bvh, ray, closest_object, closest_normal, closest_distance);
			bvh_get_closest_intersection(bvh->children[0].bvh, ray, closest_object, closest_normal, closest_distance);
		}
	} else if (intersect_l) {
		bvh_get_closest_intersection(bvh->children[0].bvh, ray, closest_object, closest_normal, closest_distance);
	} else if (intersect_r) {
		bvh_get_closest_intersection(bvh->children[1].bvh, ray, closest_object, closest_normal, closest_distance);
	}
}

bool accel_is_light_blocked(const Line *ray, const float distance, Vec3 light_intensity, const Object *emittant_object)
{
	return bvh_is_light_blocked(accel, ray, distance, light_intensity, emittant_object);
}

bool bvh_is_light_blocked(const BVH *bvh, const Line *ray, const float distance, Vec3 light_intensity, const Object *emittant_object)
{
	float tmin, tmax;

	if (bvh->is_leaf) {
		Vec3 normal;
		Object *object = bvh->children[0].object;
		if (object == emittant_object)
			return false;
		if (object->object_data->get_intersection(object, ray, &tmin, normal) && tmin < distance) {
			if (object->material->transparent)
				mul3v(light_intensity, object->material->kt, light_intensity);
			else
				return true;
		}
		return false;
	}

	size_t i;
#pragma GCC unroll 2
	for (i = 0; i < 2; i++) {
		if (bounding_cuboid_intersects(bvh->children[i].bvh->bounding_cuboid, ray, &tmax, &tmin)
			&& tmin < distance
			&& bvh_is_light_blocked(bvh->children[i].bvh, ray, distance, light_intensity, emittant_object))
			return true;
	}
	return false;
}

#ifdef DEBUG
void bvh_print(const BVH *bvh, const uint32_t depth)
{
	uint32_t i;
	size_t j;
	for (i = 0; i < depth; i++)
		printf("\t");
	if (bvh->is_leaf) {
		printf("%s\n", bvh->children[0].object->object_data->name);
	} else {
		printf("NODE\n");
		for (j = 0; j < 2; j++)
			bvh_print(bvh->children[j].bvh, depth + 1);
	}
}
#endif /* DEBUG */
