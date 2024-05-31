#include "os_loader.h"


// ext

#define TINYOBJLOADER_IMPLEMENTATION
#include <tol/tiny_obj_loader.h>

// TINYOBJLOADER_IMPLEMENTATION, TINYGLTF_IMPLEMENTATION,
// and STB_IMAGE_IMPLEMENTATION should
// be defined inside a .cpp file
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

using namespace ale;


namespace trc = ale::Tracer;

const bool COMPRESS_VERTEX_DUPLICATES = false;

Loader::Loader() { }

Loader::~Loader() { }

std::vector<char> Loader::getFileContent(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!Loader::isFileValid(file_path)) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}


// Load an .obj model using a path relative to the project root
int Loader::loadModelOBJ(char *model_path, ViewMesh& _mesh) {
    std::string _path = model_path;

    if (!Loader::isFileValid(_path)) {
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

// Attempts to find and load indices for a primitive to a mesh
int Loader::_tryLoadMeshIndices(const tinygltf::Model& in_model,
                                const tinygltf::Primitive& primitive,
                                ale::ViewMesh& out_mesh) {

    int indicesIdx = primitive.indices;

    if (indicesIdx <= -1) {
        trc::log("Indices not found!", trc::WARNING);
        return false;
    }

    const auto& accessor = in_model.accessors[indicesIdx];
    auto _type = accessor.componentType;

    if (_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) {

        const unsigned short* indices = reinterpret_cast<const unsigned short*>( _getDataByAccessor(accessor, in_model));
        for (size_t i = 0; i < accessor.count; i++) {
            out_mesh.indices.push_back(*(indices + i));
        }

        trc::log("Index type: short");
    } else if (_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT) {

        const unsigned int* indices = reinterpret_cast<const unsigned int*>( _getDataByAccessor(accessor, in_model));
        for (size_t i = 0; i < accessor.count; i++) {
            out_mesh.indices.push_back(*(indices + i));
        }

        trc::log("Index type: int");
    } else if (_type == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) {

        const unsigned char* indices = reinterpret_cast<const unsigned char*>( _getDataByAccessor(accessor, in_model));
        for (size_t i = 0; i < accessor.count; i++) {
            out_mesh.indices.push_back(*(indices + i));
        }

        trc::log("Index type: char");
    } else {
        trc::log("Unknown index type!", trc::ERROR);
    }

    switch (accessor.componentType) {
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {


    }
        break;
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        break;
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        trc::log("unsigned bytes not supported as indices!",trc::ERROR);
    }

    const unsigned short* indices = reinterpret_cast<const unsigned short*>( _getDataByAccessor(accessor, in_model));
    for (size_t i = 0; i < accessor.count; i++) {
        out_mesh.indices.push_back(*(indices + i));
    }
    return true;
}


// Loads mesh data to ale::ViewMesh
// TODO: refactor to make the code more readable
int Loader::_loadMeshGLTF(const tinygltf::Model& in_model,
                          const tinygltf::Mesh& in_mesh,
                          ale::ViewMesh& out_mesh) {


    auto loadMeshAttribute = [&](tinygltf::Primitive& prim, std::string attrName){
        int idx = prim.attributes[attrName];
        const auto& accessor = in_model.accessors[idx];
        return reinterpret_cast<const float*>(_getDataByAccessor(accessor, in_model));
    };

    size_t currentPrimitiveIndexOffsetIdx = 0;
    size_t lastIndexedSize = 0;

    for (auto primitive : in_mesh.primitives) {


        // Load indices
        bool bHasIndices = _tryLoadMeshIndices(in_model, primitive, out_mesh);

        std::unordered_map<ale::Vertex, unsigned int> uniqueVertices{};

        if (!primitive.attributes.contains("POSITION") ||
            !primitive.attributes.contains("TEXCOORD_0")) {
            trc::log("Mesh loader currently requres both positions and UV textures",
                     trc::LogLevel::ERROR);
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

        if (posAccessor.minValues.size() > 0){
            assert(posAccessor.minValues.size() == posAccessor.maxValues.size());
            for (size_t i = 0; i < posAccessor.minValues.size(); i++) {
                float min = posAccessor.minValues[i];
                float max = posAccessor.maxValues[i];
                out_mesh.minPos.push_back(min);
                out_mesh.maxPos.push_back(max);
            }
        }


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
                    uniqueVertices[vertex] = static_cast<unsigned int>(out_mesh.vertices.size());
                    out_mesh.vertices.push_back(vertex);
                }

                out_mesh.indices.push_back(uniqueVertices[vertex]);

            } else {
                out_mesh.vertices.push_back(vertex);

                if (!bHasIndices) {
                    out_mesh.indices.push_back(i);
                }
            }


        }

        if (!normals) {
            trc::log("Normals not found in the model! Generating vertex normals",
                     trc::WARNING);
            _generateVertexNormals(out_mesh);
        }

        ale::Primitive ale_primitive{
            .materialID = primitive.material,
            .offsetIdx = currentPrimitiveIndexOffsetIdx,
            .size = out_mesh.indices.size() - lastIndexedSize,
        };

        lastIndexedSize = out_mesh.indices.size();
        out_mesh.primitives.push_back(ale_primitive);
        currentPrimitiveIndexOffsetIdx += out_mesh.indices.size();
    }

    return 0;
}

