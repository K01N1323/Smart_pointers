CXX := clang++

CPPFLAGS := -IInclude -IUi -ITests
BASE_CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -MMD -MP

SANITIZE_FLAGS := -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined
DEBUG_FLAGS := -O0 -g
RELEASE_FLAGS := -O2 -DNDEBUG -DPERFORMANCE_ENABLED

SOURCES := Src/main.cpp Ui/Menu.cpp

SANITIZE_BINARY := build/sanitize/smart_pointers
DEBUG_BINARY := build/debug/smart_pointers
RELEASE_BINARY := build/release/smart_pointers

SANITIZE_OBJECTS := $(addprefix build/sanitize/,$(SOURCES:.cpp=.o))
DEBUG_OBJECTS := $(addprefix build/debug/,$(SOURCES:.cpp=.o))
RELEASE_OBJECTS := $(addprefix build/release/,$(SOURCES:.cpp=.o))

DEPENDENCIES := $(SANITIZE_OBJECTS:.o=.d) $(DEBUG_OBJECTS:.o=.d) $(RELEASE_OBJECTS:.o=.d)

.DEFAULT_GOAL := all

.PHONY: all clean

all: smart_pointers smart_pointers_leaks smart_pointers_release

$(SANITIZE_BINARY): $(SANITIZE_OBJECTS)
	$(CXX) $(SANITIZE_OBJECTS) -fsanitize=address,undefined -o $@

$(DEBUG_BINARY): $(DEBUG_OBJECTS)
	$(CXX) $(DEBUG_OBJECTS) -o $@

$(RELEASE_BINARY): $(RELEASE_OBJECTS)
	$(CXX) $(RELEASE_OBJECTS) -o $@

build/sanitize/%.o: %.cpp makefile
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(BASE_CXXFLAGS) $(SANITIZE_FLAGS) -c $< -o $@

build/debug/%.o: %.cpp makefile
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(BASE_CXXFLAGS) $(DEBUG_FLAGS) -c $< -o $@

build/release/%.o: %.cpp makefile
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(BASE_CXXFLAGS) $(RELEASE_FLAGS) -c $< -o $@

smart_pointers: $(SANITIZE_BINARY) makefile
	printf '%s\n' '#!/bin/sh' \
		'cd -- "$$(dirname -- "$$0")" || exit 1' \
		'export ASAN_OPTIONS=detect_leaks=0:halt_on_error=1' \
		'export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1' \
		'exec ./build/sanitize/smart_pointers "$$@"' > $@
	chmod +x $@

smart_pointers_leaks: $(DEBUG_BINARY) makefile
	printf '%s\n' '#!/bin/sh' \
		'cd -- "$$(dirname -- "$$0")" || exit 1' \
		'exec leaks --atExit -- ./build/debug/smart_pointers "$$@"' > $@
	chmod +x $@

smart_pointers_release: $(RELEASE_BINARY) makefile
	cp $(RELEASE_BINARY) $@

clean:
	rm -rf build
	rm -f smart_pointers smart_pointers_leaks smart_pointers_release
	rm -f $(SOURCES:.cpp=.o) $(SOURCES:.cpp=.d)

-include $(DEPENDENCIES)