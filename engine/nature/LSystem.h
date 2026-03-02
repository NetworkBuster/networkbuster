#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace NRE {

/// A single L-system production rule: predecessor symbol → successor string.
struct LSystemRule {
    char predecessor;
    std::string successor;
};

/// Parameters that control L-system tree shape.
struct LSystemParams {
    int   iterations   = 4;       ///< Number of rewriting steps
    float segmentLen   = 1.0f;    ///< Length of each 'F' forward step (world units)
    float segmentScale = 0.85f;   ///< Scale factor applied per level
    float angle        = 25.7f;   ///< Default branching angle (degrees)
    float initialRadius= 0.15f;   ///< Trunk radius (world units)
    float radiusScale  = 0.75f;   ///< Radius reduction per depth level
};

/// Predefined species presets.
enum class TreeSpecies {
    Oak,
    Pine,
    Willow,
    Custom
};

/**
 * @brief Lindenmayer-system string rewriter.
 *
 * Stores an axiom, a set of production rules, and supports
 * iterative application of those rules to produce the final
 * L-system string used by TreeRenderer for geometry generation.
 */
class LSystem {
public:
    explicit LSystem(TreeSpecies species = TreeSpecies::Oak);

    /// Replace all rules and axiom with the given preset.
    void loadSpecies(TreeSpecies species);

    /// Set a custom axiom string.
    void setAxiom(const std::string& axiom);

    /// Add or replace a production rule.
    void addRule(char predecessor, const std::string& successor);

    /// Remove all production rules.
    void clearRules();

    /// Run @p steps iterations of rewriting starting from the axiom.
    void iterate(int steps);

    /// Return the current (possibly post-iteration) L-system string.
    const std::string& getString() const;

    /// Return the parameters used by this L-system.
    const LSystemParams& getParams() const;

    /// Modify the parameters.
    LSystemParams& getParams();

private:
    std::string                          m_current;
    std::unordered_map<char,std::string> m_rules;
    LSystemParams                        m_params;

    std::string applyRules(const std::string& input) const;
    void        loadOak();
    void        loadPine();
    void        loadWillow();
};

} // namespace NRE
