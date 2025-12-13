/**
 * @file failure_learning_system.h
 * @brief Failure learning and adaptation system
 * @date 2025-12-13
 *
 * System that learns from failures and adapts to prevent them in the future.
 * Implements antifragility through continuous learning from chaos.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <fstream>
#include <mutex>
#include <atomic>

namespace xpumusic {
namespace learning {

/**
 * @brief Failure event record
 */
struct FailureEvent {
    std::string type;                    // Type of failure (e.g., "memory_exhaustion")
    std::string context;                 // Context where failure occurred
    std::string component;               // Component that failed
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;  // Additional data
    bool was_handled;                    // Whether system recovered
    double recovery_time_ms;             // Time to recover
    std::string recovery_strategy;       // Strategy used for recovery
};

/**
 * @brief Failure pattern
 */
struct FailurePattern {
    std::string pattern_id;
    std::string type;
    std::vector<std::string> triggers;   // Conditions that trigger this pattern
    std::vector<std::string> contexts;   // Common contexts
    int occurrence_count;
    double success_rate;                 // Recovery success rate
    double avg_recovery_time;
    std::string best_recovery_strategy;
    std::chrono::system_clock::time_point last_seen;
    bool is_recurring;
};

/**
 * @brief Adaptation rule
 */
struct AdaptationRule {
    std::string rule_id;
    std::string condition;               // When this rule applies
    std::vector<std::string> actions;    // Actions to take
    double effectiveness;                // How effective this rule is
    int usage_count;
    std::chrono::system_clock::time_point created;
    bool enabled;
};

/**
 * @brief Knowledge base for failures
 */
class FailureKnowledgeBase {
private:
    std::vector<FailureEvent> events_;
    std::unordered_map<std::string, FailurePattern> patterns_;
    std::vector<AdaptationRule> rules_;
    mutable std::mutex knowledge_mutex_;

    // Learning parameters
    double pattern_threshold_ = 3;      // Occurrences before pattern is recognized
    double success_rate_threshold_ = 0.7; // Minimum success rate for strategy to be good

public:
    // Event management
    void record_event(const FailureEvent& event);
    std::vector<FailureEvent> get_events(const std::string& type = "") const;
    std::vector<FailureEvent> get_recent_events(int count = 100) const;

    // Pattern management
    void update_patterns();
    std::vector<FailurePattern> get_patterns() const;
    FailurePattern* get_pattern(const std::string& pattern_id);
    bool has_pattern(const std::string& type) const;

    // Rule management
    void add_rule(const AdaptationRule& rule);
    void update_rule_effectiveness(const std::string& rule_id, bool success);
    std::vector<AdaptationRule> get_applicable_rules(const std::string& type,
                                                     const std::string& context) const;

    // Knowledge persistence
    bool save_to_file(const std::string& filename) const;
    bool load_from_file(const std::string& filename);

    // Analytics
    std::unordered_map<std::string, int> get_failure_statistics() const;
    std::vector<std::string> get_top_failure_types(int count = 10) const;
    double calculate_system_resilience() const;
};

/**
 * @brief Adaptive strategy selector
 */
class AdaptiveStrategySelector {
private:
    std::unordered_map<std::string, std::vector<std::string>> strategy_map_;
    std::unordered_map<std::string, double> strategy_success_rates_;
    FailureKnowledgeBase* knowledge_base_;

public:
    AdaptiveStrategySelector(FailureKnowledgeBase* kb) : knowledge_base_(kb) {}

    // Strategy management
    void register_strategy(const std::string& failure_type, const std::string& strategy);
    void update_strategy_performance(const std::string& strategy, bool success);

    // Strategy selection
    std::string select_strategy(const std::string& failure_type,
                               const std::string& context) const;
    std::vector<std::string> get_strategies(const std::string& failure_type) const;
};

/**
 * @brief Failure predictor
 */
class FailurePredictor {
private:
    struct PredictionModel {
        std::string failure_type;
        std::vector<std::string> indicators;
        double threshold;
        double accuracy;
        int true_positives;
        int false_positives;
        int false_negatives;
    };

