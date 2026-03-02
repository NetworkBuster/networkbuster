#include <cstdint>
#include <iostream>
#include <vector>

#include "../../engine/neural/NeuralNetwork.h"

// Demonstrates seeded neural network for reproducible procedural generation.
// Running with the same seed always produces identical outputs.
int main() {
    const uint32_t seed = 42;

    // Build a small network: 4 inputs -> 8 hidden -> 4 outputs
    NRE::NeuralNetwork nn(seed);
    nn.AddLayer(4, 8);
    nn.AddLayer(8, 4);

    std::vector<float> input = {0.5f, 0.3f, 0.8f, 0.1f};

    std::cout << "Seed: " << nn.GetSeed() << "\n";
    std::cout << "Forward pass output:\n  [";
    auto output = nn.Forward(input);
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << output[i];
        if (i + 1 < output.size()) std::cout << ", ";
    }
    std::cout << "]\n";

    // Verify reproducibility: rebuild with the same seed
    NRE::NeuralNetwork nn2(seed);
    nn2.AddLayer(4, 8);
    nn2.AddLayer(8, 4);
    auto output2 = nn2.Forward(input);

    bool reproducible = (output == output2);
    std::cout << "Reproducible with same seed: " << (reproducible ? "YES" : "NO") << "\n";

    // Show that a different seed produces different weights
    NRE::NeuralNetwork nn3(seed + 1);
    nn3.AddLayer(4, 8);
    nn3.AddLayer(8, 4);
    auto output3 = nn3.Forward(input);
    std::cout << "Different seed produces different output: "
              << (output != output3 ? "YES" : "NO") << "\n";

    return 0;
}
