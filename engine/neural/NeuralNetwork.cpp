#include "NeuralNetwork.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace NRE {

NeuralNetwork::NeuralNetwork(uint32_t seed) {
    if (seed == 0) {
        std::random_device rd;
        seed_ = rd();
    } else {
        seed_ = seed;
    }
    rng_.seed(seed_);
}

void NeuralNetwork::SetSeed(uint32_t seed) {
    if (seed == 0) {
        std::random_device rd;
        seed_ = rd();
    } else {
        seed_ = seed;
    }
    rng_.seed(seed_);
    InitialiseWeights();
}

uint32_t NeuralNetwork::GetSeed() const {
    return seed_;
}

void NeuralNetwork::AddLayer(int inputSize, int outputSize) {
    if (inputSize <= 0 || outputSize <= 0) {
        throw std::invalid_argument("Layer dimensions must be positive");
    }
    layers_.push_back({inputSize, outputSize,
                       std::vector<float>(outputSize * inputSize),
                       std::vector<float>(outputSize, 0.0f)});
    // He initialisation for this layer using the seeded RNG
    float stddev = std::sqrt(2.0f / static_cast<float>(inputSize));
    std::normal_distribution<float> dist(0.0f, stddev);
    Layer& layer = layers_.back();
    for (float& w : layer.weights) {
        w = dist(rng_);
    }
}

std::vector<float> NeuralNetwork::Forward(const std::vector<float>& input) const {
    std::vector<float> activation = input;
    for (const Layer& layer : layers_) {
        if (static_cast<int>(activation.size()) != layer.inputSize) {
            throw std::runtime_error("Input size does not match layer dimensions");
        }
        std::vector<float> output(layer.outputSize, 0.0f);
        for (int o = 0; o < layer.outputSize; ++o) {
            float sum = layer.biases[o];
            for (int i = 0; i < layer.inputSize; ++i) {
                sum += layer.weights[o * layer.inputSize + i] * activation[i];
            }
            // ReLU activation
            output[o] = std::max(0.0f, sum);
        }
        activation = std::move(output);
    }
    return activation;
}

void NeuralNetwork::InitialiseWeights() {
    rng_.seed(seed_);
    for (Layer& layer : layers_) {
        float stddev = std::sqrt(2.0f / static_cast<float>(layer.inputSize));
        std::normal_distribution<float> dist(0.0f, stddev);
        for (float& w : layer.weights) {
            w = dist(rng_);
        }
        std::fill(layer.biases.begin(), layer.biases.end(), 0.0f);
    }
}

} // namespace NRE
