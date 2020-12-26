SRC_FILES := main.c vector.c
LINUX_CFLAGS := -lm -std=c11
WINDOWS_CFLAGS := -LC:\MinGW\lib -IC:\MinGW\include -mwindows -lmingw32
RELEASE_CFLAGS := -O3
DEBUG_CFLAGS := -Wall -Wextra -Wdouble-promotion -Wpedantic -Wstrict-prototypes -Wshadow -g -fsanitize=address -fsanitize=undefined -Og -DDEBUG

SRC_PATH =

debug:
	gcc $(addprefix $(SRC_PATH), $(SRC_FILES)) -o engine $(LINUX_CFLAGS) $(DEBUG_CFLAGS)
release:
	gcc $(addprefix $(SRC_PATH), $(SRC_FILES)) -o engine $(LINUX_CFLAGS) $(RELEASE_CFLAGS)
windows:
	mingw32-gcc $(addprefix $(SRC_PATH), $(SRC_FILES)) -o engine_win.exe $(WINDOWS_CFLAGS) $(RELEASE_CFLAGS)
