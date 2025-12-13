/**
 * @file failure_learning_system.cpp
 * @brief Implementation of failure learning and adaptation system
 * @date 2025-12-13
 */

#include "failure_learning_system.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <regex>

namespace xpumusic {
namespace learning {

// ============================================================================
// FailureKnowledgeBase Implementation
// ============================================================================

void FailureKnowledgeBase::record_event(const FailureEvent& event) {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);
    events_.push_back(event);

    // Update patterns when we have enough events
    if (events_.size() % 10 == 0) {
        update_patterns();
    }
}

std::vector<FailureEvent> FailureKnowledgeBase::get_events(const std::string& type) const {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    if (type.empty()) {
        return events_;
    }

    std::vector<FailureEvent> filtered;
    for (const auto& event : events_) {
        if (event.type == type) {
            filtered.push_back(event);
        }
    }
    return filtered;
}

std::vector<FailureEvent> FailureKnowledgeBase::get_recent_events(int count) const {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    std::vector<FailureEvent> recent;
    int start = std::max(0, static_cast<int>(events_.size()) - count);

    for (int i = events_.size() - 1; i >= start; --i) {
        recent.push_back(events_[i]);
    }

    return recent;
}

void FailureKnowledgeBase::update_patterns() {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    // Group events by type and context
    std::unordered_map<std::string, std::vector<const FailureEvent*>> grouped;

    for (const auto& event : events_) {
        std::string key = event.type + "|" + event.context;
        grouped[key].push_back(&event);
    }

    // Update or create patterns
    for (const auto& [key, event_group] : grouped) {
        if (event_group.size() >= pattern_threshold_) {
            std::string pattern_id = "pattern_" + std::to_string(std::hash<std::string>{}(key) % 10000);

            auto& pattern = patterns_[pattern_id];
            if (pattern.pattern_id.empty()) {
                // New pattern
                pattern.pattern_id = pattern_id;
                pattern.type = event_group[0]->type;
                pattern.occurrence_count = 0;
                pattern.success_rate = 0.0;
            }

            // Update pattern statistics
            pattern.occurrence_count = static_cast<int>(event_group.size());
            pattern.last_seen = event_group.back()->timestamp;

            // Calculate success rate
            int handled = std::count_if(event_group.begin(), event_group.end(),
                [](const FailureEvent* e) { return e->was_handled; });
            pattern.success_rate = static_cast<double>(handled) / event_group.size();

            // Calculate average recovery time
            double total_recovery = 0;
            int recoveries = 0;
            std::unordered_map<std::string, int> strategy_counts;

            for (const auto* event : event_group) {
                if (event->was_handled && event->recovery_time_ms > 0) {
                    total_recovery += event->recovery_time_ms;
                    recoveries++;
                }
                if (!event->recovery_strategy.empty()) {
                    strategy_counts[event->recovery_strategy]++;
                }
            }

            pattern.avg_recovery_time = recoveries > 0 ? total_recovery / recoveries : 0;

            // Find best strategy
            if (!strategy_counts.empty()) {
                auto best = std::max_element(strategy_counts.begin(), strategy_counts.end(),
                    [](const auto& a, const auto& b) { return a.second < b.second; });
                pattern.best_recovery_strategy = best->first;
            }

            // Check if recurring
            auto now = std::chrono::system_clock::now();
            auto time_diff = std::chrono::duration_cast<std::chrono::hours>(now - pattern.last_seen);
            pattern.is_recurring = time_diff.count() < 24; // Within last 24 hours
        }
    }
}

std::vector<FailurePattern> FailureKnowledgeBase::get_patterns() const {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    std::vector<FailurePattern> patterns;
    for (const auto& [id, pattern] : patterns_) {
        patterns.push_back(pattern);
    }
    return patterns;
}

FailurePattern* FailureKnowledgeBase::get_pattern(const std::string& pattern_id) {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    auto it = patterns_.find(pattern_id);
    return it != patterns_.end() ? &it->second : nullptr;
}

bool FailureKnowledgeBase::has_pattern(const std::string& type) const {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    for (const auto& [id, pattern] : patterns_) {
        if (pattern.type == type) {
            return true;
        }
    }
    return false;
}

void FailureKnowledgeBase::add_rule(const AdaptationRule& rule) {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);
    rules_.push_back(rule);
}

