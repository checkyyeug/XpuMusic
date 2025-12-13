/**
 * @file interface_evolution.h
 * @brief Interface evolution mechanisms for XpuMusic system compatibility
 * @date 2025-12-13
 *
 * This file provides mechanisms to support smooth system evolution while maintaining
 * backward compatibility and forward compatibility for different component versions.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <chrono>

namespace xpumusic {

/**
 * @brief Version information structure
 */
struct Version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;

    constexpr Version(uint32_t maj = 1, uint32_t min = 0, uint32_t pat = 0)
        : major(maj), minor(min), patch(pat) {}

    std::string to_string() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    // Comparison operators
    constexpr bool operator==(const Version& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    constexpr bool operator<(const Version& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }

    constexpr bool operator>=(const Version& other) const {
        return !(*this < other);
    }
};

// Common version definitions
constexpr Version CURRENT_VERSION{1, 0, 0};
constexpr Version MINIMUM_COMPATIBLE_VERSION{0, 9, 0};

/**
 * @brief Interface feature flags
 */
enum class FeatureFlag : uint32_t {
    BASIC_AUDIO_PROCESSING = 0x00000001,
    MULTI_THREADING = 0x00000002,
    SIMD_OPTIMIZATION = 0x00000004,
    PLUGINSYSTEM = 0x00000008,
    ADVANCED_METADATA = 0x00000010,
    REAL_TIME_PROCESSING = 0x00000020,
    CACHING = 0x00000040,
    CUSTOM_EFFECTS = 0x00000080
};

/**
 * @brief Feature flag operations
 */
inline FeatureFlag operator|(FeatureFlag a, FeatureFlag b) {
    return static_cast<FeatureFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline FeatureFlag operator&(FeatureFlag a, FeatureFlag b) {
    return static_cast<FeatureFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_feature(FeatureFlag flags, FeatureFlag feature) {
    return (flags & feature) != FeatureFlag{0};
}

/**
 * @brief Interface capability descriptor
 */
struct InterfaceCapabilities {
    Version version;
    FeatureFlag supported_features;
    std::vector<std::string> supported_formats;
    std::vector<std::string> supported_protocols;

    bool supports(FeatureFlag feature) const {
        return has_feature(supported_features, feature);
    }

    bool supports_format(const std::string& format) const {
        auto it = std::find(supported_formats.begin(), supported_formats.end(), format);
        return it != supported_formats.end();
    }
};

/**
 * @brief Interface registry for version management
 */
class InterfaceRegistry {
public:
    static InterfaceRegistry& instance();

    // Register an interface implementation
    void register_interface(
        const std::string& interface_name,
        const Version& version,
        const InterfaceCapabilities& capabilities,
        std::function<void*()> factory
    );

    // Get the best compatible implementation
    void* get_implementation(const std::string& interface_name, const Version& min_version = MINIMUM_COMPATIBLE_VERSION);

    // Get capabilities for an interface
    InterfaceCapabilities get_capabilities(const std::string& interface_name, const Version& version) const;

    // Check compatibility
    bool is_compatible(const Version& required, const Version& provided) const;

    // List all registered interfaces
    std::vector<std::string> list_interfaces() const;

private:
    struct InterfaceEntry {
        Version version;
        InterfaceCapabilities capabilities;
        std::function<void*()> factory;
        std::chrono::system_clock::time_point registration_time;
    };

    std::unordered_map<std::string, std::vector<InterfaceEntry>> interfaces_;
};

/**
 * @brief Adapter base class for interface evolution
 */
template<typename InterfaceType>
class InterfaceAdapter {
public:
    virtual ~InterfaceAdapter() = default;

    // Get the interface version this adapter supports
    virtual Version get_supported_version() const = 0;

    // Get the interface capabilities
    virtual InterfaceCapabilities get_capabilities() const = 0;

    // Check if this adapter can handle a specific version
    virtual bool can_adapt(const Version& target_version) const {
        return interface_registry_.is_compatible(target_version, get_supported_version());
    }

    // Adapt the interface to the target version
    virtual std::unique_ptr<InterfaceType> adapt(const Version& target_version) = 0;

protected:
    InterfaceRegistry& interface_registry_ = InterfaceRegistry::instance();
};

/**
 * @brief Factory for creating version-appropriate instances
 */
template<typename InterfaceType>
class InterfaceFactory {
public:
    // Create an instance with the best available version
    static std::unique_ptr<InterfaceType> create(const Version& min_version = MINIMUM_COMPATIBLE_VERSION) {
        void* impl = InterfaceRegistry::instance().get_implementation(
            typeid(InterfaceType).name(), min_version
        );
        if (impl) {
            return std::unique_ptr<InterfaceType>(static_cast<InterfaceType*>(impl));
        }
        return nullptr;
    }

    // Create with specific version requirements
    static std::unique_ptr<InterfaceType> create_with_features(FeatureFlag required_features) {
        // Find implementation that supports all required features
        auto& registry = InterfaceRegistry::instance();
        auto interfaces = registry.list_interfaces();

        for (const auto& interface_name : interfaces) {
            // Try each interface to see if it supports the features
            void* impl = registry.get_implementation(interface_name);
            if (impl) {
                // Check capabilities (this would need to be implemented per interface)
                // For now, return the first available implementation
                return std::unique_ptr<InterfaceType>(static_cast<InterfaceType*>(impl));
            }
        }

        return nullptr;
    }
};

/**
 * @brief Compatibility checker for runtime version validation
 */
class CompatibilityChecker {
public:
    struct CompatibilityReport {
        bool is_compatible;
        std::vector<std::string> missing_features;
        std::vector<std::string> version_mismatches;
        std::vector<std::string> recommendations;
    };

    // Check compatibility between client and server
    static CompatibilityReport check_compatibility(
        const InterfaceCapabilities& client_caps,
        const InterfaceCapabilities& server_caps
    );

    // Generate recommendations for incompatible systems
    static std::vector<std::string> generate_recommendations(
        const CompatibilityReport& report
    );
};

/**
 * @brief Migration helper for interface evolution
 */
template<typename OldInterface, typename NewInterface>
class InterfaceMigrator {
public:
    using MigrationFunction = std::function<std::unique_ptr<NewInterface>(std::unique_ptr<OldInterface>)>;

    // Register a migration path
    static void register_migration(
        const Version& from_version,
        const Version& to_version,
        MigrationFunction migration_func
    );

    // Perform migration if possible
    static std::unique_ptr<NewInterface> migrate(
        std::unique_ptr<OldInterface> old_impl,
        const Version& target_version
    );

private:
    struct MigrationPath {
        Version from_version;
        Version to_version;
        MigrationFunction function;
    };

    static std::vector<MigrationPath> migration_paths_;
};

/**
 * @brief Feature detection and capability query
 */
class FeatureDetector {
public:
    // Detect runtime capabilities
    static FeatureFlag detect_runtime_features();

    // Check if a specific feature is available
    static bool has_feature(FeatureFlag feature);

    // Get detailed system information
    struct SystemInfo {
        std::string cpu_vendor;
        std::string cpu_model;
        bool has_sse2;
        bool has_avx;
        bool has_avx2;
        size_t cache_line_size;
        size_t memory_size;
        int cpu_cores;
        bool is_64bit;
    };

    static SystemInfo get_system_info();

    // Query specific capabilities
    static bool supports_simd_level(const std::string& level);
    static size_t get_optimal_block_size();
    static int get_optimal_thread_count();
};

/**
 * @brief Interface evolution macros for easier usage
 */

// Register an interface implementation
#define XPUMUSIC_REGISTER_INTERFACE(InterfaceType, VersionMajor, VersionMinor, VersionPatch, FactoryFunc) \
    namespace { \
        struct InterfaceType##_Registrar { \
            InterfaceType##_Registrar() { \
                xpumusic::InterfaceRegistry::instance().register_interface( \
                    typeid(InterfaceType).name(), \
                    xpumusic::Version{VersionMajor, VersionMinor, VersionPatch}, \
                    xpumusic::InterfaceCapabilities{}, \
                    FactoryFunc \
                ); \
            } \
        }; \
        static InterfaceType##_Registrar InterfaceType##_registrar; \
    }