int Loader::_loadTexturesGLTF(const tinygltf::Model& in_model, ale::Model& out_model) {

    for (auto tex: in_model.textures) {
        if (tex.source > -1) {
            ale::Image _image;
            _loadTextureGLTF(in_model.images[tex.source], _image);
            out_model.textures.push_back(_image);
        } else {
            trc::log("No texture to load", trc::LogLevel::WARNING);
            return -1;
        }
    }
    return 0;
}

// Loads data from a tinygltf Texture to the inner Image format
int Loader::_loadTextureGLTF(const tinygltf::Image& in_texture, ale::Image& out_texture) {

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

    if (in_model.nodes.size() < 1) {
        trc::log("No nodes found", trc::LogLevel::ERROR);
        return -1;
    }

    // TODO: Possibly load multiple scenes (or add scene selection)
    // int sceneIdx = in_model.defaultScene > -1? in_model.defaultScene : 0;
    const auto &scene = in_model.scenes[in_model.defaultScene];

    // Reserve some space for random writes in _bindNodeGLTF()
    out_model.nodes.resize(in_model.nodes.size());

    for (size_t i = 0; i <scene.nodes.size(); i++) {
        auto rootNodeIdx = scene.nodes[i];
        _bindNodeGLTF(in_model, in_model.nodes[rootNodeIdx], -1, rootNodeIdx,  out_model);
        out_model.rootNodes.push_back(rootNodeIdx);
    }

    return 0;
}

// Recursively builds node hierarchy
void Loader::_bindNodeGLTF(const tinygltf::Model& in_model,
                       const tinygltf::Node& n,
                       int parent, int current, ale::Model& out_model ) {
    ale::Node ale_node;

    glm::mat4 newTransform = glm::mat4(1);

    if (n.translation.size() == 3) {
		float x = n.translation[0];
        float y = n.translation[1];
		float z = n.translation[2];

        newTransform[3][0] = x;
        newTransform[3][1] = y;
        newTransform[3][2] = z;
    }

    if (n.rotation.size() == 4) {
        glm::quat rotation = glm::make_quat(n.rotation.data());
        newTransform *= glm::mat4_cast(rotation);
    }

    if (n.scale.size() == 3) {

        glm::vec3 scale = glm::make_vec3(n.scale.data());
        newTransform = glm::scale(newTransform, scale);
    }

    if (n.matrix.size() == 16) {
        newTransform = glm::make_mat4(n.matrix.data());
    }

	ale_node.transform = newTransform;
    ale_node.name = n.name;
    ale_node.children = n.children;
    ale_node.id = current;
    ale_node.parentIdx = parent;
    ale_node.meshIdx = n.mesh;

    // We know the size of .nodes and load every node
    // So we can use this trick with random writes
    out_model.nodes[current] = ale_node;

    for (size_t i = 0; i < n.children.size(); i++) {
        assert((n.children[i] >= 0) && (n.children[i] < in_model.nodes.size()));
        _bindNodeGLTF(in_model, in_model.nodes[n.children[i]], current, n.children[i], out_model);
    }
}


