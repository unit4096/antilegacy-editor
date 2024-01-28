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
        int _loadMesh(const tinygltf::Model &in_model, const tinygltf::Mesh &in_mesh, ale::ViewMesh &out_mesh);
        int _loadTexturesGLTF(const tinygltf::Model &in_model, ale::Model &out_model);        
        int _loadTexture(const tinygltf::Image &in_texture, ale::Image &out_texture);
        int _loadNodesGLTF(const tinygltf::Model &in_model, ale::Model &out_model);
        void _bindNode(const tinygltf::Model &in_model, const tinygltf::Node &n, int parent, int current, ale::Model &out_model);

    public:
        Loader();
        ~Loader();
        int loadModelOBJ(char *model_path, ViewMesh &_model);
        int loadModelGLTF(const std::string model_path, ale::Model &out_model);
        bool loadTexture(const char* path, Image& img);
        void recordCommandLineArguments(int & argc, char ** argv);
        int getFlaggedArgument(const std::string flag, std::string &result);
        const std::string &getCmdOption(const std::string &option) const;
        bool cmdOptionExists(const std::string &option) const;
        bool isFileValid(std::string file_path);
    };

} // namespace loader

#endif // ALE_LOADER



