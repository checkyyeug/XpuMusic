/**
 * @file quality_system.h
 * @brief Quality assurance system design for XpuMusic
 * @date 2025-12-13
 *
 * This file defines the quality assurance framework including automated testing,
 * quality gates, continuous integration, and code quality metrics.
 */

#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <future>

namespace xpumusic {
namespace quality {

/**
 * @brief Test result structure
 */
struct TestResult {
    bool passed;
    std::string test_name;
    std::string suite_name;
    std::chrono::milliseconds duration;
    std::string failure_message;
    std::vector<std::string> tags;

    TestResult(const std::string& name, const std::string& suite)
        : passed(false), test_name(name), suite_name(suite), duration(0) {}
};

/**
 * @brief Quality metrics for code assessment
 */
struct QualityMetrics {
    // Test coverage metrics
    double line_coverage;          // Percentage of lines covered
    double branch_coverage;        // Percentage of branches covered
    double function_coverage;      // Percentage of functions covered

    // Complexity metrics
    double cyclomatic_complexity;  // Average complexity
    double cognitive_complexity;   // Average cognitive load
    int max_nesting_level;         // Maximum nesting depth

    // Code quality metrics
    int code_smells;               // Number of code smells detected
    int duplicated_lines;          // Lines of duplicated code
    double maintainability_index;  // Maintainability score (0-100)

    // Performance metrics
    double build_time;             // Build time in seconds
    std::chrono::milliseconds avg_test_time;  // Average test execution time
    size_t memory_usage;           // Memory usage in MB

    QualityMetrics()
        : line_coverage(0.0)
        , branch_coverage(0.0)
        , function_coverage(0.0)
        , cyclomatic_complexity(0.0)
        , cognitive_complexity(0.0)
        , max_nesting_level(0)
        , code_smells(0)
        , duplicated_lines(0)
        , maintainability_index(0.0)
        , build_time(0.0)
        , avg_test_time(0)
        , memory_usage(0) {}
};

/**
 * @brief Quality gate definitions
 */
enum class QualityGate {
    TEST_COVERAGE_MINIMUM,
    CODE_SMELLS_MAXIMUM,
    DUPLICATION_MAXIMUM,
    MAINTAINABILITY_MINIMUM,
    BUILD_TIME_MAXIMUM,
    TEST_TIME_MAXIMUM
};

/**
 * @brief Quality gate configuration
 */
struct QualityGateConfig {
    QualityGate gate;
    double threshold;
    std::string description;
    bool is_blocking;  // If true, blocks CI/CD pipeline on failure

    QualityGateConfig(QualityGate g, double thresh, const std::string& desc, bool block = true)
        : gate(g), threshold(thresh), description(desc), is_blocking(block) {}
};

/**
 * @brief Test framework interface
 */
class TestFramework {
public:
    virtual ~TestFramework() = default;

    // Run a single test
    virtual TestResult run_test(const std::string& test_name) = 0;

    // Run all tests in a suite
    virtual std::vector<TestResult> run_suite(const std::string& suite_name) = 0;

    // Run all tests
    virtual std::vector<TestResult> run_all() = 0;

    // Get test coverage report
    virtual QualityMetrics get_coverage_metrics() = 0;

    // Register a test case
    virtual void register_test(
        const std::string& suite_name,
        const std::string& test_name,
        std::function<TestResult()> test_func,
        std::vector<std::string> tags = {}
    ) = 0;
};

/**
 * @brief Static analysis interface
 */
class StaticAnalyzer {
public:
    virtual ~StaticAnalyzer() = default;

    // Analyze source code
    virtual QualityMetrics analyze(const std::string& source_path) = 0;

    // Analyze specific issues
    virtual std::vector<std::string> detect_code_smells(const std::string& source_path) = 0;
    virtual std::vector<std::string> detect_duplications(const std::string& source_path) = 0;

    // Get complexity metrics
    virtual double get_cyclomatic_complexity(const std::string& source_path) = 0;
    virtual double get_maintainability_index(const std::string& source_path) = 0;
};

/**
 * @brief Performance testing interface
 */
class PerformanceTest {
public:
    virtual ~PerformanceTest() = default;

    struct BenchmarkResult {
        std::string benchmark_name;
        std::chrono::nanoseconds duration;
        size_t iterations;
        double ops_per_second;
        size_t memory_used;
    };

    // Run a benchmark
    virtual BenchmarkResult run_benchmark(const std::string& benchmark_name) = 0;

    // Run all benchmarks
    virtual std::vector<BenchmarkResult> run_all() = 0;

    // Register a benchmark
    virtual void register_benchmark(
        const std::string& name,
        std::function<BenchmarkResult()> benchmark_func
    ) = 0;
};

/**
 * @brief Quality assurance manager
 */
class QualityAssurance {
public:
    static QualityAssurance& instance();

    // Configure quality gates
    void configure_quality_gate(const QualityGateConfig& config);

    // Run full quality assessment
    struct QualityReport {
        bool passed_all_gates;
        QualityMetrics metrics;
        std::vector<std::pair<QualityGate, bool>> gate_results;  // Gate and whether it passed
        std::vector<std::string> recommendations;
        std::chrono::milliseconds total_assessment_time;
    };

    QualityReport run_quality_assessment(const std::string& project_path);

