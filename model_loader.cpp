#include "model_loader.h"

// ext

/* 
TinyOBJLoader and TinyGLTF must be included in a .cpp file. Thus, I use helper 
functions as non-class members that do not expose TinyOBJLoader and TinyGLTF
to the header.
*/

#define TINYOBJLOADER_IMPLEMENTATION
#include <tol/tiny_obj_loader.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

using namespace ale;

namespace trc = ale::Tracer;

/* 
Declarations for the helper functions. I use the notation for private members to 
distinguish them from functions in the header.
*/

const unsigned char* _getDataByAccessor(tinygltf::Accessor accessor, tinygltf::Model& model);
int _loadTinyGLTFModel(tinygltf::Model& gltfModel, const std::string& filename);

Loader::Loader() { }   

Loader::~Loader() { }

// Load an .obj model using a path relative to the project root
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

    _loadTinyGLTFModel(gltfModel, filename);
    
    if (gltfModel.textures.size() > 0) {
        trc::log("Found textures");
        tinygltf::Texture &tex = gltfModel.textures[0];
        
        if (tex.source > -1) {
            trc::log("Loading texture");
            tinygltf::Image &image = gltfModel.images[tex.source];

            // image.component; // Defines the dimensions of each texel
            // image.bits // Defines the number of bits in each dimension

            _image.data = &image.image.at(0);
            _image.w = image.width;
            _image.h = image.height;            
        } else {
            trc::log("Cannot load texture");
        }

    } else {
        trc::log("Textures not found");
    }


    // TODO: implement loading multiple nodes
    tinygltf::Mesh mesh = gltfModel.meshes[0];    

    for (auto primitive : mesh.primitives) {    

        // List model attributes for Debug
        for (auto attr: primitive.attributes) {
            trc::log("This model has " + attr.first);
        }
        
        std::unordered_map<Vertex, unsigned int> uniqueVertices{};

        // TOOD: implement generic attribute deduction

        // NOTE: in tinyGLTF an index acessor is a separate accessor
        // As for now is not used (for a reason)
        // tinygltf::Accessor indexAccessor = gltfModel.accessors[primitive.indices];

        int posAttributeIdx = primitive.attributes["POSITION"];
        int uvAttributeIdx = primitive.attributes["TEXCOORD_0"];
        
        const tinygltf::Accessor& posAccessor = gltfModel.accessors[posAttributeIdx];
        const tinygltf::Accessor& UVAccessor = gltfModel.accessors[uvAttributeIdx];

        const float* positions = reinterpret_cast<const float*>(
                                    _getDataByAccessor(posAccessor, gltfModel));
        const float* uvPositions = reinterpret_cast<const float*>(
                                    _getDataByAccessor(UVAccessor, gltfModel));
        
        Vertex vertex{};

        for (size_t i = 0; i < posAccessor.count; ++i) {

            vertex.pos = {
                positions[i * 3 + 0],
                positions[i * 3 + 1],
                positions[i * 3 + 2],                
            };
            vertex.color = {1.0f, 1.0f, 1.0f};

			float u_raw = uvPositions[i * 2 + 0];
			float v_raw = uvPositions[i * 2 + 1];
            float u, v;                   

            // Check if there are min and max UV values and normalize it
            // FIXME: it is possible that the model does not use the entire UV 
            // space of the texture. Normalization code will not work in this case
            if (UVAccessor.minValues.size() > 1 && UVAccessor.maxValues.size() > 1) {
                // Normalize if possible
                float min_u = static_cast<float>(UVAccessor.minValues[0]);
                float min_v = static_cast<float>(UVAccessor.minValues[1]);
                
                float max_u = static_cast<float>(UVAccessor.maxValues[0]);
                float max_v = static_cast<float>(UVAccessor.maxValues[1]);


                u = (u_raw - min_u) / (max_u - min_u);
                v = (v_raw + min_v) / (max_v - min_v);
            } else {
                // Use raw values if normalization is impossible
                u = u_raw;
                v = v_raw;
            }


            vertex.texCoord = {
                u,
				v,		
            };
                    
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<u_int32_t>(_model.vertices.size());
                _model.vertices.push_back(vertex);
            }
            _model.indices.push_back(uniqueVertices[vertex]);
        }
    }
    trc::log("Finished loading model");
    return 0;
}

// Load an image using a path relative to the project root
bool Loader::loadTexture(const char* path, Image& img) {
    
    unsigned char* _data = stbi_load(path, &img.w, &img.h, &img.channels, STBI_rgb_alpha);

    if (!_data) {        
        return 1;
    }
    
    img.data = _data;
    
    return 0;
}

// Just a wrapper for a wrapper for <cstdlib> free()
void Loader::unloadBuffer(unsigned char* _pixels){
    if (_pixels) {
        stbi_image_free(_pixels);        
    }
}


// A helper function to populate a tinygltf::Model object
int _loadTinyGLTFModel(tinygltf::Model& gltfModel, const std::string& filename) {
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    // TODO: implement model type deduction, test with .glb models
    bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);
    //bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename); // for binary glTF(.glb)

    if (!warn.empty()) {
        throw std::runtime_error(warn);
        return -1;
    }

    if (!err.empty()) {
        throw std::runtime_error(err);
        return -1;
    }

    if (!ret) {
        throw std::runtime_error("Could not parse a GLTF model!");
        return -1;
    }
    
    trc::log("This GLTF Model is valid");
    return 0;
}

// Load raw data from buffer by accessor
const unsigned char* _getDataByAccessor(tinygltf::Accessor accessor, 
                                                tinygltf::Model& model) {

    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& posBuffer = model.buffers[bufferView.buffer];

	return &posBuffer.data[bufferView.byteOffset + accessor.byteOffset];
}
