/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Ray-object intersection acceleration
 **/

#ifndef __ACCEL_H__
#define __ACCEL_H__

#include "type.h"

struct Ray;
struct Object;

void accel_init(void);
void accel_deinit(void);

void accel_get_closest_intersection(const struct Ray *ray, struct Object **closest_object, v3 closest_normal, float *closest_distance);
bool accel_is_light_blocked(const struct Ray *ray, const float distance, v3 light_intensity, const struct Object *emittant_object);
#ifdef DEBUG
void accel_print(uint32_t depth);
#endif

#endif /* __ACCEL_H__ */
