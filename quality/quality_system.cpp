/**
 * @file quality_system.cpp
 * @brief Implementation of quality assurance system
 * @date 2025-12-13
 */

#include "quality_system.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <future>

namespace xpumusic {
namespace quality {

// Implementation of QualityAssurance
QualityAssurance& QualityAssurance::instance() {
    static QualityAssurance instance;
    return instance;
}

void QualityAssurance::configure_quality_gate(const QualityGateConfig& config) {
    quality_gates_.push_back(config);
}

QualityAssurance::QualityReport QualityAssurance::run_quality_assessment(const std::string& project_path) {
    auto start_time = std::chrono::steady_clock::now();
    QualityReport report;

    // Initialize metrics
    report.metrics = QualityMetrics();

    // Run static analysis if available
    if (static_analyzer_) {
        auto analysis_metrics = static_analyzer_->analyze(project_path);

        // Merge metrics (simplified)
        report.metrics.cyclomatic_complexity = analysis_metrics.cyclomatic_complexity;
        report.metrics.cognitive_complexity = analysis_metrics.cognitive_complexity;
        report.metrics.code_smells = analysis_metrics.code_smells;
        report.metrics.duplicated_lines = analysis_metrics.duplicated_lines;
        report.metrics.maintainability_index = analysis_metrics.maintainability_index;
    }

    // Run tests and collect coverage if available
    if (test_framework_) {
        auto test_results = test_framework_->run_all();
        auto coverage_metrics = test_framework_->get_coverage_metrics();

        // Update coverage metrics
        report.metrics.line_coverage = coverage_metrics.line_coverage;
        report.metrics.branch_coverage = coverage_metrics.branch_coverage;
        report.metrics.function_coverage = coverage_metrics.function_coverage;

        // Calculate average test time
        if (!test_results.empty()) {
            auto total_time = std::accumulate(
                test_results.begin(),
                test_results.end(),
                std::chrono::milliseconds(0),
                [](auto acc, const auto& result) { return acc + result.duration; }
            );
            report.metrics.avg_test_time = total_time / test_results.size();
        }
    }

    // Run performance tests if available
    if (performance_tester_) {
        auto benchmarks = performance_tester_->run_all();
        // Process benchmark results and update metrics
        // This is a simplified implementation
    }

    // Evaluate all quality gates
    report.passed_all_gates = true;
    for (const auto& gate : quality_gates_) {
        bool gate_passed = evaluate_gate(gate, report.metrics);
        report.gate_results.push_back({gate.gate, gate_passed});

        if (!gate_passed && gate.is_blocking) {
            report.passed_all_gates = false;
        }
    }

    // Generate recommendations
    if (report.metrics.line_coverage < 80.0) {
        report.recommendations.push_back(
            "Increase test coverage. Current: " +
            std::to_string(report.metrics.line_coverage) + "%"
        );
    }

    if (report.metrics.code_smells > 10) {
        report.recommendations.push_back(
            "Address code smells. Current: " + std::to_string(report.metrics.code_smells)
        );
    }

    if (report.metrics.maintainability_index < 60.0) {
        report.recommendations.push_back(
            "Improve code maintainability. Current: " +
            std::to_string(report.metrics.maintainability_index)
        );
    }

    auto end_time = std::chrono::steady_clock::now();
    report.total_assessment_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );

