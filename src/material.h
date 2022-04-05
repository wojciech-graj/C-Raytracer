/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   Materials applied to objects
 *   Procedural textures
 **/

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "type.h"

enum PeriodicFunction {
	PERIODIC_FUNC_SIN,
	PERIODIC_FUNC_SAW,
	PERIODIC_FUNC_TRIANGLE,
	PERIODIC_FUNC_SQUARE,
};

struct Texture;

struct Texture {
	void (*get_color)(const struct Texture*, const v3, v3);
};

struct Material {
	int32_t id;
	v3 ks; /*specular reflection constant*/
	v3 ka; /*ambient reflection constant*/
	v3 kr; /*specular interreflection constant*/
	v3 kt; /*transparency constant*/
	v3 ke; /*emittance constant*/
	float shininess; /*shininess constant*/
	float refractive_index;
	struct Texture *texture;
	bool reflective;
	bool transparent;
	bool emittant;
};

/* requires num_materials to be set. */
void materials_init(void);
void materials_deinit(void);

void material_init(struct Material *material, int32_t id, const v3 ks, const v3 ka, const v3 kr, const v3 kt, const v3 ke, float shininess, float refractive_index, struct Texture * const texture);

struct Texture *texture_uniform_new(const v3 color);
struct Texture *texture_checkerboard_new(v3 colors[2], float scale);
struct Texture *texture_brick_new(v3 colors[2], float scale, float mortar_width);
struct Texture *texture_noisy_periodic_new(const v3 color, const v3 color_gradient, float noise_feature_scale, float noise_scale, float frequency_scale, enum PeriodicFunction func);

struct Material *get_material(int32_t id);

extern struct Material *materials;
extern size_t num_materials;

#endif /* __MATERIAL_H__ */
