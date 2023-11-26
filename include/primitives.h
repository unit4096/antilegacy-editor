#ifndef GLM
#define GLM

#include <glm/glm.hpp>

#endif // GLM

#ifndef PRIMITIVES
#define PRIMITIVES

#include <vector>
#include <functional>
#include <glm/gtx/hash.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};


struct Model {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct Image {
    int w;
    int h;
    int channels;
    unsigned char* data;
};

// TODO: this is a temporary solution, some aspects may be redundant
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return (
            (hash<glm::vec3>()(vertex.pos) ^
            (hash<glm::vec3>()(vertex.color) << 1)) >> 1)^
            (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}


// Uses equality operator of 
struct VertexEquals {
    bool operator()(const Vertex& a, const Vertex& b) const {
        return a==b;
    }
};

#endif // PRIMITIVES