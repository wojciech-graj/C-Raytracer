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
#include "argv.h"
#include "object.h"
#include "material.h"
#include "accel.h"
#include "strhash.h"
#include "calc.h"
#include "image.h"
#include "camera.h"
#include "system.h"
#include "mem.h"

#include <stdlib.h>
#include <math.h>
#include <float.h>

#ifdef MULTITHREADING
#include <omp.h>
#endif

typedef enum _ReflectionModel {
	REFLECTION_PHONG,
	REFLECTION_BLINN,
} ReflectionModel;

typedef enum _GlobalIlluminationModel{
	GLOBAL_ILLUMINATION_AMBIENT,
	GLOBAL_ILLUMINATION_PATH_TRACING,
} GlobalIlluminationModel;

typedef enum _LightAttenuation{
	LIGHT_ATTENUATION_NONE,
	LIGHT_ATTENUATION_LINEAR,
	LIGHT_ATTENUATION_SQUARE,
} LightAttenuation;

void get_closest_intersection(const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance);
bool is_light_blocked(const Line *ray, float distance, Vec3 light_intensity, const Object *emittant_object);
void cast_ray(const Line *ray, const Vec3 kr, Vec3 color, uint32_t bounce_count, Object *inside_object);

static float light_attenuation_offset = 1.f;
static float brightness = 1.f;
Vec3 global_ambient_light_intensity = {0};
static uint32_t max_bounces = 10;
static float minimum_light_intensity_sqr = .01f * .01f;
static ReflectionModel reflection_model = REFLECTION_PHONG;
static GlobalIlluminationModel global_illumination_model = GLOBAL_ILLUMINATION_AMBIENT;
static size_t samples_per_pixel = 1;
static bool normalize_colors = false;
static LightAttenuation light_attenuation = LIGHT_ATTENUATION_SQUARE;

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
		case 187940251://phong
			reflection_model = REFLECTION_PHONG;
			break;
		case 175795714://blinn
			reflection_model = REFLECTION_BLINN;
			break;
		}

	idx = argv_check_with_args("-g", 1);
	if (idx)
		switch (hash_myargv[idx + 1]) {
		case 354625309://ambient
			global_illumination_model = GLOBAL_ILLUMINATION_AMBIENT;
			break;
		case 2088095368://path
			global_illumination_model = GLOBAL_ILLUMINATION_PATH_TRACING;
			break;
		}

	idx = argv_check_with_args("-n", 1);
	if (idx)
		samples_per_pixel = abs(atoi(myargv[idx + 1]));

	idx = argv_check("-c");
	if (idx)
		normalize_colors = true;

	idx = argv_check_with_args("-l", 1);
	if (idx)
		switch (hash_myargv[idx + 1]) {
		case 2087865487://none
			light_attenuation = LIGHT_ATTENUATION_NONE;
			break;
		case 193412846://lin
			light_attenuation = LIGHT_ATTENUATION_LINEAR;
			break;
		case 193433013://sqr
			light_attenuation = LIGHT_ATTENUATION_SQUARE;
			break;
		}

	idx = argv_check_with_args("-o", 1);
	if (idx)
		light_attenuation_offset = atof(myargv[idx + 1]);

	idx = argv_check_with_args("-i", 1);
	if (idx)
		brightness = atof(myargv[idx + 1]);
}

void get_closest_intersection(const Line *ray, Object **closest_object, Vec3 closest_normal, float *closest_distance)
{
#ifdef UNBOUND_OBJECTS
	unbound_objects_get_closest_intersection(ray, closest_object, closest_normal, closest_distance);
#endif
	accel_get_closest_intersection(ray, closest_object, closest_normal, closest_distance);
}

bool is_light_blocked(const Line *ray, const float distance, Vec3 light_intensity, const Object *emittant_object)
{
#ifdef UNBOUND_OBJECTS
	return unbound_objects_is_light_blocked(ray, distance, light_intensity, emittant_object)
		|| accel_is_light_blocked(ray, distance, light_intensity, emittant_object);
#else
	return accel_is_light_blocked(ray, distance, light_intensity, emittant_object);
#endif
}

//Places all objects in scene in cube of side length 1
void normalize_scene(void)
{
	printf_log("Normalizing scene.");

	Vec3 min, max;
	get_objects_extents(min, max);

	Vec3 range;
	sub3v(max, min, range);
	float scale_factor = 1.f / max3(range);

	size_t i;
	for (i = 0; i < num_objects; i++)
		objects[i]->object_data->scale(objects[i], min, scale_factor);

	camera_scale(min, scale_factor);
	image_scale(min, scale_factor);
}

