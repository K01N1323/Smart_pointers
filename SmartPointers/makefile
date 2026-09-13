CXX := clang++
TARGET := smart_pointers

CPPFLAGS := -IInclude -IUi -ITests
BASE_CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -MMD -MP

SOURCES := Src/main.cpp Ui/Menu.cpp $(wildcard Tests/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
DEPENDENCIES := $(OBJECTS:.o=.d)

.PHONY: all debug release sanitize run clean

all: debug

debug:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(BASE_CXXFLAGS) -O0 -g" $(TARGET)

release:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(BASE_CXXFLAGS) -O2 -DNDEBUG" $(TARGET)

sanitize:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(BASE_CXXFLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" LDFLAGS="-fsanitize=address,undefined" $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: debug
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPENDENCIES)

-include $(DEPENDENCIES)