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
    
    // These four edges are links to vertex "disks". I deprecated DiskLink 
    // structures as there is no visible need for them now
    std::shared_ptr<Edge> v1_prev, v1_next, v2_prev, v2_next;

    /* 
        Compares edges. It is impossible for edges to share the same vertices,
        since edges have no winding by themseves.

        FIXME: But what if we have two different pointers pointing to the same 
        edge? Would it create false negatives then? I should eather use other
        unique identifiers rather than data addressed (potentially slow) or use
        some additional memory checks (potentially complex)
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

    bool operator==(const Loop& other) const {
		Loop* p1 = prev == nullptr ? prev.get() : nullptr;
		Loop* n1 = next == nullptr ? next.get() : nullptr;
		Loop* p2 = other.prev == nullptr ? other.prev.get() : nullptr;
		Loop* n2 = other.next ? other.next.get() : nullptr;

		bool pair1 = !p1 && !p2;
		bool pair2 = !n1 && !n2;

		if (!pair1 && !pair2) {
			return false;
		}

		return (p1 == p2) && (n1 == n2);
    }
};


struct Face {    
    // A pointer to the first loop node
    std::shared_ptr<Loop> loop;    
    glm::vec3 nor;
    unsigned int size;

	bool operator==(const Face& other) const {
        return (*loop->v.get() == *other.loop->v.get());
    }
};


} // namespace geo
} // namespace ale

/* 
	FIXME: Using pointers as hashes will cause collisions on memory reallocation.
	Should investigate this as soon as possible.
*/

namespace std {
    // A hash function for a geometry vertex. So far only the geometry matters
    template<> struct hash<ale::geo::Vertex> {
        size_t operator()(ale::geo::Vertex const& vertex) const {            
            return hash<glm::vec3>()(vertex.pos);
        }
    };
}

namespace std {
    // A hash function for an edge. There should not exist edges that share
    // the same vertices. Note that this DOES NOT check for null pointers
    template<> struct hash<ale::geo::Edge> {
        size_t operator()(ale::geo::Edge const& edge) const {
            // Quick and dirty way to hash two pointers. Should be stable enough
            // to work with 3D meshes and given that pointers are at least unique.
            return hash<ale::geo::Vertex>()(*edge.v1.get()) + 
				  (hash<ale::geo::Vertex>()(*edge.v2.get()) << 3);
        }
    };
}

namespace std {
    // A hash function for a loop. There should not exist loops that link 
    // to the same loop nodes
    template<> struct hash<ale::geo::Loop> {
        size_t operator()(ale::geo::Loop const& loop) const {
            return  (size_t)loop.prev.get() + ((size_t)loop.next.get() << 3);
        }
    };
}

namespace std {
    // A hash function for a face. There should not exist faces with the same
    // start loops
    template<> struct hash<ale::geo::Face> {
        size_t operator()(ale::geo::Face const& face) const {
            return hash<size_t>()((size_t)face.loop.get());
        }
    };
}

#endif // ALE_REMESH