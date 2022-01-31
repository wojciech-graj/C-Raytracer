TARGET = engine
COMPILER = gcc

SRC = src/*.c
LIB = lib/cJSON/cJSON.c lib/SimplexNoise/SimplexNoise.c
WARNINGS := -Wall -Wextra -Wpedantic -Wdouble-promotion -Wstrict-prototypes -Wshadow -Wduplicated-cond -Wduplicated-branches -Wjump-misses-init -Wnull-dereference -Wrestrict -Wlogical-op -Wno-maybe-uninitialized -Walloc-zero -Wformat-security -Wformat-signedness -Winit-self -Wlogical-op -Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wswitch-enum -Wundef -Wwrite-strings -Wno-address-of-packed-member -Wno-discarded-qualifiers
CFLAGS := -std=c11 -march=native -DUNBOUND_OBJECTS
LINK_CFLAGS := -lm -Ilib
RELEASE_CFLAGS := -Ofast -fopenmp -DMULTITHREADING
DEBUG_CFLAGS := -g -Og -DDEBUG -fsanitize=address -fsanitize=undefined

debug:
	$(COMPILER) $(SRC) $(LIB) -o $(TARGET) $(WARNINGS) $(CFLAGS) $(LINK_CFLAGS) $(DEBUG_CFLAGS)
release:
	$(COMPILER) $(SRC) $(LIB) -o $(TARGET) $(WARNINGS) $(CFLAGS) $(LINK_CFLAGS) $(RELEASE_CFLAGS)
clean:
	rm -rf $(TARGET)
