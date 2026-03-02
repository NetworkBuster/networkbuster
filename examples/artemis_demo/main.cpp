/**
 * @file main.cpp
 * @brief Artemis L-System Tree Demo
 *
 * Standalone demonstration of the Nature Reality Engine TreeRenderer.
 * Generates procedural trees from L-system rules, displays statistics
 * and an ASCII cross-section visualisation, and simulates wind physics
 * plus seasonal transitions — all without requiring a GPU.
 *
 * Build (from repo root):
 *   cmake -S . -B build && cmake --build build
 *   ./build/artemis_demo
 */

#include "../../engine/nature/LSystem.h"
#include "../../engine/nature/TreeRenderer.h"
#include "../../engine/nature/WaterRenderer.h"
#include "../../engine/nature/VegetationSystem.h"
#include "../../engine/nature/WeatherSystem.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Console helpers
// ---------------------------------------------------------------------------

static void printSeparator(char ch = '-', int width = 60)
{
    std::cout << std::string(width, ch) << '\n';
}

static void printHeader(const std::string& title)
{
    printSeparator('=');
    std::cout << "  " << title << '\n';
    printSeparator('=');
}

// ---------------------------------------------------------------------------
// ASCII cross-section visualisation
//
// Projects the 3-D branch segment midpoints onto the XY plane and renders
// them as '*' characters in an 80×24 terminal grid.
// ---------------------------------------------------------------------------

static void renderASCII(const NRE::Mesh& mesh, int width = 72, int height = 22)
{
    if (mesh.branches.empty()) {
        std::cout << "(no branches to display)\n";
        return;
    }

    // Find bounding box of branch midpoints.
    float minX =  1e9f, maxX = -1e9f;
    float minY =  1e9f, maxY = -1e9f;

    for (const auto& seg : mesh.branches) {
        float mx = (seg.start.x + seg.end.x) * 0.5f;
        float my = (seg.start.y + seg.end.y) * 0.5f;
        minX = std::min(minX, mx); maxX = std::max(maxX, mx);
        minY = std::min(minY, my); maxY = std::max(maxY, my);
    }

    float rangeX = maxX - minX + 1e-6f;
    float rangeY = maxY - minY + 1e-6f;

    // Build grid.
    std::vector<std::string> grid(height, std::string(width, ' '));

    auto plot = [&](float wx, float wy, char ch) {
        int col = static_cast<int>((wx - minX) / rangeX * (width  - 1));
        int row = static_cast<int>((wy - minY) / rangeY * (height - 1));
        row = (height - 1) - row; // flip Y so root is at bottom
        col = std::max(0, std::min(col, width  - 1));
        row = std::max(0, std::min(row, height - 1));
        if (grid[row][col] == ' ') grid[row][col] = ch;
    };

    // Draw trunk first, then branches.
    for (const auto& seg : mesh.branches) {
        char ch = (seg.depth == 0) ? '#'
                : (seg.depth <= 2) ? '|'
                : (seg.depth <= 4) ? '+'
                :                    '.';
        float mx = (seg.start.x + seg.end.x) * 0.5f;
        float my = (seg.start.y + seg.end.y) * 0.5f;
        plot(mx, my, ch);

        // Also plot the segment endpoints so thin branches show up.
        plot(seg.start.x, seg.start.y,
             (seg.depth == 0) ? '#' : (seg.depth <= 2) ? '|' : '+');
        plot(seg.end.x,   seg.end.y, ch);
    }

    // Draw leaf tips as 'o'.
    for (const auto& lq : mesh.leaves) {
        plot(lq.center.x, lq.center.y, 'o');
    }

    // Print.
    printSeparator('-', width + 2);
    for (const auto& row : grid) {
        std::cout << '|' << row << "|\n";
    }
    printSeparator('-', width + 2);
}

// ---------------------------------------------------------------------------
// Stats printer
// ---------------------------------------------------------------------------

static void printStats(const NRE::TreeRenderer& tree, const std::string& label)
{
    std::cout << "\n[" << label << "]\n";
    std::cout << "  L-system length : " << tree.getLSystem().getString().size() << " chars\n";
    std::cout << "  Branch segments : " << tree.getBranchCount()  << '\n';
    std::cout << "  Leaf quads      : " << tree.getLeafCount()    << '\n';
    std::cout << "  Vertices        : " << tree.getVertexCount()  << '\n';
    std::cout << "  Triangles       : " << tree.getTriangleCount()<< '\n';
}

// ---------------------------------------------------------------------------
// Demo: single species
// ---------------------------------------------------------------------------

