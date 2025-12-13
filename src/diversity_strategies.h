/**
 * @file diversity_strategies.h
 * @brief Diversity enhancement strategies for antifragility
 * @date 2025-12-13
 *
 * Multiple implementations of the same interface to create diversity.
 * If one strategy fails, others can compensate.
 */

#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <random>
#include <chrono>

namespace xpumusic {
namespace diversity {

/**
 * @brief Audio resampling strategies
 */
class IResamplingStrategy {
public:
    virtual ~IResamplingStrategy() = default;
    virtual float resample_sample(float input, float ratio) = 0;
    virtual std::string get_name() const = 0;
    virtual double get_quality_score() const = 0;
    virtual bool is_available() const = 0;
};

class LinearResampling : public IResamplingStrategy {
public:
    float resample_sample(float input, float ratio) override;
    std::string get_name() const override { return "Linear"; }
    double get_quality_score() const override { return 0.6; }
    bool is_available() const override { return true; }
};

class CubicResampling : public IResamplingStrategy {
private:
    float history_[4] = {0};
    int history_ptr_ = 0;

public:
    float resample_sample(float input, float ratio) override;
    std::string get_name() const override { return "Cubic"; }
    double get_quality_score() const override { return 0.8; }
    bool is_available() const override { return true; }
};

class LanczosResampling : public IResamplingStrategy {
private:
    static constexpr int TAPS = 8;
    std::vector<float> kernel_cache_;

public:
    LanczosResampling();
    float resample_sample(float input, float ratio) override;
    std::string get_name() const override { return "Lanczos"; }
    double get_quality_score() const override { return 0.95; }
    bool is_available() const override { return kernel_cache_.size() > 0; }
};

class FallbackResampling : public IResamplingStrategy {
private:
    std::vector<std::unique_ptr<IResamplingStrategy>> strategies_;
    size_t current_strategy_;

public:
    FallbackResampling();
    float resample_sample(float input, float ratio) override;
    std::string get_name() const override;
    double get_quality_score() const override;
    bool is_available() const override;
    void add_strategy(std::unique_ptr<IResamplingStrategy> strategy);
};

/**
 * @brief Memory allocation strategies
 */
class IMemoryStrategy {
public:
    virtual ~IMemoryStrategy() = default;
    virtual void* allocate(size_t size) = 0;
    virtual void deallocate(void* ptr) = 0;
    virtual size_t get_allocated_bytes() const = 0;
    virtual std::string get_name() const = 0;
    virtual bool is_available() = 0;
};

class StandardMemory : public IMemoryStrategy {
private:
    size_t allocated_bytes_;

public:
    StandardMemory() : allocated_bytes_(0) {}
    void* allocate(size_t size) override;
    void deallocate(void* ptr) override;
    size_t get_allocated_bytes() const override { return allocated_bytes_; }
    std::string get_name() const override { return "Standard"; }
    bool is_available() override { return true; }
};

class PoolMemory : public IMemoryStrategy {
private:
    struct Block {
        void* ptr;
        size_t size;
        bool in_use;
    };

