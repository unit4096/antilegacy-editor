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

namespace geo = ale::geo;

namespace trc = ale::Tracer;

const bool COMPRESS_VERTEX_DUPLICATES = false;

/* 
Declarations for the helper functions. I use the notation for private members to 
distinguish them from functions in the header.

This "C style incapsulation" is a handy way to not include tiny_*.h directives
inside the loader's header (since you cannot do it with these libraries)
*/

const unsigned char* _getDataByAccessor(tinygltf::Accessor accessor, tinygltf::Model& model);
int _loadTinyGLTFModel(tinygltf::Model& gltfModel, const std::string& filename);
const int _getNumEdgesInMesh(const Mesh &_mesh);
bool _populateREMesh(Mesh& _inpMesh, geo::REMesh& _outMesh );
void _generateVertexNormals(ale::Mesh &_mesh);

Loader::Loader() { }   

Loader::~Loader() { }

// Load an .obj model using a path relative to the project root
// FIXME: this function does not return any status
void Loader::loadModelOBJ(char *model_path, Mesh& _mesh) {
    std::string _path = model_path;

    if (!isFileValid(_path)) {
        trc::log("Input file is not valid!", trc::LogLevel::ERROR);
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
                uniqueVertices[vertex] = static_cast<unsigned int>(_mesh.vertices.size());
                _mesh.vertices.push_back(vertex);
            }

            _mesh.indices.push_back(uniqueVertices[vertex]);
        }
    }
}



// Now loads one mesh and one texture from the .gltf file
// FIXME: better use bool as status and give std errors if something goes wrong
// TODO: Add loading full GLTF scenes
int Loader::loadModelGLTF(const std::string model_path, ale::Mesh& _mesh, ale::Image& _image) { 

    if (!isFileValid(model_path)) {
        trc::log("Input file is not valid!", trc::LogLevel::ERROR);
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
        
        const auto& posAccessor = gltfModel.accessors[posAttributeIdx];
        const auto& UVAccessor = gltfModel.accessors[uvAttributeIdx];

        const float* positions = reinterpret_cast<const float*>(
                                    _getDataByAccessor(posAccessor, gltfModel));
        const float* uvPositions = reinterpret_cast<const float*>(
                                    _getDataByAccessor(UVAccessor, gltfModel));
        
        const float* normals = nullptr;
        if (primitive.attributes.count("NORMAL") > 0) {
            int normIdx = primitive.attributes["NORMAL"];
            const auto& normAccessor = gltfModel.accessors[normIdx];
            normals = reinterpret_cast<const float*>(
                        _getDataByAccessor(normAccessor, gltfModel));
        }

        
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

            if (normals) {
                vertex.normal = {
                    normals[i * 3 + 0],
                    normals[i * 3 + 1],
                    normals[i * 3 + 2]
                };
            }
                                
            // Checks whether the loader should remove vertex duplicates. Needed
            // for debug. Hopefully the compiler will optimize away this check
            // since the flag's value is const
            if (COMPRESS_VERTEX_DUPLICATES) {
            if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<unsigned int>(_mesh.vertices.size());
                    _mesh.vertices.push_back(vertex);
                }
                _mesh.indices.push_back(uniqueVertices[vertex]);    
            } else {
                _mesh.vertices.push_back(vertex);
                _mesh.indices.push_back(i);
            }
        }

        if (!normals) {
            trc::log("Normals not found in the model! Generating vertex normals", trc::WARNING);
            _generateVertexNormals(_mesh);
        }
        
    }
    
    // int edgesCount = _getNumEdgesInMesh(_mesh);
    // trc::raw << "edges: " << edgesCount << "\n"
    //          << "idx: " << _mesh.indices.size() << "\n"
    //          << "vertices: " << _mesh.vertices.size() << "\n"
    //          << "faces: " << _mesh.indices.size() / 3 << "\n\n";

    // trc::raw << "Euler-poincare characteristic: " 
    //          <<  _mesh.indices.size()  + (_mesh.indices.size() / 3) - 
    //              edgesCount  << "\n";
    
    ale::Model newModel;
    newModel.meshes.push_back(_mesh);
    newModel.textures.push_back(_image);

    geo::REMesh reMesh;

    _populateREMesh(_mesh, reMesh);
    
    trc::log("Finished loading model");
    return 0;
}

