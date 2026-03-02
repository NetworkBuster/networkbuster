#pragma once

#include "../renderer/Renderer.h"

namespace NRE {

/// Wave simulation modes.
enum class WaveMode {
    Calm,
    Choppy,
    Ocean
};

/**
 * @brief Real-time water surface renderer.
 *
 * Uses Gerstner wave superposition to animate the water surface,
 * and relies on the engine IRenderer for GPU submission.
 */
class WaterRenderer {
public:
    WaterRenderer() = default;

    void setWaveMode(WaveMode mode) { m_mode = mode; }
    void setWindSpeed(float kph)    { m_windSpeedKph = kph; }
    void attachRenderer(IRenderer* r) { m_renderer = r; }

    /// Rebuild the water mesh and upload to GPU.
    void generate(int gridX = 64, int gridZ = 64, float cellSize = 1.0f);

    /// Advance simulation and update the GPU buffer.
    void update(float dt);

    /// Submit water mesh for this frame.
    void render();

private:
    WaveMode    m_mode         = WaveMode::Calm;
    float       m_windSpeedKph = 10.0f;
    float       m_time         = 0.0f;
    IRenderer*  m_renderer     = nullptr;
    Mesh        m_mesh;
    BufferHandle m_gpuBuffer  = {};
};

} // namespace NRE
