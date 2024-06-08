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
#include <glm/gtc/type_ptr.hpp>
#include <thread>
#include <future>

// int
#include <primitives.h>
#include <re_mesh.h>
#include <tracer.h>
#include <tinygltf/tiny_gltf.h>
#include <tol/tiny_obj_loader.h>
#include <ale_geo_utils.h>
#include <memory.h>

namespace ale {

class Loader {
public:
    Loader();
    ~Loader();
    int loadModelOBJ(char *model_path, ViewMesh &_model);
    int loadModelGLTF(const std::string model_path, ale::Model &out_model);
    bool loadTexture(const char *path, Image &img);
    void recordCommandLineArguments(int &argc, char **argv);
    int getFlaggedArgument(const std::string flag, std::string &result);
    const std::string &getCmdOption(const std::string &option) const;
    bool cmdOptionExists(const std::string &option) const;
    static bool isFileValid(std::string file_path);
    static std::vector<char> getFileContent(const std::string& file_path);
    int populateREMesh(ViewMesh &_inpMesh, geo::REMesh &_outMesh);

private:
    std::vector<std::string> commandLineTokens;
    // System IO methods
    static bool _canReadFile(std::filesystem::path p);
    // Geometry methods
    static void _generateVertexNormals(ale::ViewMesh &_mesh);
    const int _getNumEdgesInMesh(const ViewMesh &_mesh);

    // TinyGlTF methods
    static const unsigned char *_getDataByAccessor(tinygltf::Accessor accessor, const tinygltf::Model &model);        
    static int _loadMeshGLTF(const tinygltf::Model &in_model, const tinygltf::Mesh &in_mesh, ale::ViewMesh &out_mesh);
    int _loadTinyGLTFModel(tinygltf::Model &gltfModel, const std::string &filename);
    int _loadTexturesGLTF(const tinygltf::Model &in_model, ale::Model &out_model);        
    int _loadNodesGLTF(const tinygltf::Model &in_model, ale::Model &out_model);
    int _loadTextureGLTF(const tinygltf::Image &in_texture, ale::Image &out_texture);
    void _bindNodeGLTF(const tinygltf::Model &in_model, const tinygltf::Node &n, int parent, int current, ale::Model &out_model);
    static int _tryLoadMeshIndices(const tinygltf::Model& in_model, const tinygltf::Primitive& primitive, ale::ViewMesh& out_mesh);
    static int _loadMaterialsGLTF(const tinygltf::Model& in_model, ale::Model& out_model);

};

} // namespace loader

#endif // ALE_LOADER