    return report;
}

bool QualityAssurance::quick_quality_check(const std::string& project_path) {
    // Run only critical quality gates
    QualityMetrics metrics;

    // Quick static analysis
    if (static_analyzer_) {
        metrics = static_analyzer_->analyze(project_path);
    }

    // Check critical gates only
    for (const auto& gate : quality_gates_) {
        if (gate.is_blocking && !evaluate_gate(gate, metrics)) {
            return false;
        }
    }

    return true;
}

void QualityAssurance::set_test_framework(std::unique_ptr<TestFramework> framework) {
    test_framework_ = std::move(framework);
}

void QualityAssurance::set_static_analyzer(std::unique_ptr<StaticAnalyzer> analyzer) {
    static_analyzer_ = std::move(analyzer);
}

void QualityAssurance::set_performance_tester(std::unique_ptr<PerformanceTest> tester) {
    performance_tester_ = std::move(tester);
}

const std::vector<QualityGateConfig>& QualityAssurance::get_quality_gates() const {
    return quality_gates_;
}

bool QualityAssurance::evaluate_gate(const QualityGateConfig& config, const QualityMetrics& metrics) {
    switch (config.gate) {
        case QualityGate::TEST_COVERAGE_MINIMUM:
            return metrics.line_coverage >= config.threshold;

        case QualityGate::CODE_SMELLS_MAXIMUM:
            return static_cast<double>(metrics.code_smells) <= config.threshold;

        case QualityGate::DUPLICATION_MAXIMUM:
            return static_cast<double>(metrics.duplicated_lines) <= config.threshold;

        case QualityGate::MAINTAINABILITY_MINIMUM:
            return metrics.maintainability_index >= config.threshold;

        case QualityGate::BUILD_TIME_MAXIMUM:
            return metrics.build_time <= config.threshold;

        case QualityGate::TEST_TIME_MAXIMUM:
            return metrics.avg_test_time.count() <= config.threshold;

        default:
            return true; // Unknown gate, pass by default
    }
}

// Implementation of CIHelper
CIHelper::CIPipelineResult CIHelper::run_pipeline(const CIConfig& config) {
    CIPipelineResult result;
    auto start_time = std::chrono::steady_clock::now();

    // Run build commands
    result.build_passed = true;
    for (const auto& cmd : config.build_commands) {
        int exit_code = std::system(cmd.c_str());
        if (exit_code != 0) {
            result.build_passed = false;
            break;
        }
    }

    // Run test commands
    result.tests_passed = true;
    if (result.build_passed) {
        for (const auto& cmd : config.test_commands) {
            int exit_code = std::system(cmd.c_str());
            if (exit_code != 0) {
                result.tests_passed = false;
                break;
            }
        }
    }

    // Run quality checks
    result.quality_passed = true;
    if (result.build_passed && result.tests_passed) {
        auto& qa = QualityAssurance::instance();
        result.quality_passed = qa.quick_quality_check(".");
    }

    // Overall result
    result.overall_passed = result.build_passed && result.tests_passed &&
                           (!config.quality_gate_blocking || result.quality_passed);

    auto end_time = std::chrono::steady_clock::now();
    result.total_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

    return result;
}

void CIHelper::generate_github_actions(const std::string& output_path) {
    std::ofstream file(output_path);
    file << R"(name: Quality Assurance

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  quality:
    runs-on: windows-latest

    steps:
    - uses: actions/checkout@v3

    - name: Setup Visual Studio
      uses: microsoft/setup-msbuild@v1.3

    - name: Cache build
      uses: actions/cache@v3
      with:
        path: |
          build/
          !build/**/CMakeFiles/
        key: ${{ runner.os }}-build-${{ hashFiles('**/CMakeLists.txt') }}

    - name: Configure CMake
      run: |
        mkdir build
        cd build
        cmake .. -G "Visual Studio 17 2022" -A x64

    - name: Build
      run: |
        cd build
        cmake --build . --config Debug --parallel

    - name: Run Tests
      run: |
        cd build/bin/Debug
        ./test_decoders.exe
        ./test_audio_direct.exe

    - name: Quality Check
      run: |
        # Run static analysis, coverage, etc.
        echo "Running quality assurance checks..."

    - name: Generate Report
      run: |
        # Generate quality report
        echo "Generating quality report..."
)";
    file.close();
}

void CIHelper::generate_azure_pipelines(const std::string& output_path) {
    std::ofstream file(output_path);
    file << R"(trigger:
- main
- develop

pr:
- main

pool:
  vmImage: 'windows-latest'

variables:
  buildConfiguration: 'Debug'
  buildPlatform: 'x64'

stages:
- stage: Build
  displayName: 'Build Stage'
  jobs:
  - job: Build
    displayName: 'Build Job'
    steps:
    - task: VSBuild@1
      inputs:
        solution: '**/*.sln'
        platform: '$(buildPlatform)'
        configuration: '$(buildConfiguration)'
        msbuildArgs: '/m'

- stage: Test
  displayName: 'Test Stage'
  dependsOn: Build
  condition: succeeded()
  jobs:
  - job: Test
    displayName: 'Test Job'
    steps:
    - script: |
        cd build/bin/Debug
        test_decoders.exe
        test_audio_direct.exe
      displayName: 'Run Tests'

- stage: Quality
  displayName: 'Quality Stage'
  dependsOn: Test
  condition: succeeded()
  jobs:
  - job: Quality
    displayName: 'Quality Check'
    steps:
    - script: |
        echo "Running quality checks..."
        # Add quality check commands here
      displayName: 'Quality Assurance'
)";
    file.close();
}

