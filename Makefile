INCLUDE_PATH := ./include/
LOADER_INCLUDE_PATH:= ./model_loader/
IMGUI_INCLUDE_PATH:= ./include/imgui/

CFLAGS:= -std=c++20 -O2
LDFLAGS:= -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

.PHONY: test clean shaders run

editor: renderer.pcm build/imgui/imgui_impl_glfw.o build/imgui/imgui_impl_vulkan.o
	clang++ $(CFLAGS) build/imgui/imgui*.o  main.cpp -fprebuilt-module-path=. renderer.pcm \
	build/loader.o -o build/editor -I$(INCLUDE_PATH)  $(LDFLAGS)

renderer: loader
	clang++ $(CFLAGS) build/loader.o renderer.cppm  --precompile -o renderer.pcm -I$(INCLUDE_PATH) -I$(LOADER_INCLUDE_PATH)

loader:
	clang++ -c $(CFLAGS) model_loader/model_loader.cpp -I$(INCLUDE_PATH) -o build/loader.o 

imgui: 
	cd build/imgui/ &&\
	clang++ $(CFLAGS) -c ../../include/imgui/imgui*.cpp -I$(INCLUDE_PATH) &&\
	cd ../..


test: editor
	./build/editor

run:
	./build/editor

clean:
	rm -f ./build/editor &\
	rm *.o

shaders:
	glslc shaders/shader.vert -o shaders/vert.spv & glslc shaders/shader.frag -o shaders/frag.spv