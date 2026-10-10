# AdvancePrint — static library for the Game Boy Advance (ARM, devkitARM).
# Built inside the devkitpro/devkitarm image; see ./build.sh.

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM

PREFIX := $(DEVKITARM)/bin/arm-none-eabi-
CC     := $(PREFIX)gcc
AR     := $(PREFIX)ar

TARGET := libAdvancePrint.a
BUILD  := build

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS))

CFLAGS := -mthumb -mthumb-interwork -mcpu=arm7tdmi -mtune=arm7tdmi -O2 \
          -Wall -Wextra -ffunction-sections -fdata-sections -MMD -MP -Iinclude

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(OBJS:.o=.d)

clean:
	rm -rf $(BUILD) $(TARGET)
