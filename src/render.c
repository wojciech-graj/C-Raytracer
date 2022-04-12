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

#include "render.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "accel.h"
#include "argv.h"
#include "calc.h"
#include "camera.h"
#include "image.h"
#include "material.h"
#include "mem.h"
#include "object.h"
#include "system.h"

#ifdef MULTITHREADING
#include <omp.h>
#endif

enum ReflectionModel {
	REFLECTION_PHONG,
	REFLECTION_BLINN,
};

enum GlobalIlluminationModel {
	GLOBAL_ILLUMINATION_AMBIENT,
	GLOBAL_ILLUMINATION_PATH_TRACING,
};

enum LightAttenuation {
	LIGHT_ATTENUATION_NONE,
	LIGHT_ATTENUATION_LINEAR,
	LIGHT_ATTENUATION_SQUARE,
};

void get_closest_intersection(const struct Ray *ray, struct Object **closest_object, v3 closest_normal, float *closest_distance);
bool is_light_blocked(const struct Ray *ray, float distance, v3 light_intensity, const struct Object *emittant_object);
float cast_ray(const struct Ray *ray, const v3 kr, v3 color, uint32_t bounce_count, struct Object *inside_object);

static float light_attenuation_offset = 1.f;
v3 global_ambient_light_intensity = { 0 };
static uint32_t max_bounces = 10;
static float minimum_light_intensity_sqr = .01f * .01f;
static enum ReflectionModel reflection_model = REFLECTION_PHONG;
static enum GlobalIlluminationModel global_illumination_model = GLOBAL_ILLUMINATION_AMBIENT;
static size_t samples_per_pixel = 1;
static enum LightAttenuation light_attenuation = LIGHT_ATTENUATION_SQUARE;

void render_init(void)
{
	int idx;

	idx = argv_check_with_args("-b", 1);
	if (idx)
		max_bounces = abs(atoi(myargv[idx + 1]));

	idx = argv_check_with_args("-a", 1);
	if (idx)
		minimum_light_intensity_sqr = sqr(atof(myargv[idx + 1]));

	idx = argv_check_with_args("-s", 1);
	if (idx)
		switch (hash_myargv[idx + 1]) {
		case 187940251: //phong
			reflection_model = REFLECTION_PHONG;
			break;
		case 175795714: //blinn
			reflection_model = REFLECTION_BLINN;
			break;
		}

	idx = argv_check_with_args("-g", 1);
	if (idx)
		switch (hash_myargv[idx + 1]) {
		case 354625309: //ambient
			global_illumination_model = GLOBAL_ILLUMINATION_AMBIENT;
			break;
		case 2088095368: //path
			global_illumination_model = GLOBAL_ILLUMINATION_PATH_TRACING;
			break;
		}

	idx = argv_check_with_args("-n", 1);
	if (idx)
		samples_per_pixel = abs(atoi(myargv[idx + 1]));

	idx = argv_check_with_args("-l", 1);
	if (idx)
		switch (hash_myargv[idx + 1]) {
		case 2087865487: //none
			light_attenuation = LIGHT_ATTENUATION_NONE;
			break;
		case 193412846: //lin
			light_attenuation = LIGHT_ATTENUATION_LINEAR;
			break;
		case 193433013: //sqr
			light_attenuation = LIGHT_ATTENUATION_SQUARE;
			break;
		}

	idx = argv_check_with_args("-o", 1);
	if (idx)
		light_attenuation_offset = atof(myargv[idx + 1]);
}

void get_closest_intersection(const struct Ray *ray, struct Object **closest_object, v3 closest_normal, float *closest_distance)
{
#ifdef UNBOUND_OBJECTS
	unbound_objects_get_closest_intersection(ray, closest_object, closest_normal, closest_distance);
#endif
	accel_get_closest_intersection(ray, closest_object, closest_normal, closest_distance);
}

bool is_light_blocked(const struct Ray *ray, const float distance, v3 light_intensity, const struct Object *emittant_object)
{
#ifdef UNBOUND_OBJECTS
	return unbound_objects_is_light_blocked(ray, distance, light_intensity, emittant_object)
		|| accel_is_light_blocked(ray, distance, light_intensity, emittant_object);
#else
	return accel_is_light_blocked(ray, distance, light_intensity, emittant_object);
#endif
}

