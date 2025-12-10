/**
 * @file main_debug.cpp
 * @brief Safe test version - step by step initialization
 */

#include "core_engine.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

static std::atomic<bool> g_running(true);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutdown signal received\n";
        g_running = false;
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Music Player - Safe Diagnostic Version\n";
    std::cout << "========================================\n";
    
    std::signal(SIGINT, signal_handler);
    
    std::cout << "Step 1: Creating CoreEngine...\n";
    mp::core::CoreEngine engine;
    
    // Initialize step by step
    try {
        std::cout << "Step 2: Calling initialize()...\n";
        auto result = engine.initialize();
        if (result == mp::Result::Success) {
            std::cout << "✓ Initialization successful!\n";
            
            // Just wait a bit then exit cleanly
            std::cout << "Running for 3 seconds...\n";
            for (int i = 0; i < 30 && g_running; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            std::cout << "Exiting cleanly...\n";
        } else {
            std::cout << "✗ Initialization failed: " << static_cast<int>(result) << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "✗ Exception during initialization: " << e.what() << "\n";
    } catch (...) {
        std::cout << "✗ Unknown exception during initialization\n";
    }
    
    std::cout << "Done.\n";
    std::cout << "========================================\n";
    
    return 0;
}
