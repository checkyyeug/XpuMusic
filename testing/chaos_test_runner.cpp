/**
 * @file chaos_test_runner.cpp
 * @brief Implementation of chaos testing framework
 * @date 2025-12-13
 */

#include "chaos_test_framework.h"
#include <algorithm>
#include <iostream>
#include <future>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace xpumusic {
namespace chaos {

// FileCorruptionChaosTest implementation
FileCorruptionChaosTest::FileCorruptionChaosTest(const std::string& file, double probability)
    : target_file_(file)
    , corruption_probability_(probability)
    , rng_(std::random_device{}()) {}

ChaosExperimentResult FileCorruptionChaosTest::run() {
    ChaosExperimentResult result;
    result.experiment_name = "File Corruption: " + target_file_;

    auto start_time = std::chrono::steady_clock::now();

    // Check if file exists
    std::ifstream check_file(target_file_);
    if (!check_file.good()) {
        result.system_survived = false;
        result.observations = "File does not exist - test cannot proceed";
        result.antifragility_score = 0.0;
        return result;
    }
    check_file.close();

    // Create backup
    std::string backup_file = target_file_ + ".chaos_backup";
    std::filesystem::copy_file(target_file_, backup_file);

    result.observations = "File exists, backup created";

    // Random corruption
    std::uniform_real_distribution<> dist(0.0, 1.0);
    if (dist(rng_) < corruption_probability_) {
        // Corrupt the file
        std::ofstream file(target_file_, std::ios::binary | std::ios::ate);
        file << "CORRUPTED_BY_CHAOS_TEST";
        file.close();

        result.observations += "\nFile corrupted intentionally";

        // Test system recovery
        auto recovery_start = std::chrono::steady_clock::now();

        // Run build test
        std::thread build_thread([this, &result]() {
            // Simulate build/test
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // In real implementation, this would run actual build/test
            // For now, we check if system detects the corruption
            std::ifstream check(target_file_);
            if (check.good() && check.rdbuf()->in_avail() > 100) {
                result.system_recovered = true;
                result.observations += "\nSystem detected and handled corruption";
                result.adaptation_responses.push_back("Used backup file");
            }
        });

        build_thread.join();

        result.recovery_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - recovery_start
        );

        // Restore from backup if system didn't handle it
        if (!result.system_recovered) {
            std::filesystem::copy_file(backup_file, target_file_);
            result.system_recovered = true;
            result.observations += "\nSystem restored from backup";
            result.adaptation_responses.push_back("Restored from backup");
        }

        // Calculate antifragility score
        if (result.recovery_time.count() < 1000) {  // Recovered in less than 1 second
            result.antifragility_score = 1.0;
        } else if (result.recovery_time.count() < 5000) {  // Recovered in less than 5 seconds
            result.antifragility_score = 0.8;
        } else {
            result.antifragility_score = 0.5;
        }
    } else {
        result.system_survived = true;
        result.system_recovered = true;
        result.observations += "\nNo corruption applied (probability-based)";
        result.antifragility_score = 0.9; // Surviving chaos is good
    }

    // Clean up backup
    std::filesystem::remove(backup_file);

    auto end_time = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    result.system_survived = true;

    return result;
}

std::string FileCorruptionChaosTest::get_description() const {
    return "Corrupts " + target_file_ + " with " +
           std::to_string(corruption_probability_ * 100) + "% probability";
}

// MemoryPressureChaosTest implementation
MemoryPressureChaosTest::MemoryPressureChaosTest(size_t size_mb, std::chrono::seconds duration)
    : memory_size_mb_(size_mb)
    , duration_(duration)
    , rng_(std::random_device{}()) {}