int _checkNodeCollisions(const ale::Model& in_model) {

    std::unordered_map<std::string, int> nameMap;
    std::unordered_map<int, int> idMap;

    // Check by names
    for (auto node: in_model.nodes) {

        if (nameMap.contains(node.name)) {
            trc::log("Node name collision detected!",trc::LogLevel::DEBUG);
            trc::raw << "Node name: " << node.name << " | id: " << node.id << "\n";
            return -1;
        } else {
            nameMap[node.name] = 1;
        }
    }

    for (auto node: in_model.nodes) {
        if (idMap.contains(node.id)) {
            trc::log("Node id collision detected!",trc::LogLevel::DEBUG);
            trc::raw << "Node name: " << node.name << " | id: " << node.id << "\n";
            return -1;
        } else {
            idMap[node.id] = 1;
        }
    }

    trc::log("No collisions detected",trc::LogLevel::DEBUG);
    trc::raw << "nodes:"<< "\n";

    for (auto node: in_model.nodes) {
        trc::log("List of model nodes: ",trc::LogLevel::DEBUG);
        trc::raw << "Node name: " << node.name << " | id: " << node.id << "\n";
    }

    return 0;
}

// Now loads one mesh and one texture from the .gltf file
// TODO: Add loading full GLTF scenes
int Loader::loadModelGLTF(const std::string model_path,
                          ale::Model& out_model) {

    const std::string _default_checker_texture_path = "./textures/tex_uv_checker.png";

    if (!Loader::isFileValid(model_path)) {
        trc::log("Input file is not valid!", trc::LogLevel::ERROR);
        return -1;
    }

    tinygltf::Model in_model;
    _loadTinyGLTFModel(in_model, model_path);

    // Load textures and meshes first, then bind nodes

    // Load textures to a vector
    if (in_model.textures.size() > 0) {
        trc::log("Found textures");
        _loadTexturesGLTF(in_model, out_model);
    } else {
        trc::log("Textures not found! Using fallback texture", trc::WARNING);

        ale::Image fallbackTexture;
        Loader::loadTexture(_default_checker_texture_path.data(), fallbackTexture);
        out_model.textures.push_back(fallbackTexture);

    }

    // Multithreading ahead

    // TODO: It may be good to implement thread pooling rather
    // than just spawning new threads.
    // For this task though, context switching overhead is not a big problem

    std::vector<std::future<void>> futures;
    // Temporary array of meshes that exists to maintain their
    // indexing order
    std::vector<ale::ViewMesh> out_meshes;
    out_meshes.resize(in_model.meshes.size());

    auto load = [&](int i) {
        out_meshes[i].id = i;
        _loadMeshGLTF(in_model, in_model.meshes[i], out_meshes[i]);
    };

    // Run each _loadMeshGLTF on a separate thread
    // to make loading multiple meshes FAST
    // Use explicit indexing to load them in order
    for (int i = 0; i < in_model.meshes.size(); i++) {
        futures.push_back(std::async(std::launch::async, load, i));
    }

    // Wait for threads
    for (auto& fut : futures) {
        fut.wait();
    }

    // Load meshes
    for (auto& mesh: out_meshes) {
        out_model.viewMeshes.push_back(mesh);
    }

    _loadNodesGLTF(in_model, out_model);

    out_model.reMeshes.reserve(out_model.viewMeshes.size());
    trc::log("Populating REMeshes");
    for(size_t i = 0; i < out_model.viewMeshes.size(); i++) {
        geo::REMesh r;
        out_model.reMeshes.push_back(r);
        populateREMesh(out_model.viewMeshes[i], out_model.reMeshes[i]);
    }

    trc::log("Finished loading model");
    return 0;
}

