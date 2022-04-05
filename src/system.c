/*
 * Copyright (c) 2021-2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *  Nil
 **/

#include "system.h"
#include "argv.h"
#include "error.h"
#include "strhash.h"

#include <stdlib.h>
#include <time.h>

#ifdef MULTITHREADING
#include <omp.h>
#endif

typedef enum _LogOption {
	LOG_REALTIME,
	LOG_CPUTIME,
} LogOption;

static struct timespec start_t;
static clock_t start_clock;

static LogOption log_option;

void system_init(void)
{
	timespec_get(&start_t, TIME_UTC);
	start_clock = clock();

	srand((unsigned) start_t.tv_sec);

	int idx;
	idx = argv_check_with_args("-p", 1);
	if (idx) {
		switch (hash_myargv[idx + 1]) {
		case 2088303039://real
			log_option = LOG_REALTIME;
			break;
		case 193416643://cpu
			log_option = LOG_CPUTIME;
			break;
		}
	}

	idx = argv_check_with_args("-m", 1);
#ifdef MULTITHREADING
	unsigned num_threads = 1;
#endif
	if (idx) {
#ifdef MULTITHREADING
		unsigned max_threads = omp_get_max_threads();
		if (hash_myargv[idx + 1] == 193414065u) {//max
			num_threads = max_threads;
		} else {
			num_threads = abs(atoi(myargv[idx + 1]));
			error_check(num_threads <= max_threads, "Requested thread count [%u] exceeds maximum [%u].");
		}
#else
		error("Multithreading is disabled.");
#endif /* MULTITHREADING */
	}
#ifdef MULTITHREADING
	printf_log("Using %u threads.", num_threads);
	omp_set_num_threads(num_threads);
#endif
}

double system_time(void)
{
	switch (log_option) {
	case LOG_REALTIME: {
		struct timespec cur_t;
		timespec_get(&cur_t, TIME_UTC);
		return (cur_t.tv_sec - start_t.tv_sec) + (cur_t.tv_nsec - start_t.tv_nsec) * 1e-9;
		}
		break;
	case LOG_CPUTIME: {
		clock_t cur_t = clock();
		return (double)(cur_t - start_clock) / CLOCKS_PER_SEC;
		}
		break;
	}
	return 0;
}

float rand_flt(void)
{
	return rand() / (float)RAND_MAX;
}
