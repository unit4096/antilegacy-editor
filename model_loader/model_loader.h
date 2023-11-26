#ifndef ANTI_LOADER
#define ANTI_LOADER

#pragma once
#include <vector>
#include <functional>   
#include <primitives.h>
#include <string>

namespace ale {
    class Loader {
        
    public: 
        Loader();
        ~Loader();
        void loadModelOBJ(char *model_path, std::vector<unsigned int> &indices, std::vector<Vertex> &vertices);
        int loadModelGLTF(const std::string filename, std::vector<unsigned int> &indices, std::vector<Vertex> &vertices);
        bool loadTexture(const char* path, Image& img);
        void unloadBuffer(unsigned char *_pixels);
    };

} // namespace loader

#endif // ANTI_LOADER