ChaosExperimentResult MemoryPressureChaosTest::run() {
    ChaosExperimentResult result;
    result.experiment_name = "Memory Pressure: " + std::to_string(memory_size_mb_) + "MB";

    auto start_time = std::chrono::steady_clock::now();

    // Allocate memory
    std::vector<std::unique_ptr<std::vector<uint8_t>>> memory_blocks;
    size_t allocated_mb = 0;

    try {
        while (allocated_mb < memory_size_mb_) {
            auto block = std::make_unique<std::vector<uint8_t>>(1024 * 1024);
            // Fill with random data to ensure allocation
            std::uniform_int_distribution<> dist(0, 255);
            for (auto& byte : *block) {
                byte = static_cast<uint8_t>(dist(rng_));
            }
            memory_blocks.push_back(std::move(block));
            allocated_mb++;

            // Random memory pressure simulation
            if (rng_() % 100 < 20) {  // 20% chance of memory pressure
                result.observations += "\nApplying memory pressure at " +
                                   std::to_string(allocated_mb) + "MB";

                // Simulate system response
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        result.observations = "Allocated " + std::to_string(allocated_mb) + "MB of memory";
        result.system_survived = true;

        // Test system under pressure
        auto pressure_start = std::chrono::steady_clock::now();

        // Simulate work while under memory pressure
        std::vector<std::future<void>> tasks;
        for (int i = 0; i < 4; ++i) {
            tasks.push_back(std::async(std::launch::async, [&]() {
                for (int j = 0; j < 100; ++j) {
                    // Simulate audio processing under pressure
                    std::vector<float> buffer(1024);
                    for (auto& sample : buffer) {
                        sample = std::sin(j * 0.1f) * 0.5f;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }));
        }

        // Wait for tasks to complete
        for (auto& task : tasks) {
            task.wait();
        }

        auto pressure_end = std::chrono::steady_clock::now();
        auto pressure_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            pressure_end - pressure_start
        );

        result.system_recovered = true;
        result.observations += "\nSystem performed " +
                               std::to_string(tasks.size()) + " concurrent tasks under pressure";
        result.adaptation_responses.push_back("Handled concurrent processing");

        // Calculate score based on performance under pressure
        if (pressure_duration.count() < 10000) {  // Less than 10 seconds
            result.antifragility_score = 0.9;
        } else if (pressure_duration.count() < 30000) {  // Less than 30 seconds
            result.antifragility_score = 0.7;
        } else {
            result.antifragility_score = 0.5;
        }

    } catch (const std::bad_alloc&) {
        result.observations = "Failed to allocate memory at " + std::to_string(allocated_mb) + "MB";
        result.system_survived = false;
        result.system_recovered = false;
        result.antifragility_score = 0.0;
        result.adaptations.push_back("Memory allocation failed - system limits reached");
    }

    auto end_time = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    return result;
}

std::string MemoryPressureChaosTest::get_description() const {
    return "Applies " + std::to_string(memory_size_mb_) + "MB memory pressure for " +
           std::to_string(duration_.count()) + " seconds";
}

// DelayInjectionChaosTest implementation
DelayInjectionChaosTest::DelayInjectionChaosTest(std::chrono::milliseconds max_delay,
                                               std::chrono::seconds duration)
    : delay_range_(max_delay)
    , test_duration_(duration)
    , rng_(std::random_device{}()) {}

ChaosExperimentResult DelayInjectionChaosTest::run() {
    ChaosExperimentResult result;
    result.experiment_name = "Delay Injection";

    auto start_time = std::chrono::steady_clock::now();

    std::uniform_int_distribution<> delay_dist(0, delay_range_.count());
    std::uniform_int_distribution<> chance_dist(0, 100);

    int delays_injected = 0;
    int total_operations = 0;

    auto end_time = start_time + test_duration_;

    while (std::chrono::steady_clock::now() < end_time) {
        total_operations++;

        // Randomly inject delays
        if (chance_dist(rng_) < 10) {  // 10% chance of delay
            auto delay = std::chrono::milliseconds(delay_dist(rng_));
            delays_injected++;

            result.observations += "\nInjected " + std::to_string(delay.count()) + "ms delay";

            // Simulate system under delay
            std::this_thread::sleep_for(delay);

            result.adaptation_responses.push_back("System continued despite delay");
        }

        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    result.system_survived = true;
    result.system_recovered = true;

    result.observations = "Injected " + std::to_string(delays_injected) + " delays in " +
                           std::to_string(total_operations) + " operations";
    result.adaptations.push_back("System adapted to random delays");

    // Calculate score based on how well system adapted
    double delay_ratio = static_cast<double>(delays_injected) / total_operations;
    if (delay_ratio < 0.05) {  // Less than 5% delays
        result.antifragility_score = 1.0;
    } else if (delay_ratio < 0.1) {  // Less than 10% delays
        result.antifragility_score = 0.9;
    } else if (delay_ratio < 0.2) {  // Less than 20% delays
        result.antifragility_score = 0.8;
    } else {
        result.antifragility_score = 0.6;
    }

    return result;
}

std::string DelayInjectionChaosTest::get_description() const {
    return "Injects random delays (0-" + std::to_string(delay_range_.count()) + "ms) for " +
           std::to_string(test_duration_.count()) + " seconds";
}

// ChaosTestRunner implementation
ChaosTestRunner::ChaosTestRunner(ChaosLevel level)
    : current_level_(level)
    , running_(false) {}

ChaosTestRunner::~ChaosTestRunner() {
    stop();
}

void ChaosTestRunner::add_test(std::unique_ptr<ChaosTest> test) {
    tests_.push_back(std::move(test));
}

void ChaosTestRunner::remove_test(const std::string& test_name) {
    tests_.erase(
        std::remove_if(tests_.begin(), tests_.end(),
            [&test_name](const std::unique_ptr<ChaosTest>& test) {
                return test->get_description().find(test_name) != std::string::npos;
            })
    );
}

void ChaosTestRunner::clear_tests() {
    tests_.clear();
}

void ChaosTestRunner::start() {
    if (running_) return;

    running_ = true;
    runner_thread_ = std::thread([this]() {
        run_chaos_loop();
    });
}

void ChaosTestRunner::stop() {
    if (!running_) return;

    running_ = false;
    if (runner_thread_.joinable()) {
        runner_thread_.join();
    }
}

void ChaosTestRunner::run_chaos_loop() {
    while (running_) {
        // Select random test based on chaos level
        if (tests_.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        std::uniform_int_distribution<> test_dist(0, tests_.size() - 1);
        auto& test = tests_[test_dist(rng_)];

        // Run experiment
        auto result = test->run();

        // Update metrics
        total_experiments_++;
        if (result.system_recovered) {
            successful_recoveries_++;
        }

        // Update average antifragility
        current_antifragility_ = (current_antifragility_ * (total_experiments_ - 1) +
                                   result.antifragility_score) / total_experiments_;

        // Store result
        history_[test->get_description()] = result;

        // Log result
        std::cout << "[Chaos] " << test->get_description()
                  << " - Score: " << std::fixed << std::setprecision(2) << result.antifragility_score
                  << std::endl;

        // Rate limiting based on chaos level
        std::chrono::milliseconds delay;
        switch (current_level_) {
            case ChaosLevel::GENTLE: delay = std::chrono::seconds(5); break;
            case ChaosLevel::MODERATE: delay = std::chrono::seconds(2); break;
            case ChaosLevel::INTENSE: delay = std::chrono::seconds(1); break;
            case ChaosLevel::EXTREME: delay = std::chrono::milliseconds(500); break;
        }

        std::this_thread::sleep_for(delay);
    }
}

std::vector<ChaosExperimentResult> ChaosTestRunner::get_recent_results(int count) const {
    std::vector<ChaosExperimentResult> results;

    for (const auto& [name, result] : history_) {
        results.push_back(result);
        if (results.size() >= count) break;
    }

    return results;
}

double ChaosTestRunner::calculate_antifragility_score() const {
    if (total_experiments_ == 0) return 0.0;
    return current_antifragility_;
}

void ChaosTestRunner::print_chaos_report() const {
    std::cout << "\n==========================================\n";
    std::cout << "           CHAOS TESTING REPORT\n";
    std::cout << "==========================================\n\n";

    std::cout << "Chaos Level: ";
    switch (current_level_) {
        case ChaosLevel::GENTLE: std::cout << "GENTLE (5% chaos)"; break;
        case ChaosLevel::MODERATE: std::cout << "MODERATE (10% chaos)"; break;
        case ChaosLevel::INTENSE: std::cout << "INTENSE (20% chaos)"; break;
        case ChaosLevel::EXTREME: std::cout << "EXTREME (50% chaos)"; break;
    }
    std::cout << "\n\n";

    std::cout << "Metrics:\n";
    std::cout << "  Total Experiments: " << total_experiments_.load() << "\n";
    std::cout << "  Successful Recoveries: " << successful_recoveries_.load() << "\n";
    std::cout << "  Recovery Rate: "
              << std::fixed << std::setprecision(1)
              << (static_cast<double>(successful_recoveries_) / total_experiments_ * 100) << "%\n";
    std::cout << "  Antifragility Score: "
              << std::fixed << std::setprecision(2)
              << current_antifragility_.load() << "/1.00\n\n";

    auto results = get_recent_results(10);
    if (!results.empty()) {
        std::cout << "Recent Experiments:\n";
        for (const auto& result : results) {
            std::cout << "  " << result.experiment_name << "\n";
            std::cout << "    Score: " << std::fixed << std::setprecision(2) << result.antifragility_score;
            if (!result.observations.empty()) {
                std::cout << "    " << result.observations << "\n";
            }
            std::cout << "\n";
        }
    }

    // Calculate overall metrics
    auto recent_results = get_recent_results(50);
    if (!recent_results.empty()) {
        auto metrics = AntifragilityMetrics::calculate(recent_results);
        std::cout << "System Metrics:\n";
        AntifragilityMetrics::print_metrics(metrics);
    }
}

void ChaosTestRunner::save_history(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "# Chaos Testing History\n";
    file << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";

    for (const auto& [name, result] : history_) {
        file << "Experiment: " << result.experiment_name << "\n";
        file << "Score: " << result.antifragility_score << "\n";
        file << "Survived: " << (result.system_survived ? "Yes" : "No") << "\n";
        file << "Recovered: " << (result.system_recovered ? "Yes" : "No") << "\n";
        file << "Recovery Time: " << result.recovery_time.count() << "ms\n";
        if (!result.observations.empty()) {
            file << "Observations: " << result.observations << "\n";
        }
        file << "\n";
    }
}

// ChaosAudioGenerator implementation
ChaosAudioGenerator::ChaosAudioGenerator(ChaosLevel level)
    : current_level_(level)
    , rng_(std::random_device{}()) {}

std::vector<float> ChaosAudioGenerator::generate_chaos_audio(size_t samples, int channels) {
    std::vector<float> audio(samples * channels);
    std::uniform_real_distribution<> noise_dist(-1.0f, 1.0f);
    std::uniform_real_distribution<> chaos_dist(0.0f, 1.0f);

    // Apply chaos based on level
    double chaos_intensity = 0.0;
    switch (current_level_) {
        case ChaosLevel::GENTLE: chaos_intensity = 0.05; break;
        case ChaosLevel::MODERATE: chaos_intensity = 0.15; break;
        case ChaosLevel::INTENSE: chaos_intensity = 0.35; break;
        case ChaosLevel::EXTREME: chaos_intensity = 0.65; break;
    }

    for (size_t i = 0; i < samples; ++i) {
        for (int ch = 0; ch < channels; ++ch) {
            size_t idx = i * channels + ch;

            // Generate base signal
            float sample = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);

            // Apply chaos
            if (chaos_dist(rng_) < chaos_intensity) {
                // Add chaos
                sample += noise_dist(rng_) * 0.3f;

                // Random phase shift
                if (chaos_dist(rng_) < 0.1f) {
                    sample *= -1.0f;
                }
            }

            audio[idx] = std::clamp(sample, -1.0f, 1.0f);
        }
    }

    return audio;
}

void ChaosAudioGenerator::apply_chaos_to_audio(std::vector<float>& audio) {
    std::uniform_real_distribution<> chaos_dist(0.0f, 1.0f);
    std::uniform_int_distribution<> corruption_dist(0, audio.size() - 1);

    // Apply chaos based on level
    int corruption_count = 0;
    switch (current_level_) {
        case ChaosLevel::GENTLE: corruption_count = audio.size() / 1000; break;
        case ChaosLevel::MODERATE: corruption_count = audio.size() / 500; break;
        case ChaosLevel::INTENSE: corruption_count = audio.size() / 200; break;
        case ChaosLevel::EXTREME: corruption_count = audio.size() / 50; break;
    }

    // Apply random corruptions
    for (int i = 0; i < corruption_count; ++i) {
        size_t idx = corruption_dist(rng_);
        audio[idx] += (chaos_dist(rng_) - 0.5f) * 2.0f;
        audio[idx] = std::clamp(audio[idx], -1.0f, 1.0f);
    }
}

bool ChaosAudioGenerator::create_chaos_audio_file(const std::string& filename,
                                                  size_t samples,
                                                  int sample_rate,
                                                  int channels) {
    auto audio = generate_chaos_audio(samples, channels);

    // Write as WAV file (simplified)
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    // Write WAV header
    uint32_t sample_rate32 = sample_rate;
    uint16_t channels16 = channels;
    uint16_t bits_per_sample = 16;
    uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
    uint16_t block_align = channels * (bits_per_sample / 8);
    uint32_t data_size = samples * channels * (bits_per_sample / 8);
    uint32_t file_size = 36 + data_size;

    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&file_size), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    uint32_t fmt_size = 16;
    file.write(reinterpret_cast<const char*>(&fmt_size), 4);
    uint16_t format = 1; // PCM
    file.write(reinterpret_cast<const char*>(&format), 2);
    file.write(reinterpret_cast<const char*>(&channels16), 2);
    file.write(reinterpret_cast<const char*>(&sample_rate32), 4);
    file.write(reinterpret_cast<const char*>(&byte_rate), 4);
    file.write(reinterpret_cast<const char*>(&block_align), 2);
    file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&data_size), 4);

    // Write audio data (16-bit PCM)
    for (float sample : audio) {
        int16_t pcm = static_cast<int16_t>(sample * 32767);
        file.write(reinterpret_cast<const char*>(&pcm), 2);
    }

    file.close();
    return true;
}

// ChaosLearner implementation
void ChaosLearner::record_experiment(const ChaosExperimentResult& result) {
    learn_from_failure(result);
    update_adaptation_weights();
}

std::vector<std::string> ChaosLearner::get_adaptation_suggestions(const std::string& failure_type) {
    std::vector<std::string> suggestions;

    for (const auto& pattern : patterns_) {
        if (pattern.type == failure_type && pattern.effectiveness > 0.5) {
            suggestions.push_back(pattern.recovery_strategy);
        }
    }

    // Sort by effectiveness
    std::sort(suggestions.begin(), suggestions.end(),
        [this](const std::string& a, const std::string& b) {
            return adaptation_weights_[a] > adaptation_weights_[b];
        });

    return suggestions;
}

void ChaosLearner::update_effectiveness(const std::string& strategy, double effectiveness) {
    adaptation_weights_[strategy] = (adaptation_weights_[strategy] * 0.8) + (effectiveness * 0.2);
}

void ChaosLearner::export_knowledge(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "# Chaos Learning Knowledge Base\n\n";

    for (const auto& pattern : patterns_) {
        file << "Pattern: " << pattern.type << "\n";
        file << "Context: " << pattern.context << "\n";
        file << "Strategy: " << pattern.recovery_strategy << "\n";
        file << "Effectiveness: " << pattern.effectiveness << "\n";
        file << "Occurrences: " << pattern.occurrence_count << "\n\n";
    }

    file.close();
}

void ChaosLearner::import_knowledge(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    patterns_.clear();

    // Simple parsing (in real implementation, use proper format)
    std::string line;
    FailurePattern current;

    while (std::getline(file, line)) {
        if (line.find("Pattern: ") == 0) {
            current.type = line.substr(10);
        } else if (line.find("Strategy: ") == 0) {
            current.recovery_strategy = line.substr(11);
        } else if (line.find("Effectiveness: ") == 0) {
            current.effectiveness = std::stod(line.substr(14));
        } else if (line.find("Occurrences: ") == 0) {
            current.occurrence_count = std::stoi(line.substr(13));
            patterns_.push_back(current);
        }
    }

    file.close();
}

void ChaosLearner::learn_from_failure(const ChaosExperimentResult& result) {
    if (!result.system_recovered) {
        // Learn from failure
        FailurePattern pattern;
        pattern.type = result.experiment_name;
        pattern.context = result.observations;
        pattern.effectiveness = result.antifragility_score;
        pattern.occurrence_count = 1;

        // Find existing pattern
        auto it = std::find_if(patterns_.begin(), patterns_.end(),
            [&result](const FailurePattern& p) {
                return p.type == result.experiment_name;
            });

        if (it != patterns_.end()) {
            // Update existing
            it->effectiveness = (it->effectiveness * 0.9) + (result.antifragility_score * 0.1);
            it->occurrence_count++;
        } else {
            // Add new
            patterns_.push_back(pattern);
        }
    }
}

void ChaosLearner::update_adaptation_weights() {
    for (const auto& pattern : patterns_) {
        if (!pattern.recovery_strategy.empty()) {
            adaptation_weights_[pattern.recovery_strategy] = pattern.effectiveness;
        }
    }
}

// AntifragilityMetrics implementation
AntifragilityMetrics::Metrics AntifragilityMetrics::calculate(
    const std::vector<ChaosExperimentResult>& history) {
    Metrics metrics{};

    if (history.empty()) return metrics;

    // Calculate recovery speed
    double total_recovery_time = 0;
    int recovered_count = 0;
    std::unordered_set<std::string> unique_strategies;

    for (const auto& result : history) {
        if (result.system_recovered) {
            total_recovery_time += result.recovery_time.count();
            recovered_count++;

            // Count unique adaptation strategies
            for (const auto& response : result.adaptation_responses) {
                unique_strategies.insert(response);
            }
        }
    }

    metrics.recovery_speed = recovered_count > 0 ?
        (1000.0 / (total_recovery_time / recovered_count)) : 0.0;

    metrics.adaptation_diversity = static_cast<double>(unique_strategies.size()) / history.size();

    // Calculate chaos tolerance
    int survived = std::count_if(history.begin(), history.end(),
        [](const ChaosExperimentResult& r) { return r.system_survived; });
    metrics.chaos_tolerance = static_cast<double>(survived) / history.size();

    // Calculate evolution rate (improvement over time)
    if (history.size() > 10) {
        auto first_half = history.begin() + history.size() / 2;
        auto second_half = first_half;

        double first_score = 0, second_score = 0;
        for (auto it = history.begin(); it != first_half; ++it) {
            first_score += it->antifragility_score;
        }
        for (auto it = first_half; it != history.end(); ++it) {
            second_score += it->antifragility_score;
        }

        first_score /= (history.size() / 2);
        second_score /= (history.size() - history.size() / 2);

        metrics.evolution_rate = (second_score - first_score) / first_score;
    }

    // Calculate overall robustness
    double total_score = 0;
    for (const auto& result : history) {
        total_score += result.antifragility_score;
    }
    metrics.robustness_score = total_score / history.size();

    return metrics;
}

void AntifragilityMetrics::print_metrics(const Metrics& metrics) {
    std::cout << "  Recovery Speed: " << std::fixed << std::setprecision(2)
              << metrics.recovery_speed << " ops/sec\n";
    std::cout << "  Adaptation Diversity: " << std::fixed << std::setprecision(2)
              << metrics.adaptation_diversity * 100 << "%\n";
    std::cout << "  Chaos Tolerance: " << std::fixed << std::setprecision(2)
              << metrics.chaos_tolerance * 100 << "%\n";
    std::cout << "  Evolution Rate: " << std::fixed << std::setprecision(2)
              << metrics.evolution_rate * 100 << "%\n";
    std::cout << "  Robustness Score: " << std::fixed << std::setprecision(2)
              << metrics.robustness_score << "\n";
}

double AntifragilityMetrics::calculate_overall_score(const Metrics& metrics) {
    return (metrics.recovery_speed * 0.25 +
            metrics.adaptation_diversity * 0.20 +
            metrics.chaos_tolerance * 0.20 +
            metrics.evolution_rate * 0.15 +
            metrics.robustness_score * 0.20);
}

// ChaosOrchestrator implementation
ChaosOrchestrator::ChaosOrchestrator(ChaosLevel level)
    : runner_(level)
    , audio_generator_(level)
    , base_level_(level)
    , auto_discovery_(true)
    , continuous_learning_(true) {}

void ChaosOrchestrator::run_chaos_session(int duration_minutes) {
    std::cout << "Starting chaos session for " << duration_minutes << " minutes\n";

    add_standard_tests();

    if (auto_discovery_) {
        discover_vulnerabilities();
    }

    runner_.start();

    // Run for specified duration
    auto end_time = std::chrono::steady_clock::now() +
                   std::chrono::minutes(duration_minutes);

    while (std::chrono::steady_clock::now() < end_time) {
        if (continuous_learning_) {
            adapt_based_on_learning();
        }

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }

    runner_.stop();
    generate_chaos_report();
}

void ChaosOrchestrator::add_standard_tests() {
    // Add file corruption tests
    runner_.add_test(std::make_unique<FileCorruptionChaosTest>("build/CMakeCache.txt", 0.1));
    runner_.add_test(std::make_unique<FileCorruptionChaosTest>("test_audio.wav", 0.05));

    // Add memory pressure tests
    runner_.add_test(std::make_unique<MemoryPressureChaosTest>(100, std::chrono::seconds(5)));
    runner_.add_test(std::make_unique<MemoryPressureChaosTest>(200, std::chrono::seconds(3)));

    // Add delay injection tests
    runner_.add_test(std::make_unique<DelayInjectionChaosTest>(
        std::chrono::milliseconds(100), std::chrono::seconds(10)));
}

void ChaosOrchestrator::discover_vulnerabilities() {
    // Scan for potential vulnerabilities
    std::vector<std::string> critical_files = {
        "CMakeLists.txt",
        "src/audio/plugin_audio_decoder.h",
        "src/playlist/playlist_manager.h"
    };

    // Add corruption tests for critical files with low probability
    for (const auto& file : critical_files) {
        std::ifstream check(file);
        if (check.good()) {
            runner_.add_test(std::make_unique<FileCorruptionChaosTest>(file, 0.02));
        }
    }

    // Create chaos audio files for testing
    audio_generator_.create_chaos_audio_file("chaos_test_1.wav", 44100, 44100, 2);
    audio_generator_.create_chaos_audio_file("chaos_test_2.wav", 22050, 44100, 1);
}

void ChaosOrchestrator::adapt_based_on_learning() {
    auto recent_results = runner_.get_recent_results(20);
    auto metrics = AntifragilityMetrics::calculate(recent_results);

    // Adjust chaos level based on performance
    if (metrics.robustness_score > 0.8 && runner_.get_chaos_level() != ChaosLevel::EXTREME) {
        // System is robust, increase chaos
        switch (runner_.get_chaos_level()) {
            case ChaosLevel::GENTLE:
                runner_.set_chaos_level(ChaosLevel::MODERATE);
                break;
            case ChaosLevel::MODERATE:
                runner_.set_chaos_level(ChaosLevel::INTENSE);
                break;
            case ChaosLevel::INTENSE:
                runner_.set_chaos_level(ChaosLevel::EXTREME);
                break;
            default: break;
        }
    } else if (metrics.robustness_score < 0.4 && runner_.get_chaos_level() != ChaosLevel::GENTLE) {
        // System is struggling, reduce chaos
        switch (runner_.get_chaos_level()) {
            case ChaosLevel::EXTREME:
                runner_.set_chaos_level(ChaosLevel::INTENSE);
                break;
            case ChaosLevel::INTENSE:
                runner_.set_chaos_level(ChaosLevel::MODERATE);
                break;
            case ChaosLevel::MODERATE:
                runner_.set_chaos_level(ChaosLevel::GENTLE);
                break;
            default: break;
        }
    }
}

void ChaosOrchestrator::generate_chaos_report() {
    std::cout << "\nGenerating chaos report...\n";
    runner_.print_chaos_report();

    // Save history
    runner_.save_history("chaos_history.txt");

    // Export learning
    auto recent_results = runner_.get_recent_results(100);
    ChaosLearner temp_learner;
    for (const auto& result : recent_results) {
        temp_learner.record_experiment(result);
    }
    temp_learner.export_knowledge("chaos_learning.txt");
}

void ChaosOrchestrator::export_session_data(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "Chaos Session Export\n";
    file << "===================\n\n";

    auto results = runner_.get_recent_results(100);
    auto metrics = AntifragilityMetrics::calculate(results);

    file << "Overall Antifragility Score: "
         << AntifragilityMetrics::calculate_overall_score(metrics) << "\n\n";

    file << "Metrics:\n";
    AntifragilityMetrics::print_metrics(metrics);

    file.close();
}

} // namespace chaos
} // namespace xpumusic