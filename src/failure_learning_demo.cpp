/**
 * @file failure_learning_demo.cpp
 * @brief Demonstration of failure learning and adaptation system
 * @date 2025-12-13
 */

#include "failure_learning_system.h"
#include <iostream>
#include <thread>
#include <random>

using namespace xpumusic::learning;

int main() {
    std::cout << "=== XpuMusic Failure Learning System Demo ===\n\n";

    // Initialize the autonomous adaptation system
    AutonomousAdaptationSystem adaptation_system;
    adaptation_system.initialize();

    std::cout << "Failure Learning System initialized\n\n";

    // Register adaptation callbacks
    adaptation_system.register_adaptation_callback("memory_exhaustion",
        [](const std::string& strategy) {
            std::cout << "  [ADAPT] Memory exhaustion detected! Applying: " << strategy << "\n";
        });

    adaptation_system.register_adaptation_callback("audio_dropout",
        [](const std::string& strategy) {
            std::cout << "  [ADAPT] Audio dropout! Applying: " << strategy << "\n";
        });

    adaptation_system.register_adaptation_callback("file_corruption",
        [](const std::string& strategy) {
            std::cout << "  [ADAPT] File corruption! Applying: " << strategy << "\n";
        });

    // Simulate a series of failures
    std::cout << "=== Simulating Failure Scenarios ===\n\n";

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> failure_dist(0, 2);

    const char* failure_types[] = {"memory_exhaustion", "audio_dropout", "file_corruption"};
    const char* contexts[] = {"audio_playback", "file_loading", "buffer_management"};

    for (int i = 0; i < 20; ++i) {
        int type_idx = failure_dist(rng);
        int ctx_idx = failure_dist(rng) % 3;

        std::string type = failure_types[type_idx];
        std::string context = contexts[ctx_idx];

        std::cout << "Scenario " << (i + 1) << ": " << type << " in " << context << "\n";

        // Handle failure with adaptation
        adaptation_system.handle_failure(type, context, [type]() -> bool {
            // Simulate recovery success based on type
            std::mt19937 rng(std::random_device{}());
            std::uniform_real_distribution<> success_dist(0.0, 1.0);

            double success_chance = 0.5;
            if (type == "memory_exhaustion") success_chance = 0.7;
            else if (type == "audio_dropout") success_chance = 0.8;
            else if (type == "file_corruption") success_chance = 0.6;

            bool success = success_dist(rng) < success_chance;
            std::cout << "  Recovery " << (success ? "succeeded" : "failed") << "\n";
            return success;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n=== Learning and Pattern Recognition ===\n";
    adaptation_system.periodic_learning_cycle();

    // Print knowledge summary
    adaptation_system.print_knowledge_summary();

    // Demonstrate prediction
    std::cout << "\n=== Failure Prediction Demo ===\n";

    std::unordered_map<std::string, double> metrics = {
        {"memory_usage", 0.85},
        {"cpu_usage", 0.60},
        {"io_errors", 0.10},
        {"buffer_underruns", 0.15}
    };

    for (const auto& [metric, value] : metrics) {
        std::cout << metric << ": " << value * 100 << "%\n";
    }

    std::cout << "\nAnalyzing metrics for failure prediction...\n";

    std::string predicted_failure;
    if (adaptation_system.predict_and_prevent(metrics)) {
        std::cout << "⚠️  Potential failure predicted and prevented!\n";
    } else {
        std::cout << "✅ No immediate failure risk detected\n";
    }

    // Simulate more complex failure scenario
    std::cout << "\n=== Advanced Adaptation Scenario ===\n";

    // Create a recurring failure pattern
    for (int i = 0; i < 10; ++i) {
        std::cout << "Repeated memory pressure in audio playback...\n";
        adaptation_system.handle_failure(
            "memory_exhaustion",
            "audio_playback",
            []() -> bool {
                // Initially fail, then learn to succeed
                static int attempt = 0;
                bool success = attempt++ > 3;
                std::cout << "  Recovery: " << (success ? "Success (learned!)" : "Failed") << "\n";
                return success;
            }
        );
    }

    // Update patterns and show improvement
    adaptation_system.periodic_learning_cycle();

    std::cout << "\n=== Final System State ===\n";
    std::cout << "Total Adaptations: " << adaptation_system.get_adaptations_applied() << "\n";
    std::cout << "Successful Adaptations: " << adaptation_system.get_successful_adaptations() << "\n";
    std::cout << "Success Rate: "
              << std::fixed << std::setprecision(1)
              << adaptation_system.get_adaptation_success_rate() * 100 << "%\n";

    // Save knowledge for future sessions
    if (adaptation_system.save_knowledge("demo_knowledge.dat")) {
        std::cout << "\n✓ Knowledge saved to demo_knowledge.dat\n";
    }

    std::cout << "\n=== Learning Demonstration Complete ===\n";
    std::cout << "The system has learned from failures and improved its\n";
    std::cout << "recovery strategies. This demonstrates antifragility:\n";
    std::cout << "the system becomes stronger through stress and failures.\n";

    // Cleanup
    FailureLearningManager::shutdown();

    return 0;
}