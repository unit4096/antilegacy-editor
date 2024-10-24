/*
    This is a mesh implementation that uses Radial Edge Structure
    for fast manipulation of non-manifold meshes. It omits the shell
    structure since solid modeling is out of scope of the editor.

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
#include <unordered_set>

#ifndef GLM
#define GLM
#include <glm/glm.hpp>
#define  GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#endif // GLM

// int
#include <ale_memory.h>
#include <tracer.h>
#include <ale_pool.h>


namespace ale {
// geo namespace defines geometry primitives
namespace geo {



struct Vert;
struct Edge;
struct Disk;
struct Loop;
struct Face;


struct Vert {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    // An edge in the disk loop
    Edge *edge;
    size_t id;
    size_t viewId;


    // Compares vertices. Only position is important for RE Vertices
    bool operator==(const Vert& other) const {
        return pos == other.pos;
    }
};


struct Edge {
    // Origin and destination vertices
    Vert *v1, *v2;
    Loop *loop;
    Disk *d1, *d2;
    size_t id;

    bool operator==(const Edge& other) const {
        return (
                   (*v1 == *other.v1  &&
                    *v2 == *other.v2) ||
                   (*v1 == *other.v2  &&
                    *v2 == *other.v1)
                );
    }

};

struct Disk {
    size_t id;
    Edge *prev, *next;
    // Compares vertices. Only position is important for RE Vertices
    bool operator==(const Disk& other) const {
        return *prev == *other.next &&
               *next == *other.next;
    }
};


// Loop node around the face
struct Loop {
    Vert *v;
    Edge *e;
    // The face the loop belongs to
    Face *f;
    // Loops connected to the edge
    Loop *radial_prev, *radial_next;
    // Loops forming a face
    Loop *prev, *next;
    size_t id;

    bool operator==(const Loop& other) const {
		Loop* p1 = prev == nullptr ? prev : nullptr;
		Loop* n1 = next == nullptr ? next : nullptr;
		Loop* p2 = other.prev == nullptr ? other.prev : nullptr;
		Loop* n2 = other.next ? other.next : nullptr;

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
    Loop *loop;
    glm::vec3 *nor;
    size_t id;
    unsigned int size;

	bool operator==(const Face& other) const {
        return (*loop->v == *other.loop->v);
    }
};


// TODO: this is a boilerplate mesh class, it must be extended
class REMesh {
public:
    ~REMesh() {}
    size_t id;
    ale::Pool<Face> facesPool;
    ale::Pool<Edge> edgesPool;
    ale::Pool<Loop> loopsPool;
    ale::Pool<Vert> vertsPool;
    ale::Pool<Disk> disksPool;
    std::vector<Face*> faces;
    std::vector<Edge*> edges;
    std::vector<Loop*> loops;
    std::vector<Vert*> verts;
    std::vector<Disk*> disks;
};

} // namespace geo
} // namespace ale


namespace std {
    // A hash function for a geometry vertex. So far only the geometry matters
    template<> struct hash<ale::geo::Vert>{
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
    template<> struct hash<ale::geo::Edge>{
        size_t operator()(ale::geo::Edge const& edge) const {
            // Hashes positions of two edges
            return hash<ale::geo::Vert>()(*edge.v1) ^
				   hash<ale::geo::Vert>()(*edge.v2);
        }
    };
}

namespace std {
    // A hash function for a loop. There should not exist loops that link
    // to the same loop nodes
        template<> struct hash<ale::geo::Loop>{
        size_t operator()(ale::geo::Loop const& loop) const {
            return  (size_t)loop.prev + ((size_t)loop.next << 3);
        }
    };
}

namespace std {
    // A hash function for a face. There should not exist faces with the same
    // start loops
    template<> struct hash<ale::geo::Face>{
        size_t operator()(ale::geo::Face const& face) const {
            return hash<size_t>()((size_t)face.loop);
        }
    };
}


namespace std {
    /*
    A hash function for a Disk link. There should not exist disks that share
    the same edges.
    */
    template<> struct hash<ale::geo::Disk>{
        size_t operator()(ale::geo::Disk const& disk) const {
            // Hashes positions of two edges
            return hash<ale::geo::Edge>()(*disk.prev) +
				   (hash<ale::geo::Edge>()(*disk.next) << 3);
        }
    };
}

#endif // ALE_REMESH
