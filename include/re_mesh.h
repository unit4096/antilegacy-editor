#include <primitives.h>

/* 
    This is a mesh implementation that uses Radial Edge Structure for 
    fast manipulation of non-manifold meshes.
*/

namespace ale {


class REMesh {

};


// Generic class for geometry primitives
class Geometry {

};

class Edge : Geometry {

};

class Face : Geometry {

};

class Vetex : Geometry {

};

class Loop : Geometry {

};


} // namespace ale

/* 
This is a draft for mesh classes. May translate to an UML diagram in the future.
Will be update as I learn about REDS.

Vertex:
    position as a glm vector
    pointer to one of the adjacent edges

Edge:
    two adjacent vertices
    link to a list of loops in a radial cycle

Face:
    first loop in a loop cycle

Loop:
    edge
    vertex at the start of edge
    face

*/
