// Function for geometry manipulation

//ext
#pragma once
#ifndef GLM
#define GLM

#include <glm/glm.hpp>

#endif // GLM

#include <limits>

//int
#include <primitives.h>
#include <re_mesh.h>
#include <ale_memory.h>

#ifndef ALE_GEO_UTILS

namespace ale {
namespace geo {

static bool getBoundingLoops(const sp<Vert>& vert,
                      std::vector<sp<Loop>>& out_loops) {
    // The vert is not attached to any edge. Something is
    // very wrong with your topology
    assert(vert->edge);

    auto e = vert->edge;

    if (e->loop == nullptr) {
        // This edge does not belong to any face
        return false;
    }

    out_loops = {};
    auto l = e->loop;
    auto lItr = l;
    assert(lItr->next);
    out_loops.push_back(lItr);
    lItr = lItr->next;

    while (lItr != l) {
        assert(lItr->next != nullptr);
        out_loops.push_back(lItr);
        lItr = lItr->next;
    }

    return true;
}

// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm

static bool rayIntersectsTriangle(glm::vec3 rayOrigin,
                         glm::vec3 rayDir,
                         const sp<Vert>& triangle,
                         glm::vec3& out_intersection_point) {

    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    std::vector<sp<Loop>> loops = {};

    bool bHasLoops = getBoundingLoops(triangle, loops);

    // Works only with triangle faces
    assert(bHasLoops && loops.size() == 3);

    glm::vec3 a = loops[0]->v->pos;
    glm::vec3 b = loops[1]->v->pos;
    glm::vec3 c = loops[2]->v->pos;

    glm::vec3 edge1 = b - a;
    glm::vec3 edge2 = c - a;
    glm::vec3 ray_cross_e2 = glm::cross(rayDir, edge2);
    float det = glm::dot(edge1, ray_cross_e2);

    // This ray is parallel to this triangle.
    if (det > -epsilon && det < epsilon) return false;

    float inv_det = 1.0 / det;
    glm::vec3 s = rayOrigin - a;
    float u = inv_det * glm::dot(s, ray_cross_e2);

    if (u < 0 || u > 1) return false;

    glm::vec3 s_cross_e1 = glm::cross(s, edge1);
    float v = inv_det * glm::dot(rayDir, s_cross_e1);

    if (v < 0 || u + v > 1) return false;

    // At this stage we can compute t to find out
    // where the intersection point is on the line.
    float t = inv_det * glm::dot(edge2, s_cross_e1);

    if (t > epsilon) {
        out_intersection_point = rayOrigin + rayDir * t;
        return true;
    }

    // This means that there is a line
    // intersection but not a ray intersection.
    else
        return false;
}


} // namespace geo
} // namespace ale


#define ALE_GEO_UTILS
#endif //ALE_GEO_UTILS
