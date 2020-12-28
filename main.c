#include "main.h"

//TODO: create unit tests

void init_camera(Camera *camera,
	Vec3 position,
	Vec3 vectors[2],
	float focal_length,
	int image_resolution[2],
	Vec2 image_size)
{
	memcpy(camera->position, position, sizeof(Vec3));
	memcpy(camera->vectors, vectors, sizeof(Vec3[2]));
	normalize3(camera->vectors[0]);
	normalize3(camera->vectors[1]);
	cross(camera->vectors[0], camera->vectors[1], camera->vectors[2]);
	camera->focal_length = focal_length;
	memcpy(camera->image.resolution, image_resolution, 2 * sizeof(int));
	memcpy(camera->image.size, image_size, 2 * sizeof(float));
	camera->image.pixels = malloc(image_resolution[X] * image_resolution[Y] * sizeof(Color));

	Vec3 focal_vector, plane_center, corner_offset_vectors[2];
	multiply3(camera->vectors[2], camera->focal_length, focal_vector);
	add3(focal_vector, camera->position, plane_center);
	multiply3(camera->vectors[0], camera->image.size[X] / camera->image.resolution[X], camera->image.vectors[0]);
	multiply3(camera->vectors[1], camera->image.size[Y] / camera->image.resolution[Y], camera->image.vectors[1]);
	multiply3(camera->image.vectors[X], .5 - camera->image.resolution[X] / 2., corner_offset_vectors[X]);
	multiply3(camera->image.vectors[Y], .5 - camera->image.resolution[Y] / 2., corner_offset_vectors[Y]);
	add3_3(plane_center, corner_offset_vectors[X], corner_offset_vectors[Y], camera->image.corner);
}

void init_light(Light *light, Vec3 position, Vec3 intensity)
{
	memcpy(light->position, position, sizeof(Vec3));
	memcpy(light->intensity, intensity, sizeof(Vec3));
}

float get_closest_intersection(int num_objects, Object objects[num_objects], Line *ray, Object *closest_object, Vec3 closest_normal)
{
	Vec3 normal;
	float min_distance = FLT_MAX;
	int i;
	for(i = 0; i < num_objects; i++)
	{
		float distance;
		if(objects[i].common->get_intersection(objects[i], ray, &distance, normal)) {
			if(distance < min_distance) {
				min_distance = distance;
				*closest_object = objects[i];
				memcpy(closest_normal, normal, sizeof(Vec3));
			}
		}
	}
	return min_distance;
}

bool is_intersection_in_distance(int num_objects, Object objects[num_objects], Line *ray, float min_distance)
{
	int i;
	for(i = 0; i < num_objects; i++)
	{
		if(objects[i].common->intersects_in_range(objects[i], ray, min_distance))
			return true;
	}
	return false;
}

void cast_ray(Camera *camera, int num_objects, Object objects[num_objects], int num_lights, Light *lights[num_lights], Line *ray, Vec3 kr, Vec3 color)
{
	Object closest_object;
	closest_object.common = NULL;
	Vec3 closest_normal;//TODO: Rename
	float min_distance = get_closest_intersection(num_objects, objects, ray, &closest_object, closest_normal);
	if(closest_object.common) {
		CommonObject *object = closest_object.common;

		normalize3(closest_normal);
		if(dot3(closest_normal, ray->vector) > 0.f)
			multiply3(closest_normal, -1.f, closest_normal);

		//TODO: move to algorithm.c
		//Phong reflection model
		Vec3 obj_color;

		Vec3 point;
		multiply3(ray->vector, min_distance, point);
		add3(point, ray->position, point);

		//ambient
		Vec3 ambient;
		multiply3v(object->ka, ambient_light_intensity, obj_color);

		//Line from point to light
		Line light_ray;
		memcpy(light_ray.position, point, sizeof(Vec3));

		int i;
		for(i = 0; i < num_lights; i++)
		{
			Light *light = lights[i];

			//Line from point to light
			subtract3(light->position, point, light_ray.vector);
			float light_distance = magnitude3(light_ray.vector);
			normalize3(light_ray.vector);

			float a = dot3(light_ray.vector, closest_normal);

			//direction of a reflected ray of light from point
			Vec3 reflected;
			multiply3(closest_normal, 2 * a, reflected);
			subtract3(reflected, light_ray.vector, reflected);

			if(! is_intersection_in_distance(num_objects, objects, &light_ray, light_distance)) {
				Vec3 diffuse;
				multiply3v(object->kd, light->intensity, diffuse);
				multiply3(diffuse, fmaxf(0., a), diffuse);

				Vec3 specular;
				multiply3v(object->ks, light->intensity, specular);
				multiply3(specular, fmaxf(0., pow(- dot3(reflected, ray->vector), object->alpha)), specular);

				add3_3(obj_color, diffuse, specular, obj_color);
			}
		}
		multiply3v(obj_color, kr, obj_color);
		add3(color, obj_color, color);
		//END OF PHONG REFLECTION MODEL

		//reflection
		if(object->beta > EPSILON) {
			Line reflection_ray;
			memcpy(reflection_ray.position, point, sizeof(Vec3));
			multiply3(closest_normal, 2 * dot3(ray->vector, closest_normal), reflection_ray.vector);
			subtract3(ray->vector, reflection_ray.vector, reflection_ray.vector);
			cast_ray(camera, num_objects, objects, num_lights, lights, &reflection_ray, object->kr, color);
		}
	}
}

