/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Image parameters and frame buffer
 **/

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "type.h"

struct Image {
	uint32_t resolution[2];
	size_t pixels;

	v3 *raster;
	float *z_buffer;
};

void image_load(void);
void image_deinit(void);

void save_image(void);

extern struct Image image;

#endif /* __IMAGE_H__ */
