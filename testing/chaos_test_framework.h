/**
 * @file chaos_test_framework.h
 * @brief Chaos testing framework for enhancing antifragility
 * @date 2025-12-13
 *
 * Based on the 5th version analysis, this framework introduces chaos engineering
 * principles to make XpuMusic more resilient through controlled chaos.
 */

#pragma once

#include <random>
#include <chrono>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace xpumusic {
namespace chaos {

/**
 * @brief Chaos experiment result
 */
struct ChaosExperimentResult {
    std::string experiment_name;
    bool system_survived;
    bool system_recovered;
    std::chrono::milliseconds recovery_time;
    std::string observations;
    std::vector<std::string> adaptation_responses;
    double antifragility_score; // 0.0 - 1.0
};

/**
 * @brief Chaos level configuration
 */
enum class ChaosLevel {
    GENTLE,      // 5% chance of minor disturbances
    MODERATE,    // 10% chance of moderate chaos
    INTENSE,      // 20% chance of major chaos
    EXTREME      // 50% chance of extreme chaos
};

/**
 * @brief Chaos test types
 */
enum class ChaosType {
    FILE_CORRUPTION,     // Random file corruption
    MEMORY_PRESSURE,     // Memory pressure injection
    DELAY_INJECTION,     // Random delays in execution
    RESOURCE_SCARCITY,   // Temporary resource limitation
    INTERFACE_FAILURE,   // Random interface failures
    CONFIGURATION_DRIFT,  // Configuration changes
    NETWORK_DISRUPTION   // Network failures (if applicable)
};

/**
 * @brief Chaos test base class
 */
class ChaosTest {
public:
    virtual ~ChaosTest() = default;

    virtual ChaosExperimentResult run() = 0;
    virtual std::string get_description() const = 0;
    virtual ChaosType get_type() const = 0;
    virtual double get_severity() const = 0; // 0.0 - 1.0
};

/**
 * @brief File corruption chaos test
 */
class FileCorruptionChaosTest : public ChaosTest {
private:
    std::string target_file_;
    double corruption_probability_;
    std::mt19937 rng_;

public:
    FileCorruptionChaosTest(const std::string& file, double probability = 0.1);

    ChaosExperimentResult run() override;
    std::string get_description() const override;
    ChaosType get_type() const override { return ChaosType::FILE_CORRUPTION; }
    double get_severity() const override { return 0.3; }
};

/**
 * @brief Memory pressure chaos test
 */
class MemoryPressureChaosTest : public ChaosTest {
private:
    size_t memory_size_mb_;
    std::chrono::seconds duration_;
    std::mt19937 rng_;

public:
    MemoryPressureChaosTest(size_t size_mb, std::chrono::seconds duration);

    ChaosExperimentResult run() override;
    std::string get_description() const override;
    ChaosType get_type() const override { return ChaosType::MEMORY_PRESSURE; }
    double get_severity() const override { return 0.5; }
};

/**
 * @brief Delay injection chaos test
 */
class DelayInjectionChaosTest : public ChaosTest {
private:
    std::chrono::milliseconds delay_range_;
    std::chrono::seconds test_duration_;
    std::mt19937 rng_;

public:
    DelayInjectionChaosTest(std::chrono::milliseconds max_delay,
                           std::chrono::seconds duration);

    ChaosExperimentResult run() override;
    std::string get_description() const override;
    ChaosType get_type() const override { return ChaosType::DELAY_INJECTION; }
    double get_severity() const override { return 0.2; }
};

/**
 * @brief Chaos test runner
 */
class ChaosTestRunner {
private:
    std::vector<std::unique_ptr<ChaosTest>> tests_;
    std::unordered_map<std::string, ChaosExperimentResult> history_;
    ChaosLevel current_level_;
    std::atomic<bool> running_;
    std::thread runner_thread_;

    // Metrics
    std::atomic<int> total_experiments_{0};
    std::atomic<int> successful_recoveries_{0};
    std::atomic<double> average_antifragility_{0.0};

public:
    ChaosTestRunner(ChaosLevel level = ChaosLevel::MODERATE);
    ~ChaosTestRunner();

    // Test management
    void add_test(std::unique_ptr<ChaosTest> test);
    void remove_test(const std::string& test_name);
    void clear_tests();

