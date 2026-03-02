#pragma once

#include <cstdint>
#include <random>
#include <vector>

namespace NRE {

// A fully-connected feedforward neural network with seeded weight initialization
// for reproducible results in procedural generation.
class NeuralNetwork {
public:
    // Construct with an optional seed (default 0 = use std::random_device)
    explicit NeuralNetwork(uint32_t seed = 0);

    // Set (or reset) the RNG seed and reinitialise all weights
    void SetSeed(uint32_t seed);

    // Return the seed currently in use
    uint32_t GetSeed() const;

    // Append a fully-connected layer (inputSize x outputSize weights + bias)
    void AddLayer(int inputSize, int outputSize);

    // Run a forward pass and return the output activations
    std::vector<float> Forward(const std::vector<float>& input) const;

private:
    void InitialiseWeights();

    struct Layer {
        int inputSize;
        int outputSize;
        std::vector<float> weights; // row-major [outputSize][inputSize]
        std::vector<float> biases;  // [outputSize]
    };

    uint32_t seed_;
    std::mt19937 rng_;
    std::vector<Layer> layers_;
};

} // namespace NRE
