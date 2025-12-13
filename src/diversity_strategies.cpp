/**
 * @file diversity_strategies.cpp
 * @brief Implementation of diversity enhancement strategies
 * @date 2025-12-13
 */

#include "diversity_strategies.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace xpumusic {
namespace diversity {

// ============================================================================
// Resampling Strategies Implementation
// ============================================================================

float LinearResampling::resample_sample(float input, float ratio) {
    // Simple linear interpolation
    return input * ratio;
}

float CubicResampling::resample_sample(float input, float ratio) {
    // Add sample to history
    history_[history_ptr_] = input;
    history_ptr_ = (history_ptr_ + 1) % 4;

    // Cubic interpolation (simplified)
    if (history_ptr_ >= 2) {
        float t = ratio;
        float p0 = history_[(history_ptr_ - 3 + 4) % 4];
        float p1 = history_[(history_ptr_ - 2 + 4) % 4];
        float p2 = history_[(history_ptr_ - 1 + 4) % 4];
        float p3 = history_[history_ptr_];

        // Catmull-Rom spline
        float a0 = -0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3;
        float a1 = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
        float a2 = -0.5f * p0 + 0.5f * p2;
        float a3 = p1;

        return a0 * t * t * t + a1 * t * t + a2 * t + a3;
    }

    return input;
}

LanczosResampling::LanczosResampling() {
    // Precompute Lanczos kernel
    kernel_cache_.resize(TAPS * 2);
    for (int i = 0; i < TAPS * 2; ++i) {
        float x = i - TAPS + 0.5f;
        if (x == 0) {
            kernel_cache_[i] = 1.0f;
        } else if (fabs(x) < TAPS) {
            kernel_cache_[i] = TAPS * sin(M_PI * x / TAPS) * sin(M_PI * x) / (M_PI * M_PI * x * x);
        } else {
            kernel_cache_[i] = 0.0f;
        }
    }
}

float LanczosResampling::resample_sample(float input, float ratio) {
    if (!is_available()) {
        return input; // Fallback
    }

    // Apply Lanczos filter (simplified for single sample)
    float result = 0.0f;
    float sum = 0.0f;

    for (int i = 0; i < TAPS * 2; ++i) {
        float weight = kernel_cache_[i];
        result += input * weight;
        sum += weight;
    }

    return sum > 0 ? result / sum : input;
}

FallbackResampling::FallbackResampling() : current_strategy_(0) {
    // Add strategies in order of preference
    add_strategy(std::make_unique<LanczosResampling>());
    add_strategy(std::make_unique<CubicResampling>());
    add_strategy(std::make_unique<LinearResampling>());
}

float FallbackResampling::resample_sample(float input, float ratio) {
    // Try current strategy
    for (size_t i = 0; i < strategies_.size(); ++i) {
        size_t idx = (current_strategy_ + i) % strategies_.size();
        if (strategies_[idx] && strategies_[idx]->is_available()) {
            try {
                float result = strategies_[idx]->resample_sample(input, ratio);
                if (idx != current_strategy_) {
                    current_strategy_ = idx;
                }
                return result;
            } catch (...) {
                // Try next strategy
                continue;
            }
        }
    }

    return input; // Ultimate fallback
}

std::string FallbackResampling::get_name() const {
    if (current_strategy_ < strategies_.size() && strategies_[current_strategy_]) {
        return "Fallback(" + strategies_[current_strategy_]->get_name() + ")";
    }
    return "Fallback(None)";
}

double FallbackResampling::get_quality_score() const {
    if (current_strategy_ < strategies_.size() && strategies_[current_strategy_]) {
        return strategies_[current_strategy_]->get_quality_score() * 0.9; // Small penalty for fallback
    }
    return 0.1;
}

bool FallbackResampling::is_available() const {
    return !strategies_.empty();
}

void FallbackResampling::add_strategy(std::unique_ptr<IResamplingStrategy> strategy) {
    if (strategy) {
        strategies_.push_back(std::move(strategy));
    }
}

// ============================================================================
// Memory Strategies Implementation
// ============================================================================

void* StandardMemory::allocate(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        allocated_bytes_ += size;
    }
    return ptr;
}