/*
    WORK IN PROGRESS
    Populates a geo::Mesh object using default view mesh.
    Assumes that the mesh is triangular and that each 3 indices form a face.

    This function is more of a test chamber for the REMesh data structure.
    I do not recommend to use it in a product code.
*/
bool Loader::populateREMesh(ViewMesh& _inpMesh, geo::REMesh& _outMesh ) {
    auto numVerts = _inpMesh.vertices.size();

    // FIXME: Use growing storage to avoid use of excess memory
    int MULT = 2;
    _outMesh.facesPool.init(numVerts*3*MULT);
    _outMesh.vertsPool.init(numVerts*3*MULT);
    _outMesh.edgesPool.init(numVerts*3*MULT);
    _outMesh.loopsPool.init(numVerts*3*2*MULT);
    _outMesh.disksPool.init(numVerts*3*MULT);

    auto* fp = &_outMesh.facesPool;
    auto* vp = &_outMesh.vertsPool;
    auto* ep = &_outMesh.edgesPool;
    auto* lp = &_outMesh.loopsPool;
    auto* dp = &_outMesh.disksPool;


    const bool TEST_2_MANIFOLD = false;

    // Only accepts manifold meshes consisting of triangles
    assert(_inpMesh.vertices.size() >= 3);
    assert(_inpMesh.indices.size() >= 3);
    assert(_inpMesh.indices.size() % 3 == 0);

	// Hash tables for the mesh. Needed for debug, will optimize later
    std::unordered_map<geo::Vert, geo::Vert*> uniqueVerts;
    std::unordered_map<geo::Edge, geo::Edge*> uniqueEdges;
	std::unordered_map<geo::Loop, geo::Loop*> uniqueLoops;
    std::unordered_map<geo::Face, geo::Face*> uniqueFaces;
    std::unordered_map<geo::Disk, geo::Disk*> uniqueDisks;

    // Helper binding funcitons to make the code DRY

    // Get ViewMesh vertex to populate basic fields
    auto bindVert = [&](geo::Vert& v, unsigned int i){
        Vertex _v = _inpMesh.vertices[_inpMesh.indices[i]];
        v.pos = _v.pos;
        v.color = _v.color;
        v.texCoord = _v.texCoord;
    };

    auto bindLoop = [&](geo::Loop* l,
                        geo::Vert* v, geo::Edge* e ){
        l->v = v;
        l->e = e;
        // No loop == loops to itself
        l->radial_next = l;
        l->radial_prev = l;
    };

	auto bindFace = [&](geo::Face* f, geo::Loop* l1,
						geo::Loop* l2, geo::Loop* l3) {
		f->loop = l1;
        l1->f = f;
        l2->f = f;
        l3->f = f;
	};

    auto try_get_edge = [&](geo::Edge* e) {
        if (uniqueEdges.contains(*e)) {
            auto result = uniqueEdges[*e];
            return result;
        } else {
            return e;
        }
    };

    // Iterate over each face
    for (unsigned int i = 0; i < _inpMesh.indices.size(); i+=3) {

		std::vector<geo::Vert*> verts = {nullptr,nullptr,nullptr};
		std::vector<geo::Edge*> edges = {nullptr,nullptr,nullptr};
		std::vector<geo::Loop*> loops = {nullptr,nullptr,nullptr};
        std::vector<bool> contains = {false,false,false};


        // Create and bind verts

        for (size_t j = 0; j < 3; j++) {
            geo::Vert v;

            bindVert(v, i + j);
            if (uniqueVerts.contains(v)) {
                verts[j] = uniqueVerts[v];
            } else {
                size_t id;
                verts[j] = vp->request(id);
                *verts[j] = v;
                verts[j]->id = id;
                uniqueVerts[v] = verts[j];
            }
        }

        // Bind edges

        for (size_t j = 0; j < 3; j++) {
			int next = (j + 1) % 3;
            geo::Edge e;
            e.v1 = verts[j];
            e.v2 = verts[next];

            if (uniqueEdges.contains(e)) {
                edges[j] = uniqueEdges[e];
                contains[j] = true;
            } else {
                size_t id;
                edges[j] = ep->request(id);
                edges[j]->id = id;
                *edges[j] = e;
            }

            verts[j]->edge = edges[j];
            edges[j]->v1 = verts[j];
            edges[j]->v2 = verts[next];
        }

        // Bind disks

        for (size_t j = 0; j < 3; j++) {
			int next = (j + 1) % 3;
			int prev = (3 + (j - 1)) % 3;

            if (contains[j]) {
                edges[j]->d1->next = try_get_edge(edges[prev]);
                edges[j]->d2->prev = try_get_edge(edges[next]);
            } else {
                size_t id1, id2;
                edges[j]->d1 = dp->request(id1);
                edges[j]->d1->id = id1;
                edges[j]->d2 = dp->request(id2);
                edges[j]->d2->id = id2;
                edges[j]->d1->prev = try_get_edge(edges[next]);
                edges[j]->d2->next = try_get_edge(edges[prev]);
            }
        }

        // Time to create face boundary loooops

        // They are always new
        size_t id1, id2, id3;
        auto l1 = lp->request(id1);
        l1->id = id1;
        auto l2 = lp->request(id2);
        l2->id = id2;
        auto l3 = lp->request(id3);
        l3->id = id3;

        size_t f_id;
        auto f = fp->request(f_id);
        f->id = f_id;

        loops = {l1,l2,l3};

        for (int j = 0; j < 3; j++) {
			int next = (j + 1) % 3;
			int prev = (3 + (j - 1)) % 3;

            // Populate basic loop data
        	bindLoop(loops[j], verts[j], edges[j]);

            // Bind all three loops together
            // FIXME: Seems like an overkill tho
            loops[j]->prev    = loops[prev];
            loops[j]->next    = loops[next];
            loops[prev]->prev = loops[next];
            loops[prev]->next = loops[j];
            loops[next]->prev = loops[j];
            loops[next]->next = loops[prev];
        }


		for (size_t j = 0; j < 3; j++) {

            auto _e = edges[j];
            geo::addLoopToEdge(_e, loops[j]);

            // Update the edge table with the new edge
            uniqueEdges[*_e] = _e;
            uniqueLoops[*loops[j]] = loops[j];
		}

        // Bind the face afterwards
		bindFace(f, l1, l2, l3);

        f->size = 3;
        uniqueFaces[*f] = f;
    }

    for (auto v : uniqueVerts) {
        auto _v = v.second;
        assert(_v->edge);
        assert(_v->edge->v1 == _v || _v->edge->v2 == _v);
        _outMesh.verts.push_back(_v);
    }

    for (auto e : uniqueEdges) {
        auto _e = e.second;
        assert(_e->v1);
        assert(_e->v2);
        assert(_e->loop);
        assert(_e->d1);
        assert(_e->d2);

        if (TEST_2_MANIFOLD) {
            assert(_e->d2->next);
            assert(_e->d2->prev);
            assert(_e->d1->next);
            assert(_e->d1->prev);
        }
        _outMesh.edges.push_back(_e);
    }

    for (auto l : uniqueLoops) {
        auto _l = l.second;
        assert(_l->e);
        assert(_l->v);
        assert(_l->f);

        if (TEST_2_MANIFOLD) {
            assert(_l->radial_next != _l);
            assert(_l->radial_prev != _l);
            assert(_l->radial_prev->radial_prev == _l);
            assert(_l->radial_next->radial_next == _l);
            assert(_l->next->next->next == _l);
            assert(_l->prev->prev->prev == _l);
        }
        _outMesh.loops.push_back(_l);
    }


    for (auto f : uniqueFaces) {
        auto _f = f.second;
        _outMesh.faces.push_back(_f);
        auto l = _f->loop;
        for (int i = 0 ; i < 3; i++) {
            assert(l->f == _f);
            l = l->next;
        }
    }

    for (auto d : uniqueDisks) {
        auto _d = d.second;
        if (!_d->prev || !_d->next) {
            delete _d;
        } else {
            _outMesh.disks.push_back(_d);
        }
    }

    /* trc::raw << "\n" */
    /*     << uniqueVerts.size() <<  " vertices" << "\n" */
    /*     << uniqueEdges.size() <<  " edges" << "\n" */
    /*     << uniqueLoops.size() <<  " loops" << "\n" */
    /*     << uniqueFaces.size() <<  " faces" << "\n" */
    /*     << "REMesh E.P. characteristic: " */
    /*     << uniqueVerts.size() - uniqueEdges.size() + uniqueFaces.size() << "\n" */
    /*     << "All asserts passed somehow!" << "\n" */
    /*     << "\n"; */

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
int Loader::_loadTinyGLTFModel(tinygltf::Model& gltfModel, const std::string& filename) {
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
        if (Loader::isFileValid(file)) {
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
        if (Loader::_canReadFile(filePath)) {
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
const unsigned char* Loader::_getDataByAccessor(tinygltf::Accessor accessor,
                                                const tinygltf::Model& model) {

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& posBuffer = model.buffers[bufferView.buffer];

	return &posBuffer.data[bufferView.byteOffset + accessor.byteOffset];
}

void Loader::_generateVertexNormals(ale::ViewMesh &_mesh) {

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
const int Loader::_getNumEdgesInMesh(const ViewMesh &_mesh) {
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
