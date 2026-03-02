#include "LSystem.h"

#include <cmath>
#include <stdexcept>

namespace NRE {

// ---------------------------------------------------------------------------
// Construction / species loading
// ---------------------------------------------------------------------------

LSystem::LSystem(TreeSpecies species)
{
    loadSpecies(species);
}

void LSystem::loadSpecies(TreeSpecies species)
{
    m_rules.clear();
    switch (species) {
        case TreeSpecies::Oak:    loadOak();    break;
        case TreeSpecies::Pine:   loadPine();   break;
        case TreeSpecies::Willow: loadWillow(); break;
        case TreeSpecies::Custom: /* leave axiom/rules to caller */ break;
    }
}

void LSystem::loadOak()
{
    // Classic stochastic branching for a broad deciduous canopy.
    m_current              = "X";
    m_params.iterations    = 5;
    m_params.segmentLen    = 1.2f;
    m_params.segmentScale  = 0.82f;
    m_params.angle         = 25.7f;
    m_params.initialRadius = 0.18f;
    m_params.radiusScale   = 0.72f;

    m_rules['X'] = "F+[[X]-X]-F[-FX]+X";
    m_rules['F'] = "FF";
}

void LSystem::loadPine()
{
    // Conical evergreen with tight upward branching.
    m_current              = "A";
    m_params.iterations    = 6;
    m_params.segmentLen    = 1.0f;
    m_params.segmentScale  = 0.90f;
    m_params.angle         = 22.5f;
    m_params.initialRadius = 0.14f;
    m_params.radiusScale   = 0.78f;

    m_rules['A'] = "F[+AL][-AL]FA";
    m_rules['F'] = "FF";
    m_rules['L'] = "L";   // leaf marker — kept for turtle interpretation
}

void LSystem::loadWillow()
{
    // Drooping, weeping form.
    m_current              = "F";
    m_params.iterations    = 4;
    m_params.segmentLen    = 1.5f;
    m_params.segmentScale  = 0.88f;
    m_params.angle         = 35.0f;
    m_params.initialRadius = 0.20f;
    m_params.radiusScale   = 0.70f;

    m_rules['F'] = "FF-[-F+F+F]+[+F-F-F]";
}

// ---------------------------------------------------------------------------
// Manual configuration
// ---------------------------------------------------------------------------

void LSystem::setAxiom(const std::string& axiom)
{
    m_current = axiom;
}

void LSystem::addRule(char predecessor, const std::string& successor)
{
    m_rules[predecessor] = successor;
}

void LSystem::clearRules()
{
    m_rules.clear();
}

// ---------------------------------------------------------------------------
// Rewriting
// ---------------------------------------------------------------------------

std::string LSystem::applyRules(const std::string& input) const
{
    std::string result;
    result.reserve(input.size() * 4);
    for (char c : input) {
        auto it = m_rules.find(c);
        if (it != m_rules.end()) {
            result += it->second;
        } else {
            result += c;
        }
    }
    return result;
}

void LSystem::iterate(int steps)
{
    for (int i = 0; i < steps; ++i) {
        m_current = applyRules(m_current);
    }
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

const std::string& LSystem::getString() const
{
    return m_current;
}

const LSystemParams& LSystem::getParams() const
{
    return m_params;
}

LSystemParams& LSystem::getParams()
{
    return m_params;
}

} // namespace NRE
