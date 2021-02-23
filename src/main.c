#include "main.h"

float get_closest_intersection(unsigned num_objects, Object *objects, Line *ray, Object *closest_object, Vec3 closest_normal)
{
	Vec3 normal;
	float min_distance = FLT_MAX;
	unsigned i;
	for (i = 0; i < num_objects; i++) {
		float distance;
		if (objects[i].common->get_intersection(objects[i], ray, &distance, normal))
			if (distance < min_distance) {
				min_distance = distance;
				*closest_object = objects[i];
				memcpy(closest_normal, normal, sizeof(Vec3));
			}
	}
	return min_distance;
}

bool is_intersection_in_distance(unsigned num_objects, Object *objects, Line *ray, float min_distance, CommonObject *excl_object, Vec3 light_intensity)
{
	unsigned i;
	for (i = 0; i < num_objects; i++) {
		CommonObject *object = objects[i].common;
		if (object != excl_object)
			if (object->intersects_in_range(objects[i], ray, min_distance)) {
				if (object->transparent)
					multiply3v(light_intensity, object->kt, light_intensity);
				else
					return true;
			}
	}
	return false;
}

void cast_ray(Camera *camera, unsigned num_objects, Object *objects, unsigned num_lights, Light *lights, Line *ray, Vec3 kr, Vec3 color, unsigned bounce_count, CommonObject *inside_object)
{
	Object closest_object;
	closest_object.common = NULL;
	Vec3 normal;
	float min_distance;

	if (inside_object) {
		Object obj;
		obj.common = inside_object;
		if (inside_object->get_intersection(obj, ray, &min_distance, normal)) {
			closest_object.common = inside_object;
		} else {
			min_distance = get_closest_intersection(num_objects, objects, ray, &closest_object, normal);
		}
	} else {
		min_distance = get_closest_intersection(num_objects, objects, ray, &closest_object, normal);
	}
	if (closest_object.common) {
		CommonObject *object = closest_object.common;

		normalize3(normal);
		float b = dot3(normal, ray->vector);
		if (b > 0.f)
			multiply3(normal, -1.f, normal);

		//LIGHTING MODEL
		Vec3 obj_color;

		Vec3 point;
		multiply3(ray->vector, min_distance, point);
		add3(point, ray->position, point);

		//ambient
		multiply3v(object->ka, ambient_light_intensity, obj_color);

		//Line from point to light
		Line light_ray;
		memcpy(light_ray.position, point, sizeof(Vec3));

		unsigned i;
		for (i = 0; i < num_lights; i++) {
			Light *light = &lights[i];

			//Line from point to light
			subtract3(light->position, point, light_ray.vector);
			float light_distance = magnitude3(light_ray.vector);
			normalize3(light_ray.vector);

			float a = dot3(light_ray.vector, normal);

			Vec3 incoming_light_intensity = {1., 1., 1.};
			if (object != inside_object && ! is_intersection_in_distance(num_objects, objects, &light_ray, light_distance, (inside_object == object) ? inside_object : NULL, incoming_light_intensity)) {
				Vec3 intensity;
				multiply3v(incoming_light_intensity, light->intensity, intensity);
				Vec3 diffuse;
				multiply3v(object->kd, intensity, diffuse);
				multiply3(diffuse, fmaxf(0., a), diffuse);

				Vec3 reflected;
				float specular_mul;
				switch (reflection_model) {
				case REFLECTION_PHONG:
					multiply3(normal, 2 * a, reflected);
					subtract3(reflected, light_ray.vector, reflected);
					specular_mul = - dot3(reflected, ray->vector);
					break;
				case REFLECTION_BLINN:
					multiply3(light_ray.vector, -1.f, reflected);
					add3(reflected, ray->vector, reflected);
					normalize3(reflected);
					specular_mul = - dot3(normal, reflected);
					break;
				}
				Vec3 specular;
				multiply3v(object->ks, intensity, specular);
				multiply3(specular, fmaxf(0., powf(specular_mul, object->shininess)), specular);

				add3_3(obj_color, diffuse, specular, obj_color);
			}
		}
		multiply3v(obj_color, kr, obj_color);
		add3(color, obj_color, color);

		//reflection
		if (inside_object != object && object->reflective && bounce_count < max_bounces) {
			Vec3 reflected_kr;
			multiply3v(kr, object->kr, reflected_kr);
			if (minimum_light_intensity_sqr < sqr(reflected_kr[X]) + sqr(reflected_kr[Y]) + sqr(reflected_kr[Z])) {
				Line reflection_ray;
				memcpy(reflection_ray.position, point, sizeof(Vec3));
				multiply3(normal, 2 * dot3(ray->vector, normal), reflection_ray.vector);
				subtract3(ray->vector, reflection_ray.vector, reflection_ray.vector);
				cast_ray(camera, num_objects, objects, num_lights, lights, &reflection_ray, reflected_kr, color, bounce_count + 1, NULL);
			}
		}

		//transparency
		if (object->transparent && bounce_count < max_bounces) {
			Vec3 refracted_kt;
			multiply3v(kr, object->kt, refracted_kt);
			if (minimum_light_intensity_sqr < sqr(refracted_kt[X]) + sqr(refracted_kt[Y]) + sqr(refracted_kt[Z])) {
				Line refraction_ray;
				memcpy(refraction_ray.position, point, sizeof(Vec3));
				float incident_angle = acosf(fabs(b));
				float refractive_multiplier = inside_object ? (object->refractive_index) : (1.f / object->refractive_index);
				float refracted_angle = asinf(sinf(incident_angle) * refractive_multiplier);
				float delta_angle = refracted_angle - incident_angle;
				Vec3 c, f, g, h;
				cross(ray->vector, normal, c);
				normalize3(c);
				cross(c, ray->vector, f);
				multiply3(ray->vector, cosf(delta_angle), g);
				multiply3(f, sinf(delta_angle), h);
				add3(g, h, refraction_ray.vector);
				normalize3(refraction_ray.vector);
				cast_ray(camera, num_objects, objects, num_lights, lights, &refraction_ray, refracted_kt, color, bounce_count + 1, object);
			}
		}
	}
}

