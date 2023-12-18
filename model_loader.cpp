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
// FIXME: this function does not return any status
void Loader::loadModelOBJ(char *model_path, Mesh& _mesh) {
    std::string _path = model_path;

    if (!isFileValid(_path)) {
        trc::log("Input file is not valid!", ale::Tracer::ERROR);
        return;
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path)) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<ale::Vertex, unsigned int> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            ale::Vertex vertex{};

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
                uniqueVertices[vertex] = static_cast<u_int32_t>(_mesh.vertices.size());
                _mesh.vertices.push_back(vertex);
            }

            _mesh.indices.push_back(uniqueVertices[vertex]);
        }
    }
}

// Populates half-edges of the model, allowing for easier mesh manipulation
// FIXME: this function does not work yet
int Loader::populateHalfEdges(ale::Model &_model) {

    // TODO: add more checks for mesh orientation and manifolddness 
    if (_model.meshes.size() < 1) {
        trc::log("Cannot populate half-edges for a model: mesh not found!", trc::ERROR);
        return 1;
    }
    

    // Mesh current_mesh = _model.meshes[0];

    // for (size_t i = 0; i < current_mesh.indices.size(); i += 3) {
    //     Face face;
    //     std::vector<std::shared_ptr<HalfEdge>> faceEdges;

    //     std::vector<std::shared_ptr<HalfEdge>> vertexOutgoingEdges(current_mesh.vertices.size(), nullptr);

    //     for (size_t j = 0; j < 3; ++j) {
    //         HalfEdge edge;
    //         edge.vertex = std::make_shared<ale::Vertex>(&current_mesh.vertices[current_mesh.indices[i + j]]);

    //         edge.vertex->halfEdge = std::make_shared<HalfEdge>(&edge);

    //         faceEdges.push_back(std::make_shared<HalfEdge>(&edge));
    //         current_mesh.halfEdges.push_back(edge);

    //         if (!vertexOutgoingEdges[current_mesh.indices[i + j]]) {
    //             vertexOutgoingEdges[current_mesh.indices[i + j]] = std::make_shared<HalfEdge> (&edge);
    //         }
    //     }

    //     for (size_t j = 0; j < 3; ++j) {
    //         faceEdges[j]->next = std::make_shared<HalfEdge>(faceEdges[(j + 1) % 3]);
    //         faceEdges[j]->twin = std::make_shared<HalfEdge> (faceEdges[j]->vertex->halfEdge);
    //         faceEdges[j]->face = std::make_shared<Face>(&face);
    //     }

    //     face.halfEdge = std::make_shared<HalfEdge>(faceEdges[0]);
    //     current_mesh.faces.push_back(face);
    // }

    // return 0;


    trc::log("This function is not implemented!", trc::ERROR);    
    return 1;
    
}

// Now loads one mesh and one texture from the .gltf file
// FIXME: better use bool as status and give std errors if something goes wrong
// TODO: Add loading full GLTF scenes
int Loader::loadModelGLTF(const std::string model_path, ale::Mesh& _mesh, ale::Image& _image) { 

    if (!isFileValid(model_path)) {
        trc::log("Input file is not valid!", ale::Tracer::ERROR);
        return 1;
    }

    tinygltf::Model gltfModel;

    _loadTinyGLTFModel(gltfModel, model_path);
    
    if (gltfModel.textures.size() > 0) {
        trc::log("Found textures");
        tinygltf::Texture &tex = gltfModel.textures[0];
        
        if (tex.source > -1) {
            trc::log("Loading texture");
            tinygltf::Image &image = gltfModel.images[tex.source];

            // image.component; // Defines the dimensions of each texel
            // image.bits // Defines the number of bits in each dimension

            // This copies data to the Image struct directly to keep control
            // over image data
            _image.data = image.image;
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
        
        std::unordered_map<ale::Vertex, unsigned int> uniqueVertices{};

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
                uniqueVertices[vertex] = static_cast<u_int32_t>(_mesh.vertices.size());
                _mesh.vertices.push_back(vertex);
            }
            _mesh.indices.push_back(uniqueVertices[vertex]);
        }
    }
    trc::log("Finished loading model");
    return 0;
}

// Load an image using a path relative to the project root
bool Loader::loadTexture(const char* path, Image& img) {
    // Clear image data before writing
    img.h = 0;
    img.w = 0;
    img.channels = 0;
    img.data.resize(0);    

    // Load the image using stbi
    unsigned char* _data = stbi_load(path, &img.w, &img.h, &img.channels, STBI_rgb_alpha);

    if (!_data) {
        trc::log("Cannot get texture!", ale::Tracer::ERROR);
        return 1;
    }
    // STBI_rgb_alpha forces the images to be loaded with an alpha channel. 
    // Hence, the in-line int 4
    size_t size = img.w * img.h * 4;
    
    // A byte after the last byte in the array
    const unsigned char* _end = _data + size;

    // Write image data
    for (unsigned char* i = _data; i != _end; i++) {
        img.data.push_back(*i);
    }
    free(_data);
    return 0;
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

void Loader::recordCommandLineArguments(int &argc, char **argv) {
    for (int i=1; i < argc; ++i) {
        this->commandLineTokens.push_back(std::string(argv[i]));
    }
}

// I borrowed this code from here: 
// https://stackoverflow.com/questions/865668/parsing-command-line-arguments-in-c
// Command line input is used only for debugging now. If this changes, I will
// use or implement a proper library

const std::string& Loader::getCmdOption(const std::string &option) const{

    std::vector<std::string>::const_iterator itr;
    itr =  std::find(this->commandLineTokens.begin(), this->commandLineTokens.end(), option);
    if (itr != this->commandLineTokens.end() && ++itr != this->commandLineTokens.end()){
            return *itr;
    }
    static const std::string empty_string("");
    return empty_string;
}

bool  Loader::cmdOptionExists(const std::string &option) const{

    return std::find(this->commandLineTokens.begin(), this->commandLineTokens.end(), option)
            != this->commandLineTokens.end();
}



bool Loader::isFileValid(std::string file_path) {
    std::filesystem::path filePath(file_path);

    if (std::filesystem::exists(filePath)) {
        if (_canReadFile(filePath)) {
            return true;    
        }
    }
    return false;
    
}

bool Loader::_canReadFile(std::filesystem::path p) {
    std::error_code ec;
    auto perms = std::filesystem::status(p, ec).permissions();
    if ((perms & std::filesystem::perms::owner_read) != std::filesystem::perms::none &&
        (perms & std::filesystem::perms::group_read) != std::filesystem::perms::none &&
        (perms & std::filesystem::perms::others_read) != std::filesystem::perms::none
        ) {
        return true;
    }
    return false;
}

// Load raw data from buffer by accessor
const unsigned char* _getDataByAccessor(tinygltf::Accessor accessor, 
                                                tinygltf::Model& model) {

    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& posBuffer = model.buffers[bufferView.buffer];

	return &posBuffer.data[bufferView.byteOffset + accessor.byteOffset];
}