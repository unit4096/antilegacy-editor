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

// Get Face bounding loops
[[maybe_unused]]
static bool getBoundingLoops(const Face* face,
                      std::vector<Loop*>& out_loops) {

    // The vert is not attached to any edge. Something is
    // very wrong with your topology


    if (face->loop == nullptr) {
        // This edge does not belong to any face
        return false;
    }

    out_loops = {};
    auto l = face->loop;
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

/*

// Edge cycles are a collection of loops connected to the sides
// of the current loop
// They are unordered
static void appenLoopToRadialLoopCycle(geo::Edge* e, geo::Loop* new_l) {
    auto old_l = e->loop;
    if (old_l->radial_next == nullptr && old_l->radial_prev == nullptr) {
        old_l->radial_prev = new_l;
        old_l->radial_next = new_l;
    }
    old_l->radial_next->prev = new_l;
    old_l->radial_next = new_l;
};
static bool dHasE(geo::Disk* d, geo::Edge* e) {
    return d->next == e || d->prev == e;
};

static auto getNextDInLoop(geo::Edge* e, sp<geo::Disk* d) {
    trc::raw << &d << " ";
    assert(e);
    assert(d);
    geo::Disk> res = nullptr;

    auto next = d->next;

    if (next->d1->prev == e) {
        res = next->d1;
        return res;
    }

    if (next->d2->prev == e) {
        res = next->d2;
        return res;
    }

    trc::log("NO BACK LINK FOUND!", trc::ERROR);
    return res;
};

static bool vLoopHasE(geo::Vert> v, sp<geo::Edge> e) {
    assert(e);
    assert(v->edge);

    // If an edge exists, it should have a link to at least one vertex
    assert(v->edge->d1 || v->edge->d2);


    auto _e = v->edge;
    if (_e == e) {
        return true;
    }

    auto d = _e->d1;

    if (!isV1InE(v,_e)) {
        d = _e->d2;
    }

    auto eItr = _e;
    auto dItr = d;

    if (d->next == e) {
        return true;
    }

    dItr = getNextDInLoop(eItr, dItr);

    int limit = 5;

    while (dItr->next != _e) {
        if (limit == 0) {
            trc::log("LiMIT REACHED! ", trc::ERROR);
            return false;
        }

        if (dItr->next == e) {
            return true;
        }

        dItr = getNextDInLoop(eItr, dItr);
        eItr = dItr->prev;
        limit--;
    }

    return false;
};

static void addEToVertL(geo::Vert> v, sp<geo::Edge> e_new) {
    if (geo::isV1InE(v,e_new)) {
        geo::addEToD1(v->edge, e_new);
    } else {
        geo::addEToD2(v->edge, e_new);
    }
}

*/

// Append a geo::Loop to the geo::Edge's radial loop cycle
[[maybe_unused]]
static void addLoopToEdge(geo::Edge* e, geo::Loop* l) {
    if (!e->loop) {
        l->radial_next = l;
        l->radial_prev = l;
        e->loop = l;
        return;
    }

    auto _l = e->loop;
    _l->radial_prev->radial_next = l;
    _l->radial_prev = l;
    e->loop = l;
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
