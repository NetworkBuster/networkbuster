#pragma once

#include <cstdint>
#include <vector>

namespace NRE {

/// 3-component float vector used throughout the engine.
struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s)       const { return {x*s,   y*s,   z*s};   }
    Vec3& operator+=(const Vec3& o){ x+=o.x; y+=o.y; z+=o.z; return *this; }
};

/// 4-component color (RGBA, each in [0,1]).
struct Color4 {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

/// A single mesh vertex with position, normal, UV, and color.
struct Vertex {
    Vec3   position;
    Vec3   normal;
    float  u = 0.0f, v = 0.0f;
    Color4 color;
};

/// A triangle index triple.
struct Triangle {
    uint32_t i0, i1, i2;
};

/**
 * @brief Represents a section of a tree branch as a tapered cylinder.
 *
 * Stores origin, direction, radii at both ends, depth, and a precomputed
 * index into the vertex/index arrays after mesh generation.
 */
struct BranchSegment {
    Vec3  start;          ///< Start position in world space
    Vec3  end;            ///< End position in world space
    float radiusStart;    ///< Cylinder radius at start
    float radiusEnd;      ///< Cylinder radius at end
    int   depth;          ///< Branching depth (0 = trunk)
    bool  isLeaf = false; ///< True when this segment bears leaf geometry
    uint32_t meshBaseVertex = 0; ///< First vertex index in Mesh::vertices
};

/**
 * @brief Simple leaf quad attached to a branch tip.
 */
struct LeafQuad {
    Vec3  center;     ///< Center position
    Vec3  normal;     ///< Facing direction
    float size;       ///< Half-extent
    Color4 color;     ///< Seasonal color tint
};

/**
 * @brief Triangle mesh produced by TreeRenderer.
 *
 * Vertices and triangles are indexed so the same buffer can be uploaded
 * directly to a GPU (Vulkan VkBuffer, OpenGL VBO, etc.).
 */
struct Mesh {
    std::vector<Vertex>        vertices;
    std::vector<Triangle>      triangles;
    std::vector<BranchSegment> branches;  ///< Source branch data (for physics)
    std::vector<LeafQuad>      leaves;

    void clear() {
        vertices.clear();
        triangles.clear();
        branches.clear();
        leaves.clear();
    }
};

} // namespace NRE
