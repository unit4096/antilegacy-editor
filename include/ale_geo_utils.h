// Function for geometry manipulation
// TODO: add function descriptions

//ext
#pragma once
#ifndef GLM
#define GLM
#include <glm/glm.hpp>
#endif // GLM

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>

#include <limits>

//int
#include <primitives.h>
#include <re_mesh.h>
#include <ale_memory.h>
#include <tracer.h>

#ifndef ALE_GEO_UTILS

namespace trc = ale::Tracer;

namespace ale {
namespace geo {

/*
    Following functions are generic geometry functions
*/


// FIXME: This function does not return corect output
// Generates frustum planes for a ModelViewProjection matrix
[[maybe_unused]]
static std::vector<glm::vec4> getFrustumPlanes(const glm::mat4& mvp) {
    glm::transpose(mvp);
    std::vector<glm::vec4> frustum;
    frustum.resize(6);

    // Right
    frustum[0] = glm::vec4(mvp[3] - mvp[0]);
    // Left
    frustum[1] = glm::vec4(mvp[3] + mvp[0]);
    // Bottom
    frustum[2] = glm::vec4(mvp[3] + mvp[1]);
    // Top
    frustum[3] = glm::vec4(mvp[3] - mvp[1]);
    // Far
    frustum[4] = glm::vec4(mvp[3] - mvp[2]);
    // Near
    frustum[5] = glm::vec4(mvp[3] + mvp[2]);

    return frustum;
}

// Gets the distance from a point to a plane (not absolute)
[[maybe_unused]]
static float getDistanceToPlane(const glm::vec3& point, const glm::vec4& plane) {
    auto norm = glm::vec3(plane.x, plane.y, plane.z);
    return glm::dot(point, norm) + plane.w;

}

// Checks if a point is contained inside frustum planes
[[maybe_unused]]
static bool isPointInFrustum(glm::vec3 point,
                             std::vector<glm::vec4>& frustum) {
for (auto plane : frustum) {
    if (getDistanceToPlane(point, plane) > 0.0f) {
        return false;
    }
}
    return true;
}



/*
    Following functions manage REMesh primitives.
*/

// Append a geo::Loop to the geo::Edge's radial loop cycle
[[maybe_unused]]
static void addLoopToEdge(geo::Edge* e, geo::Loop* l) {
    assert(e);
    assert(l);
    // If an edge does not have a loop, add l as e->loop
    // and make l loop to itself
    if (!e->loop) {
        l->radial_next = l;
        l->radial_prev = l;
        e->loop = l;
        return;
    }

    auto _l = e->loop;
    // e->loop loops to itself
    if (_l->radial_next == _l) {
        // Make e->loop to loop back to l
        l->radial_next = _l;
        l->radial_prev = _l;
        _l->radial_next = l;
        _l->radial_prev = l;
        return;
    }
    // There is a radial cycle and it is not just one loop

    // Insert l between e->loop and its radial_next
    l->radial_prev = _l;
    l->radial_next = _l->radial_next;
    _l->radial_next->radial_prev = l;
    _l->radial_next = l;
}

// Get bounding loops of geo::Face
[[maybe_unused]]
static bool getBoundingLoops(const Face* face,
                      std::vector<Loop*>& out_loops) {

    assert(face->size >= 3);
    if (face->loop == nullptr) {
        // This edge does not belong to any face
        return false;
    }
    auto l = face->loop;
    auto lItr = l;
    assert(lItr->next);
    // Reset out_ variable
    out_loops = {};

    out_loops.push_back(lItr);
    lItr = lItr->next;

    // Iterate over the loop cycle
    while (lItr != l) {
        assert(lItr->next != nullptr);
        out_loops.push_back(lItr);
        lItr = lItr->next;
    }

    return true;
}


// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
// Checks intersection of a ray and a triangle
[[maybe_unused]]
static bool rayIntersectsTriangle(const glm::vec3& rayOrigin,
                                 const glm::vec3& rayDir,
                                 const Face* face,
                                 glm::vec2& out_intersection_point,
                                 float& distance) {

    std::vector<Loop*> loops = {};

    bool bHasLoops = getBoundingLoops(face, loops);

    // Works only with triangle faces
    assert(bHasLoops && loops.size() == 3);

    glm::vec3 a = loops[0]->v->pos;
    glm::vec3 b = loops[1]->v->pos;
    glm::vec3 c = loops[2]->v->pos;

    return glm::intersectRayTriangle(rayOrigin, rayDir, a, b, c, out_intersection_point,distance);
}

} // namespace geo
} // namespace ale


#define ALE_GEO_UTILS
#endif //ALE_GEO_UTILS