void FailureKnowledgeBase::update_rule_effectiveness(const std::string& rule_id, bool success) {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    for (auto& rule : rules_) {
        if (rule.rule_id == rule_id) {
            double learning_rate = 0.1;
            rule.effectiveness = rule.effectiveness * (1 - learning_rate) +
                                (success ? 1.0 : 0.0) * learning_rate;
            rule.usage_count++;
            break;
        }
    }
}

std::vector<AdaptationRule> FailureKnowledgeBase::get_applicable_rules(
    const std::string& type, const std::string& context) const {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    std::vector<AdaptationRule> applicable;

    for (const auto& rule : rules_) {
        if (!rule.enabled) continue;

        // Simple pattern matching (in real implementation, use proper expression parser)
        if (rule.condition.find(type) != std::string::npos ||
            rule.condition.find(context) != std::string::npos) {
            applicable.push_back(rule);
        }
    }

    // Sort by effectiveness
    std::sort(applicable.begin(), applicable.end(),
        [](const AdaptationRule& a, const AdaptationRule& b) {
            return a.effectiveness > b.effectiveness;
        });

    return applicable;
}

bool FailureKnowledgeBase::save_to_file(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    std::ofstream file(filename);
    if (!file.is_open()) return false;

    // Save patterns
    file << "# Failure Patterns\n";
    for (const auto& [id, pattern] : patterns_) {
        file << "PATTERN|" << id << "|" << pattern.type << "|"
             << pattern.occurrence_count << "|" << pattern.success_rate << "|"
             << pattern.best_recovery_strategy << "\n";
    }

    // Save rules
    file << "\n# Adaptation Rules\n";
    for (const auto& rule : rules_) {
        file << "RULE|" << rule.rule_id << "|" << rule.condition << "|"
             << rule.effectiveness << "|" << rule.usage_count << "\n";
    }

    return true;
}

bool FailureKnowledgeBase::load_from_file(const std::string& filename) {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    std::ifstream file(filename);
    if (!file.is_open()) return false;

    patterns_.clear();
    rules_.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> parts;

        while (std::getline(ss, token, '|')) {
            parts.push_back(token);
        }

        if (parts.size() >= 6 && parts[0] == "PATTERN") {
            FailurePattern pattern;
            pattern.pattern_id = parts[1];
            pattern.type = parts[2];
            pattern.occurrence_count = std::stoi(parts[3]);
            pattern.success_rate = std::stod(parts[4]);
            pattern.best_recovery_strategy = parts[5];
            patterns_[pattern.pattern_id] = pattern;
        } else if (parts.size() >= 5 && parts[0] == "RULE") {
            AdaptationRule rule;
            rule.rule_id = parts[1];
            rule.condition = parts[2];
            rule.effectiveness = std::stod(parts[3]);
            rule.usage_count = std::stoi(parts[4]);
            rule.enabled = true;
            rule.created = std::chrono::system_clock::now();
            rules_.push_back(rule);
        }
    }

    return true;
}

std::unordered_map<std::string, int> FailureKnowledgeBase::get_failure_statistics() const {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    std::unordered_map<std::string, int> stats;

    for (const auto& event : events_) {
        stats[event.type]++;
    }

    return stats;
}

std::vector<std::string> FailureKnowledgeBase::get_top_failure_types(int count) const {
    auto stats = get_failure_statistics();

    std::vector<std::pair<std::string, int>> sorted(stats.begin(), stats.end());
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<std::string> top;
    for (int i = 0; i < std::min(count, static_cast<int>(sorted.size())); ++i) {
        top.push_back(sorted[i].first);
    }

    return top;
}