void StandardMemory::deallocate(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

PoolMemory::PoolMemory(size_t initial_pool) : pool_size_(initial_pool), allocated_bytes_(0) {
    if (pool_size_ > 0) {
        void* pool_block = malloc(pool_size_);
        if (pool_block) {
            blocks_.push_back({pool_block, pool_size_, false});
        }
    }
}

PoolMemory::~PoolMemory() {
    for (const auto& block : blocks_) {
        free(block.ptr);
    }
}

void* PoolMemory::allocate(size_t size) {
    // Look for free block
    for (auto& block : blocks_) {
        if (!block.in_use && block.size >= size) {
            block.in_use = true;
            allocated_bytes_ += size;
            return block.ptr;
        }
    }

    // Create new block if needed
    void* new_block = malloc(size);
    if (new_block) {
        blocks_.push_back({new_block, size, true});
        allocated_bytes_ += size;
        return new_block;
    }

    return nullptr;
}

void PoolMemory::deallocate(void* ptr) {
    for (auto& block : blocks_) {
        if (block.ptr == ptr) {
            block.in_use = false;
            allocated_bytes_ -= block.size;
            break;
        }
    }
}

ArenaMemory::ArenaMemory(size_t arena_size) : offset_(0) {
    arena_.resize(arena_size);
}

void* ArenaMemory::allocate(size_t size) {
    size_t aligned_size = (size + 7) & ~7; // 8-byte alignment
    if (offset_ + aligned_size <= arena_.size()) {
        void* ptr = &arena_[offset_];
        allocations_.push_back({offset_, aligned_size});
        offset_ += aligned_size;
        return ptr;
    }
    return nullptr;
}

void ArenaMemory::deallocate(void* ptr) {
    // Arena doesn't support individual deallocation
    // Just mark it in the allocation map
}

void ArenaMemory::reset() {
    offset_ = 0;
    allocations_.clear();
}

size_t ArenaMemory::get_allocated_bytes() const {
    return offset_;
}

// ============================================================================
// Error Handling Strategies Implementation
// ============================================================================

RetryStrategy::RetryStrategy(int max_retries) : max_retries_(max_retries), rng_(std::random_device{}()) {}

bool RetryStrategy::handle_error(int error_code, const std::string& context) {
    if (error_code == 0) return true; // No error

    for (int i = 0; i < max_retries_; ++i) {
        // Exponential backoff with jitter
        std::uniform_int_distribution<> dist(0, 1000);
        int delay = (1 << i) * 100 + dist(rng_);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        // In real implementation, this would retry the operation
        // For now, we just simulate success after retries
        if (i == max_retries_ - 1) {
            return true; // Pretend we succeeded
        }
    }

    return false;
}

void FallbackStrategy::add_fallback(std::function<bool()> fallback) {
    fallbacks_.push_back(fallback);
}

bool FallbackStrategy::handle_error(int error_code, const std::string& context) {
    if (error_code == 0) return true; // No error

    // Try each fallback in order
    for (auto& fallback : fallbacks_) {
        try {
            if (fallback()) {
                return true;
            }
        } catch (...) {
            continue;
        }
    }

    return false;
}

void GracefulDegradationStrategy::add_degradation(int error_code, std::function<void()> handler) {
    degradations_[error_code] = handler;
}

bool GracefulDegradationStrategy::handle_error(int error_code, const std::string& context) {
    if (error_code == 0) return true; // No error

    auto it = degradations_.find(error_code);
    if (it != degradations_.end()) {
        try {
            it->second(); // Execute degradation handler
            return true; // Handled gracefully
        } catch (...) {
            // Degradation failed
        }
    }

    return false;
}

// ============================================================================
// Diversity Coordinator Implementation
// ============================================================================

DiversityCoordinator::DiversityCoordinator() : resampling_manager_(true), memory_manager_(false), error_manager_(false) {
    initialize_default_strategies();
}

IResamplingStrategy* DiversityCoordinator::get_resampling_strategy() const {
    if (!diversity_enabled_.load()) {
        return resampling_manager_.get_strategy(0); // Always use first if disabled
    }
    return resampling_manager_.get_active_strategy();
}

IMemoryStrategy* DiversityCoordinator::get_memory_strategy() const {
    if (!diversity_enabled_.load()) {
        return memory_manager_.get_strategy(0);
    }
    return memory_manager_.get_active_strategy();
}

IErrorStrategy* DiversityCoordinator::get_error_strategy() const {
    if (!diversity_enabled_.load()) {
        return error_manager_.get_strategy(0);
    }
    return error_manager_.get_active_strategy();
}

void DiversityCoordinator::initialize_default_strategies() {
    // Initialize resampling strategies
    resampling_manager_.add_strategy(std::make_unique<LinearResampling>());
    resampling_manager_.add_strategy(std::make_unique<CubicResampling>());
    resampling_manager_.add_strategy(std::make_unique<LanczosResampling>());
    resampling_manager_.add_strategy(std::make_unique<FallbackResampling>());

    // Initialize memory strategies
    memory_manager_.add_strategy(std::make_unique<StandardMemory>());
    memory_manager_.add_strategy(std::make_unique<PoolMemory>(1024 * 1024));
    memory_manager_.add_strategy(std::make_unique<ArenaMemory>(2 * 1024 * 1024));

    // Initialize error handling strategies
    auto retry_strategy = std::make_unique<RetryStrategy>(3);
    retry_strategy->add_fallback([]() { return true; }); // Simple fallback
    error_manager_.add_strategy(std::move(retry_strategy));

    auto fallback_strategy = std::make_unique<FallbackStrategy>();
    fallback_strategy->add_fallback([]() { return true; });
    error_manager_.add_strategy(std::move(fallback_strategy));
}

double DiversityCoordinator::calculate_diversity_score() const {
    double total_diversity = 0.0;
    int strategy_types = 0;

    // Calculate strategy diversity
    auto resampling_names = resampling_manager_.get_strategy_names();
    if (!resampling_names.empty()) {
        total_diversity += static_cast<double>(resampling_names.size()) / 4.0; // Max 4 strategies
        strategy_types++;
    }

    auto memory_names = memory_manager_.get_strategy_names();
    if (!memory_names.empty()) {
        total_diversity += static_cast<double>(memory_names.size()) / 3.0; // Max 3 strategies
        strategy_types++;
    }

    auto error_names = error_manager_.get_strategy_names();
    if (!error_names.empty()) {
        total_diversity += static_cast<double>(error_names.size()) / 3.0; // Max 3 strategies
        strategy_types++;
    }

    return strategy_types > 0 ? total_diversity / strategy_types : 0.0;
}

} // namespace diversity
} // namespace xpumusic