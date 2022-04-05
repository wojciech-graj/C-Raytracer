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
#include "mem.h"
#include "calc.h"
#include "error.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "SimplexNoise/SimplexNoise.h"

#define MATERIAL_THRESHOLD 1e-6f

typedef struct _TextureUniform {
	Texture texture;
	Vec3 color;
} TextureUniform;

typedef struct _TextureCheckerboard {
	Texture texture;
	Vec3 colors[2];
	float scale;
} TextureCheckerboard;

typedef struct _TextureBrick {
	Texture texture;
	Vec3 colors[2];
	float scale;
	float mortar_width;
} TextureBrick;

typedef struct _TextureNoisyPeriodic {
	Texture texture;
	Vec3 color;
	Vec3 color_gradient;
	float noise_feature_scale;
	float noise_scale;
	float frequency_scale;
	PeriodicFunction func;
} TextureNoisyPeriodic;

void texture_get_color_uniform(const Texture *tex, const Vec3 point, Vec3 color);
void texture_get_color_checkerboard(const Texture *tex, const Vec3 point, Vec3 color);
void texture_get_color_brick(const Texture *tex, const Vec3 point, Vec3 color);
void texture_get_color_noisy_periodic(const Texture *tex, const Vec3 point, Vec3 color);

Material *materials;
size_t num_materials;

void materials_init(void)
{
	printf_log("Initializing materials.");

	materials = safe_malloc(sizeof(Material) * num_materials);
}

void material_init(Material *material, const int32_t id, const Vec3 ks, const Vec3 ka, const Vec3 kr, const Vec3 kt, const Vec3 ke, const float shininess, const float refractive_index, Texture * const texture)
{
	material->id = id;
	memcpy(material->ks, ks, sizeof(Vec3));
	memcpy(material->ka, ka, sizeof(Vec3));
	memcpy(material->kr, kr, sizeof(Vec3));
	memcpy(material->kt, kt, sizeof(Vec3));
	memcpy(material->ke, ke, sizeof(Vec3));
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

Material *get_material(const int32_t id)
{
	size_t i;
	for (i = 0; i < num_materials; i++)
		if (materials[i].id == id)
			return &materials[i];
	error("Failed to get material id [%d].", id);
	return NULL;
}

Texture *texture_uniform_new(const Vec3 color)
{
	TextureUniform *texture = safe_malloc(sizeof(TextureUniform));

	texture->texture.get_color = &texture_get_color_uniform;
	memcpy(texture->color, color, sizeof(Vec3));

	return (Texture*)texture;
}

Texture *texture_checkerboard_new(Vec3 colors[2], const float scale)
{
	TextureCheckerboard *texture = safe_malloc(sizeof(TextureCheckerboard));

	texture->texture.get_color = &texture_get_color_checkerboard;
	texture->scale = scale;
	memcpy(texture->colors, colors, sizeof(Vec3[2]));

	return (Texture*)texture;
}

Texture *texture_brick_new(Vec3 colors[2], const float scale, const float mortar_width)
{
	TextureBrick *texture = safe_malloc(sizeof(TextureBrick));

	texture->texture.get_color = &texture_get_color_brick;
	texture->scale = scale;
	texture->mortar_width = mortar_width;
	memcpy(texture->colors, colors, sizeof(Vec3[2]));

	return (Texture*)texture;
}

Texture *texture_noisy_periodic_new(const Vec3 color, const Vec3 color_gradient, const float noise_feature_scale, const float noise_scale, const float frequency_scale, const PeriodicFunction func)
{
	TextureNoisyPeriodic *texture = safe_malloc(sizeof(TextureNoisyPeriodic));

	texture->texture.get_color = &texture_get_color_noisy_periodic;
	texture->noise_feature_scale = noise_feature_scale;
	texture->noise_scale = noise_scale;
	texture->frequency_scale = frequency_scale;
	texture->func = func;
	memcpy(texture->color, color, sizeof(Vec3));
	memcpy(texture->color_gradient, color_gradient, sizeof(Vec3));

	return (Texture*)texture;
}

void texture_get_color_uniform(const Texture *tex, const Vec3 point, Vec3 color)
{
	(void)point;
	TextureUniform *texture = (TextureUniform*)tex;
	memcpy(color, texture->color, sizeof(Vec3));
}

void texture_get_color_checkerboard(const Texture *tex, const Vec3 point, Vec3 color)
{
	TextureCheckerboard *texture = (TextureCheckerboard*)tex;
	Vec3 scaled_point;
	mul3s(point, texture->scale, scaled_point);
	uint32_t parity = ((uint32_t)scaled_point[X] + (uint32_t)scaled_point[Y] + (uint32_t)scaled_point[Z]) % 2u;
	memcpy(color, texture->colors[parity], sizeof(Vec3));
}

void texture_get_color_brick(const Texture *tex, const Vec3 point, Vec3 color)
{
	TextureBrick *texture = (TextureBrick*)tex;
	Vec3 scaled_point;
	mul3s(point, texture->scale, scaled_point);
	uint32_t parity = (uint32_t)scaled_point[X] % 2u;
	scaled_point[Y] -= parity * .5f;
	uint32_t is_mortar = (scaled_point[X] - floorf(scaled_point[X]) < texture->mortar_width)
	|| (scaled_point[Y] - floorf(scaled_point[Y]) < texture->mortar_width);
	memcpy(color, texture->colors[is_mortar], sizeof(Vec3));
}

void texture_get_color_noisy_periodic(const Texture *tex, const Vec3 point, Vec3 color)
{
	TextureNoisyPeriodic *texture = (TextureNoisyPeriodic*)tex;
	Vec3 scaled_point;
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
