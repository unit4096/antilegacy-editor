#pragma once
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

const unsigned char* _getDataByAccessor(tinygltf::Accessor accessor, const tinygltf::Model& model);
int _loadTinyGLTFModel(tinygltf::Model& gltfModel, const std::string& filename);
const int _getNumEdgesInMesh(const Mesh &_mesh);
bool _populateREMesh(Mesh& _inpMesh, geo::REMesh& _outMesh );
void _generateVertexNormals(ale::Mesh &_mesh);

Loader::Loader() { }   

Loader::~Loader() { }

// Load an .obj model using a path relative to the project root
int Loader::loadModelOBJ(char *model_path, Mesh& _mesh) {
    std::string _path = model_path;

    if (!isFileValid(_path)) {
        trc::log("Input file is not valid!", trc::LogLevel::ERROR);
        return -1;
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials,
                                &warn, &err, model_path)) {
        trc::log("Could not load OBJ file!", trc::LogLevel::ERROR);
        return -1;
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
                uniqueVertices[vertex] = 
                            static_cast<unsigned int>(_mesh.vertices.size());
                _mesh.vertices.push_back(vertex);
            }

            _mesh.indices.push_back(uniqueVertices[vertex]);
        }
    }
    return 0;
}



int Loader::_loadMesh(const tinygltf::Model& in_model,
                      const tinygltf::Mesh& in_mesh, 
                      ale::Mesh& out_mesh) {

    auto loadMeshAttribute = [&](tinygltf::Primitive& prim,
                             std::string attrName){
        int idx = prim.attributes[attrName];
        const auto& accessor = in_model.accessors[idx];
        return reinterpret_cast<const float*>(
                    _getDataByAccessor(accessor, in_model));
    };

    for (auto primitive : in_mesh.primitives) {    

        // List model attributes for Debug
        for (auto attr: primitive.attributes) {
            trc::log("This mesh has " + attr.first);
        }
        
        std::unordered_map<ale::Vertex, unsigned int> uniqueVertices{};

        if (!primitive.attributes.contains("POSITION") || 
            !primitive.attributes.contains("TEXCOORD_0")) {
            trc::log("Mesh loader currently requres both positions and UV textures", trc::LogLevel::ERROR);
            return -1;
        }
        

        const float* positions = nullptr;
        const float* uvPositions = nullptr;
        const float* normals = nullptr;

        positions = loadMeshAttribute(primitive, "POSITION");
        auto posAccessor = in_model.accessors[primitive.attributes["POSITION"]];

        uvPositions = loadMeshAttribute(primitive, "TEXCOORD_0");
        auto UVAccessor = in_model.accessors[primitive.attributes["TEXCOORD_0"]];

        if (primitive.attributes.contains("NORMAL")) {
            normals = loadMeshAttribute(primitive, "NORMAL");
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
            if (UVAccessor.minValues.size() > 1 && 
                UVAccessor.maxValues.size() > 1) {
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
                    uniqueVertices[vertex] = static_cast<unsigned int>(
                                                out_mesh.vertices.size());
                    out_mesh.vertices.push_back(vertex);
                }
                out_mesh.indices.push_back(uniqueVertices[vertex]);    
            } else {
                out_mesh.vertices.push_back(vertex);
                out_mesh.indices.push_back(i);
            }
        }

        if (!normals) {
            trc::log("Normals not found in the model! Generating vertex normals", trc::WARNING);
            _generateVertexNormals(out_mesh);
        }
    }

    return 0;
}

// Loads data from a tinygltf Texture to the inner Image format
int Loader::_loadTexture(const tinygltf::Image& in_texture, ale::Image& out_texture) {
    trc::log("Loading texture");

    if (in_texture.width < 1 || in_texture.height < 1) {
        trc::log("Invalid tex parameters, aborting", trc::LogLevel::ERROR);
        return -1;
    }
    
    // image.component; // Defines the dimensions of each texel
    // image.bits // Defines the number of bits in each dimension

    // This copies data to the Image struct directly to keep control
    // over image data
    out_texture.data = in_texture.image;
    out_texture.w = in_texture.width;
    out_texture.h = in_texture.height;            
    return 0;
}

int Loader::_loadNodesGLTF(const tinygltf::Model& in_model,
                           ale::Model& out_model ) {
    
    trc::log("Not implemented!", trc::LogLevel::ERROR);
    return -1;
    
    if (in_model.nodes.size() < 1) {
        trc::log("No nodes found", trc::LogLevel::ERROR);
        return -1;
    }
    
    return 0;
}


