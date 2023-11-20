#ifndef ANTI_LOADER
#define ANTI_LOADER

#include <vector>
#include <functional>   
#include "../primitives.h"

namespace loader {
    class Loader {
        
    public: 
        Loader();
        ~Loader();
        void loadModel(char *model_path, std::vector<unsigned int> &indices, std::vector<Vertex> &vertices);
        unsigned char *loadTexture(char *tex_path, int &texWidth, int &texHeight, int &texChannels);
        void unloadBuffer(unsigned char *_pixels);
    };

} // namespace loader

#endif // ANTI_LOADER