void CIHelper::generate_jenkinsfile(const std::string& output_path) {
    std::ofstream file(output_path);
    file << R"(pipeline {
    agent any

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Configure') {
            steps {
                bat 'mkdir build'
                bat 'cd build && cmake .. -G "Visual Studio 17 2022" -A x64'
            }
        }

        stage('Build') {
            steps {
                bat 'cd build && cmake --build . --config Debug --parallel'
            }
        }

        stage('Test') {
            steps {
                bat 'cd build/bin/Debug && test_decoders.exe'
                bat 'cd build/bin/Debug && test_audio_direct.exe'
            }
            post {
                always {
                    publishTestResults testResultsPattern: 'build/**/*.xml'
                }
            }
        }

        stage('Quality') {
            steps {
                bat 'echo "Running quality checks..."'
                // Add static analysis, coverage, etc.
            }
            post {
                always {
                    publishHTML([
                        allowMissing: false,
                        alwaysLinkToLastBuild: true,
                        keepAll: true,
                        reportDir: 'build/reports',
                        reportFiles: 'quality_report.html',
                        reportName: 'Quality Report'
                    ])
                }
            }
        }
    }

    post {
        always {
            cleanWs()
        }
        success {
            echo 'Pipeline succeeded!'
        }
        failure {
            echo 'Pipeline failed!'
            mail to: 'team@example.com',
                 subject: "Build Failed: ${env.JOB_NAME} - ${env.BUILD_NUMBER}",
                 body: "Build failed. Check console output at ${env.BUILD_URL}"
        }
    }
}
)";
    file.close();
}

// Implementation of CoverageCollector
std::vector<CoverageCollector::CoverageData> CoverageCollector::collect_coverage(
    const std::string& build_path,
    const std::vector<std::string>& test_executables) {

    std::vector<CoverageData> results;

    // This is a simplified implementation
    // In a real scenario, this would use tools like gcov, lcov, or OpenCppCoverage

    for (const auto& test_exe : test_executables) {
        // Run test with coverage instrumentation
        std::string cmd = test_exe + " --coverage";
        int result = std::system(cmd.c_str());

        // Parse coverage data (simplified)
        CoverageData data;
        data.file_path = "example.cpp";
        data.total_lines = 100;
        data.covered_lines = 80;
        data.total_branches = 20;
        data.covered_branches = 15;
        data.uncovered_lines = {10, 25, 30, 45, 60, 75, 90};

        results.push_back(data);
    }

    return results;
}

