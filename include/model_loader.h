#ifndef ALE_LOADER
#define ALE_LOADER

// ext
#pragma once
#include <vector>
#include <functional>   
#include <string>
#include <unordered_map>

// int
#include <primitives.h>
#include <tracer.h>

namespace ale {
    class Loader {
        
    public: 
        Loader();
        ~Loader();
        void loadModelOBJ(char *model_path, Mesh &_model);
        int loadModelGLTF(const std::string filename, Mesh &_model, Image &_image);
        bool loadTexture(const char* path, Image& img);
    };

} // namespace loader

#endif // ALE_LOADER



