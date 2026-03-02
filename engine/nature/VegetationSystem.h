#pragma once

#include "Mesh.h"
#include "../renderer/Renderer.h"

#include <vector>

namespace NRE {

/// Controls grass / undergrowth density.
enum class VegetationDensity {
    Sparse,
    Normal,
    Dense
};

/**
 * @brief Procedural vegetation (grass, shrubs, flowers) system.
 *
 * Scatters instanced blade geometry across a terrain patch, using
 * wind-driven vertex shader animation for performance.
 */
class VegetationSystem {
public:
    VegetationSystem() = default;

    void setDensity(VegetationDensity d) { m_density = d; }
    void setTerrainSize(float sizeX, float sizeZ) {
        m_sizeX = sizeX; m_sizeZ = sizeZ;
    }
    void attachRenderer(IRenderer* r) { m_renderer = r; }

    /// Scatter vegetation instances across the terrain area.
    void generate();

    /// Per-frame update (wind sway, culling).
    void update(float dt);

    /// Submit all visible vegetation for rendering.
    void render();

    size_t getInstanceCount() const { return m_instanceCount; }

private:
    VegetationDensity m_density       = VegetationDensity::Normal;
    float             m_sizeX         = 100.0f;
    float             m_sizeZ         = 100.0f;
    size_t            m_instanceCount = 0;
    IRenderer*        m_renderer      = nullptr;
    Mesh              m_bladeMesh;    ///< Shared blade geometry (instanced)
    BufferHandle      m_gpuBuffer     = {};
};

} // namespace NRE
