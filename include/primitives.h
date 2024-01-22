#ifndef GLM
#define GLM

#include <glm/glm.hpp>

#endif // GLM

#ifndef PRIMITIVES
#define PRIMITIVES

#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <glm/gtx/hash.hpp>


namespace ale { }

using namespace ale;

// TOOD: add namespace 
namespace ale {
struct Model;
struct Mesh;
struct Vertex;
struct Node;
struct NodeData;

// A vertex with a position, a vertex color, and UV coordinates
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal = glm::vec3(0);

    bool operator==(const ale::Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
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
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};


struct Node {
    std::string name;
    glm::vec3 pos;
    int id;
    int parent = -1;
    int mesh = -1;
    std::vector<int> children{};
};

struct Model {
    std::vector<Mesh> meshes;
    std::vector<Image> textures;    
    std::vector<Node> nodes;
    std::vector<int> rootNodes;
};

// Basic 2D vector from 2 floats. Can be used as a vector implementation wrapper
struct v2f {
    float x,y;
    v2f(float _x, float _y):x(_x), y(_y){}
};

// Basic 2D vector from 2 doubles. Can be used as a vector implementation wrapper
struct v2d {
    double x,y;
    v2d(double _x, double _y):x(_x), y(_y){}
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


#endif // PRIMITIVES