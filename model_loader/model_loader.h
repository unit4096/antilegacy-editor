#ifndef ANTI_LOADER
#define ANTI_LOADER

#include <vector>
#include "../primitives.h"

namespace loader {
    class ModelLoader {
    public: 
        ModelLoader();
        ~ModelLoader();
        void loadModel(char *model_path, std::vector<unsigned int> &indices, std::vector<Vertex> &vertices);
    };

} // namespace loader

#endif // ANTI_LOADER



