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

#include "camera.h"
#include "error.h"

Camera camera;

void camera_init(const Vec3 position, Vec3 vectors[2], const float fov, const float focal_length)
{
	printf_log("Initializing camera.");

	error_check(fov > 0.f && fov < 180.f, "Expected camera fov [%.2f] between [0.] and [180.].", (double)fov);

	camera.fov = fov;
	camera.focal_length = focal_length;

	memcpy(camera.position, position, sizeof(Vec3));
	memcpy(camera.vectors, vectors, sizeof(Vec3) * 2);
	norm3(camera.vectors[0]);
	norm3(camera.vectors[1]);
	cross(camera.vectors[0], camera.vectors[1], camera.vectors[2]);
}

void camera_scale(const Vec3 neg_shift, const float scale)
{
	sub3v(camera.position, neg_shift, camera.position);
	mul3s(camera.position, scale, camera.position);
	camera.focal_length *= scale;
}
