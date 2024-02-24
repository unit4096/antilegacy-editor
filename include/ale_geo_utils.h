// Function for geometry manipulation
// TODO: add function descriptions

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
#include <tracer.h>

#ifndef ALE_GEO_UTILS

namespace trc = ale::Tracer;

namespace ale {
namespace geo {

/*
    Following functions are generic geometry functions
*/




// Generates frustum planes for a ModelViewProjection matrix
// w is for normals
[[maybe_unused]]
static std::vector<glm::vec4> getFrustumPlanes(const glm::mat4& mvp) {
    std::vector<glm::vec4> frustum;
    frustum.resize(6);

    auto mvpRow2 = glm::vec4(mvp[1][0],mvp[1][1],mvp[1][2],mvp[1][3]);
    auto mvpRow3 = glm::vec4(mvp[2][0],mvp[2][1],mvp[2][2],mvp[2][3]);
    auto mvpRow4 = glm::vec4(mvp[3][0],mvp[3][1],mvp[3][2],mvp[3][3]);

    frustum[0] = mvpRow4 - mvpRow2;
    frustum[1] = mvpRow4 + mvpRow2;
    frustum[2] = mvpRow4 + mvpRow3;
    frustum[3] = mvpRow4 - mvpRow3;
    frustum[4] = mvpRow4 - mvpRow4;
    frustum[5] = mvpRow4 + mvpRow4;

    // Normalize the planes
    for (int i = 0; i < 6; ++i) {
        frustum[i] = glm::normalize(frustum[i]);
    }
    return frustum;
}

// Gets the distance from a point to a plane (not absolute)
[[maybe_unused]]
static float getDistanceToPlane(const glm::vec3& point, const glm::vec4& plane) {
    auto planePos = glm::vec3(plane.x, plane.y, plane.z);
    return glm::dot(point, planePos) + plane.w;

}

// Checks if a point is contained inside frustum planes
[[maybe_unused]]
static bool isPointInFrustum(glm::vec3 point,
                             std::vector<glm::vec4>& frustum) {
    for (auto plane : frustum) {
        // I am not sure why it works this way and not the way around
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
// FIXME: not tested properly
[[maybe_unused]]
static bool rayIntersectsTriangle(const glm::vec3& rayOrigin,
                                 const glm::vec3& rayDir,
                                 const Face* face,
                                 glm::vec3& out_intersection_point) {

    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    std::vector<Loop*> loops = {};

    bool bHasLoops = getBoundingLoops(face, loops);

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
    if (det > -epsilon && det < epsilon) {
        return false;
    }

    float inv_det = 1.0 / det;
    glm::vec3 s = rayOrigin - a;
    float u = inv_det * glm::dot(s, ray_cross_e2);

    if (u < 0 || u > 1) {
        return false;
    }

    glm::vec3 s_cross_e1 = glm::cross(s, edge1);
    float v = inv_det * glm::dot(rayDir, s_cross_e1);

    if (v < 0 || u + v > 1) {
        return false;
    }

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