// Declare interface version
#define XPUMUSIC_INTERFACE_VERSION(InterfaceType, VersionMajor, VersionMinor, VersionPatch) \
    static constexpr xpumusic::Version InterfaceType##_VERSION{VersionMajor, VersionMinor, VersionPatch}; \
    virtual xpumusic::Version get_interface_version() const override { \
        return InterfaceType##_VERSION; \
    }

// Check feature availability
#define XPUMUSIC_REQUIRE_FEATURE(Feature) \
    if (!xpumusic::FeatureDetector::has_feature(xpumusic::FeatureFlag::Feature)) { \
        throw std::runtime_error("Required feature " #Feature " is not available"); \
    }

} // namespace xpumusic

/**
 * @example Using the interface evolution system
 *
 * // 1. Define your interface
 * class AudioProcessor {
 * public:
 *     virtual ~AudioProcessor() = default;
 *     virtual Version get_interface_version() const = 0;
 *     virtual void process(float* buffer, size_t frames) = 0;
 * };
 *
 * // 2. Create implementation with version
 * class ModernAudioProcessor : public AudioProcessor {
 * public:
 *     XPUMUSIC_INTERFACE_VERSION(AudioProcessor, 1, 2, 0)
 *
 *     void process(float* buffer, size_t frames) override {
 *         XPUMUSIC_REQUIRE_FEATURE(SIMD_OPTIMIZATION)
 *         // Modern SIMD implementation
 *     }
 * };
 *
 * // 3. Register the implementation
 * std::unique_ptr<AudioProcessor> create_modern_processor() {
 *     return std::make_unique<ModernAudioProcessor>();
 * }
 * XPUMUSIC_REGISTER_INTERFACE(AudioProcessor, 1, 2, 0, create_modern_processor)
 *
 * // 4. Use with version awareness
 * auto processor = InterfaceFactory<AudioProcessor>::create();
 * if (processor) {
 *     auto caps = processor->get_capabilities();
 *     if (caps.supports(FeatureFlag::SIMD_OPTIMIZATION)) {
 *         // Use optimized path
 *     }
 * }
 */