void create_image(Camera *camera, unsigned num_objects, Object *objects, unsigned num_lights, Light *lights)
{
	Vec3 kr = {1.f, 1.f, 1.f};
	#ifdef MULTITHREADING
	#pragma omp parallel for
	#endif
	for (unsigned row = 0; row < camera->image.resolution[Y]; row++) {
		Vec3 pixel_position;
		multiply3(camera->image.vectors[Y], row, pixel_position);
		add3(pixel_position, camera->image.corner, pixel_position);
		Line ray;
		memcpy(ray.position, camera->position, sizeof(Vec3));
		unsigned pixel_index = camera->image.resolution[X] * row;
		unsigned col;
		for (col = 0; col < camera->image.resolution[X]; col++) {
			add3(pixel_position, camera->image.vectors[X], pixel_position);
			Vec3 color = {0., 0., 0.};
			subtract3(pixel_position, camera->position, ray.vector);
			normalize3(ray.vector);
			cast_ray(camera, num_objects, objects, num_lights, lights, &ray, kr, color, 0, NULL);
			multiply3(color, 255., color);
			uint8_t *pixel = camera->image.pixels[pixel_index];
			pixel[0] = fmaxf(fminf(color[0], 255.), 0.);
			pixel[1] = fmaxf(fminf(color[1], 255.), 0.);
			pixel[2] = fmaxf(fminf(color[2], 255.), 0.);
			pixel_index++;
		}
	}
}