// Now loads one mesh and one texture from the .gltf file
// TODO: Add loading full GLTF scenes
int Loader::loadModelGLTF(const std::string model_path,
                          ale::Mesh& out_mesh, ale::Image& _image) { 

    if (!isFileValid(model_path)) {
        trc::log("Input file is not valid!", trc::LogLevel::ERROR);
        return 1;
    }

    tinygltf::Model in_model;
    ale::Model out_model;

    _loadTinyGLTFModel(in_model, model_path);

    // Load textures and meshes first, then bind nodes

    // Load textures to a vector
    if (in_model.textures.size() > 0) {
        trc::log("Found textures");

        for (auto tex: in_model.textures) {    
            if (tex.source > -1) {
                _loadTexture(in_model.images[tex.source], _image);
            } else {
                trc::log("No texture to load", trc::LogLevel::WARNING);
            }
        }
    } else {
        trc::log("Textures not found");
    }

    // Load meshes to a vector
    for (auto mesh: in_model.meshes) {
        ale::Mesh _out_mesh;
        int result = _loadMesh(in_model, mesh, _out_mesh);
        if (result != 0) return -1;
        out_model.meshes.push_back(_out_mesh);
    }

    out_mesh = out_model.meshes[0];

    geo::REMesh reMesh;
    _populateREMesh(out_mesh, reMesh);
    
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

	// Hash tables for the mesh. Needed for debug, will optimize later
    std::unordered_map<geo::Vert, std::shared_ptr<geo::Vert>> uniqueVerts;
    std::unordered_map<geo::Edge, std::shared_ptr<geo::Edge>> uniqueEdges;
	std::unordered_map<geo::Loop, std::shared_ptr<geo::Loop>> uniqueLoops;
    std::unordered_map<geo::Face, std::shared_ptr<geo::Face>> uniqueFaces;

    // Helper binding funcitons to make the code DRY

    auto bindVert = [&](std::shared_ptr<geo::Vert> v, unsigned int i){
        Vertex _v = _inpMesh.vertices[_inpMesh.indices[i]];
        v->pos = _v.pos;
        v->color = _v.color;
        v->texCoord = _v.texCoord;   
    };    

    auto bindEdge = [&](std::shared_ptr<geo::Edge> e, 
                        std::shared_ptr<geo::Vert>& first, 
                        std::shared_ptr<geo::Vert>& second,
                        std::shared_ptr<geo::Loop>& l
                        ){
        e->v1 = first;
        e->v2 = second;

        first->edge = e;		
        bool isNew = true;

		// Checks if there is an edge with this data in the hash table
        // Either hashes the new edge it or assigns an existing pointer
        if (uniqueEdges.contains(*e.get())) {    
			isNew = false;
            e = uniqueEdges[*e.get()];
        } else {
            
        }
	
		auto _l = e->loop;

        if (_l != nullptr) {

            // Iterate over the loop until it hits the beginning
            while (_l->radial_next != e->loop) {
                // Close the loop if it isn't already
                if (_l->radial_next == nullptr ) {
                    // Add l as the end of the loop
                    /* trc::raw
                    << "new radial loop created for edge"  << e.get()
                    << "\n"; */
                    _l = _l->radial_next;
                    _l->radial_prev = l;
                    _l->radial_next = l;
                    break;
                }
            }    

        } else {
            e->loop = l;
        }
        
		return isNew;
    };

    auto bindLoop = [&](std::shared_ptr<geo::Loop>& l, 
                        std::shared_ptr<geo::Vert>& v,                
                        std::shared_ptr<geo::Edge>& e ){
        l->v = v;
        l->e = e; 
        l->radial_next = l;
        l->radial_prev = l;
    };

	auto bindFace = [&](std::shared_ptr<geo::Face>& f,
						std::shared_ptr<geo::Loop>& l1,
						std::shared_ptr<geo::Loop>& l2,
						std::shared_ptr<geo::Loop>& l3) {
		f->loop = l1;

        l1->f = f;
        l2->f = f;
        l3->f = f;
	};

    // Iterate over each face
    for (unsigned int i = 0; i < _inpMesh.indices.size(); i+=3) {
        // Create vertices
        std::shared_ptr v1 = std::make_shared<geo::Vert>();
        std::shared_ptr v2 = std::make_shared<geo::Vert>();
        std::shared_ptr v3 = std::make_shared<geo::Vert>();
		
        // Create edges and loops for each pair of vertices
        // order: 1,2 2,3 3,1

        // 1,2
        std::shared_ptr e1 = std::make_shared<geo::Edge>();
        std::shared_ptr l1 = std::make_shared<geo::Loop>();

        // 2,3
        std::shared_ptr e2 = std::make_shared<geo::Edge>();
        std::shared_ptr l2 = std::make_shared<geo::Loop>();

        // 3,1
        std::shared_ptr e3 = std::make_shared<geo::Edge>();
        std::shared_ptr l3 = std::make_shared<geo::Loop>();

		// Create face 
        std::shared_ptr f = std::make_shared<geo::Face>();

		// Create vectors for the binding loop
		std::vector<std::shared_ptr<geo::Vert>> verts = {v1,v2,v3};
		std::vector<std::shared_ptr<geo::Edge>> edges = {e1,e2,e3};
		std::vector<std::shared_ptr<geo::Loop>> loops = {l1,l2,l3};		
		std::vector<bool> isNewArr = {false,false,false};

		for (size_t j = 0; j < 3; j++) {	
			bindVert(verts[j], i + j);

			// Previous and next indices for each j. Note that 2 loops to 0
			int prev = (3 + (j - 1)) % 3;
			int next = (j + 1) % 3;
            
			isNewArr[j] = bindEdge(edges[j], verts[j], verts[next], loops[j]);
        	bindLoop(loops[j], verts[j], edges[j]);
			
			// If this edge is new, add adjacent edges to it. 
            // If not, add new edges
			if (isNewArr[j]) {            
				edges[j]->v1_prev = edges[prev];
				edges[j]->v2_next = edges[next];
			} else {
                std::shared_ptr<geo::Edge> tmpEdge = 
                                                uniqueEdges[*edges[j].get()];
                
                tmpEdge->v1_next = edges[prev];
                tmpEdge->v2_prev = edges[next];
                edges[j] = tmpEdge;
			}
			
			// Bind loops to each other to form a face
			loops[j]->next = loops[next];
			loops[j]->prev = loops[prev];
		}
                
        // Bind the face afterwards
		bindFace(f, l1, l2, l3);

        f->size = 3;
        uniqueFaces[*f.get()] = f;


        for (size_t j = 0; j < 3; j++) {
            uniqueLoops[*loops[j].get()] = loops[j];
            uniqueVerts[*verts[j].get()] = verts[j];
            uniqueEdges[*edges[j].get()] = edges[j];
        }
    }

    trc::raw << "\n\n\n" 
        << uniqueVerts.size() <<  " vertices" << "\n"
        << uniqueEdges.size() <<  " edges" << "\n"
        << uniqueLoops.size() <<  " loops" << "\n"
        << uniqueFaces.size() <<  " faces" << "\n"        
        << "REMesh E P characteristic: " 
        << uniqueVerts.size() - uniqueEdges.size() + uniqueFaces.size() 
        << "\n"
        << "\n";
    
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
    unsigned char* _data = stbi_load(path, &img.w,
                                     &img.h, &img.channels, STBI_rgb_alpha);

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

int Loader::getFlaggedArgument(const std::string flag, std::string& result) {

    if (this->commandLineTokens.size() < 1) {
        trc::log("No tokens loaded!", trc::LogLevel::WARNING);
        return -1;
    }

    if (cmdOptionExists("-f")) {
        std::string file = getCmdOption("-f");
        trc::log("Loaded argument: " + file);         
        if (isFileValid(file)) {
            trc::log("File can be loaded: " + file);
            result = file;
            return 0;
        } else {
            trc::log("File path is not valid, falling back to defaults!", trc::WARNING);
            return -1;
        }
    } else {
        trc::log("No files specified, attempting to load the default file", trc::DEBUG);
        return -1;
    }
    

    return 0;
}

// I borrowed this code from here: 
// https://stackoverflow.com/questions/865668/parsing-command-line-arguments-in-c
// Command line input is used only for debugging now. If this changes, I will
// use or implement a proper library

const std::string& Loader::getCmdOption(const std::string &option) const{

    std::vector<std::string>::const_iterator itr;
    itr =  std::find(this->commandLineTokens.begin(),
                     this->commandLineTokens.end(), option);
    if (itr != this->commandLineTokens.end() &&
      ++itr != this->commandLineTokens.end()){
        return *itr;
    }
    static const std::string empty_string("");
    return empty_string;
}

bool  Loader::cmdOptionExists(const std::string &option) const {
    return std::find(this->commandLineTokens.begin(),
                     this->commandLineTokens.end(), option)
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
    auto none = std::filesystem::perms::none;
    if ((perms & std::filesystem::perms::owner_read) != none  &&
        (perms & std::filesystem::perms::group_read) != none &&
        (perms & std::filesystem::perms::others_read) != none ) {
        return true;
    }
    return false;
}

// Load raw data from buffer by accessor
const unsigned char* _getDataByAccessor(tinygltf::Accessor accessor, 
                                                const tinygltf::Model& model) {

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& posBuffer = model.buffers[bufferView.buffer];

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