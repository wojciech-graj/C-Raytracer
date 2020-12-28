SRC_FILES := main.c vector.c objects.c stl.c algorithm.c
LINUX_CFLAGS := -lm -std=c11 -Wno-address-of-packed-member
WINDOWS_CFLAGS := -LC:\MinGW\lib -IC:\MinGW\include -mwindows -lmingw32 -fopenmp
RELEASE_CFLAGS := -Ofast -DMULTITHREADING -fopenmp
DEBUG_CFLAGS := -Wall -Wextra -Wdouble-promotion -Wpedantic -Wstrict-prototypes -Wshadow -g -fsanitize=address -fsanitize=undefined -Og -DDEBUG

SRC_PATH =

debug:
	gcc $(addprefix $(SRC_PATH), $(SRC_FILES)) -o engine $(LINUX_CFLAGS) $(DEBUG_CFLAGS)
release:
	gcc $(addprefix $(SRC_PATH), $(SRC_FILES)) -o engine $(LINUX_CFLAGS) $(RELEASE_CFLAGS)
windows:
	mingw32-gcc $(addprefix $(SRC_PATH), $(SRC_FILES)) -o engine_win.exe $(WINDOWS_CFLAGS) $(RELEASE_CFLAGS)
