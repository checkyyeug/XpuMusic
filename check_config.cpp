#include "config/config_manager.h"
#include <iostream>

using namespace xpumusic;

int main() {
    ConfigManager& config = ConfigManagerSingleton::get_instance();

    if (!config.initialize("config/default_config.json")) {
        std::cerr << "Failed to initialize config!" << std::endl;
        return 1;
    }

    const auto& cfg = config.get_config();
    std::cout << "Resampler Configuration:\n";
    std::cout << "  Quality: " << cfg.resampler.quality << "\n";
    std::cout << "  Floating Precision: " << cfg.resampler.floating_precision << " bits\n";
    std::cout << "  Adaptive: " << (cfg.resampler.enable_adaptive ? "Yes" : "No") << "\n";
    std::cout << "  CPU Threshold: " << cfg.resampler.cpu_threshold << "\n";

    ConfigManagerSingleton::shutdown();
    return 0;
}