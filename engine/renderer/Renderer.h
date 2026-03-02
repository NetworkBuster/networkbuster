#pragma once

#include "../nature/Mesh.h"
#include <cstdint>
#include <string>

namespace NRE {

/// Describes a GPU buffer allocation returned by IRenderer.
struct BufferHandle {
    uint64_t id   = 0;
    size_t   size = 0;
};

/// Opaque texture handle returned by IRenderer.
struct TextureHandle {
    uint64_t id = 0;
};

/// Simple camera description used for rendering.
struct Camera {
    Vec3  position    = {0.0f, 5.0f, 20.0f};
    Vec3  target      = {0.0f, 5.0f, 0.0f};
    Vec3  up          = {0.0f, 1.0f, 0.0f};
    float fovDegrees  = 60.0f;
    float nearPlane   = 0.1f;
    float farPlane    = 1000.0f;
};

/// Directional light (sun/moon).
struct DirectionalLight {
    Vec3   direction  = {-0.5f, -1.0f, -0.3f};
    Color4 color      = {1.0f, 0.95f, 0.8f, 1.0f};
    float  intensity  = 1.0f;
};

/**
 * @brief Abstract renderer interface.
 *
 * Concrete implementations supply a Vulkan backend (VulkanRenderer),
 * a software rasterizer for testing (SoftwareRenderer), or any other
 * GPU API.  TreeRenderer depends only on this interface, never on a
 * specific backend.
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;

    /// Initialise the renderer (create device, swap chain, etc.).
    virtual bool init(uint32_t width, uint32_t height,
                      const std::string& appName) = 0;

    /// Release all GPU resources.
    virtual void shutdown() = 0;

    /// Begin a new frame.
    virtual void beginFrame() = 0;

    /// Submit a mesh for rendering with the given transform (column-major 4×4).
    virtual void submitMesh(const Mesh& mesh,
                            const float transform[16]) = 0;

    /// Set the active camera.
    virtual void setCamera(const Camera& cam) = 0;

    /// Set the primary directional light.
    virtual void setDirectionalLight(const DirectionalLight& light) = 0;

    /// Set ambient intensity in [0,1].
    virtual void setAmbientIntensity(float ambient) = 0;

    /// Present the current frame to the display.
    virtual void endFrame() = 0;

    /// Upload a vertex/index buffer and return a handle for later updates.
    virtual BufferHandle uploadMesh(const Mesh& mesh) = 0;

    /// Update a previously uploaded buffer (for wind deformation, etc.).
    virtual void updateBuffer(BufferHandle handle, const Mesh& mesh) = 0;

    /// Load a texture from disk and return its handle.
    virtual TextureHandle loadTexture(const std::string& path) = 0;
};

} // namespace NRE