void create_image(Camera *camera, int num_objects, Object objects[num_objects], int num_lights, Light *lights[num_lights])
{
	Vec3 kr = {1.f, 1.f, 1.f};
	#ifdef MULTITHREADING
	#pragma omp parallel for
	#endif
	for(int row = 0; row < camera->image.resolution[Y]; row++)
	{
		Vec3 pixel_position;
		multiply3(camera->image.vectors[Y], row, pixel_position);
		add3(pixel_position, camera->image.corner, pixel_position);
		Line ray;
		memcpy(ray.position, camera->position, sizeof(Vec3));
		int pixel_index = camera->image.resolution[X] * row;
		int col;
		for(col = 0; col < camera->image.resolution[X]; col++)
		{
			add3(pixel_position, camera->image.vectors[X], pixel_position);
			Vec3 color = {0., 0., 0.};
			subtract3(pixel_position, camera->position, ray.vector);
			normalize3(ray.vector);
			cast_ray(camera, num_objects, objects, num_lights, lights, &ray, kr, color);
			multiply3(color, 255., color);
			uint8_t *pixel = camera->image.pixels[pixel_index];
			pixel[0] = fmaxf(fminf(color[0], 255.), 0.);
			pixel[1] = fmaxf(fminf(color[1], 255.), 0.);
			pixel[2] = fmaxf(fminf(color[2], 255.), 0.);
			pixel_index++;
		}
	}
}

void save_image(char *filename, Image *image)
{
	FILE *file = fopen(filename, "wb");
	assert(file);
	fprintf(file, "P6\n%d %d\n255\n", image->resolution[X], image->resolution[Y]);
	size_t num_pixels = image->resolution[X] * image->resolution[Y];
    assert(fwrite(image->pixels, sizeof(Color), num_pixels, file) == num_pixels);
    fclose(file);
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	#ifdef DISPLAY_TIME
	struct timespec current_t;
	clock_gettime(CLOCK_MONOTONIC, &current_t);
	float init_time = current_t.tv_sec + current_t.tv_nsec * 1e-9f;
	#endif

	#ifdef MULTITHREADING
	NUM_THREADS = omp_get_max_threads();
	omp_set_num_threads(NUM_THREADS);
	#endif

	Camera camera;
	init_camera(&camera,
		(Vec3) {0.f, 0.f, 0.f},
		(Vec3[2]) {
			{1.f, 0.f, 0.f},
			{0.f, 1.f, 0.f}},
		1.f,
		(int[2]) {640, 480},
		(Vec2) {4.f/3.f, 1.f});

	const int num_objects = 4;

	Object objects[num_objects];

	objects[0].sphere = init_sphere(
		(Vec3) {0.f, 0.f, 0.f},
		(Vec3) {0.f, 1.f, 0.f},
		(Vec3) {0.f, .5f, 0.f},
		(Vec3) {0.f, 0.f, 0.f},
		0.f,
		(Vec3) {-3.f, 0.f, 9.f},
		1.f);

	objects[1].sphere = init_sphere(
		(Vec3) {.9f, .9f, .9f},
		(Vec3) {1.f, 0.f, 0.f},
		(Vec3) {.5f, 0.f, 0.f},
		(Vec3) {1.f, 1.f, 1.f},
		15.f,
		(Vec3) {-3.f, 1.f, 7.f},
		1.5f);

	objects[2].plane = init_plane(
		(Vec3) {0.f, 0.f, 0.f},
		(Vec3) {.5f, .5f, .5f},
		(Vec3) {1.f, 1.f, 1.f},
		(Vec3) {0.f, 0.f, 0.f},
		0.f,
		(Vec3) {0.f, -1.f, 0.f},
		(Vec3) {0.f, 4.f, 0.f});

	FILE *file = fopen("meshes/utah_teapot.stl", "rb");
	assert(file);
	objects[3].mesh = stl_load(
		(Vec3) {.9f, .9f, .9f},
		(Vec3) {0.f, 0.f, .5f},
		(Vec3) {0.f, 0.f, .5f},
		(Vec3) {1.f, 1.f, 1.f},
		20.f,
		file,
		(Vec3) {2.f, 2.5f, 8.f},
		(Vec3) {PI/2.f, 0.f, 0.f},
		0.4f);
	fclose(file);

	const int num_lights = 1;

	Light *lights[num_lights];

	lights[0] = malloc(sizeof(Light));
	init_light(lights[0],
		(Vec3) {5.f, -5.f, 5.f},
		(Vec3) {1.f, 1.f, 1.f});

	PRINT_TIME("[%07.3f] INITIALIZED SCENE. BEGAN RENDERING.\n");

	create_image(&camera, num_objects, objects, num_lights, lights);

	PRINT_TIME("[%07.3f] FINISHED RENDERING.\n");

	save_image("test.ppm", &camera.image);

	PRINT_TIME("[%07.3f] SAVED IMAGE.\n");

	free(camera.image.pixels);
	int i;
	for(i = 0; i < num_objects; i++)
	{
		objects[i].common->delete(objects[i]);
	}
	for(i = 0; i < num_lights; i++)
	{
		free(lights[i]);
	}

	PRINT_TIME("[%07.3f] TERMINATED PROGRAM.\n");
}
