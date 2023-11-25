#include <unordered_map>
#include "model_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tol/tiny_obj_loader.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

using namespace loader;

Loader::Loader() { }   

Loader::~Loader() { }

void Loader::loadModelOBJ(char *model_path,
                        std::vector<unsigned int> &indices,
                        std::vector<Vertex> &vertices) {

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path)) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, unsigned int> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<u_int32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

// TODO: this is a WIP, not working for now
int Loader::loadModelGLTF(const std::string filename,
                        std::vector<unsigned int> &indices,
                        std::vector<Vertex> &vertices) {

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    //bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb)

    if (!warn.empty()) {
        throw std::runtime_error(warn);
    }

    if (!err.empty()) {
        throw std::runtime_error(err);
    }

    if (!ret) {
        throw std::runtime_error("Could not parse a GLTF model!");
        return -1;
    }

    throw std::runtime_error("loadModelGLTF: function not implemented!");
    return -1;

    tinygltf::Mesh mesh = model.meshes[0];
    

    
    // TODO: implement proper model loading
    

    // for (auto primitive : mesh.primitives) {
    //     for (auto i : primitive.attributes) {
            
    //     }
    // }

    // return 0;
}





unsigned char* Loader::loadTexture(char *tex_path,
                            int &texWidth,
                            int &texHeight,
                            int &texChannels) {
    
    unsigned char* _pixels = stbi_load(tex_path, 
                            &texWidth,
                            &texHeight,
                            &texChannels,
                            STBI_rgb_alpha);

    if (!_pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    
    
    return _pixels;
}


void Loader::unloadBuffer(unsigned char* _pixels){
    if (_pixels) {
        stbi_image_free(_pixels);        
    }
}