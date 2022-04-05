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
#include "calc.h"

#include <stdint.h>

typedef uint8_t Color[3];

typedef struct _Image {
	uint32_t resolution[2];
	Vec2 size;
	Vec3 corner; //Top left corner of image
	Vec3 vectors[2]; //Vectors for image plane traversal by 1 pixel in X and Y directions
	Color *pixels;
} Image;

void image_init(void);
void image_deinit(void);

void image_scale(const Vec3 neg_shift, float scale);
void save_image(void);

Image image;

#endif /* __IMAGE_H__ */