double FailureKnowledgeBase::calculate_system_resilience() const {
    std::lock_guard<std::mutex> lock(knowledge_mutex_);

    if (events_.empty()) return 1.0;

    int handled = std::count_if(events_.begin(), events_.end(),
        [](const FailureEvent& e) { return e->was_handled; });

    return static_cast<double>(handled) / events_.size();
}

// ============================================================================
// AdaptiveStrategySelector Implementation
// ============================================================================

void AdaptiveStrategySelector::register_strategy(const std::string& failure_type,
                                                 const std::string& strategy) {
    strategy_map_[failure_type].push_back(strategy);
    if (strategy_success_rates_.find(strategy) == strategy_success_rates_.end()) {
        strategy_success_rates_[strategy] = 0.5; // Initial neutral rating
    }
}

void AdaptiveStrategySelector::update_strategy_performance(const std::string& strategy,
                                                           bool success) {
    if (strategy_success_rates_.find(strategy) != strategy_success_rates_.end()) {
        double learning_rate = 0.1;
        double current = strategy_success_rates_[strategy];
        strategy_success_rates_[strategy] = current * (1 - learning_rate) +
                                          (success ? 1.0 : 0.0) * learning_rate;
    }
}

std::string AdaptiveStrategySelector::select_strategy(const std::string& failure_type,
                                                      const std::string& context) const {
    auto it = strategy_map_.find(failure_type);
    if (it == strategy_map_.end()) {
        // Try knowledge base patterns
        auto patterns = knowledge_base_->get_patterns();
        for (const auto& pattern : patterns) {
            if (pattern.type == failure_type && !pattern.best_recovery_strategy.empty()) {
                return pattern.best_recovery_strategy;
            }
        }
        return "default";
    }

    const auto& strategies = it->second;
    if (strategies.empty()) return "default";

    // Select best performing strategy
    std::string best = strategies[0];
    double best_score = strategy_success_rates_.count(best) ?
                        strategy_success_rates_.at(best) : 0.5;

    for (const auto& strategy : strategies) {
        double score = strategy_success_rates_.count(strategy) ?
                       strategy_success_rates_.at(strategy) : 0.5;
        if (score > best_score) {
            best = strategy;
            best_score = score;
        }
    }

    return best;
}

std::vector<std::string> AdaptiveStrategySelector::get_strategies(
    const std::string& failure_type) const {
    auto it = strategy_map_.find(failure_type);
    if (it != strategy_map_.end()) {
        return it->second;
    }
    return {};
}

// ============================================================================
// FailurePredictor Implementation
// ============================================================================

void FailurePredictor::train_models() {
    // Simple training based on historical data
    auto events = knowledge_base_->get_events();
    auto patterns = knowledge_base_->get_patterns();

    models_.clear();

    for (const auto& pattern : patterns) {
        if (pattern.success_rate < 0.5 && pattern.occurrence_count > 5) {
            PredictionModel model;
            model.failure_type = pattern.type;
            model.threshold = 0.7;
            model.accuracy = 0.6; // Initial estimate
            model.true_positives = 0;
            model.false_positives = 0;
            model.false_negatives = 0;

            // Add indicators based on failure type
            if (pattern.type == "memory_exhaustion") {
                model.indicators = {"memory_usage", "allocation_failures"};
            } else if (pattern.type == "file_corruption") {
                model.indicators = {"io_errors", "checksum_failures"};
            } else if (pattern.type == "audio_dropout") {
                model.indicators = {"buffer_underruns", "cpu_overload"};
            }

            models_.push_back(model);
        }
    }
}

bool FailurePredictor::predict_failure(const std::unordered_map<std::string, double>& indicators,
                                      std::string& predicted_type) const {
    for (const auto& model : models_) {
        double score = 0.0;
        int matches = 0;

        for (const auto& indicator : model.indicators) {
            auto it = indicators.find(indicator);
            if (it != indicators.end()) {
                score += it->second;
                matches++;
            }
        }

        if (matches > 0) {
            score /= matches;
            if (score >= model.threshold) {
                predicted_type = model.failure_type;
                return true;
            }
        }
    }

    return false;
}

