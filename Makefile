SRC_FILES := main.c vector.c objects.c stl.c algorithm.c scene.c
LINUX_CFLAGS := -lm -std=c11 -Wno-address-of-packed-member -DDISPLAY_TIME
RELEASE_CFLAGS := -Ofast -DMULTITHREADING -fopenmp
DEBUG_CFLAGS := -Wall -Wextra -Wdouble-promotion -Wpedantic -Wstrict-prototypes -Wshadow -g -fsanitize=address -fsanitize=undefined -Og -DDEBUG

SRC_PATH =

debug:
	gcc $(addprefix $(SRC_PATH), $(SRC_FILES)) -o engine $(LINUX_CFLAGS) $(DEBUG_CFLAGS)
release:
	gcc $(addprefix $(SRC_PATH), $(SRC_FILES)) -o engine $(LINUX_CFLAGS) $(RELEASE_CFLAGS)
