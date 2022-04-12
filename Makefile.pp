TARGET := postprocess
CC := gcc

WARNINGS := -Wall -Wextra -Wpedantic -Wdouble-promotion -Wstrict-prototypes -Wshadow -Wduplicated-cond -Wduplicated-branches -Wjump-misses-init -Wnull-dereference -Wrestrict -Wlogical-op -Walloc-zero -Wformat-security -Wformat-signedness -Winit-self -Wlogical-op -Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wswitch-enum -Wundef -Wwrite-strings -Wno-address-of-packed-member -Wno-discarded-qualifiers
CFLAGS := -std=c11 -march=native -flto $(WARNINGS)
LDFLAGS := -lm -ltiff

BUILD_DIR := ./obj/postprocess
SRC_DIRS := ./src/core ./src/postprocess

ifeq ($(MAKECMDGOALS),debug)
CLFAGS += -g -Og -DDEBUG
LDFLAGS += -fsanitize=address -fsanitize=undefined -Og -g
else
CFLAGS += -O3
LDFLAGS += -O3
endif

SRCS := $(shell find $(SRC_DIRS) -name '*.c')

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(INC_FLAGS) $(CFLAGS) -c $< -o $@

debug: $(TARGET)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)

-include $(DEPS)
