/*
 * Structs for 3d objects and helper functions for Antilegacy Editor
*/



#ifndef GLM
#define GLM

#include <glm/glm.hpp>

#endif // GLM

#ifndef ALE_PRIMITIVES
#define ALE_PRIMITIVES

#pragma once
//ext
#include <vector>
#include <string>
#include <functional>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

//int
#include <re_mesh.h>


namespace ale {
namespace geo {
    template<typename T>
    float* glmMatToPtr(T& mat) {
        return glm::value_ptr(mat);
    }
} // namespace geo
} // namespace ale

using namespace ale;


// TOOD: add namespace
namespace ale {
struct Model;
struct ViewMesh;
struct Vertex;
struct Node;
struct NodeData;
struct Transform;

struct MVP {
    glm::mat4 m;
    glm::mat4 v;
    glm::mat4 p;
};

struct Transform {
    glm::vec3 pos;
    glm::quat rot;
    glm::mat4 sca;
};

// A vertex with a position, a vertex color, and UV coordinates
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal = glm::vec3(0);

    bool operator==(const ale::Vertex& other) const {
      return pos == other.pos && color == other.color &&
             texCoord == other.texCoord;
    }
};


// An image struct with img. parameters and a raw pointer to data. Used for stbi
struct Image {
    int w;
    int h;
    int channels;
    std::vector<unsigned char> data;
};


// Basic mesh, contains arrays of indices and vertices
struct ViewMesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<float> minPos{};
    std::vector<float> maxPos{};
};


struct Node {
    std::string name;
    glm::mat4 transform;
    int id;
    int parent = -1;
    int mesh = -1;
    std::vector<int> children{};
    bool bVisible = true;
};

// TODO: Might be good to move Model to a separate TU and link both re_mesh
// and primitives.


// Generic view model for rendering and scene description
struct Model {
    std::vector<ViewMesh> meshes;
    std::vector<Image> textures;
    std::vector<Node> nodes;
    std::vector<int> rootNodes;
    std::vector<geo::REMesh> reMeshes;

    void applyNodeParentTransforms(int nodeID, glm::mat4& result) const {
        auto n = nodes[nodeID];
        while (n.parent > -1) {
            const auto parentID = n.parent;
            const auto parent = nodes[parentID];
            result = parent.transform * result;
            n = parent;
        }
    };
};


} // namespace ale


// A hash funciton to compare vertices. For removing duplicates in unordered_map
namespace std {
    template<> struct hash<ale::Vertex> {
        size_t operator()(ale::Vertex const& vertex) const {
            return (
            (hash<glm::vec3>()(vertex.pos) ^
            (hash<glm::vec3>()(vertex.color) << 1)) >> 1)^
            (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}


#endif // ALE_PRIMITIVES