    std::vector<Block> blocks_;
    size_t pool_size_;
    size_t allocated_bytes_;

public:
    PoolMemory(size_t initial_pool = 1024 * 1024);
    ~PoolMemory();
    void* allocate(size_t size) override;
    void deallocate(void* ptr) override;
    size_t get_allocated_bytes() const override { return allocated_bytes_; }
    std::string get_name() const override { return "Pool"; }
    bool is_available() override { return pool_size_ > 0; }
};

class ArenaMemory : public IMemoryStrategy {
private:
    std::vector<uint8_t> arena_;
    size_t offset_;
    std::vector<std::pair<size_t, size_t>> allocations_;

public:
    ArenaMemory(size_t arena_size = 2 * 1024 * 1024);
    void* allocate(size_t size) override;
    void deallocate(void* ptr) override;
    void reset();
    size_t get_allocated_bytes() const override;
    std::string get_name() const override { return "Arena"; }
    bool is_available() override { return arena_.size() > 0; }
};

/**
 * @brief Error handling strategies
 */
class IErrorStrategy {
public:
    virtual ~IErrorStrategy() = default;
    virtual bool handle_error(int error_code, const std::string& context) = 0;
    virtual std::string get_name() const = 0;
    virtual bool is_available() = 0;
};

class RetryStrategy : public IErrorStrategy {
private:
    int max_retries_;
    std::mt19937 rng_;

public:
    RetryStrategy(int max_retries = 3);
    bool handle_error(int error_code, const std::string& context) override;
    std::string get_name() const override { return "Retry"; }
    bool is_available() override { return true; }
};

class FallbackStrategy : public IErrorStrategy {
private:
    std::vector<std::function<bool()>> fallbacks_;

public:
    void add_fallback(std::function<bool()> fallback);
    bool handle_error(int error_code, const std::string& context) override;
    std::string get_name() const override { return "Fallback"; }
    bool is_available() override { return !fallbacks_.empty(); }
};

class GracefulDegradationStrategy : public IErrorStrategy {
private:
    std::map<int, std::function<void()>> degradations_;

public:
    void add_degradation(int error_code, std::function<void()> handler);
    bool handle_error(int error_code, const std::string& context) override;
    std::string get_name() const override { return "Graceful Degradation"; }
    bool is_available() override { return !degradations_.empty(); }
};

/**
 * @brief Strategy manager that selects the best strategy
 */
template<typename StrategyType>
class StrategyManager {
private:
    std::vector<std::unique_ptr<StrategyType>> strategies_;
    mutable size_t current_index_;
    mutable std::mt19937 rng_;
    bool use_random_selection_;

public:
    StrategyManager(bool random = false)
        : current_index_(0)
        , rng_(std::random_device{}())
        , use_random_selection_(random) {}

    void add_strategy(std::unique_ptr<StrategyType> strategy) {
        if (strategy && strategy->is_available()) {
            strategies_.push_back(std::move(strategy));
        }
    }

    StrategyType* get_active_strategy() const {
        if (strategies_.empty()) return nullptr;

        if (use_random_selection_) {
            std::uniform_int_distribution<> dist(0, strategies_.size() - 1);
            current_index_ = dist(rng_);
        } else {
            // Select highest quality available strategy
            double best_quality = -1.0;
            size_t best_index = 0;

            for (size_t i = 0; i < strategies_.size(); ++i) {
                if (strategies_[i]->is_available()) {
                    double quality = strategies_[i]->get_quality_score();
                    if (quality > best_quality) {
                        best_quality = quality;
                        best_index = i;
                    }
                }
            }

            current_index_ = best_index;
        }

        return strategies_[current_index_].get();
    }

    StrategyType* get_strategy(size_t index) const {
        if (index < strategies_.size()) {
            return strategies_[index].get();
        }
        return nullptr;
    }

    size_t get_strategy_count() const {
        return strategies_.size();
    }

    void enable_random_selection(bool enable) {
        use_random_selection_ = enable;
    }

    std::vector<std::string> get_strategy_names() const {
        std::vector<std::string> names;
        for (const auto& strategy : strategies_) {
            if (strategy->is_available()) {
                names.push_back(strategy->get_name());
            }
        }
        return names;
    }
};

/**
 * @brief Diversity coordinator
 */
class DiversityCoordinator {
private:
    StrategyManager<IResamplingStrategy> resampling_manager_;
    StrategyManager<IMemoryStrategy> memory_manager_;
    StrategyManager<IErrorStrategy> error_manager_;

    std::atomic<bool> diversity_enabled_{true};
    std::atomic<uint64_t> strategy_switches_{0};

public:
    DiversityCoordinator();

    // Strategy selection
    IResamplingStrategy* get_resampling_strategy() const;
    IMemoryStrategy* get_memory_strategy() const;
    IErrorStrategy* get_error_strategy() const;

    // Configuration
    void enable_diversity(bool enable) { diversity_enabled_ = enable; }
    bool is_diversity_enabled() const { return diversity_enabled_.load(); }

    // Metrics
    uint64_t get_strategy_switches() const { return strategy_switches_.load(); }
    void reset_switch_counter() { strategy_switches_ = 0; }

    // Initialize default strategies
    void initialize_default_strategies();

    // Diversity score calculation
    double calculate_diversity_score() const;
};

} // namespace diversity
} // namespace xpumusic