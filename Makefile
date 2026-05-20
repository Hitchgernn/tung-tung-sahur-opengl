CXX := g++
TARGET := tung_tung_sahur
SRC := src/main.cpp

CXXFLAGS := -std=c++17 -Wall -Wextra -O2
CPPFLAGS := $(shell pkg-config --cflags glfw3 glew)
LDLIBS := $(shell pkg-config --libs glfw3 glew) -lGL -lm

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $< -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
