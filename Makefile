STB_INCLUDE_PATH = ./include/stb/
TOL_INCLUDE_PATH = ./include/tol/


CFLAGS = -std=c++20 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS) -I$(STB_INCLUDE_PATH) -I$(TOL_INCLUDE_PATH)

.PHONY: test clean

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest