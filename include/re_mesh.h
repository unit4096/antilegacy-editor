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

// int
#include <ale_memory.h>


namespace ale {
// geo namespace defines geometry primitives
namespace geo {



struct Vert;
struct Edge;
struct VertDiskLink;
struct Loop;
struct Face;

// TODO: this is a boilerplate mesh class, it must be extended
class REMesh {
public:
    std::vector<sp<Face>> faces;
    std::vector<sp<Edge>> edges;
    std::vector<sp<Loop>> loops;
    std::vector<sp<Vert>> verts;
};

struct Vert {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    // An edge in the disk loop
    sp<Edge> edge;

    // Compares vertices. Only position is important for RE Vertices
    bool operator==(const Vert& other) const {
        return pos == other.pos;
    }
};

struct VertDiskLink {
    sp<Edge> prev, next;
};

struct Edge {
    // Origin and destination vertices
    sp<Vert> v1, v2;
    sp<Loop> loop;
    sp<VertDiskLink> d1, d2;
    int dbID = -1;

    bool operator==(const Edge& other) const {

        if (!v1 && !v2 && !other.v1 && !other.v2) {
            return false;
        }

        return (
                   (*v1.get() == *other.v1.get()  &&
                    *v2.get() == *other.v2.get()) ||
                   (*v1.get() == *other.v2.get()  &&
                    *v2.get() == *other.v1.get())
                );
    }

};


// Loop node around the face
struct Loop {
    sp<Vert> v;
    sp<Edge> e;
    // The face the loop belongs to
    sp<Face> f;
    // Loops connected to the edge
    sp<Loop> radial_prev, radial_next;
    // Loops forming a face
    sp<Loop> prev, next;

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
    sp<Loop> loop;
    glm::vec3 nor;
    unsigned int size;

	bool operator==(const Face& other) const {
        return (*loop->v.get() == *other.loop->v.get());
    }
};


} // namespace geo
} // namespace ale


namespace std {
    // A hash function for a geometry vertex. So far only the geometry matters
    template<> struct hash<ale::geo::Vert> {
        size_t operator()(ale::geo::Vert const& vertex) const {
            return hash<glm::vec3>()(vertex.pos);
        }
    };
}

namespace std {
    /*
    A hash function for an edge. There should not exist edges that share
    the same vertices. Note that this DOES NOT check for null pointers.
    This hash function is *order independent*.
    */
    template<> struct hash<ale::geo::Edge> {
        size_t operator()(ale::geo::Edge const& edge) const {
            // Hashes positions of two edges
            return hash<ale::geo::Vert>()(*edge.v1.get()) ^
				   hash<ale::geo::Vert>()(*edge.v2.get());
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
