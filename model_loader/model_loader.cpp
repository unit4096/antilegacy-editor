#include <unordered_map>
#include "model_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tol/tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace loader;

Loader::Loader() { }   

Loader::~Loader() { }

void Loader::loadModel(char *model_path, std::vector<unsigned int> &indices, std::vector<Vertex> &vertices) {

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


unsigned char* Loader::loadTexture(char *tex_path,
                            int &texWidth,
                            int &texHeight,
                            int &texChannels) {
    
    unsigned char* _pixels = stbi_load(tex_path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    

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