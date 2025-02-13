/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Image rendering
 **/

#ifndef __RENDER_H__
#define __RENDER_H__

#include "type.h"

void render_init(void);
void render(void);

extern v3 global_ambient_light_intensity;

#endif /* __RENDER_H__ */
