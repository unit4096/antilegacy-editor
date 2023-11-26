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


void Loader::loadModelOBJ(char *model_path, Model& _model) {

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
                uniqueVertices[vertex] = static_cast<u_int32_t>(_model.vertices.size());
                _model.vertices.push_back(vertex);
            }

            _model.indices.push_back(uniqueVertices[vertex]);
        }
    }
}

// TODO: Add loading full GLTF scenes
// Now loads one mesh and one texture from the .gltf file
int Loader::loadModelGLTF(const std::string filename, Model& _model, Image& _image) {

    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    // TODO: implement model type deduction, test with .glb models
    bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);
    //bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename); // for binary glTF(.glb)

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
    
    if (gltfModel.textures.size() > 0) {
        tinygltf::Texture &tex = gltfModel.textures[0];
        
        if (tex.source > -1) {
            tinygltf::Image &image = gltfModel.images[tex.source];
            
            _image.data = &image.image.at(0);
            _image.w = image.width;
            _image.h = image.height;            
        }
        

    }    

    // TODO: implement loading multiple nodes
    tinygltf::Mesh mesh = gltfModel.meshes[0];    

    for (auto primitive : mesh.primitives) {

        std::unordered_map<Vertex, unsigned int> uniqueVertices{};
        
        // TOOD: implement generic attribute deduction
        const tinygltf::Accessor& posAccessor = gltfModel.accessors[primitive.attributes["POSITION"]];
        const tinygltf::Accessor& UVAccessor = gltfModel.accessors[primitive.attributes["TEXCOORD_0"]];
        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[posAccessor.bufferView];

        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
        
        
        const float* positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + posAccessor.byteOffset]);
        const float* uvPositions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + UVAccessor.byteOffset]);
        
        for (size_t i = 0; i < posAccessor.count; ++i) {
            Vertex vertex{};

            vertex.pos = {
                positions[i * 3 + 0],
                positions[i * 3 + 1],
                positions[i * 3 + 2],
            };
            
            // FIXME: the UV layout is completely wrong
            vertex.texCoord = {
                uvPositions[i * 2 + 0],
                1.0f - uvPositions[i * 2 + 1],
            };
            vertex.color = {1.0f, 1.0f, 1.0f};                        


            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<u_int32_t>(_model.vertices.size());
                _model.vertices.push_back(vertex);
            }

            _model.indices.push_back(uniqueVertices[vertex]);
        }
    }
    
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