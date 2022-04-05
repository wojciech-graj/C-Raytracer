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
#include "calc.h"

#include <stdint.h>
#include <stddef.h>

typedef enum _PeriodicFunction {
	PERIODIC_FUNC_SIN,
	PERIODIC_FUNC_SAW,
	PERIODIC_FUNC_TRIANGLE,
	PERIODIC_FUNC_SQUARE,
} PeriodicFunction;

typedef struct _Texture Texture;

struct _Texture {
	void (*get_color)(const Texture*, const Vec3, Vec3);
};

typedef struct _Material {
	int32_t id;
	Vec3 ks; /*specular reflection constant*/
	Vec3 ka; /*ambient reflection constant*/
	Vec3 kr; /*specular interreflection constant*/
	Vec3 kt; /*transparency constant*/
	Vec3 ke; /*emittance constant*/
	float shininess; /*shininess constant*/
	float refractive_index;
	Texture *texture;
	bool reflective;
	bool transparent;
	bool emittant;
} Material;

/* requires num_materials to be set. */
void materials_init(void);
void materials_deinit(void);

void material_init(Material *material, int32_t id, const Vec3 ks, const Vec3 ka, const Vec3 kr, const Vec3 kt, const Vec3 ke, float shininess, float refractive_index, Texture * const texture);

Texture *texture_uniform_new(const Vec3 color);
Texture *texture_checkerboard_new(Vec3 colors[2], float scale);
Texture *texture_brick_new(Vec3 colors[2], float scale, float mortar_width);
Texture *texture_noisy_periodic_new(const Vec3 color, const Vec3 color_gradient, float noise_feature_scale, float noise_scale, float frequency_scale, PeriodicFunction func);

Material *get_material(int32_t id);

extern Material *materials;
extern size_t num_materials;

#endif /* __MATERIAL_H__ */