double FailurePredictor::get_model_accuracy(const std::string& failure_type) const {
    for (const auto& model : models_) {
        if (model.failure_type == failure_type) {
            int total = model.true_positives + model.false_positives + model.false_negatives;
            return total > 0 ? static_cast<double>(model.true_positives) / total : 0.0;
        }
    }
    return 0.0;
}

void FailurePredictor::update_model_performance(const std::string& failure_type,
                                                bool predicted, bool actual) {
    for (auto& model : models_) {
        if (model.failure_type == failure_type) {
            if (predicted && actual) {
                model.true_positives++;
            } else if (predicted && !actual) {
                model.false_positives++;
            } else if (!predicted && actual) {
                model.false_negatives++;
            }

            // Update accuracy
            int total = model.true_positives + model.false_positives + model.false_negatives;
            model.accuracy = total > 0 ? static_cast<double>(model.true_positives) / total : 0.0;
            break;
        }
    }
}

// ============================================================================
// AutonomousAdaptationSystem Implementation
// ============================================================================

AutonomousAdaptationSystem::AutonomousAdaptationSystem() {
    knowledge_base_ = std::make_unique<FailureKnowledgeBase>();
    strategy_selector_ = std::make_unique<AdaptiveStrategySelector>(knowledge_base_.get());
    predictor_ = std::make_unique<FailurePredictor>(knowledge_base_.get());
}

void AutonomousAdaptationSystem::initialize() {
    // Load existing knowledge if available
    load_knowledge("failure_knowledge.dat");

    // Train prediction models
    predictor_->train_models();

    // Register default strategies
    strategy_selector_->register_strategy("memory_exhaustion", "free_cache");
    strategy_selector_->register_strategy("memory_exhaustion", "reduce_buffer_size");
    strategy_selector_->register_strategy("memory_exhaustion", "switch_to_arena");

    strategy_selector_->register_strategy("file_corruption", "use_backup");
    strategy_selector_->register_strategy("file_corruption", "revalidate_file");
    strategy_selector_->register_strategy("file_corruption", "skip_corrupted");

    strategy_selector_->register_strategy("audio_dropout", "increase_buffer");
    strategy_selector_->register_strategy("audio_dropout", "reduce_quality");
    strategy_selector_->register_strategy("audio_dropout", "switch_to_simpler_resampler");
}

void AutonomousAdaptationSystem::learn_from_failure(const FailureEvent& event) {
    knowledge_base_->record_event(event);

    // Update strategy performance
    if (!event.recovery_strategy.empty()) {
        strategy_selector_->update_strategy_performance(event.recovery_strategy, event.was_handled);
    }

    // Re-train models periodically
    static int learn_counter = 0;
    if (++learn_counter % 50 == 0) {
        predictor_->train_models();
    }
}

void AutonomousAdaptationSystem::handle_failure(const std::string& type,
                                               const std::string& context,
                                               std::function<bool()> recovery_action) {
    if (!adaptation_enabled_.load()) return;

    adaptations_applied_++;

    // Select best strategy
    std::string strategy = strategy_selector_->select_strategy(type, context);

    // Get applicable adaptation rules
    auto rules = knowledge_base_->get_applicable_rules(type, context);
    for (const auto& rule : rules) {
        // Apply rule actions (simplified)
        if (rule.actions.size() > 0) {
            strategy = rule.actions[0];
        }
    }

    // Execute adaptation callback if registered
    auto callback_it = adaptation_callbacks_.find(type);
    if (callback_it != adaptation_callbacks_.end()) {
        callback_it->second(strategy);
    }

    // Attempt recovery
    bool success = false;
    auto start = std::chrono::steady_clock::now();

    if (recovery_action) {
        success = recovery_action();
    }

    auto end = std::chrono::steady_clock::now();
    double recovery_time = std::chrono::duration<double, std::milli>(end - start).count();

    // Record the outcome
    FailureEvent event;
    event.type = type;
    event.context = context;
    event.timestamp = std::chrono::system_clock::now();
    event.was_handled = success;
    event.recovery_time_ms = recovery_time;
    event.recovery_strategy = strategy;

    learn_from_failure(event);

    if (success) {
        successful_adaptations_++;
    }
}

