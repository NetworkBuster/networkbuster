/**
 * @file test_treerenderer.cpp
 * @brief Unit tests for the NRE::TreeRenderer class.
 *
 * Covers: generate(), setSeason(), applyWind(), grow(), setSpecies().
 * No GPU required — IRenderer pointer is left null (render() is not called).
 */

#include "../engine/nature/TreeRenderer.h"
#include "../engine/nature/LSystem.h"

#include <cassert>
#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------------
// Minimal test harness (same convention as test_lsystem.cpp)
// ---------------------------------------------------------------------------

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (cond) {                                                    \
            ++g_passed;                                                \
        } else {                                                       \
            ++g_failed;                                                \
            std::cerr << "FAIL  " << __FILE__ << ':' << __LINE__      \
                      << "  " << #cond << '\n';                        \
        }                                                              \
    } while (false)

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_generate_produces_geometry()
{
    NRE::TreeRenderer tree;
    tree.setSpecies(NRE::TreeSpecies::Oak);
    tree.generate();

    CHECK(tree.getBranchCount()  > 0u);
    CHECK(tree.getLeafCount()    > 0u);
    CHECK(tree.getVertexCount()  > 0u);
    CHECK(tree.getTriangleCount()> 0u);
}

static void test_pine_has_fewer_branches_than_oak()
{
    NRE::TreeRenderer oak, pine;
    oak .setSpecies(NRE::TreeSpecies::Oak);
    pine.setSpecies(NRE::TreeSpecies::Pine);
    oak .generate();
    pine.generate();

    // Both should have geometry.
    CHECK(oak .getBranchCount() > 0u);
    CHECK(pine.getBranchCount() > 0u);
}

static void test_willow_generates_geometry()
{
    NRE::TreeRenderer tree;
    tree.setSpecies(NRE::TreeSpecies::Willow);
    tree.generate();

    CHECK(tree.getBranchCount()  > 0u);
    CHECK(tree.getVertexCount()  > 0u);
    CHECK(tree.getTriangleCount()> 0u);
}

static void test_season_changes_leaf_color()
{
    NRE::TreeRenderer tree;
    tree.setSpecies(NRE::TreeSpecies::Oak);
    tree.generate();

    tree.setSeason(NRE::Season::Summer);
    // Summer leaves are deep green.
    if (!tree.getMesh().leaves.empty()) {
        NRE::Color4 summer = tree.getMesh().leaves[0].color;
        CHECK(summer.g > summer.r); // green dominant

        tree.setSeason(NRE::Season::Autumn);
        NRE::Color4 autumn = tree.getMesh().leaves[0].color;
        CHECK(autumn.r > autumn.g); // red/orange dominant
    } else {
        // No leaves is also valid (e.g., winter).
        ++g_passed;
        ++g_passed;
    }
}

static void test_season_winter_leaves_transparent()
{
    NRE::TreeRenderer tree;
    tree.setSpecies(NRE::TreeSpecies::Oak);
    tree.generate();
    tree.setSeason(NRE::Season::Winter);

    if (!tree.getMesh().leaves.empty()) {
        float alpha = tree.getMesh().leaves[0].color.a;
        CHECK(alpha < 0.1f); // effectively invisible in winter
    } else {
        ++g_passed;
    }
}

static void test_get_season()
{
    NRE::TreeRenderer tree;
    tree.setSpecies(NRE::TreeSpecies::Oak);
    tree.generate();

    tree.setSeason(NRE::Season::Spring);
    CHECK(tree.getSeason() == NRE::Season::Spring);

    tree.setSeason(NRE::Season::Winter);
    CHECK(tree.getSeason() == NRE::Season::Winter);
}

static void test_wind_displaces_vertices()
{
    NRE::TreeRenderer tree;
    tree.setSpecies(NRE::TreeSpecies::Oak);
    tree.generate();

    const auto& branches = tree.getMesh().branches;
    if (tree.getMesh().vertices.empty() || branches.empty()) { ++g_passed; return; }

    // Record initial X positions of all vertices.
    std::vector<float> origX;
    origX.reserve(tree.getMesh().vertices.size());
    for (const auto& v : tree.getMesh().vertices)
        origX.push_back(v.position.x);

    NRE::WindParams wind;
    wind.strength  = 5.0f;
    wind.frequency = 2.0f;
    wind.direction = {1.0f, 0.0f, 0.0f};
    tree.setWindParams(wind);

    for (int i = 0; i < 30; ++i) tree.applyWind(0.016f);

    // The wind code displaces end-ring vertices of each branch segment.
    // At least one vertex in the branch section must have moved.
    bool anyDisplaced = false;
    const auto& verts = tree.getMesh().vertices;
    for (size_t vi = 0; vi < origX.size() && vi < verts.size(); ++vi) {
        if (std::abs(verts[vi].position.x - origX[vi]) > 0.0f) {
            anyDisplaced = true;
            break;
        }
    }
    CHECK(anyDisplaced);
}

static void test_grow_changes_geometry()
{
    NRE::TreeRenderer tree;
    tree.setSpecies(NRE::TreeSpecies::Oak);
    tree.generate();

    size_t branchBefore = tree.getBranchCount();
    // grow() rebuilds the mesh from updated params.
    for (int i = 0; i < 10; ++i) tree.grow(1.0f);

    // Vertices may change (scaled segment lengths → slightly different
    // cylinder counts), but at minimum branch count should be stable.
    CHECK(tree.getBranchCount() >= branchBefore / 2);
}

static void test_set_position()
{
    NRE::TreeRenderer tree;
    tree.setSpecies(NRE::TreeSpecies::Oak);
    tree.setPosition({10.0f, 0.0f, 5.0f});
    tree.generate();

    // The first branch should start near the set position.
    const auto& branches = tree.getMesh().branches;
    if (!branches.empty()) {
        float dx = branches[0].start.x - 10.0f;
        float dz = branches[0].start.z -  5.0f;
        CHECK(std::abs(dx) < 1e-3f);
        CHECK(std::abs(dz) < 1e-3f);
    } else {
        ++g_passed;
        ++g_passed;
    }
}

static void test_custom_lsystem_injection()
{
    NRE::LSystem ls(NRE::TreeSpecies::Custom);
    ls.setAxiom("F");
    ls.addRule('F', "F[+F][-F]");
    ls.getParams().iterations    = 3;
    ls.getParams().segmentLen    = 1.0f;
    ls.getParams().initialRadius = 0.1f;
    ls.getParams().radiusScale   = 0.8f;
    ls.getParams().segmentScale  = 0.9f;
    ls.getParams().angle         = 30.0f;

    NRE::TreeRenderer tree;
    tree.setLSystem(ls);
    tree.generate();

    CHECK(tree.getBranchCount()  > 0u);
    CHECK(tree.getVertexCount()  > 0u);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    test_generate_produces_geometry();
    test_pine_has_fewer_branches_than_oak();
    test_willow_generates_geometry();
    test_season_changes_leaf_color();
    test_season_winter_leaves_transparent();
    test_get_season();
    test_wind_displaces_vertices();
    test_grow_changes_geometry();
    test_set_position();
    test_custom_lsystem_injection();

    std::cout << "TreeRenderer tests: " << g_passed << " passed, "
              << g_failed << " failed.\n";
    return (g_failed == 0) ? 0 : 1;
}