static void demoSpecies(NRE::TreeSpecies species, const std::string& name)
{
    printHeader("Species: " + name);

    NRE::TreeRenderer tree;
    tree.setSpecies(species);
    tree.generate();

    printStats(tree, "Initial (Summer)");
    renderASCII(tree.getMesh());

    // Seasonal cycle.
    const std::pair<NRE::Season, std::string> seasons[] = {
        { NRE::Season::Spring, "Spring" },
        { NRE::Season::Autumn, "Autumn" },
        { NRE::Season::Winter, "Winter" },
    };

    for (const auto& [season, sname] : seasons) {
        tree.setSeason(season);
        std::cout << "\n  -> Season changed to: " << sname
                  << "  (leaf alpha="
                  << std::fixed << std::setprecision(2)
                  << (tree.getMesh().leaves.empty() ? 0.0f
                      : tree.getMesh().leaves[0].color.a)
                  << ")\n";
    }

    // Wind simulation (10 frames × 0.016 s).
    tree.setSeason(NRE::Season::Summer);
    NRE::WindParams wind;
    wind.strength  = 2.0f;
    wind.frequency = 1.2f;
    wind.direction = {1.0f, 0.0f, 0.0f};
    tree.setWindParams(wind);

    std::cout << "\n  Wind simulation (10 frames):\n";
    float totalDisp = 0.0f;
    for (int f = 0; f < 10; ++f) {
        tree.applyWind(0.016f);
        // Measure average tip displacement.
        const auto& verts = tree.getMesh().vertices;
        if (!verts.empty()) {
            float disp = std::abs(verts.back().position.x - 0.0f);
            totalDisp += disp;
        }
    }
    std::cout << "    Avg tip X displacement across 10 frames: "
              << std::fixed << std::setprecision(4)
              << (totalDisp / 10.0f) << " units\n";

    // Growth.
    std::cout << "\n  Growth simulation (5 steps × 1 s each):\n";
    size_t startVtx = tree.getVertexCount();
    for (int g = 0; g < 5; ++g) {
        tree.grow(1.0f);
    }
    std::cout << "    Vertices before: " << startVtx
              << "  after: " << tree.getVertexCount() << '\n';
}

// ---------------------------------------------------------------------------
// Demo: engine ecosystem overview
// ---------------------------------------------------------------------------

static void demoEcosystem()
{
    printHeader("Nature Reality Engine — Ecosystem Overview");

    std::cout << R"(
  Component          | Status
  -------------------+------------------------------------------
  LSystem            | Operational  (Oak, Pine, Willow)
  TreeRenderer       | Operational  (Generate/Render/Wind/Season)
  WaterRenderer      | Stub ready   (Gerstner waves)
  VegetationSystem   | Stub ready   (instanced grass/shrubs)
  WeatherSystem      | Stub ready   (sky/precipitation)
  IRenderer          | Interface defined (Vulkan backend hookable)
)";

    // Show that all headers compile and are usable.
    NRE::WaterRenderer     water;
    NRE::VegetationSystem  veg;
    NRE::WeatherSystem     weather;

    water.setWaveMode(NRE::WaveMode::Choppy);
    water.setWindSpeed(30.0f);

    veg.setDensity(NRE::VegetationDensity::Dense);
    veg.setTerrainSize(200.0f, 200.0f);

    weather.setPreset(NRE::WeatherPreset::Stormy);
    weather.setPrecipitation(NRE::PrecipitationType::Rain);

    std::cout << "\n  [OK] All engine subsystems constructed successfully.\n";
}

// ---------------------------------------------------------------------------
// Demo: Vulkan renderer interface
// ---------------------------------------------------------------------------

static void demoVulkanInterface()
{
    printHeader("Vulkan Renderer Interface");
    std::cout << R"(
  IRenderer interface (engine/renderer/Renderer.h) provides:
    init(width, height, appName)         — device/swapchain creation
    beginFrame() / endFrame()            — frame lifecycle
    submitMesh(mesh, transform)          — draw call submission
    setCamera(cam)                       — view/projection setup
    setDirectionalLight(light)           — sun/moon illumination
    setAmbientIntensity(f)               — ambient term
    uploadMesh(mesh)  -> BufferHandle    — VBO/IBO upload
    updateBuffer(handle, mesh)           — dynamic update (wind)
    loadTexture(path) -> TextureHandle   — texture streaming

  A concrete VulkanRenderer can implement this interface and be
  injected into TreeRenderer::attachRenderer() without any changes
  to the tree generation or physics code.

  Example:
    VulkanRenderer vk;
    vk.init(1920, 1080, "Nature Reality Engine");
    tree.attachRenderer(&vk);
    tree.generate();

    while (running) {
        vk.beginFrame();
        tree.applyWind(dt);
        tree.render();
        vk.endFrame();
    }
    vk.shutdown();
)";
    std::cout << "  [OK] Interface documented.\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "\n";
    printHeader("Artemis L-System Tree Demo — Nature Reality Engine");
    std::cout << "\n";

    demoSpecies(NRE::TreeSpecies::Oak,    "Oak");
    std::cout << '\n';
    demoSpecies(NRE::TreeSpecies::Pine,   "Pine");
    std::cout << '\n';
    demoSpecies(NRE::TreeSpecies::Willow, "Willow");
    std::cout << '\n';

    demoEcosystem();
    std::cout << '\n';

    demoVulkanInterface();
    std::cout << '\n';

    printHeader("Demo Complete");
    std::cout << "\n  All systems operational.\n\n";
    return 0;
}