bool AutonomousAdaptationSystem::predict_and_prevent(
    const std::unordered_map<std::string, double>& system_metrics) {
    if (!adaptation_enabled_.load()) return false;

    std::string predicted_failure;
    if (predictor_->predict_failure(system_metrics, predicted_failure)) {
        // Apply preventive measures
        handle_failure(predicted_failure, "preventive", nullptr);
        return true;
    }

    return false;
}

void AutonomousAdaptationSystem::register_adaptation_callback(
    const std::string& type, std::function<void(const std::string&)> callback) {
    adaptation_callbacks_[type] = callback;
}

double AutonomousAdaptationSystem::get_adaptation_success_rate() const {
    int total = adaptations_applied_.load();
    int successful = successful_adaptations_.load();
    return total > 0 ? static_cast<double>(successful) / total : 0.0;
}

bool AutonomousAdaptationSystem::save_knowledge(const std::string& filename) const {
    return knowledge_base_->save_to_file(filename);
}

bool AutonomousAdaptationSystem::load_knowledge(const std::string& filename) {
    return knowledge_base_->load_from_file(filename);
}

void AutonomousAdaptationSystem::print_knowledge_summary() const {
    std::cout << "\n=== Failure Learning Summary ===\n";

    auto stats = knowledge_base_->get_failure_statistics();
    std::cout << "Failure Types Recorded: " << stats.size() << "\n";

    auto patterns = knowledge_base_->get_patterns();
    std::cout << "Patterns Identified: " << patterns.size() << "\n";

    auto top_failures = knowledge_base_->get_top_failure_types(5);
    std::cout << "\nTop Failure Types:\n";
    for (const auto& type : top_failures) {
        int count = stats[type];
        std::cout << "  " << type << ": " << count << " occurrences\n";
    }

    std::cout << "\nSystem Resilience Score: "
              << std::fixed << std::setprecision(2)
              << knowledge_base_->calculate_system_resilience() * 100 << "%\n";

    std::cout << "Adaptation Success Rate: "
              << std::fixed << std::setprecision(2)
              << get_adaptation_success_rate() * 100 << "%\n";
}

void AutonomousAdaptationSystem::periodic_learning_cycle() {
    // Update patterns
    knowledge_base_->update_patterns();

    // Re-train prediction models
    predictor_->train_models();

    // Optimize strategies based on recent performance
    optimize_strategies();

    // Save knowledge
    save_knowledge("failure_knowledge.dat");
}

void AutonomousAdaptationSystem::optimize_strategies() {
    auto recent_events = knowledge_base_->get_recent_events(100);

    // Analyze recent failures and adjust strategy preferences
    std::unordered_map<std::string, std::pair<int, int>> strategy_performance;

    for (const auto& event : recent_events) {
        if (!event.recovery_strategy.empty()) {
            auto& perf = strategy_performance[event.recovery_strategy];
            perf.first++;  // Total uses
            if (event.was_handled) {
                perf.second++;  // Successes
            }
        }
    }

    // Update strategy success rates
    for (const auto& [strategy, perf] : strategy_performance) {
        double success_rate = perf.first > 0 ? static_cast<double>(perf.second) / perf.first : 0.0;
        strategy_selector_->update_strategy_performance(strategy, success_rate > 0.7);
    }
}

// ============================================================================
// ChaosLearningIntegration Implementation
// ============================================================================

ChaosLearningIntegration::ChaosLearningIntegration(AutonomousAdaptationSystem* system)
    : adaptation_system_(system) {
    last_learning_cycle_ = std::chrono::system_clock::now();
}

void ChaosLearningIntegration::on_chaos_experiment_start(const std::string& experiment_id) {
    // Record experiment start for correlation analysis
}

