STB_INCLUDE_PATH = ./include/stb/
TOL_INCLUDE_PATH = ./include/tol/


CFLAGS = -std=c++20 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

editor: main.cpp
	g++ $(CFLAGS) -o build/editor main.cpp $(LDFLAGS) -I$(STB_INCLUDE_PATH) -I$(TOL_INCLUDE_PATH)

.PHONY: test clean shaders

test: editor
	./build/editor

clean:
	rm -f ./build/editor

shaders:
	glslc shaders/shader.vert -o shaders/vert.spv & glslc shaders/shader.frag -o shaders/frag.spv
	