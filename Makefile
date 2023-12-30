# Include directories
INCLUDE_PATH := ./include/
IMGUI_INCLUDE_PATH:= ./include/imgui/

# Compiler flags
CXX = clang++
CXXFLAGS = -std=c++20 -O2 -Wall -g -fsanitize=address

# Library dependencies
LDFLAGS:= -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# Source directories
SRC_DIR = ./src
IMGUI_SRC_DIR = include/imgui

OBJ_DIR = obj
IMGUI_OBJ_DIR = obj/imgui

BIN_DIR = build

# All include directories
INCLUDE_ALL := -I$(INCLUDE_PATH) -I$(IMGUI_INCLUDE_PATH)

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
IMGUI_SRCS = $(wildcard $(IMGUI_SRC_DIR)/*.cpp)

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
IMGUI_OBJS = $(patsubst $(IMGUI_SRC_DIR)/%.cpp, $(IMGUI_OBJ_DIR)/%.o, $(IMGUI_SRCS))

# Executable file
MAIN = $(BIN_DIR)/editor

.PHONY: all clean test shaders clean_main
# Targets

clean_main:
	rm $(MAIN)

test: all
	./$(MAIN) -f ./models/fox/Fox.gltf

all: $(MAIN)

# Main target
$(MAIN):  $(IMGUI_OBJS) $(OBJS) 
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(INCLUDE_ALL) $(LDFLAGS)

# Object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_ALL)

# Imgui object files. I build them separately
$(IMGUI_OBJ_DIR)/%.o: $(IMGUI_SRC_DIR)/%.cpp
	@mkdir -p $(IMGUI_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(IMGUI_INCLUDE_PATH)

# Include dependency files
-include $(OBJS:.o=.d)

# Clean target
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

shaders:
	glslc shaders/shader.vert -o shaders/vert.spv & glslc shaders/shader.frag -o shaders/frag.spv