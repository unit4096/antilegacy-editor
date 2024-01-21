#ifndef ALE_LOADER
#define ALE_LOADER

// ext
#pragma once
#include <vector>
#include <functional>   
#include <string>
#include <unordered_map>
#include <filesystem>
#include <set>
#include <utility>

// int
#include <primitives.h>
#include <re_mesh.h>
#include <tracer.h>
#include <tinygltf/tiny_gltf.h>
#include <tol/tiny_obj_loader.h>

namespace ale {
    class Loader {
    private:
        std::vector<std::string> commandLineTokens;
        
        bool _canReadFile(std::filesystem::path p);
        int _loadMesh(const tinygltf::Model &in_model, const tinygltf::Mesh &in_mesh, ale::Mesh &out_mesh);
        int _loadTexture(const tinygltf::Image &in_texture, ale::Image &out_texture);
        int _loadNodesGLTF(const tinygltf::Model &in_model, ale::Model &out_model);

    public:
        Loader();
        ~Loader();
        int loadModelOBJ(char *model_path, Mesh &_model);
        int loadModelGLTF(const std::string filename, Mesh &_model, Image &_image);
        bool loadTexture(const char* path, Image& img);
        void recordCommandLineArguments(int & argc, char ** argv);
        int getFlaggedArgument(const std::string flag, std::string &result);
        const std::string &getCmdOption(const std::string &option) const;
        bool cmdOptionExists(const std::string &option) const;
        bool isFileValid(std::string file_path);
    };

} // namespace loader

#endif // ALE_LOADER