    // Quick check (subset of gates)
    bool quick_quality_check(const std::string& project_path);

    // Set test framework
    void set_test_framework(std::unique_ptr<TestFramework> framework);

    // Set static analyzer
    void set_static_analyzer(std::unique_ptr<StaticAnalyzer> analyzer);

    // Set performance tester
    void set_performance_tester(std::unique_ptr<PerformanceTest> tester);

    // Get current configuration
    const std::vector<QualityGateConfig>& get_quality_gates() const;

private:
    std::vector<QualityGateConfig> quality_gates_;
    std::unique_ptr<TestFramework> test_framework_;
    std::unique_ptr<StaticAnalyzer> static_analyzer_;
    std::unique_ptr<PerformanceTest> performance_tester_;

    bool evaluate_gate(const QualityGateConfig& config, const QualityMetrics& metrics);
};

/**
 * @brief Continuous integration helper
 */
class CIHelper {
public:
    struct CIConfig {
        std::string build_system;
        std::vector<std::string> build_commands;
        std::vector<std::string> test_commands;
        std::vector<std::string> analysis_commands;
        bool quality_gate_blocking;
        std::string report_format;  // "json", "xml", "html"
    };

    // Run CI pipeline
    struct CIPipelineResult {
        bool build_passed;
        bool tests_passed;
        bool quality_passed;
        bool overall_passed;
        std::string report_path;
        std::chrono::seconds total_time;
    };

    static CIPipelineResult run_pipeline(const CIConfig& config);

    // Generate CI configuration files
    static void generate_github_actions(const std::string& output_path);
    static void generate_azure_pipelines(const std::string& output_path);
    static void generate_jenkinsfile(const std::string& output_path);
};

/**
 * @brief Code coverage collector
 */
class CoverageCollector {
public:
    struct CoverageData {
        std::string file_path;
        int total_lines;
        int covered_lines;
        int total_branches;
        int covered_branches;
        std::vector<int> uncovered_lines;
    };

    // Collect coverage data
    static std::vector<CoverageData> collect_coverage(
        const std::string& build_path,
        const std::vector<std::string>& test_executables
    );

    // Generate coverage report
    static void generate_report(
        const std::vector<CoverageData>& data,
        const std::string& output_path,
        const std::string& format = "html"
    );

    // Merge multiple coverage reports
    static std::vector<CoverageData> merge_reports(
        const std::vector<std::string>& report_paths
    );
};

/**
 * @brief Quality metrics dashboard
 */
class QualityDashboard {
public:
    struct HistoricalData {
        std::chrono::system_clock::time_point timestamp;
        QualityMetrics metrics;
        std::string commit_hash;
    };

    // Record metrics
    void record_metrics(const QualityMetrics& metrics, const std::string& commit_hash = "");

    // Get trend data
    std::vector<HistoricalData> get_trend_data(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end
    );

    // Generate dashboard
    void generate_dashboard(const std::string& output_path);

    // Check for quality degradation
    std::vector<std::string> check_quality_degradation(size_t window_size = 10);

private:
    std::vector<HistoricalData> historical_data_;
};

/**
 * @brief Quality assertion macros
 */

#define XPUMUSIC_ASSERT_TRUE(condition, message) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("Assertion failed: " message); \
        } \
    } while(0)

#define XPUMUSIC_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            throw std::runtime_error( \
                "Assertion failed: " message " - Expected: " + std::to_string(expected) + \
                ", Actual: " + std::to_string(actual)); \
        } \
    } while(0)

#define XPUMUSIC_QUALITY_GATE_MINIMUM(metric, threshold) \
    if (metrics.metric < threshold) { \
        report.recommendations.push_back( \
            #metric " is below threshold (" + std::to_string(metrics.metric) + \
            " < " + std::to_string(threshold) + ")"); \
    }

#define XPUMUSIC_QUALITY_GATE_MAXIMUM(metric, threshold) \
    if (metrics.metric > threshold) { \
        report.recommendations.push_back( \
            #metric " exceeds threshold (" + std::to_string(metrics.metric) + \
            " > " + std::to_string(threshold) + ")"); \
    }

} // namespace quality
} // namespace xpumusic

/**
 * @example Quality assurance setup
 *
 * // 1. Initialize quality system
 * auto& qa = xpumusic::quality::QualityAssurance::instance();
 *
 * // 2. Configure quality gates
 * qa.configure_quality_gate({
 *     xpumusic::quality::QualityGate::TEST_COVERAGE_MINIMUM,
 *     80.0,
 *     "Minimum test coverage of 80%",
 *     true  // Blocking
 * });
 *
 * qa.configure_quality_gate({
 *     xpumusic::quality::QualityGate::CODE_SMELLS_MAXIMUM,
 *     10.0,
 *     "Maximum 10 code smells allowed",
 *     false  // Warning only
 * });
 *
 * // 3. Run quality assessment
 * auto report = qa.run_quality_assessment("./src");
 * if (!report.passed_all_gates) {
 *     for (const auto& rec : report.recommendations) {
 *         std::cout << "Recommendation: " << rec << std::endl;
 *     }
 * }
 *
 * // 4. Generate CI configuration
 * xpumusic::quality::CIHelper::generate_github_actions("./.github/workflows/quality.yml");
 */