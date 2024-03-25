/*
    Functions for geometry manipulation.
*/


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
// TODO: Assumes the face has 3 triangles. Handling full Radial Edge
// structures is WIP
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



[[maybe_unused]]
static glm::vec3 getFrontViewAABB(const ale::ViewMesh& mesh){
    const int Y = 1;
    const int Z = 2;
    assert(mesh.minPos.size() ==
           mesh.maxPos.size());
    assert(mesh.maxPos.size() >=3);

    float sizeZ = std::abs(mesh.minPos[Z] - mesh.maxPos[Z]);

    float frontEdgeZ = mesh.minPos[Z] - sizeZ;
    float middleY = (mesh.minPos[Y] + mesh.maxPos[Y]) / 2;

    return glm::vec3(0, middleY, -frontEdgeZ);
}

// Checks intersection of a ray and an axis aligned bounding box
// FIXME: Does not apply viewing frustum checks. Need additional work
[[maybe_unused]]
static bool rayIntersectsAABB(const glm::vec3& origin,
                                 const glm::vec3& direction,
                                 const glm::vec3& minB,
                                 const glm::vec3& maxB) {


    float tmin = (minB.x - origin.x) / direction.x;
    float tmax = (maxB.x - origin.x) / direction.x;

    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (minB.y - origin.y) / direction.y;
    float tymax = (maxB.y - origin.y) / direction.y;

    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;

    float tzmin = (minB.z - origin.z) / direction.z;
    float tzmax = (maxB.z - origin.z) / direction.z;

    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    return true;
}

[[maybe_unused]]
static std::pair<glm::vec3, glm::vec3> extractMinMaxAABB(const ale::ViewMesh& mesh) {
    assert(mesh.maxPos.size() == 3 && mesh.maxPos.size() == 3);
    glm::vec3 min = {mesh.minPos[0], mesh.minPos[1], mesh.minPos[2]};
    glm::vec3 max = {mesh.maxPos[0], mesh.maxPos[1], mesh.maxPos[2]};
    return {min, max};

}

// A simple function that indicates if a point is "in front" enough
// of the camera. Used mostly to avoid inversing geometry behind the
// camera. More sophisticated methods will be used in the user-space
// functions
[[maybe_unused]]
static bool isBehind(glm::mat4 viewMatrix, glm::vec3 point, float limit = 0) {

    // Inverse view matrix for world space camera orientation
    auto inv = glm::inverse(viewMatrix);

    // Camera forward vector
    auto camNorm = glm::vec3(inv[2]);
    // Camera world space position
    auto camPos  = glm::vec3(inv[3]);

    // Invert the vector for correct orientation
    camNorm*=-1;

    // Dot product of point direction relative to camera
    auto dot = glm::dot(glm::normalize(point-camPos),camNorm);

    return dot < limit;
}


// Converts global space to screen space coordinates
[[maybe_unused]]
static glm::vec2 worldToScreen(const glm::mat4& mvp, glm::vec2 displaySize,
                                   const glm::vec3& pos) {

    // Transform world position to clip space
    glm::vec4 clipCoords = mvp * glm::vec4(pos,1.0f);

    // Convert clip space coordinates to NDC (-1 to 1)
    clipCoords /= clipCoords.w;

    // Convert NDC coordinates to screen space pixel coordinates
    float screenX = (clipCoords.x + 1.0f) * 0.5f * displaySize.x;
    float screenY = (1.0f - clipCoords.y) * 0.5f * displaySize.y;

    return glm::vec2(screenX, screenY);
}


[[maybe_unused]]
static glm::vec3 screenToWorld(const glm::mat4& pv,
                               const glm::vec2& mousePos,
                               const glm::vec2& displaySize) {
    // get NCD
    float ndcX = mousePos.x / (displaySize.x * 0.5f) - 1.0f;
    float ndcY = 1.0f - mousePos.y / (displaySize.y * 0.5f);
    // get screen space offsets
    glm::vec4 rayClip = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    // Unproject offsets to get world-space coordinates
    glm::vec4 rayWorld = glm::inverse(pv) * rayClip;

    return glm::normalize(glm::vec3(rayWorld));
}


} // namespace geo
} // namespace ale


#define ALE_GEO_UTILS
#endif //ALE_GEO_UTILS