    std::vector<PredictionModel> models_;
    FailureKnowledgeBase* knowledge_base_;

public:
    FailurePredictor(FailureKnowledgeBase* kb) : knowledge_base_(kb) {}

    // Model management
    void train_models();
    bool predict_failure(const std::unordered_map<std::string, double>& indicators,
                        std::string& predicted_type) const;

    // Model evaluation
    double get_model_accuracy(const std::string& failure_type) const;
    void update_model_performance(const std::string& failure_type,
                                 bool predicted, bool actual);
};

/**
 * @brief Autonomous adaptation system
 */
class AutonomousAdaptationSystem {
private:
    std::unique_ptr<FailureKnowledgeBase> knowledge_base_;
    std::unique_ptr<AdaptiveStrategySelector> strategy_selector_;
    std::unique_ptr<FailurePredictor> predictor_;

    std::atomic<bool> adaptation_enabled_{true};
    std::atomic<int> adaptations_applied_{0};
    std::atomic<int> successful_adaptations_{0};

    // Adaptation callbacks
    std::unordered_map<std::string, std::function<void(const std::string&)>> adaptation_callbacks_;

public:
    AutonomousAdaptationSystem();

    // Core functionality
    void initialize();
    void learn_from_failure(const FailureEvent& event);
    void handle_failure(const std::string& type, const std::string& context,
                       std::function<bool()> recovery_action);

    // Prediction and prevention
    bool predict_and_prevent(const std::unordered_map<std::string, double>& system_metrics);
    void update_metrics(const std::unordered_map<std::string, double>& metrics);

    // Adaptation
    void register_adaptation_callback(const std::string& type,
                                     std::function<void(const std::string&)> callback);
    void enable_adaptation(bool enable) { adaptation_enabled_ = enable; }
    bool is_adaptation_enabled() const { return adaptation_enabled_.load(); }

    // Analytics
    int get_adaptations_applied() const { return adaptations_applied_.load(); }
    int get_successful_adaptations() const { return successful_adaptations_.load(); }
    double get_adaptation_success_rate() const;

    // Knowledge management
    bool save_knowledge(const std::string& filename) const;
    bool load_knowledge(const std::string& filename);
    void print_knowledge_summary() const;

    // Self-improvement
    void periodic_learning_cycle();
    void optimize_strategies();
};

/**
 * @brief Chaos learning integration
 */
class ChaosLearningIntegration {
private:
    AutonomousAdaptationSystem* adaptation_system_;
    std::vector<std::string> chaos_monitored_components_;
    std::chrono::system_clock::time_point last_learning_cycle_;

public:
    ChaosLearningIntegration(AutonomousAdaptationSystem* system);

    // Chaos event handling
    void on_chaos_experiment_start(const std::string& experiment_id);
    void on_chaos_event(const std::string& component, const std::string& chaos_type);
    void on_chaos_experiment_end(const std::string& experiment_id, bool success);

    // Learning from chaos
    void extract_lessons_from_chaos();
    void generate_adaptations_from_patterns();

    // Configuration
    void add_monitored_component(const std::string& component);
    void remove_monitored_component(const std::string& component);
};

/**
 * @brief Global failure learning manager
 */
class FailureLearningManager {
private:
    static std::unique_ptr<AutonomousAdaptationSystem> instance_;
    static std::mutex instance_mutex_;

public:
    // Singleton access
    static AutonomousAdaptationSystem& get_instance();
    static void shutdown();

    // Convenience methods
    static void record_failure(const std::string& type, const std::string& context,
                              bool handled = false, const std::string& strategy = "");
    static void record_recovery(const std::string& failure_id, const std::string& strategy,
                               double recovery_time_ms);
    static std::string get_best_strategy(const std::string& failure_type);
    static bool predict_failure(const std::unordered_map<std::string, double>& metrics);
};

} // namespace learning
} // namespace xpumusic