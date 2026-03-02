#pragma once

#include "../renderer/Renderer.h"
#include <cmath>

namespace NRE {

/// Precipitation types.
enum class PrecipitationType {
    None,
    Rain,
    Snow,
    Hail
};

/// Atmospheric condition preset.
enum class WeatherPreset {
    Clear,
    Overcast,
    Foggy,
    Stormy
};

/**
 * @brief Atmospheric weather and sky rendering system.
 *
 * Manages sky colour, sun/moon position, cloud coverage, precipitation
 * particles, and provides a DirectionalLight that matches the current
 * solar angle for physically-based sky illumination.
 */
class WeatherSystem {
public:
    WeatherSystem() = default;

    void setPreset(WeatherPreset preset) { m_preset = preset; applyPreset(); }
    void setPrecipitation(PrecipitationType p) { m_precipitation = p; }

    /// Advance the time of day by @p dt seconds of in-game time.
    void update(float dt)
    {
        m_timeOfDay = std::fmod(m_timeOfDay + dt / 86400.0f, 1.0f);
        updateSunDirection();
    }

    /// Push the current sky light into the given renderer.
    void applyToRenderer(IRenderer* renderer) const
    {
        if (!renderer) return;
        DirectionalLight light;
        light.direction = m_sunDir;
        light.intensity = std::max(0.0f, -m_sunDir.y);
        renderer->setDirectionalLight(light);
        renderer->setAmbientIntensity(m_ambient);
    }

    /// Return current sun direction (normalised, pointing away from sun).
    Vec3 getSunDirection() const { return m_sunDir; }

    /// Current ambient intensity in [0,1].
    float getAmbientIntensity() const { return m_ambient; }

    /// Set time of day, 0 = midnight, 0.5 = noon.
    void setTimeOfDay(float normalised)
    {
        m_timeOfDay = normalised;
        updateSunDirection();
    }

private:
    WeatherPreset     m_preset       = WeatherPreset::Clear;
    PrecipitationType m_precipitation= PrecipitationType::None;
    float             m_timeOfDay    = 0.5f; ///< [0,1] normalised
    Vec3              m_sunDir       = {-0.5f, -1.0f, -0.3f};
    float             m_ambient      = 0.2f;

    void applyPreset()
    {
        switch (m_preset) {
            case WeatherPreset::Clear:    m_ambient = 0.20f; break;
            case WeatherPreset::Overcast: m_ambient = 0.40f; break;
            case WeatherPreset::Foggy:    m_ambient = 0.55f; break;
            case WeatherPreset::Stormy:   m_ambient = 0.15f; break;
        }
    }

    void updateSunDirection()
    {
        // Simple arc: timeOfDay 0=midnight, 0.5=noon.
        float angle = static_cast<float>((m_timeOfDay - 0.25) * 2.0 * 3.14159265358979323846);
        m_sunDir = { 0.0f, -std::cos(angle), -std::sin(angle) };
    }
};

} // namespace NRE