void ChaosLearningIntegration::on_chaos_event(const std::string& component,
                                             const std::string& chaos_type) {
    // Convert chaos events to failure events for learning
    FailureEvent event;
    event.type = "chaos_injection";
    event.context = component + ":" + chaos_type;
    event.timestamp = std::chrono::system_clock::now();
    event.metadata["component"] = component;
    event.metadata["chaos_type"] = chaos_type;
    event.was_handled = true;  // Chaos is intentional

    adaptation_system_->learn_from_failure(event);
}

void ChaosLearningIntegration::on_chaos_experiment_end(const std::string& experiment_id,
                                                       bool success) {
    // Learn from chaos experiment outcomes
    auto now = std::chrono::system_clock::now();
    auto time_since_last = std::chrono::duration_cast<std::chrono::minutes>(now - last_learning_cycle_);

    if (time_since_last.count() > 10) {  // Every 10 minutes
        extract_lessons_from_chaos();
        last_learning_cycle_ = now;
    }
}

void ChaosLearningIntegration::extract_lessons_from_chaos() {
    adaptation_system_->periodic_learning_cycle();
}

void ChaosLearningIntegration::generate_adaptations_from_patterns() {
    auto patterns = adaptation_system_->knowledge_base_->get_patterns();

    for (const auto& pattern : patterns) {
        if (pattern.is_recurring && pattern.success_rate < 0.5) {
            // Generate adaptation rule for recurring failures
            AdaptationRule rule;
            rule.rule_id = "auto_" + pattern.pattern_id;
            rule.condition = pattern.type;
            rule.effectiveness = 0.5;
            rule.usage_count = 0;
            rule.created = std::chrono::system_clock::now();
            rule.enabled = true;

            // Add best recovery strategy as action
            if (!pattern.best_recovery_strategy.empty()) {
                rule.actions.push_back(pattern.best_recovery_strategy);
            }

            adaptation_system_->knowledge_base_->add_rule(rule);
        }
    }
}

void ChaosLearningIntegration::add_monitored_component(const std::string& component) {
    chaos_monitored_components_.push_back(component);
}

void ChaosLearningIntegration::remove_monitored_component(const std::string& component) {
    auto it = std::find(chaos_monitored_components_.begin(),
                       chaos_monitored_components_.end(), component);
    if (it != chaos_monitored_components_.end()) {
        chaos_monitored_components_.erase(it);
    }
}

// ============================================================================
// FailureLearningManager Implementation
// ============================================================================

std::unique_ptr<AutonomousAdaptationSystem> FailureLearningManager::instance_;
std::mutex FailureLearningManager::instance_mutex_;

AutonomousAdaptationSystem& FailureLearningManager::get_instance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (!instance_) {
        instance_ = std::make_unique<AutonomousAdaptationSystem>();
        instance_->initialize();
    }
    return *instance_;
}

void FailureLearningManager::shutdown() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_) {
        instance_->save_knowledge("failure_knowledge_final.dat");
        instance_.reset();
    }
}

void FailureLearningManager::record_failure(const std::string& type,
                                           const std::string& context,
                                           bool handled,
                                           const std::string& strategy) {
    FailureEvent event;
    event.type = type;
    event.context = context;
    event.timestamp = std::chrono::system_clock::now();
    event.was_handled = handled;
    event.recovery_strategy = strategy;

    get_instance().learn_from_failure(event);
}

void FailureLearningManager::record_recovery(const std::string& failure_id,
                                            const std::string& strategy,
                                            double recovery_time_ms) {
    // This would update an existing failure event
    // For simplicity, we create a new recovery event
    record_failure("recovery", failure_id, true, strategy);
}

std::string FailureLearningManager::get_best_strategy(const std::string& failure_type) {
    auto& system = get_instance();
    auto selector = std::make_unique<AdaptiveStrategySelector>(
        system.knowledge_base_.get());
    return selector->select_strategy(failure_type, "");
}

bool FailureLearningManager::predict_failure(
    const std::unordered_map<std::string, double>& metrics) {
    return get_instance().predict_and_prevent(metrics);
}

} // namespace learning
} // namespace xpumusic