void process_arguments(int argc, char *argv[], FILE **scene_file, FILE **output_file, unsigned resolution[2], int *fov)
{
	if (argc <= 7) {
		if (argc == 2) {
			if (! strcmp("--help", argv[1]) || ! strcmp("-h", argv[1])) {
				printf("%s", HELPTEXT);
				exit(0);
			}
		}
		err_assert(false, ERR_ARGC);
	}

	int i;
	for (i = 1; i < argc; i += 2) {
		switch (djb_hash(argv[i])) {
			case 5859054://-f
			case 3325380195://--file
				*scene_file = fopen(argv[i + 1], "rb");
				break;
			case 5859047://-o
			case 739088698://--output
				err_assert(strstr(argv[i + 1], ".ppm"), ERR_ARGV_FILE_EXT);
				*output_file = fopen(argv[i + 1], "wb");
				break;
			case 5859066: //-r
			case 2395108907://--resolution
				resolution[0] = abs(atoi(argv[i + 1]));
				resolution[1] = abs(atoi(argv[i++ + 2]));
				break;
			case 5859045://-m
				#ifdef MULTITHREADING
				if (djb_hash(argv[i + 1]) == 193414065) {//if "max"
					NUM_THREADS = omp_get_max_threads();
				} else {
					NUM_THREADS = atoi(argv[i + 1]);
					err_assert(NUM_THREADS <= omp_get_max_threads(), ERR_ARGV_NUM_THREADS);
				}
				#else
				err_assert(false, ERR_ARGV_MULTITHREADING);
				#endif /* MULTITHREADING */
				break;
			case 5859050://-b
				max_bounces = abs(atoi(argv[i + 1]));
				break;
			case 5859049://-a
				minimum_light_intensity_sqr = sqr(atof(argv[i + 1]));
				break;
			case 5859067://-s
				switch (djb_hash(argv[i + 1])) {
				case 187940251://phong
					reflection_model = REFLECTION_PHONG;
					break;
				case 175795714://blinn
					reflection_model = REFLECTION_BLINN;
					break;
				default:
					err_assert(false, ERR_ARGV_REFLECTION);
				}
				break;
			case 2085543063: //-fov
				*fov = atoi(argv[i + 1]);
				err_assert(*fov < 180 && *fov > 0, ERR_ARGV_FOV);
				break;
			default:
				err_assert(false, ERR_ARGV_UNRECOGNIZED);
		}
	}
	err_assert(*scene_file, ERR_ARGV_IO_OPEN_SCENE);
	err_assert(*output_file, ERR_ARGV_IO_OPEN_OUTPUT);
}

int main(int argc, char *argv[])
{
	#ifdef DISPLAY_TIME
	struct timespec start_t, current_t;
	timespec_get(&start_t, TIME_UTC);
	#endif

	FILE *scene_file = NULL, *output_file = NULL;
	unsigned resolution[2] = {0, 0};
	int fov = 90;
	float focal_length = 1.f; //NOTE: Currently, user cannot change this parameter

	process_arguments(argc, argv, &scene_file, &output_file, resolution, &fov);

	#ifdef MULTITHREADING
	omp_set_num_threads(NUM_THREADS);
	#endif

	Vec2 image_size;
	image_size[X] = 2 * focal_length * tanf(fov * PI / 360.f);
	image_size[Y] = (image_size[X] * resolution[Y]) / resolution[X];

	Camera *camera = NULL;
	Object *objects = NULL;
	Light *lights = NULL;
	unsigned num_objects = 0, num_lights = 0;

	PRINT_TIME("[%07.3f] INITIALIZING SCENE.\n");

	scene_load(scene_file, &camera, &num_objects, &objects, &num_lights, &lights, ambient_light_intensity, resolution, image_size, focal_length);
	fclose(scene_file);

	PRINT_TIME("[%07.3f] INITIALIZED SCENE. BEGAN RENDERING.\n");

	create_image(camera, num_objects, objects, num_lights, lights);

	PRINT_TIME("[%07.3f] FINISHED RENDERING.\n");

	save_image(output_file, &camera->image);
	fclose(output_file);

	PRINT_TIME("[%07.3f] SAVED IMAGE.\n");

	free(camera->image.pixels);
	free(camera);
	unsigned i;
	for (i = 0; i < num_objects; i++)
		free(objects[i].common);
	free(objects);
	free(lights);

	PRINT_TIME("[%07.3f] TERMINATED PROGRAM.\n");
}
