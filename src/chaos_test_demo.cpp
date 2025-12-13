#include "testing/chaos_test_framework.h"
#include <iostream>

using namespace xpumusic::chaos;

int main() {
    std::cout << "=== XpuMusic Chaos Testing Demo ===\n\n";

    // Create chaos orchestrator with moderate chaos level
    ChaosOrchestrator orchestrator(ChaosLevel::MODERATE);

    // Configure orchestrator
    orchestrator.set_auto_discovery(true);
    orchestrator.set_continuous_learning(true);

    std::cout << "Starting 2-minute chaos session...\n";
    std::cout << "This will:\n";
    std::cout << "  1. Inject random file corruptions\n";
    std::cout << "  2. Apply memory pressure\n";
    std::cout << "  3. Inject execution delays\n";
    std::cout << "  4. Learn from system responses\n";
    std::cout << "  5. Adapt chaos level based on performance\n\n";

    // Run chaos session for 2 minutes
    orchestrator.run_chaos_session(2);

    // Export session data
    orchestrator.export_session_data("chaos_session_report.txt");

    std::cout << "\nChaos session completed!\n";
    std::cout << "Check 'chaos_session_report.txt' for detailed results.\n";

    return 0;
}