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

#include "material.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "calc.h"
#include "error.h"
#include "mem.h"

#include "SimplexNoise.h"

#define MATERIAL_THRESHOLD 1e-6f

struct TextureUniform {
	struct Texture texture;
	v3 color;
};

struct TextureCheckerboard {
	struct Texture texture;
	v3 colors[2];
	float scale;
};

struct TextureBrick {
	struct Texture texture;
	v3 colors[2];
	float scale;
	float mortar_width;
};

struct TextureNoisyPeriodic {
	struct Texture texture;
	v3 color;
	v3 color_gradient;
	float noise_feature_scale;
	float noise_scale;
	float frequency_scale;
	enum PeriodicFunction func;
};

void texture_get_color_uniform(const struct Texture *tex, const v3 point, v3 color);
void texture_get_color_checkerboard(const struct Texture *tex, const v3 point, v3 color);
void texture_get_color_brick(const struct Texture *tex, const v3 point, v3 color);
void texture_get_color_noisy_periodic(const struct Texture *tex, const v3 point, v3 color);

struct Material *materials;
size_t num_materials;

void materials_init(void)
{
	printf_log("Initializing materials.");

	materials = safe_malloc(sizeof(struct Material) * num_materials);
}

void material_init(struct Material *material, const int32_t id, const v3 ks, const v3 ka, const v3 kr, const v3 kt, const v3 ke, const float shininess, const float refractive_index, struct Texture *const texture)
{
	material->id = id;
	assign3(material->ks, ks);
	assign3(material->ka, ka);
	assign3(material->kr, kr);
	assign3(material->kt, kt);
	assign3(material->ke, ke);
	material->shininess = shininess;
	material->refractive_index = refractive_index;
	material->texture = texture;
	material->emittant = mag3(ke) > MATERIAL_THRESHOLD;
	material->reflective = mag3(kr) > MATERIAL_THRESHOLD;
	material->transparent = mag3(kt) > MATERIAL_THRESHOLD;
}

void materials_deinit(void)
{
	size_t i;
	for (i = 0; i < num_materials; i++)
		free(materials[i].texture);
	free(materials);
}

struct Material *get_material(const int32_t id)
{
	size_t i;
	for (i = 0; i < num_materials; i++)
		if (materials[i].id == id)
			return &materials[i];
	error("Failed to get material id [%d].", id);
	return NULL;
}

struct Texture *texture_uniform_new(const v3 color)
{
	struct TextureUniform *texture = safe_malloc(sizeof(struct TextureUniform));

	texture->texture.get_color = &texture_get_color_uniform;
	assign3(texture->color, color);

	return (struct Texture *)texture;
}

struct Texture *texture_checkerboard_new(v3 colors[2], const float scale)
{
	struct TextureCheckerboard *texture = safe_malloc(sizeof(struct TextureCheckerboard));

	texture->texture.get_color = &texture_get_color_checkerboard;
	texture->scale = scale;
	memcpy(texture->colors, colors, sizeof(v3[2]));

	return (struct Texture *)texture;
}

struct Texture *texture_brick_new(v3 colors[2], const float scale, const float mortar_width)
{
	struct TextureBrick *texture = safe_malloc(sizeof(struct TextureBrick));

	texture->texture.get_color = &texture_get_color_brick;
	texture->scale = scale;
	texture->mortar_width = mortar_width;
	memcpy(texture->colors, colors, sizeof(v3[2]));

	return (struct Texture *)texture;
}

struct Texture *texture_noisy_periodic_new(const v3 color, const v3 color_gradient, const float noise_feature_scale, const float noise_scale, const float frequency_scale, const enum PeriodicFunction func)
{
	struct TextureNoisyPeriodic *texture = safe_malloc(sizeof(struct TextureNoisyPeriodic));

	texture->texture.get_color = &texture_get_color_noisy_periodic;
	texture->noise_feature_scale = noise_feature_scale;
	texture->noise_scale = noise_scale;
	texture->frequency_scale = frequency_scale;
	texture->func = func;
	assign3(texture->color, color);
	assign3(texture->color_gradient, color_gradient);

	return (struct Texture *)texture;
}

void texture_get_color_uniform(const struct Texture *tex, const v3 point, v3 color)
{
	(void)point;
	struct TextureUniform *texture = (struct TextureUniform *)tex;
	assign3(color, texture->color);
}

void texture_get_color_checkerboard(const struct Texture *tex, const v3 point, v3 color)
{
	struct TextureCheckerboard *texture = (struct TextureCheckerboard *)tex;
	v3 scaled_point;
	mul3s(point, texture->scale, scaled_point);
	uint32_t parity = ((uint32_t)scaled_point[X] + (uint32_t)scaled_point[Y] + (uint32_t)scaled_point[Z]) % 2u;
	assign3(color, texture->colors[parity]);
}

void texture_get_color_brick(const struct Texture *tex, const v3 point, v3 color)
{
	struct TextureBrick *texture = (struct TextureBrick *)tex;
	v3 scaled_point;
	mul3s(point, texture->scale, scaled_point);
	uint32_t parity = (uint32_t)scaled_point[X] % 2u;
	scaled_point[Y] -= parity * .5f;
	uint32_t is_mortar = (scaled_point[X] - floorf(scaled_point[X]) < texture->mortar_width) || (scaled_point[Y] - floorf(scaled_point[Y]) < texture->mortar_width);
	assign3(color, texture->colors[is_mortar]);
}

void texture_get_color_noisy_periodic(const struct Texture *tex, const v3 point, v3 color)
{
	struct TextureNoisyPeriodic *texture = (struct TextureNoisyPeriodic *)tex;
	v3 scaled_point;
	mul3s(point, texture->noise_feature_scale, scaled_point);
	float angle = (point[X] + simplex_noise(scaled_point[X], scaled_point[Y], scaled_point[Z]) * texture->noise_scale) * texture->frequency_scale;
	switch (texture->func) {
	case PERIODIC_FUNC_SIN:
		mul3s(texture->color_gradient, (1.f + sinf(angle)) * .5f, color);
		break;
	case PERIODIC_FUNC_SAW:
		mul3s(texture->color_gradient, angle - floorf(angle), color);
		break;
	case PERIODIC_FUNC_TRIANGLE:
		mul3s(texture->color_gradient, fabs(2.f * (angle - floorf(angle) - .5f)), color);
		break;
	case PERIODIC_FUNC_SQUARE:
		mul3s(texture->color_gradient, !signbit(sinf(angle)), color);
		break;
	}
	add3v(color, texture->color, color);
}