/* 
    WORK IN PROGRESS
    Populates a geo::Mesh object using default view mesh. Assumes that the mesh 
    is triangular and that each 3 indices form a face
*/
bool _populateREMesh(Mesh& _inpMesh, geo::REMesh& _outMesh ) {

    trc::log("Not implemented!", trc::ERROR);
    return 1;

    std::unordered_map<ale::geo::Vertex, std::shared_ptr<geo::Vertex>> uniqueVertices;
    std::unordered_map<geo::Edge, std::shared_ptr<geo::Edge>> uniqueEdges;
        

    // Helper funcitons to make the code DRY

    auto bindVert = [&](std::shared_ptr<geo::Vertex> v, unsigned int i){        
        Vertex _v = _inpMesh.vertices[_inpMesh.indices[i]];
        v->pos = _v.pos;
        v->color = _v.color;
        v->texCoord = _v.texCoord;    
    };    

    auto bindEdge = [&](std::shared_ptr<geo::Edge> e, 
                        std::shared_ptr<geo::Vertex>& first, 
                        std::shared_ptr<geo::Vertex>& second,
                        std::shared_ptr<geo::Loop>& l ){

        e->v1 = first;
        e->v2 = second;

        // Checks if there is an edge with this data in the hash table
        // Either hashes the new edge it or assigns an existing pointer
        if (uniqueEdges.count(*e.get()) != 0) {    
            e = uniqueEdges[*e.get()];
        } else {
            uniqueEdges[*e.get()] = e;
        }
        
        //  /// TODO: here should go the code to load face loops to a radial 
        //  /// disk data structure (note that the order of loops matters)


        // auto _l = e->loop;

        
        // while (_l != nullptr) {
        //     _l = _l->radial_next;
        // }

        // _l->radial_next = l;
    };

    auto bindLoop = [&](std::shared_ptr<geo::Loop>& l, 
                        std::shared_ptr<geo::Vertex>& v,                
                        std::shared_ptr<geo::Edge>& e ){
        l->v = v;
        l->e = e; 
        l->radial_next = l;
        l->radial_prev = l;
        
    };

    // Iterate over each face
    for (unsigned int i = 0; i < _inpMesh.indices.size(); i+=3) {

        int i1 = i + 0;
        int i2 = i + 1;
        int i3 = i + 2;

        // FIXME: Compare vertices only by their position

        // Create vertices
        std::shared_ptr v1 = std::make_shared<geo::Vertex>();
        std::shared_ptr v2 = std::make_shared<geo::Vertex>();
        std::shared_ptr v3 = std::make_shared<geo::Vertex>();
        bindVert(v1, i1);
        bindVert(v2, i2);
        bindVert(v3, i3);

        // Create edges and loops for each pair of vertices
        // order: 1,2 2,3 3,1

        // 1,2
        std::shared_ptr e1 = std::make_shared<geo::Edge>();
        std::shared_ptr l1 = std::make_shared<geo::Loop>();

        bindEdge(e1, v1, v2, l1);
        bindLoop(l1, v1, e1);        

        // 2,3
        std::shared_ptr e2 = std::make_shared<geo::Edge>();
        std::shared_ptr l2 = std::make_shared<geo::Loop>();
        
        bindEdge(e2, v2,v3,l2);
        bindLoop(l2, v2, e2);

        // 3,1
        std::shared_ptr e3 = std::make_shared<geo::Edge>();
        std::shared_ptr l3 = std::make_shared<geo::Loop>();
        
        bindEdge(e3, v3,v1,l3);
        bindLoop(l3, v3, e3);

        // Create face
        std::shared_ptr f = std::make_shared<geo::Face>();
        f->loop = l1;

        l1->prev = l3;
        l2->prev = l1;
        l3->prev = l2;

        l1->next = l2;
        l2->next = l3;
        l3->next = l1;


        l1->f = f;
        l2->f = f;
        l3->f = f;


        /*
            TODO: Add new edges to disk loops and radial edge loops as they 
            appear. Implement checking for adjacent edges. Without this, 
            adjacency relations would not be full
            Clockwise order
        */
        
        // Populate disks for each vertex
        
        // auto appendDisk = [](std::shared_ptr<geo::Edge> e1, 
        //                      std::shared_ptr<geo::Edge> e2){
        //     std::shared_ptr d = std::make_shared<geo::DiskLink>();
        //     e1->v1_disk = d;
        //     e2->v2_disk = d;
        //     d->prev = e2;
        //     d->next = e2;
        // };

        // // v1
        // std::shared_ptr d1 = std::make_shared<geo::DiskLink>();

        // d1->prev = e3;
        // d1->next = e1;
        // e1->v1_disk = d1;

        // // v2
        // std::shared_ptr d2 = std::make_shared<geo::DiskLink>();
        // d2->prev = e3;
        // d2->next = e1;
        // e2->v1_disk = d2;

        // // v3
        // std::shared_ptr d3 = std::make_shared<geo::DiskLink>();
        // d2->prev = e3;
        // d2->next = e1;
        // e3->v1_disk = d3;

        // FIXME: these vectors contain duplicates for obvious reasons. Implement
        // hash tables

        _outMesh.edges.push_back(e1);
        _outMesh.edges.push_back(e2);
        _outMesh.edges.push_back(e3);

        _outMesh.loops.push_back(l1);
        _outMesh.loops.push_back(l2);
        _outMesh.loops.push_back(l3);

        _outMesh.vertices.push_back(v1);
        _outMesh.vertices.push_back(v2);
        _outMesh.vertices.push_back(v3);

        _outMesh.faces.push_back(f);
    }

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
        trc::log("Cannot get texture!", trc::LogLevel::ERROR);
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

void _generateVertexNormals(ale::Mesh &_mesh) {
    
    if (_mesh.indices.size() % 3 != 0) {
        trc::log("Cannot calculate normals, input mesh is not manifold!", trc::ERROR);
        return;
    }

    for (size_t i = 0; i < _mesh.indices.size(); i+=3) {
        int i1 = _mesh.indices[i + 0];
        int i2 = _mesh.indices[i + 1];
        int i3 = _mesh.indices[i + 2];
        
        glm::vec3 pos1 = _mesh.vertices[i1].pos;
        glm::vec3 pos2 = _mesh.vertices[i2].pos;
        glm::vec3 pos3 = _mesh.vertices[i3].pos;

        glm::vec3 v1 = pos2 - pos1;
        glm::vec3 v2 = pos3 - pos1;        

        glm::vec3 normal = glm::normalize(glm::cross(v1,v2));

        _mesh.vertices[i1].normal = normal;
        _mesh.vertices[i2].normal = normal;
        _mesh.vertices[i3].normal = normal;
    }
}


// Returns a number of ints 
const int _getNumEdgesInMesh(const Mesh &_mesh) {
    std::set<std::pair<int,int>> uniqueEdges;

    auto sortedPair = [](int first, int second ) {
        if (first > second) {
            return std::pair<int,int>(second,first);
        } else {
            return std::pair<int,int>(first,second);
        }
    };

    for (size_t i = 0; i < _mesh.indices.size(); i += 3) {
        std::pair<int,int> a = sortedPair(i + 0, i + 1);
        std::pair<int,int> b = sortedPair(i + 1, i + 2);
        std::pair<int,int> c = sortedPair(i + 0, i + 2);
        uniqueEdges.insert(a);
        uniqueEdges.insert(b);
        uniqueEdges.insert(c);
    }

    return uniqueEdges.size();
} 