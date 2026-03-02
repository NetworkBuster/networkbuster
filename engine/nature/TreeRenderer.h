#pragma once

#include "LSystem.h"
#include "Mesh.h"
#include "../renderer/Renderer.h"

#include <array>
#include <memory>
#include <string>

namespace NRE {

/// Season identifiers for foliage appearance.
enum class Season {
    Spring,
    Summer,
    Autumn,
    Winter
};

/// Wind parameters exposed for runtime tweaking.
struct WindParams {
    float strength    = 1.0f;   ///< Overall wind force scale
    float frequency   = 0.5f;   ///< Base oscillation frequency (Hz)
    Vec3  direction   = {1.0f, 0.0f, 0.0f}; ///< Dominant wind direction (normalised)
    float gustiness   = 0.3f;   ///< Amplitude of random gusts (0–1)
    float damping     = 0.85f;  ///< Per-frame velocity damping coefficient
};

/**
 * @brief Full-featured procedural tree renderer for the Nature Reality Engine.
 *
 * ## Usage
 * @code
 * NRE::TreeRenderer tree;
 * tree.setSpecies(NRE::TreeSpecies::Oak);
 * tree.setSeason(NRE::Season::Summer);
 * tree.generate();                      // L-system + mesh build
 * tree.attachRenderer(&myVulkanRenderer);
 * tree.render();                        // Upload + draw
 *
 * // Per-frame update
 * tree.applyWind(deltaTime);
 * tree.grow(deltaTime);
 * @endcode
 */
class TreeRenderer {
public:
    TreeRenderer();
    ~TreeRenderer() = default;

    // ── Configuration ─────────────────────────────────────────────────────

    /// Choose a predefined tree species (resets L-system rules).
    void setSpecies(TreeSpecies species);

    /// Directly inject a custom L-system for fine-grained control.
    void setLSystem(const LSystem& lsystem);

    /// Set the tree's base position in world space.
    void setPosition(const Vec3& pos);

    /// Attach an IRenderer implementation; does not take ownership.
    void attachRenderer(IRenderer* renderer);

    // ── Core pipeline ─────────────────────────────────────────────────────

    /**
     * @brief Run the full generation pipeline.
     *
     * Steps performed:
     *  1. Iterate L-system for the configured number of steps.
     *  2. Interpret the resulting string with a 3-D turtle to produce
     *     BranchSegment data.
     *  3. Tessellate each segment into cylinder mesh geometry.
     *  4. Generate leaf quads at branch tips.
     *  5. Apply seasonal colour tints.
     */
    void generate();

    /**
     * @brief Submit the current mesh to the attached IRenderer.
     *
     * Uploads the mesh on the first call; subsequent calls update only
     * the vertices that were displaced by wind physics.
     */
    void render();

    // ── Dynamics ──────────────────────────────────────────────────────────

    /**
     * @brief Advance wind-physics simulation by @p dt seconds.
     *
     * Each branch segment is treated as a damped harmonic oscillator
     * driven by a sinusoidal wind force.  Tip segments receive more
     * displacement than trunk segments (proportional to 1/radius).
     */
    void applyWind(float dt);

    /// Configure wind parameters.
    void setWindParams(const WindParams& params);

    /**
     * @brief Simulate growth by @p dt seconds of in-game time.
     *
     * Scales branch radii and segment lengths slightly, then rebuilds
     * the mesh.  Designed for slow time-lapse growth sequences.
     */
    void grow(float dt);

    // ── Seasonal system ───────────────────────────────────────────────────

    /**
     * @brief Switch the active season.
     *
     * Changes leaf colours, leaf density, and ambient light colour
     * passed to the attached renderer.
     * Triggers a lightweight mesh rebuild (colours only, no topology change).
     */
    void setSeason(Season season);

    Season getSeason() const { return m_season; }

    // ── Accessors ─────────────────────────────────────────────────────────

    const Mesh&    getMesh()    const { return m_mesh; }
    const LSystem& getLSystem() const { return m_lsystem; }

    /// Total number of branch segments in the current mesh.
    size_t getBranchCount()  const { return m_mesh.branches.size(); }

    /// Total number of leaf quads in the current mesh.
    size_t getLeafCount()    const { return m_mesh.leaves.size(); }

    /// Total vertex count.
    size_t getVertexCount()  const { return m_mesh.vertices.size(); }

    /// Total triangle count.
    size_t getTriangleCount()const { return m_mesh.triangles.size(); }

private:
    // ── Internal state ────────────────────────────────────────────────────
    LSystem     m_lsystem;
    Mesh        m_mesh;
    Vec3        m_position    = {0.0f, 0.0f, 0.0f};
    Season      m_season      = Season::Summer;
    WindParams  m_windParams;
    IRenderer*  m_renderer    = nullptr;
    BufferHandle m_gpuBuffer  = {};
    bool         m_gpuDirty   = true;

    float m_growthAge       = 0.0f; ///< Accumulated growth time (seconds)
    float m_windPhase       = 0.0f; ///< Current oscillation phase (radians)

    // Per-segment wind displacement (same size as m_mesh.branches)
    std::vector<Vec3>  m_windDisplacement;
    std::vector<Vec3>  m_windVelocity;

    // ── Private helpers ───────────────────────────────────────────────────

    /// Turtle-interpret the L-system string into BranchSegment list.
    void interpretLSystem();

    /// Tessellate BranchSegment data into Mesh vertices/triangles.
    void buildMesh();

    /// Add a tapered cylinder between two points with @p rings rings.
    void addCylinder(const Vec3& start, const Vec3& end,
                     float r0, float r1,
                     int rings, int segments,
                     const Color4& color);

    /// Add a leaf quad at a branch tip.
    void addLeaf(const Vec3& tip, const Vec3& normal, float size,
                 const Color4& color);

    /// Recolour all vertices according to the current season (no topology change).
    void applySeasonalColors();

    /// Return the seasonal leaf colour for the current season.
    Color4 leafColor() const;

    /// Return the seasonal bark colour.
    Color4 barkColor() const;

    /// Build a column-major identity 4×4 float matrix with translation.
    void buildTransform(float out[16]) const;

    /// Cross product of two Vec3.
    static Vec3 cross(const Vec3& a, const Vec3& b);

    /// Dot product.
    static float dot(const Vec3& a, const Vec3& b);

    /// Euclidean length.
    static float length(const Vec3& v);

    /// Normalise; returns zero vector if length < eps.
    static Vec3 normalise(const Vec3& v);
};

} // namespace NRE
