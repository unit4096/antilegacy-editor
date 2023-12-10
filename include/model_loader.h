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
        void loadModelOBJ(char *model_path, Model &_model);
        int loadModelGLTF(const std::string filename, Model &_model, Image &_image);
        bool loadTexture(const char* path, Image& img);
        void unloadBuffer(unsigned char *_pixels);
    };

} // namespace loader

#endif // ALE_LOADER



