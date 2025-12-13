/**
 * @file volume_plugin.cpp
 * @brief Example volume control plugin for XpuMusic
 * @date 2025-12-13
 */

#include "volume_plugin.h"
#include <cstring>
#include <cmath>

// Plugin export function - required for all XpuMusic plugins
extern "C" __declspec(dllexport)
xpumusic::IEffectPlugin* create_effect_plugin() {
    return new VolumeControlPlugin();
}

// Plugin info function - optional but recommended
extern "C" __declspec(dllexport)
const char* get_plugin_info() {
    return "Volume Control v1.0\n"
           "Author: XpuMusic Team\n"
           "Description: Simple volume control with mute support\n"
           "Parameters:\n"
           "  volume_db: Volume in decibels (-60 to 0)\n"
           "  volume_linear: Volume as linear multiplier (0 to 1)\n"
           "  mute: Mute toggle (0 or 1)";
}

// Plugin version function
extern "C" __declspec(dllexport)
int get_plugin_version() {
    return 1;  // Version 1.0
}