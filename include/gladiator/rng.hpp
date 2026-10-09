// =============================================================================
//  rng.hpp — обёртка над std::mt19937 (детерминированность симуляции)
//  Детерминированный ГПСЧ важен для эксперимента: одна и та же серия seed
//  даёт идентичный бой, что позволяет сравнивать стратегии (раздел 5 ПЗ).
// =============================================================================
#pragma once

#include <random>
#include <cstdint>

namespace glad {

class Rng {
public:
    explicit Rng(uint64_t seed = 12345ULL) : engine_(seed) {}

    void reseed(uint64_t seed) { engine_.seed(seed); }

    // Вещественное [lo, hi)
    float uniform(float lo, float hi) {
        std::uniform_real_distribution<float> d(lo, hi);
        return d(engine_);
    }

    // Целое [lo, hi]
    int uniformInt(int lo, int hi) {
        std::uniform_int_distribution<int> d(lo, hi);
        return d(engine_);
    }

    // Событие происходит с вероятностью p
    bool chance(float p) {
        return uniform(0.0f, 1.0f) < p;
    }

private:
    std::mt19937 engine_;
};

} // namespace glad
