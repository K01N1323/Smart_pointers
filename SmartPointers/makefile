CXX := clang++

TARGET := smart_pointers
MODE ?= debug

CPPFLAGS := -IInclude -IUi -ITests
BASE_CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -MMD -MP

SOURCES := Src/main.cpp Ui/Menu.cpp

BUILD_DIR := build/$(MODE)
BINARY := $(BUILD_DIR)/$(TARGET)
OBJECTS := $(addprefix $(BUILD_DIR)/,$(SOURCES:.cpp=.o))
DEPENDENCIES := $(OBJECTS:.o=.d)

ifeq ($(MODE),release)
	MODE_FLAGS := -O2 -DNDEBUG
	CPPFLAGS += -DPERFORMANCE_ENABLED
else ifeq ($(MODE),sanitize)
	MODE_FLAGS := -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined
	LDFLAGS += -fsanitize=address,undefined
else
	MODE_FLAGS := -O0 -g
endif

.DEFAULT_GOAL := all

.PHONY: all debug release sanitize run test check-sanitize check-leaks clean

all: $(BINARY)
	cp $(BINARY) $(TARGET)

$(BINARY): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.cpp makefile
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(BASE_CXXFLAGS) $(MODE_FLAGS) -c $< -o $@

debug:
	$(MAKE) MODE=debug all

release:
	$(MAKE) MODE=release all

sanitize:
	$(MAKE) MODE=sanitize all

run: all
	./$(TARGET)

test:
	$(MAKE) MODE=debug all
	printf '3\n4\n5\n0\n' | ./build/debug/$(TARGET)

check-sanitize:
	$(MAKE) MODE=sanitize all
	printf '3\n4\n5\n0\n' | ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./build/sanitize/$(TARGET)

check-leaks:
	$(MAKE) MODE=sanitize all
	printf '3\n4\n5\n0\n' | ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./build/sanitize/$(TARGET)

clean:
	rm -rf build
	rm -f $(TARGET) $(SOURCES:.cpp=.o) $(SOURCES:.cpp=.d)

-include $(DEPENDENCIES)
