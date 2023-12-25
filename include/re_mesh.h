/* 
    This is a mesh implementation that uses Radial Edge Structure for 
    fast manipulation of non-manifold meshes. It omits the shell structure since
    solid modeling is out of scope of the editor.

    Main inspiration and reference for this data structure: 
    https://wiki.blender.org/wiki/Source/Modeling/BMesh/Design
    [23.12.2023] Source code of the original data structure (well-documented!):
    https://github.com/blender/blender/blob/main/source/blender/bmesh/bmesh_class.hh

*/

#ifndef ALE_REMESH
#define ALE_REMESH

// ext
#pragma once
#include <vector>
#include <memory>

#ifndef GLM
#define GLM

#include <glm/glm.hpp>

#endif // GLM




namespace ale {
// geo namespace defines geometry primitives
namespace geo {



struct Vertex;
struct Edge;
struct Loop;
struct DiskLink;
struct Face;

// TODO: this is a boilerplate mesh class, it must be extended
class REMesh {    
public:
    std::vector<std::shared_ptr<Face>> faces;
    std::vector<std::shared_ptr<Edge>> edges;
    std::vector<std::shared_ptr<Loop>> loops;
    std::vector<std::shared_ptr<Vertex>> vertices;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    // An edge in the disk loop
    std::shared_ptr<Edge> edge;
    
    // Compares vertices. Only position is important for RE Vertices
    bool operator==(const Vertex& other) const {
        return pos == other.pos;
    }
};

struct Edge {
    // Origin and destination vertices
    std::shared_ptr<Vertex> v1, v2;
    std::shared_ptr<Loop> loop;
    std::shared_ptr<DiskLink> v1_disk, v2_disk;

    /* 
        Compares edges. It is impossible for edges to share the same vertices, 
        since edges have no winding by themseves.
    */
    bool operator==(const Edge& other) const {
        return (
                   (v1.get() == other.v1.get()  &&
                    v2.get() == other.v2.get()) || 
                   (v1.get() == other.v2.get()  &&
                    v2.get() == other.v1.get())
                );
    }

};


// Loop node around the face
struct Loop {
    std::shared_ptr<Vertex> v;
    std::shared_ptr<Edge> e;
    // The face the loop belongs to
    std::shared_ptr<Face> f;
    // Loops connected to the edge
    std::shared_ptr<Loop> radial_prev, radial_next;
    // Loops forming a face
    std::shared_ptr<Loop> prev, next;
};

// Node of a linked list that loops around edges
struct DiskLink {
    std::shared_ptr<Edge> prev, next;    
};

struct Face {    
    // A pointer to the first loop node
    std::shared_ptr<Loop> loop;    
    glm::vec3 nor;
};


} // namespace geo
} // namespace ale


namespace std {
    // A hash function for a geometry vertex. So far only the geometry matters
    template<> struct hash<ale::geo::Vertex> {
        size_t operator()(ale::geo::Vertex const& vertex) const {            
            return hash<glm::vec3>()(vertex.pos);
        }
    };
}


namespace std {
    // A hash function for an edge. There are should not exist edges that share
    // the same vertices. Note that this DOES NOT check for null pointers
    template<> struct hash<ale::geo::Edge> {
        size_t operator()(ale::geo::Edge const& edge) const {
            // Quick and dirty way to hash two pointers. Should be stable enough
            // to work with 3D meshes and given that pointers are at least unique.
            return  (size_t)edge.v1.get() + ((size_t)edge.v1.get() << 3);
        }
    };
}



#endif // ALE_REMESH