/**
 * @file diversity_demo.cpp
 * @brief Demonstration of diversity enhancement strategies
 * @date 2025-12-13
 */

#include "diversity_strategies.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>

using namespace xpumusic::diversity;

int main() {
    std::cout << "=== XpuMusic Diversity Strategies Demo ===\n\n";

    // Initialize diversity coordinator
    DiversityCoordinator coordinator;
    std::cout << "Diversity Coordinator initialized with multiple strategies.\n";

    // Show diversity score
    double diversity_score = coordinator.calculate_diversity_score();
    std::cout << "Initial Diversity Score: " << std::fixed << std::setprecision(2) << diversity_score << "\n\n";

    // Test resampling strategies
    std::cout << "=== Resampling Strategies Test ===\n";

    std::vector<float> test_samples = {0.1f, 0.5f, 0.9f, 1.0f, -0.5f};
    float ratio = 1.25f;

    for (int i = 0; i < 10; ++i) {
        auto strategy = coordinator.get_resampling_strategy();
        if (strategy) {
            std::cout << "Using " << strategy->get_name() << " strategy (Quality: "
                      << strategy->get_quality_score() << ")\n";

            for (float sample : test_samples) {
                float result = strategy->resample_sample(sample, ratio);
                std::cout << "  " << sample << " -> " << result << "\n";
            }
            std::cout << "\n";
        }
    }

    // Test memory strategies
    std::cout << "=== Memory Strategies Test ===\n";

    std::vector<void*> allocations;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> size_dist(16, 4096);

    for (int i = 0; i < 5; ++i) {
        auto strategy = coordinator.get_memory_strategy();
        if (strategy) {
            std::cout << "Using " << strategy->get_name() << " memory strategy\n";

            // Allocate some memory
            for (int j = 0; j < 3; ++j) {
                size_t size = size_dist(rng);
                void* ptr = strategy->allocate(size);
                if (ptr) {
                    allocations.push_back(ptr);
                    std::cout << "  Allocated " << size << " bytes at " << ptr << "\n";
                }
            }

            std::cout << "Total allocated: " << strategy->get_allocated_bytes() << " bytes\n\n";
        }
    }

    // Deallocate memory
    auto memory_strategy = coordinator.get_memory_strategy();
    for (void* ptr : allocations) {
        if (memory_strategy) {
            memory_strategy->deallocate(ptr);
        }
    }

    // Test error handling strategies
    std::cout << "=== Error Handling Strategies Test ===\n";

    std::vector<int> error_codes = {1, 2, 3, 0}; // 0 = no error

    for (int error_code : error_codes) {
        auto strategy = coordinator.get_error_strategy();
        if (strategy) {
            std::cout << "Handling error " << error_code << " with " << strategy->get_name() << " strategy\n";

            auto start = std::chrono::steady_clock::now();
            bool handled = strategy->handle_error(error_code, "Test context");
            auto end = std::chrono::steady_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "  " << (handled ? "Handled" : "Failed") << " in " << duration.count() << "ms\n";
        }
    }

    // Simulate strategy failure and fallback
    std::cout << "\n=== Strategy Failure Simulation ===\n";

    // Disable diversity to see fallback behavior
    coordinator.enable_diversity(false);
    std::cout << "Diversity disabled - using primary strategies only\n";

    for (int i = 0; i < 3; ++i) {
        auto strategy = coordinator.get_resampling_strategy();
        if (strategy) {
            std::cout << "Consistently using: " << strategy->get_name() << "\n";
        }
    }

    // Re-enable diversity
    coordinator.enable_diversity(true);
    std::cout << "\nDiversity re-enabled - using multiple strategies\n";

    // Calculate final metrics
    std::cout << "\n=== Final Metrics ===\n";
    std::cout << "Diversity Score: " << coordinator.calculate_diversity_score() << "\n";
    std::cout << "Strategy Switches: " << coordinator.get_strategy_switches() << "\n";

    std::cout << "\nDiversity strategies demonstration completed!\n";
    std::cout << "This shows how having multiple implementations of the same interface\n";
    std::cout << "improves system resilience and antifragility.\n";

    return 0;
}