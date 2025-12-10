#include "xpumusic_plugin_sdk.h"

// This file contains the base plugin implementations
// Provide default implementations for IPlugin methods

namespace xpumusic {

// Default implementations for IPlugin methods
// Since the IPlugin class declares state_ and last_error_ as protected members,
// we can provide default implementations here

PluginState IPlugin::get_state() const {
    return state_;
}

void IPlugin::set_state(PluginState state) {
    state_ = state;
}

std::string IPlugin::get_last_error() const {
    return last_error_;
}

// Helper methods for derived classes
void IPlugin::set_error(const std::string& error) {
    last_error_ = error;
    set_state(PluginState::Error);
}

} // namespace xpumusic