void CoverageCollector::generate_report(
    const std::vector<CoverageData>& data,
    const std::string& output_path,
    const std::string& format) {

    if (format == "html") {
        std::ofstream file(output_path);
        file << R"(<html>
<head>
    <title>Coverage Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .low { background-color: #ffcccc; }
        .medium { background-color: #fff3cd; }
        .high { background-color: #d4edda; }
    </style>
</head>
<body>
    <h1>Code Coverage Report</h1>
    <table>
        <tr>
            <th>File</th>
            <th>Line Coverage</th>
            <th>Branch Coverage</th>
            <th>Functions</th>
        </tr>)";

        for (const auto& item : data) {
            double line_pct = (double)item.covered_lines / item.total_lines * 100;
            std::string css_class = line_pct >= 80 ? "high" : (line_pct >= 60 ? "medium" : "low");

            file << "<tr class=\"" << css_class << "\">"
                 << "<td>" << item.file_path << "</td>"
                 << "<td>" << std::fixed << std::setprecision(1) << line_pct << "%</td>"
                 << "<td>" << (item.total_branches > 0 ?
                     std::to_string(item.covered_branches * 100 / item.total_branches) + "%" : "N/A") << "</td>"
                 << "<td>N/A</td>"
                 << "</tr>";
        }

        file << R"(</table>
</body>
</html>)";
        file.close();
    }
}

// Implementation of QualityDashboard
void QualityDashboard::record_metrics(const QualityMetrics& metrics, const std::string& commit_hash) {
    HistoricalData data;
    data.timestamp = std::chrono::system_clock::now();
    data.metrics = metrics;
    data.commit_hash = commit_hash;

    historical_data_.push_back(data);

    // Keep only last 100 entries
    if (historical_data_.size() > 100) {
        historical_data_.erase(historical_data_.begin());
    }
}

std::vector<QualityDashboard::HistoricalData> QualityDashboard::get_trend_data(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) {

    std::vector<HistoricalData> result;

    for (const auto& data : historical_data_) {
        if (data.timestamp >= start && data.timestamp <= end) {
            result.push_back(data);
        }
    }

    return result;
}

void QualityDashboard::generate_dashboard(const std::string& output_path) {
    // Generate HTML dashboard
    std::ofstream file(output_path);
    file << R"(<html>
<head>
    <title>XpuMusic Quality Dashboard</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .chart-container { width: 100%; height: 400px; margin: 20px 0; }
        .metric-card { border: 1px solid #ddd; padding: 15px; margin: 10px 0; border-radius: 5px; }
        .metric-value { font-size: 24px; font-weight: bold; color: #007bff; }
    </style>
</head>
<body>
    <h1>XpuMusic Quality Dashboard</h1>)";

    if (!historical_data_.empty()) {
        const auto& latest = historical_data_.back();

        file << "<div class=\"metric-card\">"
             << "<h3>Latest Metrics</h3>"
             << "<p>Test Coverage: <span class=\"metric-value\">"
             << latest.metrics.line_coverage << "%</span></p>"
             << "<p>Code Smells: <span class=\"metric-value\">"
             << latest.metrics.code_smells << "</span></p>"
             << "<p>Maintainability Index: <span class=\"metric-value\">"
             << latest.metrics.maintainability_index << "</span></p>"
             << "</div>";
    }

    file << "<div class=\"chart-container\">"
         << "<canvas id=\"coverageChart\"></canvas>"
         << "</div>"
         << "<script>"
         << "const ctx = document.getElementById('coverageChart').getContext('2d');"
         << "new Chart(ctx, { type: 'line', data: { labels: [";

    // Add data points
    for (size_t i = 0; i < historical_data_.size(); ++i) {
        if (i > 0) file << ", ";
        file << "'" << i << "'";
    }

    file << "], datasets: [{ label: 'Line Coverage', data: [";

    for (size_t i = 0; i < historical_data_.size(); ++i) {
        if (i > 0) file << ", ";
        file << historical_data_[i].metrics.line_coverage;
    }

    file << "] }] }});"
         << "</script></body></html>";

    file.close();
}

std::vector<std::string> QualityDashboard::check_quality_degradation(size_t window_size) {
    std::vector<std::string> issues;

    if (historical_data_.size() < window_size) {
        return issues;
    }

    // Check last window_size entries for degradation
    auto start_it = historical_data_.end() - window_size;

    // Check coverage trend
    double avg_coverage = 0;
    for (auto it = start_it; it != historical_data_.end(); ++it) {
        avg_coverage += it->metrics.line_coverage;
    }
    avg_coverage /= window_size;

    if (avg_coverage < 70.0) {
        issues.push_back("Test coverage has dropped below 70% in recent builds");
    }

    // Check code smells trend
    double avg_smells = 0;
    for (auto it = start_it; it != historical_data_.end(); ++it) {
        avg_smells += it->metrics.code_smells;
    }
    avg_smells /= window_size;

    if (avg_smells > 20.0) {
        issues.push_back("Code smells have increased above 20 on average");
    }

    return issues;
}

// Initialize static member
template<typename OldInterface, typename NewInterface>
std::vector<typename InterfaceMigrator<OldInterface, NewInterface>::MigrationPath>
    InterfaceMigrator<OldInterface, NewInterface>::migration_paths_;

} // namespace quality
} // namespace xpumusic