float cast_ray(const struct Ray *ray, const v3 kr, v3 color, const uint32_t remaining_bounces, struct Object *inside_object)
{
	struct Object *object = NULL;
	v3 normal;
	float min_distance;

	/* get ray intersection */
	if (inside_object && inside_object->object_data->get_intersection(inside_object, ray, &min_distance, normal)) {
		object = inside_object;
	} else {
		min_distance = FLT_MAX;
		get_closest_intersection(ray, &object, normal, &min_distance);
	}

	if (!object)
		return 0.f;

	//Ray originating at point of intersection
	struct Ray outgoing_ray;
	mul3s(ray->direction, min_distance, outgoing_ray.point);
	add3v(outgoing_ray.point, ray->point, outgoing_ray.point);

	//LIGHTING MODEL
	v3 obj_color;

	struct Material *material = object->material;

	//emittance
	assign3(obj_color, material->ke);

	float b = dot3(normal, ray->direction);
	bool is_outside = signbit(b);

	size_t i, j;
	for (i = 0; i < num_emittant_objects; i++) {
		struct Object *emittant_object = emittant_objects[i];
		if (unlikely(emittant_object == object))
			continue;
		v3 light_intensity;
		mul3s(emittant_object->material->ke, 1.f / emittant_object->num_lights, light_intensity);
		for (j = 0; j < emittant_object->num_lights; j++) {
			v3 light_point, incoming_light_intensity;
			emittant_object->object_data->get_light_point(emittant_object, outgoing_ray.point, light_point);
			assign3(incoming_light_intensity, light_intensity);

			sub3v(light_point, outgoing_ray.point, outgoing_ray.direction);
			float light_distance = mag3(outgoing_ray.direction);
			mul3s(outgoing_ray.direction, 1.f / light_distance, outgoing_ray.direction);

			float a = dot3(outgoing_ray.direction, normal);

			if (is_outside
				&& !is_light_blocked(&outgoing_ray, light_distance, incoming_light_intensity, emittant_object)) {
				v3 distance;
				sub3v(light_point, outgoing_ray.point, distance);
				switch (light_attenuation) {
				case LIGHT_ATTENUATION_NONE:
					break;
				case LIGHT_ATTENUATION_LINEAR:
					mul3s(incoming_light_intensity, 1.f / (light_attenuation_offset + mag3(distance)), incoming_light_intensity);
					break;
				case LIGHT_ATTENUATION_SQUARE:
					mul3s(incoming_light_intensity, 1.f / (light_attenuation_offset + magsqr3(distance)), incoming_light_intensity);
					break;
				}

				v3 diffuse;
				material->texture->get_color(material->texture, outgoing_ray.point, diffuse);
				mul3v(diffuse, incoming_light_intensity, diffuse);
				mul3s(diffuse, fmaxf(0., a), diffuse);

				v3 reflected;
				float specular_mul;
				switch (reflection_model) {
				case REFLECTION_PHONG:
					mul3s(normal, 2 * a, reflected);
					sub3v(reflected, outgoing_ray.direction, reflected);
					specular_mul = -dot3(reflected, ray->direction);
					break;
				case REFLECTION_BLINN:
					mul3s(outgoing_ray.direction, -1.f, reflected);
					add3v(reflected, ray->direction, reflected);
					norm3(reflected);
					specular_mul = -dot3(normal, reflected);
					break;
				}
				v3 specular;
				mul3v(material->ks, incoming_light_intensity, specular);
				mul3s(specular, fmaxf(0., powf(specular_mul, material->shininess)), specular);

				add3v3(obj_color, diffuse, specular, obj_color);
			}
		}
	}

	//global illumination
	switch (global_illumination_model) {
	case GLOBAL_ILLUMINATION_AMBIENT: {
		v3 ambient_light;
		mul3v(material->ka, global_ambient_light_intensity, ambient_light);
		add3v(obj_color, ambient_light, obj_color);
	} break;
	case GLOBAL_ILLUMINATION_PATH_TRACING:
		if (remaining_bounces && is_outside) {
			m3 rotation_matrix;
			if (normal[Y] - object->epsilon < -1.f) {
				m3 vx = {
					{ 1.f, 0.f, 0.f },
					{ 0.f, -1.f, 0.f },
					{ 0.f, 0.f, -1.f },
				};
				assignm(rotation_matrix, vx);
			} else {
				float mul = 1.f / (1.f + dot3((v3){ 0.f, 1.f, 0.f }, normal));
				m3 vx = {
					{
						1.f - sqr(normal[X]) * mul,
						normal[X],
						-normal[X] * normal[Z] * mul,
					},
					{
						-normal[X],
						1.f - (sqr(normal[X]) + sqr(normal[Z])) * mul,
						-normal[Z],
					},
					{
						-normal[X] * normal[Z] * mul,
						normal[Z],
						1.f - sqr(normal[Z]) * mul,
					},
				};
				assignm(rotation_matrix, vx);
			}

			v3 delta = { 1.f, 1.f, 1.f };
			size_t num_samples;
			if (remaining_bounces == max_bounces) {
				num_samples = samples_per_pixel;
				mul3s(delta, 1.f / (float)num_samples, delta);
			} else {
				num_samples = 1;
			}

			v3 light_mul;
			for (i = 0; i < num_samples; i++) {
				float inclination = acosf(rand_flt() * 2.f - 1.f);
				float azimuth = rand_flt() * PI;
				mulmv(rotation_matrix, (v3)SPHERICAL_TO_CARTESIAN(1, inclination, azimuth), outgoing_ray.direction);
				mul3s(delta, dot3(normal, outgoing_ray.direction), light_mul);
				cast_ray(&outgoing_ray, light_mul, obj_color, 0, NULL);
			}
		}
		break;
	}

	mul3v(obj_color, kr, obj_color);
	switch (light_attenuation) {
	case LIGHT_ATTENUATION_NONE:
		break;
	case LIGHT_ATTENUATION_LINEAR:
		mul3s(obj_color, 1.f / (light_attenuation_offset + min_distance), obj_color);
		break;
	case LIGHT_ATTENUATION_SQUARE:
		mul3s(obj_color, 1.f / sqr(light_attenuation_offset + min_distance), obj_color);
		break;
	}
	add3v(color, obj_color, color);

	if (!remaining_bounces)
		return 0.f;

	//reflection
	if (inside_object != object
		&& material->reflective) {
		v3 reflected_kr;
		mul3v(kr, material->kr, reflected_kr);
		if (minimum_light_intensity_sqr < magsqr3(reflected_kr)) {
			mul3s(normal, 2 * b, outgoing_ray.direction);
			sub3v(ray->direction, outgoing_ray.direction, outgoing_ray.direction);
			cast_ray(&outgoing_ray, reflected_kr, color, remaining_bounces - 1, NULL);
		}
	}

	//transparency
	if (material->transparent) {
		v3 refracted_kt;
		mul3v(kr, material->kt, refracted_kt);
		if (minimum_light_intensity_sqr < magsqr3(refracted_kt)) {
			float incident_angle = acosf(fabs(b));
			float refractive_multiplier = is_outside ? 1.f / material->refractive_index : material->refractive_index;
			float refracted_angle = asinf(sinf(incident_angle) * refractive_multiplier);
			float delta_angle = refracted_angle - incident_angle;
			v3 c, f, g, h;
			cross(ray->direction, normal, c);
			norm3(c);
			if (!is_outside)
				mul3s(c, -1.f, c);
			cross(c, ray->direction, f);
			mul3s(ray->direction, cosf(delta_angle), g);
			mul3s(f, sinf(delta_angle), h);
			add3v(g, h, outgoing_ray.direction);
			norm3(outgoing_ray.direction);
			cast_ray(&outgoing_ray, refracted_kt, color, remaining_bounces - 1, object);
		}
	}

	return min_distance;
}

void render(void)
{
	printf_log("Commencing raytracing.");
	v3 kr = { 1.f, 1.f, 1.f };
#ifdef MULTITHREADING
#pragma omp parallel for
#endif
	for (uint32_t row = 0; row < image.resolution[Y]; row++) {
		v3 pixel_position;
		mul3s(image.vectors[Y], row, pixel_position);
		add3v(pixel_position, image.corner, pixel_position);
		struct Ray ray;
		assign3(ray.point, camera.position);
		uint32_t pixel_index = image.resolution[X] * row;
		uint32_t col;
		for (col = 0; col < image.resolution[X]; col++) {
			add3v(pixel_position, image.vectors[X], pixel_position);
			sub3v(pixel_position, camera.position, ray.direction);
			norm3(ray.direction);
			image.z_buffer[pixel_index] = cast_ray(&ray, kr, image.raster[pixel_index], max_bounces, NULL);
			pixel_index++;
		}
	}
}
