STB_INCLUDE_PATH = ./include/stb/
TOL_INCLUDE_PATH = ./include/tol/
TOL_H_PATH = ./include/tol/tiny_object_loader.h


CFLAGS = -std=c++20 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

editor: main.cpp
	g++ $(CFLAGS) -o build/editor main.cpp model_loader/model_loader.cpp model_loader/model_loader.h primitives.h  $(LDFLAGS) -I$(STB_INCLUDE_PATH) -I$(TOL_INCLUDE_PATH)


.PHONY: test clean shaders

test: editor
	./build/editor

clean:
	rm -f ./build/editor

shaders:
	glslc shaders/shader.vert -o shaders/vert.spv & glslc shaders/shader.frag -o shaders/frag.spv