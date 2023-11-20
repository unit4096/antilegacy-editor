INCLUDE_PATH = ./include/
LOADER_INCLUDE_PATH = ./model_loader/


CFLAGS = -std=c++20 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SOURCES:= main.cpp model_loader/model_loader.cpp model_loader/model_loader.h include/primitives.h


editor: main.cpp
	g++ $(CFLAGS) -o build/editor $(SOURCES)  $(LDFLAGS) -I$(INCLUDE_PATH) -I$(LOADER_INCLUDE_PATH)


.PHONY: test clean shaders

test: editor
	./build/editor

clean:
	rm -f ./build/editor

shaders:
	glslc shaders/shader.vert -o shaders/vert.spv & glslc shaders/shader.frag -o shaders/frag.spv