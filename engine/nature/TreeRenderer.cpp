#include "TreeRenderer.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <stack>

// We use M_PI if available, otherwise define it.
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace NRE {

// ---------------------------------------------------------------------------
// Helper math
// ---------------------------------------------------------------------------

Vec3 TreeRenderer::cross(const Vec3& a, const Vec3& b)
{
    return { a.y*b.z - a.z*b.y,
             a.z*b.x - a.x*b.z,
             a.x*b.y - a.y*b.x };
}

float TreeRenderer::dot(const Vec3& a, const Vec3& b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

float TreeRenderer::length(const Vec3& v)
{
    return std::sqrt(dot(v, v));
}

Vec3 TreeRenderer::normalise(const Vec3& v)
{
    float len = length(v);
    if (len < 1e-6f) return {0.0f, 1.0f, 0.0f};
    return v * (1.0f / len);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TreeRenderer::TreeRenderer()
{
    m_lsystem.loadSpecies(TreeSpecies::Oak);
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void TreeRenderer::setSpecies(TreeSpecies species)
{
    m_lsystem.loadSpecies(species);
    m_gpuDirty = true;
}

void TreeRenderer::setLSystem(const LSystem& lsystem)
{
    m_lsystem  = lsystem;
    m_gpuDirty = true;
}

void TreeRenderer::setPosition(const Vec3& pos)
{
    m_position = pos;
}

void TreeRenderer::attachRenderer(IRenderer* renderer)
{
    m_renderer = renderer;
}

// ---------------------------------------------------------------------------
// Core pipeline: generate()
// ---------------------------------------------------------------------------

void TreeRenderer::generate()
{
    // 1. Run L-system rewriting.
    const LSystemParams& p = m_lsystem.getParams();

    // Reset to axiom by reloading (iterate builds from the current string, so
    // we need a fresh start each call).
    LSystem fresh = m_lsystem;
    // The LSystem already holds the axiom as the pre-iterate string.
    // We created `fresh` as a copy — now iterate it.
    fresh.iterate(p.iterations);

    // 2. Interpret L-system string into BranchSegments.
    // Store the iterated string back for reference.
    m_lsystem = fresh; // keep the iterated form so getMesh is consistent.

    interpretLSystem();

    // 3. Build triangle mesh.
    buildMesh();

    // 4. Apply seasonal colours.
    applySeasonalColors();

    // Initialise per-segment wind state.
    size_t n = m_mesh.branches.size();
    m_windDisplacement.assign(n, {0.0f, 0.0f, 0.0f});
    m_windVelocity    .assign(n, {0.0f, 0.0f, 0.0f});

    m_gpuDirty = true;
}

// ---------------------------------------------------------------------------
// Turtle interpreter
// ---------------------------------------------------------------------------

namespace {

/// 3-D turtle state used during L-system interpretation.
struct TurtleState {
    Vec3  pos       = {0.0f, 0.0f, 0.0f};
    Vec3  heading   = {0.0f, 1.0f, 0.0f}; // +Y = up
    Vec3  left      = {-1.0f, 0.0f, 0.0f};
    Vec3  up        = {0.0f, 0.0f, 1.0f};
    float radius    = 0.15f;
    float segLen    = 1.0f;
    int   depth     = 0;
};

/// Rotate vector `v` around unit axis `axis` by `angleDeg` degrees.
Vec3 rotateAround(const Vec3& v, const Vec3& axis, float angleDeg)
{
    float rad = static_cast<float>(angleDeg * M_PI / 180.0);
    float c = std::cos(rad), s = std::sin(rad);
    // Rodrigues' rotation formula
    float dp = v.x*axis.x + v.y*axis.y + v.z*axis.z;
    Vec3 crossed = { v.y*axis.z - v.z*axis.y,
                     v.z*axis.x - v.x*axis.z,
                     v.x*axis.y - v.y*axis.x };
    return { v.x*c + crossed.x*s + axis.x*dp*(1.0f-c),
             v.y*c + crossed.y*s + axis.y*dp*(1.0f-c),
             v.z*c + crossed.z*s + axis.z*dp*(1.0f-c) };
}

} // anonymous namespace

void TreeRenderer::interpretLSystem()
{
    m_mesh.branches.clear();
    m_mesh.leaves.clear();

    const LSystemParams& p = m_lsystem.getParams();
    const std::string&   s = m_lsystem.getString();

    float angle = p.angle;

    TurtleState cur;
    cur.radius = p.initialRadius;
    cur.segLen = p.segmentLen;
    cur.pos    = m_position;

    std::stack<TurtleState> stateStack;

    for (char c : s) {
        switch (c) {
        case 'F':
        case 'A':
        {
            Vec3 newPos = cur.pos + cur.heading * cur.segLen;
            BranchSegment seg;
            seg.start       = cur.pos;
            seg.end         = newPos;
            seg.radiusStart = cur.radius;
            seg.radiusEnd   = cur.radius * p.radiusScale;
            seg.depth       = cur.depth;
            seg.isLeaf      = false;
            m_mesh.branches.push_back(seg);
            cur.pos = newPos;
            // Taper radius slightly as we move forward.
            cur.radius *= p.radiusScale;
            // Scale segment length with depth.
            cur.segLen *= p.segmentScale;
            break;
        }
        case 'L':
        {
            // Leaf marker — emit a leaf quad at the current position.
            LeafQuad lq;
            lq.center = cur.pos;
            lq.normal = cur.up;
            lq.size   = cur.segLen * 0.5f;
            lq.color  = {0.2f, 0.7f, 0.1f, 1.0f}; // overwritten by applySeasonalColors
            m_mesh.leaves.push_back(lq);
            break;
        }
        case '+': // Turn left (around up axis)
            cur.heading = rotateAround(cur.heading, cur.up,  angle);
            cur.left    = rotateAround(cur.left,    cur.up,  angle);
            break;
        case '-': // Turn right
            cur.heading = rotateAround(cur.heading, cur.up, -angle);
            cur.left    = rotateAround(cur.left,    cur.up, -angle);
            break;
        case '^': // Pitch up (around left axis)
            cur.heading = rotateAround(cur.heading, cur.left,  angle);
            cur.up      = rotateAround(cur.up,      cur.left,  angle);
            break;
        case '&': // Pitch down
            cur.heading = rotateAround(cur.heading, cur.left, -angle);
            cur.up      = rotateAround(cur.up,      cur.left, -angle);
            break;
        case '\\': // Roll left (around heading axis)
            cur.left = rotateAround(cur.left, cur.heading,  angle);
            cur.up   = rotateAround(cur.up,   cur.heading,  angle);
            break;
        case '/': // Roll right
            cur.left = rotateAround(cur.left, cur.heading, -angle);
            cur.up   = rotateAround(cur.up,   cur.heading, -angle);
            break;
        case '[': // Push state
            stateStack.push(cur);
            cur.depth++;
            break;
        case ']': // Pop state — emit leaf at branch tip if thin
        {
            bool wasThin = (cur.radius < p.initialRadius * 0.15f);
            cur = stateStack.top();
            stateStack.pop();
            if (wasThin) {
                LeafQuad lq;
                lq.center = cur.pos;
                lq.normal = cur.heading;
                lq.size   = cur.segLen * 0.6f;
                lq.color  = {0.2f, 0.7f, 0.1f, 1.0f};
                m_mesh.leaves.push_back(lq);
            }
            break;
        }
        case '!': // Decrement radius
            cur.radius *= 0.9f;
            break;
        default:
            break; // Unknown symbols are silently skipped.
        }
    }
}

// ---------------------------------------------------------------------------
// Mesh construction
// ---------------------------------------------------------------------------

void TreeRenderer::buildMesh()
{
    m_mesh.vertices.clear();
    m_mesh.triangles.clear();

    const Color4 bark = barkColor();

    for (auto& seg : m_mesh.branches) {
        seg.meshBaseVertex = static_cast<uint32_t>(m_mesh.vertices.size());
        int radialSlices = (seg.radiusStart > 0.05f) ? 8 : 5;
        addCylinder(seg.start, seg.end,
                    seg.radiusStart, seg.radiusEnd,
                    0, radialSlices, bark);
    }

    const Color4 leaf = leafColor();
    for (const auto& lq : m_mesh.leaves) {
        addLeaf(lq.center, lq.normal, lq.size, leaf);
    }
}

void TreeRenderer::addCylinder(const Vec3& start, const Vec3& end,
                                float r0, float r1,
                                int /*rings*/, int segments,
                                const Color4& color)
{
    Vec3 axis  = normalise(end - start);
    float len  = length(end - start);
    if (len < 1e-6f) return;

    // Build an orthonormal frame around `axis`.
    Vec3 ref   = (std::abs(axis.y) < 0.9f) ? Vec3{0,1,0} : Vec3{1,0,0};
    Vec3 perp1 = normalise(cross(axis, ref));
    Vec3 perp2 = cross(axis, perp1);

    int   radialSlices = segments;
    float dTheta = static_cast<float>(2.0 * M_PI / radialSlices);

    // Two rings: bottom (start) and top (end).
    for (int ring = 0; ring <= 1; ++ring) {
        Vec3  center  = (ring == 0) ? start : end;
        float radius  = (ring == 0) ? r0    : r1;
        float v_coord = static_cast<float>(ring);

        for (int i = 0; i < radialSlices; ++i) {
            float theta  = i * dTheta;
            float cosT   = std::cos(theta);
            float sinT   = std::sin(theta);
            Vec3  offset = perp1 * (cosT * radius) + perp2 * (sinT * radius);
            Vec3  pos    = center + offset;
            Vec3  nrm    = normalise(offset);

            Vertex vtx;
            vtx.position = pos;
            vtx.normal   = nrm;
            vtx.u        = static_cast<float>(i) / radialSlices;
            vtx.v        = v_coord;
            vtx.color    = color;
            m_mesh.vertices.push_back(vtx);
        }
    }

    // Stitch into triangles.
    uint32_t base = static_cast<uint32_t>(m_mesh.vertices.size()) - 2 * radialSlices;
    for (int i = 0; i < radialSlices; ++i) {
        int next = (i + 1) % radialSlices;
        uint32_t b0 = base + i;
        uint32_t b1 = base + next;
        uint32_t t0 = base + radialSlices + i;
        uint32_t t1 = base + radialSlices + next;
        m_mesh.triangles.push_back({b0, t0, b1});
        m_mesh.triangles.push_back({b1, t0, t1});
    }
}

void TreeRenderer::addLeaf(const Vec3& tip, const Vec3& normal, float size,
                            const Color4& color)
{
    // Build two orthogonal axes in the leaf plane.
    Vec3 nrm = normalise(normal);
    Vec3 ref = (std::abs(nrm.y) < 0.9f) ? Vec3{0,1,0} : Vec3{1,0,0};
    Vec3 t   = normalise(cross(nrm, ref));
    Vec3 b   = cross(nrm, t);

    // Four vertices of the leaf quad.
    uint32_t base = static_cast<uint32_t>(m_mesh.vertices.size());
    const Vec3 corners[4] = {
        tip + t * (-size) + b * (-size),
        tip + t * ( size) + b * (-size),
        tip + t * ( size) + b * ( size),
        tip + t * (-size) + b * ( size),
    };
    const float us[4] = {0,1,1,0};
    const float vs[4] = {0,0,1,1};

    for (int i = 0; i < 4; ++i) {
        Vertex v;
        v.position = corners[i];
        v.normal   = nrm;
        v.u        = us[i];
        v.v        = vs[i];
        v.color    = color;
        m_mesh.vertices.push_back(v);
    }
    m_mesh.triangles.push_back({base+0, base+1, base+2});
    m_mesh.triangles.push_back({base+0, base+2, base+3});
}

// ---------------------------------------------------------------------------
// Seasonal colours
// ---------------------------------------------------------------------------

Color4 TreeRenderer::leafColor() const
{
    switch (m_season) {
        case Season::Spring: return {0.45f, 0.85f, 0.30f, 1.0f};  // bright green
        case Season::Summer: return {0.13f, 0.55f, 0.13f, 1.0f};  // deep green
        case Season::Autumn: return {0.80f, 0.40f, 0.05f, 1.0f};  // orange/red
        case Season::Winter: return {0.50f, 0.50f, 0.50f, 0.0f};  // no leaves (alpha=0)
    }
    return {0.2f, 0.6f, 0.2f, 1.0f};
}

Color4 TreeRenderer::barkColor() const
{
    switch (m_season) {
        case Season::Spring: return {0.45f, 0.32f, 0.20f, 1.0f};
        case Season::Summer: return {0.38f, 0.26f, 0.16f, 1.0f};
        case Season::Autumn: return {0.42f, 0.30f, 0.18f, 1.0f};
        case Season::Winter: return {0.30f, 0.22f, 0.14f, 1.0f}; // darker, wet bark
    }
    return {0.40f, 0.28f, 0.18f, 1.0f};
}

void TreeRenderer::applySeasonalColors()
{
    Color4 lc = leafColor();
    Color4 bc = barkColor();

    // Leaf quads are always appended after branch vertices.
    // Most robust split: total vertices - 4 * leaves.size()
    size_t firstLeafVtx = m_mesh.vertices.size() - m_mesh.leaves.size() * 4;

    for (size_t i = 0; i < m_mesh.vertices.size(); ++i) {
        m_mesh.vertices[i].color = (i >= firstLeafVtx) ? lc : bc;
    }

    for (auto& lq : m_mesh.leaves) {
        lq.color = lc;
    }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void TreeRenderer::buildTransform(float out[16]) const
{
    // Column-major identity with translation from m_position.
    std::memset(out, 0, 16 * sizeof(float));
    out[0]  = 1.0f;
    out[5]  = 1.0f;
    out[10] = 1.0f;
    out[15] = 1.0f;
    out[12] = m_position.x;
    out[13] = m_position.y;
    out[14] = m_position.z;
}

void TreeRenderer::render()
{
    if (!m_renderer) return;

    float transform[16];
    buildTransform(transform);

    if (m_gpuDirty) {
        m_gpuBuffer = m_renderer->uploadMesh(m_mesh);
        m_gpuDirty  = false;
    } else if (!m_windDisplacement.empty()) {
        // Only re-upload if wind displaced vertices.
        m_renderer->updateBuffer(m_gpuBuffer, m_mesh);
    }

    m_renderer->submitMesh(m_mesh, transform);
}

// ---------------------------------------------------------------------------
// Wind physics
// ---------------------------------------------------------------------------

void TreeRenderer::setWindParams(const WindParams& params)
{
    m_windParams = params;
}

void TreeRenderer::applyWind(float dt)
{
    if (m_mesh.branches.empty()) return;

    m_windPhase += m_windParams.frequency * dt * static_cast<float>(2.0 * M_PI);

    const float baseForce = m_windParams.strength;
    Vec3        windDir   = m_windParams.direction;

    // Ensure wind displacement vector matches branch count.
    size_t n = m_mesh.branches.size();
    if (m_windDisplacement.size() != n) {
        m_windDisplacement.assign(n, {0,0,0});
        m_windVelocity    .assign(n, {0,0,0});
    }

    for (size_t i = 0; i < n; ++i) {
        BranchSegment& seg = m_mesh.branches[i];

        // Thinner branches (higher depth) sway more.
        float stiffness = seg.radiusStart * 80.0f; // spring constant
        float mass      = seg.radiusStart * seg.radiusStart; // proportional mass

        // Sinusoidal driving force with a small random-ish gust term.
        float gust   = 1.0f + m_windParams.gustiness *
                       std::sin(m_windPhase * 3.7f + static_cast<float>(i) * 0.13f);
        float fMag   = baseForce * gust * std::sin(m_windPhase + static_cast<float>(i) * 0.05f);

        Vec3 force = windDir * fMag;

        // Spring-damper integration (Euler).
        Vec3& disp = m_windDisplacement[i];
        Vec3& vel  = m_windVelocity[i];

        Vec3 spring = disp * (-stiffness);
        Vec3 accel  = (force + spring) * (1.0f / (mass + 1e-6f));

        vel  += accel * dt;
        vel.x *= m_windParams.damping;
        vel.y *= m_windParams.damping;
        vel.z *= m_windParams.damping;
        disp += vel * dt;

        // Apply displacement to the end vertices of this segment.
        // meshBaseVertex points to the start ring; end ring follows.
        // Each cylinder has 2 * radialSlices vertices; we displace the
        // top ring (indices meshBaseVertex + radialSlices .. +2*radialSlices-1).
        uint32_t base = seg.meshBaseVertex;
        // Determine how many vertices belong to this segment by checking the
        // base of the *next* segment (or end of branch vertices).
        uint32_t nextBase = (i + 1 < n) ? m_mesh.branches[i+1].meshBaseVertex
                                         : static_cast<uint32_t>(
                                               m_mesh.vertices.size() -
                                               m_mesh.leaves.size() * 4);
        uint32_t total = nextBase - base;
        uint32_t half  = total / 2;

        for (uint32_t v = base + half; v < base + total; ++v) {
            if (v >= m_mesh.vertices.size()) break;
            m_mesh.vertices[v].position.x += disp.x;
            m_mesh.vertices[v].position.y += disp.y;
            m_mesh.vertices[v].position.z += disp.z;
        }
    }

    m_gpuDirty = true;
}

// ---------------------------------------------------------------------------
// Growth simulation
// ---------------------------------------------------------------------------

void TreeRenderer::grow(float dt)
{
    m_growthAge += dt;

    // Very slow growth: scale radii and segment lengths by a tiny amount.
    float growthFactor = 1.0f + 0.001f * dt; // 0.1% per second of game time

    LSystemParams& p = m_lsystem.getParams();
    p.segmentLen    *= growthFactor;
    p.initialRadius *= growthFactor;

    // Rebuild mesh to reflect new dimensions.
    interpretLSystem();
    buildMesh();
    applySeasonalColors();
    m_gpuDirty = true;
}

// ---------------------------------------------------------------------------
// Seasonal changes
// ---------------------------------------------------------------------------

void TreeRenderer::setSeason(Season season)
{
    m_season = season;

    if (!m_mesh.vertices.empty()) {
        applySeasonalColors();
        m_gpuDirty = true;
    }
}

} // namespace NRE