void cast_ray(const Line *ray, const Vec3 kr, Vec3 color, const uint32_t remaining_bounces, Object *inside_object)
{
	Object *closest_object = NULL;
	Vec3 normal;
	float min_distance;
	Object *obj = inside_object;

	if (inside_object && inside_object->object_data->get_intersection(obj, ray, &min_distance, normal)) {
		closest_object = inside_object;
	} else {
		min_distance = FLT_MAX;
		get_closest_intersection(ray, &closest_object, normal, &min_distance);
	}
	Object *object = closest_object;
	if (! object)
		return;

	//LIGHTING MODEL
	Vec3 obj_color;

	Material *material = object->material;

	//emittance
	memcpy(obj_color, material->ke, sizeof(Vec3));

	//Line originating at point of intersection
	Line outgoing_ray;
	mul3s(ray->vector, min_distance, outgoing_ray.position);
	add3v(outgoing_ray.position, ray->position, outgoing_ray.position);

	float b = dot3(normal, ray->vector);
	bool is_outside = signbit(b);

	size_t i, j;
	for (i = 0; i < num_emittant_objects; i++) {
		Object *emittant_object = emittant_objects[i];
		if (emittant_object == object)
			continue;
		Vec3 light_intensity;
		mul3s(emittant_object->material->ke, 1.f / emittant_object->num_lights, light_intensity);
		for (j = 0; j < emittant_object->num_lights; j++) {
			Vec3 light_point, incoming_light_intensity;
			emittant_object->object_data->get_light_point(emittant_object, outgoing_ray.position, light_point);
			memcpy(incoming_light_intensity, light_intensity, sizeof(Vec3));

			sub3v(light_point, outgoing_ray.position, outgoing_ray.vector);
			float light_distance = mag3(outgoing_ray.vector);
			mul3s(outgoing_ray.vector, 1.f / light_distance, outgoing_ray.vector);

			float a = dot3(outgoing_ray.vector, normal);

			if (is_outside
				&& !is_light_blocked(&outgoing_ray, light_distance, incoming_light_intensity, emittant_object)) {

				Vec3 distance;
				sub3v(light_point, outgoing_ray.position, distance);
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

				Vec3 diffuse;
				material->texture->get_color(material->texture, outgoing_ray.position, diffuse);
				mul3v(diffuse, incoming_light_intensity, diffuse);
				mul3s(diffuse, fmaxf(0., a), diffuse);

				Vec3 reflected;
				float specular_mul;
				switch (reflection_model) {
				case REFLECTION_PHONG:
					mul3s(normal, 2 * a, reflected);
					sub3v(reflected, outgoing_ray.vector, reflected);
					specular_mul = - dot3(reflected, ray->vector);
					break;
				case REFLECTION_BLINN:
					mul3s(outgoing_ray.vector, -1.f, reflected);
					add3v(reflected, ray->vector, reflected);
					norm3(reflected);
					specular_mul = - dot3(normal, reflected);
					break;
				}
				Vec3 specular;
				mul3v(material->ks, incoming_light_intensity, specular);
				mul3s(specular, fmaxf(0., powf(specular_mul, material->shininess)), specular);

				add3v3(obj_color, diffuse, specular, obj_color);
			}
		}
	}

	//global illumination
	switch (global_illumination_model) {
	case GLOBAL_ILLUMINATION_AMBIENT: {
		Vec3 ambient_light;
		mul3v(material->ka, global_ambient_light_intensity, ambient_light);
		add3v(obj_color, ambient_light, obj_color);
		}
		break;
	case GLOBAL_ILLUMINATION_PATH_TRACING:
		if (remaining_bounces && is_outside) {
			Mat3 rotation_matrix;
			if (normal[Y] - object->epsilon < -1.f) {
				Mat3 vx = {
					{1.f, 0.f, 0.f},
					{0.f, -1.f, 0.f},
					{0.f, 0.f, -1.f},
				};
				memcpy(rotation_matrix, vx, sizeof(Mat3));
			} else {
				float mul = 1.f / (1.f + dot3((Vec3){0.f, 1.f, 0.f}, normal));
				Mat3 vx = {
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
				memcpy(rotation_matrix, vx, sizeof(Mat3));
			}

			Vec3 delta = {1.f, 1.f, 1.f};
			size_t num_samples;
			if (remaining_bounces == max_bounces) {
				num_samples = samples_per_pixel;
				mul3s(delta, 1.f / (float)num_samples, delta);
			} else {
				num_samples = 1;
			}

			Vec3 light_mul;
			for (i = 0; i < num_samples; i++) {
				float inclination = acosf(rand_flt() * 2.f - 1.f);
				float azimuth = rand_flt() * PI;
				mulm3(rotation_matrix, (Vec3)SPHERICAL_TO_CARTESIAN(1, inclination, azimuth), outgoing_ray.vector);
				mul3s(delta, dot3(normal, outgoing_ray.vector), light_mul);
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
		return;

	//reflection
	if (inside_object != object
		&& material->reflective) {
		Vec3 reflected_kr;
		mul3v(kr, material->kr, reflected_kr);
		if (minimum_light_intensity_sqr < magsqr3(reflected_kr)) {
			mul3s(normal, 2 * b, outgoing_ray.vector);
			sub3v(ray->vector, outgoing_ray.vector, outgoing_ray.vector);
			cast_ray(&outgoing_ray, reflected_kr, color, remaining_bounces - 1, NULL);
		}
	}

	//transparency
	if (material->transparent) {
		Vec3 refracted_kt;
		mul3v(kr, material->kt, refracted_kt);
		if (minimum_light_intensity_sqr < magsqr3(refracted_kt)) {
			float incident_angle = acosf(fabs(b));
			float refractive_multiplier = is_outside ? 1.f / material->refractive_index : material->refractive_index;
			float refracted_angle = asinf(sinf(incident_angle) * refractive_multiplier);
			float delta_angle = refracted_angle - incident_angle;
			Vec3 c, f, g, h;
			cross(ray->vector, normal, c);
			norm3(c);
			if (!is_outside)
				mul3s(c, -1.f, c);
			cross(c, ray->vector, f);
			mul3s(ray->vector, cosf(delta_angle), g);
			mul3s(f, sinf(delta_angle), h);
			add3v(g, h, outgoing_ray.vector);
			norm3(outgoing_ray.vector);
			cast_ray(&outgoing_ray, refracted_kt, color, remaining_bounces - 1, object);
		}
	}
}

void create_image(void)
{
	printf_log("Commencing raytracing.");
	Vec3 kr = {1.f, 1.f, 1.f};
	Vec3 *raw_pixels = safe_calloc(image.resolution[X] * image.resolution[Y], sizeof(Vec3));
#ifdef MULTITHREADING
#pragma omp parallel for
#endif
	for (uint32_t row = 0; row < image.resolution[Y]; row++) {
		Vec3 pixel_position;
		mul3s(image.vectors[Y], row, pixel_position);
		add3v(pixel_position, image.corner, pixel_position);
		Line ray;
		memcpy(ray.position, camera.position, sizeof(Vec3));
		uint32_t pixel_index = image.resolution[X] * row;
		uint32_t col;
		for (col = 0; col < image.resolution[X]; col++) {
			add3v(pixel_position, image.vectors[X], pixel_position);
			sub3v(pixel_position, camera.position, ray.vector);
			norm3(ray.vector);
			cast_ray(&ray, kr, raw_pixels[pixel_index], max_bounces, NULL);
			pixel_index++;
		}
	}

	printf_log("Postprocessing.");
	size_t image_size = image.resolution[Y] * image.resolution[X];
	size_t i;
	float slope = brightness;
	if (normalize_colors) {
		float min = FLT_MAX, max = FLT_MIN;
		for (i = 0; i < image_size; i++) {
			float *raw_pixel = raw_pixels[i];
			float pixel_min = min3(raw_pixel), pixel_max = max3(raw_pixel);
			if (pixel_min < min)
				min = pixel_min;
			if (pixel_max > max)
				max = pixel_max;
		}

		slope /= (max - min);

		for (i = 0; i < image_size; i++) {
			sub3s(raw_pixels[i], min, raw_pixels[i]);
		}
	}

	for (i = 0; i < image_size; i++) {
		mul3s(raw_pixels[i], slope, raw_pixels[i]);
		uint8_t *pixel = image.pixels[i];
		pixel[0] = (uint8_t)fmaxf(fminf(raw_pixels[i][0] * 255.f, 255.f), 0.f);
		pixel[1] = (uint8_t)fmaxf(fminf(raw_pixels[i][1] * 255.f, 255.f), 0.f);
		pixel[2] = (uint8_t)fmaxf(fminf(raw_pixels[i][2] * 255.f, 255.f), 0.f);
	}

	free(raw_pixels);
}
