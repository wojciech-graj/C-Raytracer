/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Camera used when generating image
 **/

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "calc.h"

typedef struct _Camera {
	Vec3 position;
	Vec3 vectors[3]; //vectors are perpendicular to eachother and normalized. vectors[3] is normal to projection_plane.
	float fov;
	float focal_length;
} Camera;

void camera_init(const Vec3 position, Vec3 vectors[2], float fov, float focal_length);
void camera_scale(const Vec3 neg_shift, float scale);

extern Camera camera;

#endif /* __CAMERA_H__ */