    // Execution control
    void start();
    void stop();
    bool is_running() const { return running_.load(); }

    // Results
    std::vector<ChaosExperimentResult> get_recent_results(int count = 10) const;
    double calculate_antifragility_score() const;
    void print_chaos_report() const;

    // Configuration
    void set_chaos_level(ChaosLevel level) { current_level_ = level; }
    ChaosLevel get_chaos_level() const { return current_level_; }

    // History management
    void save_history(const std::string& filename);
    void load_history(const std::string& filename);
};

/**
 * @brief Chaos audio generator for stress testing
 */
class ChaosAudioGenerator {
private:
    std::mt19937 rng_;
    ChaosLevel current_level_;

public:
    ChaosAudioGenerator(ChaosLevel level = ChaosLevel::MODERATE);

    // Generate chaotic audio data
    std::vector<float> generate_chaos_audio(size_t samples, int channels);

    // Apply chaos to existing audio
    void apply_chaos_to_audio(std::vector<float>& audio);

    // Create corrupted audio file for testing
    bool create_chaos_audio_file(const std::string& filename,
                                  size_t samples = 44100,
                                  int sample_rate = 44100,
                                  int channels = 2);
};

/**
 * @brief Chaos learning system
 */
class ChaosLearner {
private:
    struct FailurePattern {
        std::string type;
        std::string context;
        std::string recovery_strategy;
        double effectiveness;
        int occurrence_count;
    };

    std::vector<FailurePattern> patterns_;
    std::unordered_map<std::string, double> adaptation_weights_;

public:
    // Learning from experiments
    void record_experiment(const ChaosExperimentResult& result);

    // Get adaptation suggestions
    std::vector<std::string> get_adaptation_suggestions(const std::string& failure_type);

    // Update effectiveness
    void update_effectiveness(const std::string& strategy, double effectiveness);

    // Export/Import learning
    void export_knowledge(const std::string& filename);
    void import_knowledge(const std::string& filename);

private:
    void learn_from_failure(const ChaosExperimentResult& result);
    void update_adaptation_weights();
};

/**
 * @brief Antifragility metrics calculator
 */
class AntifragilityMetrics {
public:
    struct Metrics {
        double recovery_speed;          // How fast system recovers
        double adaptation_diversity;    // Number of different strategies
        double chaos_tolerance;        // How much chaos can be tolerated
        double evolution_rate;          // Rate of improvement over time
        double robustness_score;        // Overall robustness
    };

    // Calculate metrics from experiment history
    static Metrics calculate(const std::vector<ChaosExperimentResult>& history);

    // Print detailed metrics
    static void print_metrics(const Metrics& metrics);

    // Generate antifragility score
    static double calculate_overall_score(const Metrics& metrics);
};

/**
 * @brief Chaos testing orchestrator
 */
class ChaosOrchestrator {
private:
    ChaosTestRunner runner_;
    ChaosLearner learner_;
    ChaosAudioGenerator audio_generator_;

    // Configuration
    bool auto_discovery_;
    bool continuous_learning_;
    ChaosLevel base_level_;

public:
    ChaosOrchestrator(ChaosLevel level = ChaosLevel::MODERATE);

    // Main orchestration
    void run_chaos_session(int duration_minutes);

    // Component management
    void add_standard_tests();
    void discover_vulnerabilities();
    void adapt_based_on_learning();

    // Configuration
    void set_auto_discovery(bool enabled) { auto_discovery_ = enabled; }
    void set_continuous_learning(bool enabled) { continuous_learning_ = enabled; }
    void set_base_chaos_level(ChaosLevel level) { base_level_ = level; }

    // Results
    void generate_chaos_report();
    void export_session_data(const std::string& filename);
};

} // namespace chaos
} // namespace xpumusic

/**
 * Example usage:
 *
 * // Create chaos orchestrator
 * xpumusic::chaos::ChaosOrchestrator orchestrator(xpumusic::chaos::ChaosLevel::MODERATE);
 *
 * // Add standard tests
 * orchestrator.add_standard_tests();
 *
 * // Run chaos session for 30 minutes
 * orchestrator.run_chaos_session(30);
 *
 * // Generate report
 * orchestrator.generate_chaos_report();
 */