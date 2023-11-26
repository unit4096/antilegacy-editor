#include <unordered_map>
#include "model_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tol/tiny_obj_loader.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

using namespace ale;

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

    // throw std::runtime_error("loadModelGLTF: function not implemented!");
    // return -1;
    

    tinygltf::Mesh mesh = model.meshes[0];
    
    
    
    // TODO: implement proper model loading
    

    for (auto primitive : mesh.primitives) {

        std::unordered_map<Vertex, unsigned int> uniqueVertices{};

        const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes["POSITION"]];
        const tinygltf::Accessor& UVAccessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
        const tinygltf::BufferView& bufferView = model.bufferViews[posAccessor.bufferView];

        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        
        
        const float* positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + posAccessor.byteOffset]);
        const float* uvPositions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + UVAccessor.byteOffset]);
        
        for (size_t i = 0; i < posAccessor.count; ++i) {
                // Positions are Vec3 components, so for each vec3 stride, offset for x, y, and z.
            Vertex vertex{};

            vertex.pos = {
                positions[i * 3 + 0],
                positions[i * 3 + 1],
                positions[i * 3 + 2],
            };

            vertex.texCoord = {
                uvPositions[i * 2 + 0],
                1.0f - uvPositions[i * 2 + 1],
            };
            vertex.color = {1.0f, 1.0f, 1.0f};                        


            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<u_int32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
    
    // std::cout << "indices:" << std::endl;
    
    // for (auto i: indices) {
    //     std::cout << i << std::endl;
    // }

    // std::cout << "vertices: "<< std::endl;
    // for (auto i: vertices) {
    //     std::cout << i.pos[0] << " " << i.pos[1] << " " << i.pos[2] << std::endl;    
    // }
    
    return 0;
}


bool Loader::loadTexture(const char* path, Image& img) {
    
    unsigned char* _data = stbi_load(path, &img.w, &img.h, &img.channels, STBI_rgb_alpha);

    if (!_data) {        
        return 1;
    }
    
    img.data = _data;
    
    return 0;
}


void Loader::unloadBuffer(unsigned char* _pixels){
    if (_pixels) {
        stbi_image_free(_pixels);        
    }
}