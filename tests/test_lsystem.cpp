/**
 * @file test_lsystem.cpp
 * @brief Unit tests for the NRE::LSystem class.
 *
 * Uses only the C++ standard library (no external test framework required).
 * Exit code 0 = all tests passed.
 */

#include "../engine/nature/LSystem.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Minimal test harness
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

static void test_oak_axiom()
{
    NRE::LSystem ls(NRE::TreeSpecies::Oak);
    // Before any iteration the axiom should be the single character 'X'.
    CHECK(ls.getString() == "X");
}

static void test_iteration_grows_string()
{
    NRE::LSystem ls(NRE::TreeSpecies::Oak);
    ls.iterate(1);
    std::string after1 = ls.getString();
    CHECK(after1.size() > 1u);   // must have grown

    // A second iteration should produce an even longer string.
    ls.iterate(1);
    CHECK(ls.getString().size() > after1.size());
}

static void test_pine_axiom()
{
    NRE::LSystem ls(NRE::TreeSpecies::Pine);
    CHECK(ls.getString() == "A");
    ls.iterate(1);
    CHECK(ls.getString().find('F') != std::string::npos);
}

static void test_willow_axiom()
{
    NRE::LSystem ls(NRE::TreeSpecies::Willow);
    CHECK(ls.getString() == "F");
    ls.iterate(1);
    CHECK(!ls.getString().empty());
}

static void test_custom_rules()
{
    NRE::LSystem ls(NRE::TreeSpecies::Custom);
    ls.setAxiom("A");
    ls.addRule('A', "AB");
    ls.addRule('B', "A");
    // Fibonacci sequence: A→AB, B→A
    ls.iterate(1); CHECK(ls.getString() == "AB");
    ls.iterate(1); CHECK(ls.getString() == "ABA");
    ls.iterate(1); CHECK(ls.getString() == "ABAAB");
}

static void test_clear_rules()
{
    NRE::LSystem ls(NRE::TreeSpecies::Oak);
    ls.clearRules();
    std::string before = ls.getString();
    ls.iterate(3);
    // With no rules the string should not change.
    CHECK(ls.getString() == before);
}

static void test_params_default_oak()
{
    NRE::LSystem ls(NRE::TreeSpecies::Oak);
    const NRE::LSystemParams& p = ls.getParams();
    CHECK(p.iterations    == 5);
    CHECK(p.angle         > 0.0f);
    CHECK(p.initialRadius > 0.0f);
    CHECK(p.radiusScale   > 0.0f && p.radiusScale < 1.0f);
}

static void test_params_mutable()
{
    NRE::LSystem ls(NRE::TreeSpecies::Oak);
    ls.getParams().angle = 45.0f;
    CHECK(ls.getParams().angle == 45.0f);
}

static void test_load_species_resets()
{
    NRE::LSystem ls(NRE::TreeSpecies::Oak);
    ls.iterate(3);
    ls.loadSpecies(NRE::TreeSpecies::Pine);
    // After reload the axiom should be fresh (A for pine).
    CHECK(ls.getString() == "A");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    test_oak_axiom();
    test_iteration_grows_string();
    test_pine_axiom();
    test_willow_axiom();
    test_custom_rules();
    test_clear_rules();
    test_params_default_oak();
    test_params_mutable();
    test_load_species_resets();

    std::cout << "LSystem tests: " << g_passed << " passed, "
              << g_failed << " failed.\n";
    return (g_failed == 0) ? 0 : 1;
}
